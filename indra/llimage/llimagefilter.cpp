/** 
 * @file llimagefilter.cpp
 * @brief Simple Image Filtering. See https://wiki.lindenlab.com/wiki/SL_Viewer_Image_Filters for complete documentation.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llimagefilter.h"

#include "llmath.h"
#include "v3color.h"
#include "v4coloru.h"
#include "m3math.h"
#include "v3math.h"
#include "llsdserialize.h"
#include "llstring.h"

//---------------------------------------------------------------------------
// LLImageFilter
//---------------------------------------------------------------------------

LLImageFilter::LLImageFilter(const std::string& file_path) :
    mFilterData(LLSD::emptyArray()),
    mImage(NULL),
    mHistoRed(NULL),
    mHistoGreen(NULL),
    mHistoBlue(NULL),
    mHistoBrightness(NULL),
    mStencilBlendMode(STENCIL_BLEND_MODE_BLEND),
    mStencilShape(STENCIL_SHAPE_UNIFORM),
    mStencilGamma(1.0),
    mStencilMin(0.0),
    mStencilMax(1.0)
{
    // Load filter description from file
	llifstream filter_xml(file_path.c_str());
	if (filter_xml.is_open())
	{
		// Load and parse the file
		LLPointer<LLSDParser> parser = new LLSDXMLParser();
		parser->parse(filter_xml, mFilterData, LLSDSerialize::SIZE_UNLIMITED);
		filter_xml.close();
	}
}

LLImageFilter::~LLImageFilter()
{
    mImage = NULL;
    ll_aligned_free_16(mHistoRed);
    ll_aligned_free_16(mHistoGreen);
    ll_aligned_free_16(mHistoBlue);
    ll_aligned_free_16(mHistoBrightness);
}

/*
 *TODO 
 * Rename stencil to mask
 * Improve perf: use LUT for alpha blending in uniform case
 * Add gradient coloring as a filter
 */

//============================================================================
// Apply the filter data to the image passed as parameter
//============================================================================

void LLImageFilter::executeFilter(LLPointer<LLImageRaw> raw_image)
{
    mImage = raw_image;
    
	//std::cout << "Filter : size = " << mFilterData.size() << std::endl;
	for (S32 i = 0; i < mFilterData.size(); ++i)
	{
        std::string filter_name = mFilterData[i][0].asString();
        // Dump out the filter values (for debug)
        //std::cout << "Filter : name = " << mFilterData[i][0].asString() << ", params = ";
        //for (S32 j = 1; j < mFilterData[i].size(); ++j)
        //{
        //    std::cout << mFilterData[i][j].asString() << ", ";
        //}
        //std::cout << std::endl;
        
        if (filter_name == "stencil")
        {
            // Get the shape of the stencil, that is how the procedural alpha is computed geometrically
            std::string filter_shape = mFilterData[i][1].asString();
            EStencilShape shape = STENCIL_SHAPE_UNIFORM;
            if (filter_shape == "uniform")
            {
                shape = STENCIL_SHAPE_UNIFORM;
            }
            else if (filter_shape == "gradient")
            {
                shape = STENCIL_SHAPE_GRADIENT;
            }
            else if (filter_shape == "vignette")
            {
                shape = STENCIL_SHAPE_VIGNETTE;
            }
            else if (filter_shape == "scanlines")
            {
                shape = STENCIL_SHAPE_SCAN_LINES;
            }
            // Get the blend mode of the stencil, that is how the effect is blended in the background through the stencil
            std::string filter_mode  = mFilterData[i][2].asString();
            EStencilBlendMode mode = STENCIL_BLEND_MODE_BLEND;
            if (filter_mode == "blend")
            {
                mode = STENCIL_BLEND_MODE_BLEND;
            }
            else if (filter_mode == "add")
            {
                mode = STENCIL_BLEND_MODE_ADD;
            }
            else if (filter_mode == "add_back")
            {
                mode = STENCIL_BLEND_MODE_ABACK;
            }
            else if (filter_mode == "fade")
            {
                mode = STENCIL_BLEND_MODE_FADE;
            }
            // Get the float params: mandatory min, max then the optional parameters (4 max)
            F32 min = (F32)(mFilterData[i][3].asReal());
            F32 max = (F32)(mFilterData[i][4].asReal());
            F32 params[4] = {0.0, 0.0, 0.0, 0.0};
            for (S32 j = 5; (j < mFilterData[i].size()) && (j < 9); j++)
            {
                params[j-5] = (F32)(mFilterData[i][j].asReal());
            }
            // Set the stencil
            setStencil(shape,mode,min,max,params);
        }
        else if (filter_name == "sepia")
        {
            filterSepia();
        }
        else if (filter_name == "grayscale")
        {
            filterGrayScale();
        }
        else if (filter_name == "saturate")
        {
            filterSaturate((float)(mFilterData[i][1].asReal()));
        }
        else if (filter_name == "rotate")
        {
            filterRotate((float)(mFilterData[i][1].asReal()));
        }
        else if (filter_name == "gamma")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterGamma((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "colorize")
        {
            LLColor3 color((float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()));
            LLColor3 alpha((F32)(mFilterData[i][4].asReal()),(float)(mFilterData[i][5].asReal()),(float)(mFilterData[i][6].asReal()));
            filterColorize(color,alpha);
        }
        else if (filter_name == "contrast")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterContrast((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "brighten")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterBrightness((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "darken")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterBrightness((float)(-mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "linearize")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterLinearize((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "posterize")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterEqualize((S32)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "screen")
        {
            std::string screen_name = mFilterData[i][1].asString();
            EScreenMode mode = SCREEN_MODE_2DSINE;
            if (screen_name == "2Dsine")
            {
                mode = SCREEN_MODE_2DSINE;
            }
            else if (screen_name == "line")
            {
                mode = SCREEN_MODE_LINE;
            }
            filterScreen(mode,(F32)(mFilterData[i][2].asReal()),(F32)(mFilterData[i][3].asReal()));
        }
        else if (filter_name == "blur")
        {
            LLMatrix3 kernel;
            for (S32 i = 0; i < NUM_VALUES_IN_MAT3; i++)
                for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
                    kernel.mMatrix[i][j] = 1.0;
            convolve(kernel,true,false);
        }
        else if (filter_name == "sharpen")
        {
            LLMatrix3 kernel;
            for (S32 k = 0; k < NUM_VALUES_IN_MAT3; k++)
                for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
                    kernel.mMatrix[k][j] = -1.0;
            kernel.mMatrix[1][1] = 9.0;
            convolve(kernel,false,false);
        }
        else if (filter_name == "gradient")
        {
            LLMatrix3 kernel;
            for (S32 k = 0; k < NUM_VALUES_IN_MAT3; k++)
                for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
                    kernel.mMatrix[k][j] = -1.0;
            kernel.mMatrix[1][1] = 8.0;
            convolve(kernel,false,true);
        }
        else if (filter_name == "convolve")
        {
            LLMatrix3 kernel;
            S32 index = 1;
            bool normalize = (mFilterData[i][index++].asReal() > 0.0);
            bool abs_value = (mFilterData[i][index++].asReal() > 0.0);
            for (S32 k = 0; k < NUM_VALUES_IN_MAT3; k++)
                for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
                    kernel.mMatrix[k][j] = mFilterData[i][index++].asReal();
            convolve(kernel,normalize,abs_value);
        }
        else if (filter_name == "colortransform")
        {
            LLMatrix3 transform;
            S32 index = 1;
            for (S32 k = 0; k < NUM_VALUES_IN_MAT3; k++)
                for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
                    transform.mMatrix[k][j] = mFilterData[i][index++].asReal();
            transform.transpose();
            colorTransform(transform);
        }
        else
        {
            LL_WARNS() << "Filter unknown, cannot execute filter command : " << filter_name << LL_ENDL;
        }
    }
}

//============================================================================
// Filter Primitives
//============================================================================

void LLImageFilter::blendStencil(F32 alpha, U8* pixel, U8 red, U8 green, U8 blue)
{
    F32 inv_alpha = 1.0 - alpha;
    switch (mStencilBlendMode)
    {
        case STENCIL_BLEND_MODE_BLEND:
            // Classic blend of incoming color with the background image
            pixel[VRED]   = inv_alpha * pixel[VRED]   + alpha * red;
            pixel[VGREEN] = inv_alpha * pixel[VGREEN] + alpha * green;
            pixel[VBLUE]  = inv_alpha * pixel[VBLUE]  + alpha * blue;
            break;
        case STENCIL_BLEND_MODE_ADD:
            // Add incoming color to the background image
            pixel[VRED]   = llclampb(pixel[VRED]   + alpha * red);
            pixel[VGREEN] = llclampb(pixel[VGREEN] + alpha * green);
            pixel[VBLUE]  = llclampb(pixel[VBLUE]  + alpha * blue);
            break;
        case STENCIL_BLEND_MODE_ABACK:
            // Add back background image to the incoming color
            pixel[VRED]   = llclampb(inv_alpha * pixel[VRED]   + red);
            pixel[VGREEN] = llclampb(inv_alpha * pixel[VGREEN] + green);
            pixel[VBLUE]  = llclampb(inv_alpha * pixel[VBLUE]  + blue);
            break;
        case STENCIL_BLEND_MODE_FADE:
            // Fade incoming color to black
            pixel[VRED]   = alpha * red;
            pixel[VGREEN] = alpha * green;
            pixel[VBLUE]  = alpha * blue;
            break;
    }
}

void LLImageFilter::colorCorrect(const U8* lut_red, const U8* lut_green, const U8* lut_blue)
{
	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
	S32 width  = mImage->getWidth();
    S32 height = mImage->getHeight();
    
	U8* dst_data = mImage->getData();
	for (S32 j = 0; j < height; j++)
	{
        for (S32 i = 0; i < width; i++)
        {
            // Blend LUT value
            blendStencil(getStencilAlpha(i,j), dst_data, lut_red[dst_data[VRED]], lut_green[dst_data[VGREEN]], lut_blue[dst_data[VBLUE]]);
            dst_data += components;
        }
	}
}

void LLImageFilter::colorTransform(const LLMatrix3 &transform)
{
	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
	S32 width  = mImage->getWidth();
    S32 height = mImage->getHeight();
    
	U8* dst_data = mImage->getData();
	for (S32 j = 0; j < height; j++)
	{
        for (S32 i = 0; i < width; i++)
        {
            // Compute transform
            LLVector3 src((F32)(dst_data[VRED]),(F32)(dst_data[VGREEN]),(F32)(dst_data[VBLUE]));
            LLVector3 dst = src * transform;
            dst.clamp(0.0f,255.0f);
            
            // Blend result
            blendStencil(getStencilAlpha(i,j), dst_data, dst.mV[VRED], dst.mV[VGREEN], dst.mV[VBLUE]);
            dst_data += components;
        }
	}
}

void LLImageFilter::convolve(const LLMatrix3 &kernel, bool normalize, bool abs_value)
{
	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
    // Compute normalization factors
    F32 kernel_min = 0.0;
    F32 kernel_max = 0.0;
    for (S32 i = 0; i < NUM_VALUES_IN_MAT3; i++)
    {
        for (S32 j = 0; j < NUM_VALUES_IN_MAT3; j++)
        {
            if (kernel.mMatrix[i][j] >= 0.0)
                kernel_max += kernel.mMatrix[i][j];
            else
                kernel_min += kernel.mMatrix[i][j];
        }
    }
    if (abs_value)
    {
        kernel_max = llabs(kernel_max);
        kernel_min = llabs(kernel_min);
        kernel_max = llmax(kernel_max,kernel_min);
        kernel_min = 0.0;
    }
    F32 kernel_range = kernel_max - kernel_min;
    
    // Allocate temporary buffers and initialize algorithm's data
	S32 width  = mImage->getWidth();
    S32 height = mImage->getHeight();
    
	U8* dst_data = mImage->getData();

	S32 buffer_size = width * components;
	llassert_always(buffer_size > 0);
	std::vector<U8> even_buffer(buffer_size);
	std::vector<U8> odd_buffer(buffer_size);
	
    U8* south_data = dst_data + buffer_size;
    U8* east_west_data;
    U8* north_data;
    
    // Line 0 : we set the line to 0 (debatable)
    memcpy( &even_buffer[0], dst_data, buffer_size );	/* Flawfinder: ignore */
    for (S32 i = 0; i < width; i++)
    {
        blendStencil(getStencilAlpha(i,0), dst_data, 0, 0, 0);
        dst_data += components;
    }
    south_data += buffer_size;
    
    // All other lines
    for (S32 j = 1; j < (height-1); j++)
	{
        // We need to buffer 2 lines. We flip north and east-west (current) to avoid moving too much memory around
        if (j % 2)
        {
            memcpy( &odd_buffer[0], dst_data, buffer_size );	/* Flawfinder: ignore */
            east_west_data = &odd_buffer[0];
            north_data = &even_buffer[0];
        }
        else
        {
            memcpy( &even_buffer[0], dst_data, buffer_size );	/* Flawfinder: ignore */
            east_west_data = &even_buffer[0];
            north_data = &odd_buffer[0];
        }
        // First pixel : set to 0
        blendStencil(getStencilAlpha(0,j), dst_data, 0, 0, 0);
        dst_data += components;
        // Set pointers to kernel
        U8* NW = north_data;
        U8* N = NW+components;
        U8* NE = N+components;
        U8* W = east_west_data;
        U8* C = W+components;
        U8* E = C+components;
        U8* SW = south_data;
        U8* S = SW+components;
        U8* SE = S+components;
        // All other pixels
        for (S32 i = 1; i < (width-1); i++)
        {
            // Compute convolution
            LLVector3 dst;
            dst.mV[VRED] = (kernel.mMatrix[0][0]*NW[VRED] + kernel.mMatrix[0][1]*N[VRED] + kernel.mMatrix[0][2]*NE[VRED] +
                            kernel.mMatrix[1][0]*W[VRED]  + kernel.mMatrix[1][1]*C[VRED] + kernel.mMatrix[1][2]*E[VRED] +
                            kernel.mMatrix[2][0]*SW[VRED] + kernel.mMatrix[2][1]*S[VRED] + kernel.mMatrix[2][2]*SE[VRED]);
            dst.mV[VGREEN] = (kernel.mMatrix[0][0]*NW[VGREEN] + kernel.mMatrix[0][1]*N[VGREEN] + kernel.mMatrix[0][2]*NE[VGREEN] +
                              kernel.mMatrix[1][0]*W[VGREEN]  + kernel.mMatrix[1][1]*C[VGREEN] + kernel.mMatrix[1][2]*E[VGREEN] +
                              kernel.mMatrix[2][0]*SW[VGREEN] + kernel.mMatrix[2][1]*S[VGREEN] + kernel.mMatrix[2][2]*SE[VGREEN]);
            dst.mV[VBLUE] = (kernel.mMatrix[0][0]*NW[VBLUE] + kernel.mMatrix[0][1]*N[VBLUE] + kernel.mMatrix[0][2]*NE[VBLUE] +
                             kernel.mMatrix[1][0]*W[VBLUE]  + kernel.mMatrix[1][1]*C[VBLUE] + kernel.mMatrix[1][2]*E[VBLUE] +
                             kernel.mMatrix[2][0]*SW[VBLUE] + kernel.mMatrix[2][1]*S[VBLUE] + kernel.mMatrix[2][2]*SE[VBLUE]);
            if (abs_value)
            {
                dst.mV[VRED]   = llabs(dst.mV[VRED]);
                dst.mV[VGREEN] = llabs(dst.mV[VGREEN]);
                dst.mV[VBLUE]  = llabs(dst.mV[VBLUE]);
            }
            if (normalize)
            {
                dst.mV[VRED]   = (dst.mV[VRED] - kernel_min)/kernel_range;
                dst.mV[VGREEN] = (dst.mV[VGREEN] - kernel_min)/kernel_range;
                dst.mV[VBLUE]  = (dst.mV[VBLUE] - kernel_min)/kernel_range;
            }
            dst.clamp(0.0f,255.0f);
            
            // Blend result
            blendStencil(getStencilAlpha(i,j), dst_data, dst.mV[VRED], dst.mV[VGREEN], dst.mV[VBLUE]);
            
            // Next pixel
            dst_data += components;
            NW += components;
            N += components;
            NE += components;
            W += components;
            C += components;
            E += components;
            SW += components;
            S += components;
            SE += components;
        }
        // Last pixel : set to 0
        blendStencil(getStencilAlpha(width-1,j), dst_data, 0, 0, 0);
        dst_data += components;
        south_data += buffer_size;
	}
    
    // Last line
    for (S32 i = 0; i < width; i++)
    {
        blendStencil(getStencilAlpha(i,0), dst_data, 0, 0, 0);
        dst_data += components;
    }
}

void LLImageFilter::filterScreen(EScreenMode mode, const F32 wave_length, const F32 angle)
{
	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
	S32 width  = mImage->getWidth();
    S32 height = mImage->getHeight();
    
    F32 wave_length_pixels = wave_length * (F32)(height) / 2.0;
    F32 sin = sinf(angle*DEG_TO_RAD);
    F32 cos = cosf(angle*DEG_TO_RAD);

    // Precompute the gamma table : gives us the gray level to use when cutting outside the screen (prevents strong aliasing on the screen)
    U8 gamma[256];
    for (S32 i = 0; i < 256; i++)
    {
        F32 gamma_i = llclampf((float)(powf((float)(i)/255.0,1.0/4.0)));
        gamma[i] = (U8)(255.0 * gamma_i);
    }
    
	U8* dst_data = mImage->getData();
	for (S32 j = 0; j < height; j++)
	{
        for (S32 i = 0; i < width; i++)
        {
            // Compute screen value
            F32 value = 0.0;
            F32 di = 0.0;
            F32 dj = 0.0;
            switch (mode)
            {
                case SCREEN_MODE_2DSINE:
                    di =  cos*i + sin*j;
                    dj = -sin*i + cos*j;
                    value = (sinf(2*F_PI*di/wave_length_pixels)*sinf(2*F_PI*dj/wave_length_pixels)+1.0)*255.0/2.0;
                    break;
                case SCREEN_MODE_LINE:
                    dj = sin*i - cos*j;
                    value = (sinf(2*F_PI*dj/wave_length_pixels)+1.0)*255.0/2.0;
                    break;
            }
            U8 dst_value = (dst_data[VRED] >= (U8)(value) ? gamma[dst_data[VRED] - (U8)(value)] : 0);
            
            // Blend result
            blendStencil(getStencilAlpha(i,j), dst_data, dst_value, dst_value, dst_value);
            dst_data += components;
        }
	}
}

//============================================================================
// Procedural Stencils
//============================================================================
void LLImageFilter::setStencil(EStencilShape shape, EStencilBlendMode mode, F32 min, F32 max, F32* params)
{
    mStencilShape = shape;
    mStencilBlendMode = mode;
    mStencilMin = llmin(llmax(min, -1.0f), 1.0f);
    mStencilMax = llmin(llmax(max, -1.0f), 1.0f);
    
    // Each shape will interpret the 4 params differenly.
    // We compute each systematically, though, clearly, values are meaningless when the shape doesn't correspond to the parameters
    mStencilCenterX = (S32)(mImage->getWidth()  + params[0] * (F32)(mImage->getHeight()))/2;
    mStencilCenterY = (S32)(mImage->getHeight() + params[1] * (F32)(mImage->getHeight()))/2;
    mStencilWidth = (S32)(params[2] * (F32)(mImage->getHeight()))/2;
    mStencilGamma = (params[3] <= 0.0 ? 1.0 : params[3]);

    mStencilWavelength = (params[0] <= 0.0 ? 10.0 : params[0] * (F32)(mImage->getHeight()) / 2.0);
    mStencilSine   = sinf(params[1]*DEG_TO_RAD);
    mStencilCosine = cosf(params[1]*DEG_TO_RAD);

    mStencilStartX = ((F32)(mImage->getWidth())  + params[0] * (F32)(mImage->getHeight()))/2.0;
    mStencilStartY = ((F32)(mImage->getHeight()) + params[1] * (F32)(mImage->getHeight()))/2.0;
    F32 end_x      = ((F32)(mImage->getWidth())  + params[2] * (F32)(mImage->getHeight()))/2.0;
    F32 end_y      = ((F32)(mImage->getHeight()) + params[3] * (F32)(mImage->getHeight()))/2.0;
    mStencilGradX  = end_x - mStencilStartX;
    mStencilGradY  = end_y - mStencilStartY;
    mStencilGradN  = mStencilGradX*mStencilGradX + mStencilGradY*mStencilGradY;
}

F32 LLImageFilter::getStencilAlpha(S32 i, S32 j)
{
    F32 alpha = 1.0;    // That init actually takes care of the STENCIL_SHAPE_UNIFORM case...
    if (mStencilShape == STENCIL_SHAPE_VIGNETTE)
    {
        // alpha is a modified gaussian value, with a center and fading in a circular pattern toward the edges
        // The gamma parameter controls the intensity of the drop down from alpha 1.0 (center) to 0.0
        F32 d_center_square = (i - mStencilCenterX)*(i - mStencilCenterX) + (j - mStencilCenterY)*(j - mStencilCenterY);
        alpha = powf(F_E, -(powf((d_center_square/(mStencilWidth*mStencilWidth)),mStencilGamma)/2.0f));
    }
    else if (mStencilShape == STENCIL_SHAPE_SCAN_LINES)
    {
        // alpha varies according to a squared sine function.
        F32 d = mStencilSine*i - mStencilCosine*j;
        alpha = (sinf(2*F_PI*d/mStencilWavelength) > 0.0 ? 1.0 : 0.0);
    }
    else if (mStencilShape == STENCIL_SHAPE_GRADIENT)
    {
        alpha = (((F32)(i) - mStencilStartX)*mStencilGradX + ((F32)(j) - mStencilStartY)*mStencilGradY) / mStencilGradN;
        alpha = llclampf(alpha);
    }
    
    // We rescale alpha between min and max
    return (mStencilMin + alpha * (mStencilMax - mStencilMin));
}

//============================================================================
// Histograms
//============================================================================

U32* LLImageFilter::getBrightnessHistogram()
{
    if (!mHistoBrightness)
    {
        computeHistograms();
    }
    return mHistoBrightness;
}

void LLImageFilter::computeHistograms()
{
 	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
    // Allocate memory for the histograms
    if (!mHistoRed)
    {
        mHistoRed = (U32*) ll_aligned_malloc_16(256*sizeof(U32));
    }
    if (!mHistoGreen)
    {
        mHistoGreen = (U32*) ll_aligned_malloc_16(256*sizeof(U32));
    }
    if (!mHistoBlue)
    {
        mHistoBlue = (U32*) ll_aligned_malloc_16(256*sizeof(U32));
    }
    if (!mHistoBrightness)
    {
        mHistoBrightness = (U32*) ll_aligned_malloc_16(256*sizeof(U32));
    }
    
    // Initialize them
    for (S32 i = 0; i < 256; i++)
    {
        mHistoRed[i] = 0;
        mHistoGreen[i] = 0;
        mHistoBlue[i] = 0;
        mHistoBrightness[i] = 0;
    }
    
    // Compute them
	S32 pixels = mImage->getWidth() * mImage->getHeight();
	U8* dst_data = mImage->getData();
	for (S32 i = 0; i < pixels; i++)
	{
        mHistoRed[dst_data[VRED]]++;
        mHistoGreen[dst_data[VGREEN]]++;
        mHistoBlue[dst_data[VBLUE]]++;
        // Note: this is a very simple shorthand for brightness but it's OK for our use
        S32 brightness = ((S32)(dst_data[VRED]) + (S32)(dst_data[VGREEN]) + (S32)(dst_data[VBLUE])) / 3;
        mHistoBrightness[brightness]++;
        // next pixel...
		dst_data += components;
	}
}

//============================================================================
// Secondary Filters
//============================================================================

void LLImageFilter::filterGrayScale()
{
    LLMatrix3 gray_scale;
    LLVector3 luminosity(0.2125, 0.7154, 0.0721);
    gray_scale.setRows(luminosity, luminosity, luminosity);
    gray_scale.transpose();
    colorTransform(gray_scale);
}

void LLImageFilter::filterSepia()
{
    LLMatrix3 sepia;
    sepia.setRows(LLVector3(0.3588, 0.7044, 0.1368),
                  LLVector3(0.2990, 0.5870, 0.1140),
                  LLVector3(0.2392, 0.4696, 0.0912));
    sepia.transpose();
    colorTransform(sepia);
}

void LLImageFilter::filterSaturate(F32 saturation)
{
    // Matrix to Lij
    LLMatrix3 r_a;
    LLMatrix3 r_b;
    
    // 45 degre rotation around z
    r_a.setRows(LLVector3( OO_SQRT2,  OO_SQRT2, 0.0),
                LLVector3(-OO_SQRT2,  OO_SQRT2, 0.0),
                LLVector3( 0.0,       0.0,      1.0));
    // 54.73 degre rotation around y
    float oo_sqrt3 = 1.0f / F_SQRT3;
    float sin_54 = F_SQRT2 * oo_sqrt3;
    r_b.setRows(LLVector3(oo_sqrt3, 0.0, -sin_54),
                LLVector3(0.0,      1.0,  0.0),
                LLVector3(sin_54,   0.0,  oo_sqrt3));
    
    // Coordinate conversion
    LLMatrix3 Lij = r_b * r_a;
    LLMatrix3 Lij_inv = Lij;
    Lij_inv.transpose();
    
    // Local saturation transform
    LLMatrix3 s;
    s.setRows(LLVector3(saturation, 0.0,  0.0),
              LLVector3(0.0,  saturation, 0.0),
              LLVector3(0.0,        0.0,  1.0));
    
    // Global saturation transform
    LLMatrix3 transfo = Lij_inv * s * Lij;
    colorTransform(transfo);
}

void LLImageFilter::filterRotate(F32 angle)
{
    // Matrix to Lij
    LLMatrix3 r_a;
    LLMatrix3 r_b;
    
    // 45 degre rotation around z
    r_a.setRows(LLVector3( OO_SQRT2,  OO_SQRT2, 0.0),
                LLVector3(-OO_SQRT2,  OO_SQRT2, 0.0),
                LLVector3( 0.0,       0.0,      1.0));
    // 54.73 degre rotation around y
    float oo_sqrt3 = 1.0f / F_SQRT3;
    float sin_54 = F_SQRT2 * oo_sqrt3;
    r_b.setRows(LLVector3(oo_sqrt3, 0.0, -sin_54),
                LLVector3(0.0,      1.0,  0.0),
                LLVector3(sin_54,   0.0,  oo_sqrt3));
    
    // Coordinate conversion
    LLMatrix3 Lij = r_b * r_a;
    LLMatrix3 Lij_inv = Lij;
    Lij_inv.transpose();
    
    // Local color rotation transform
    LLMatrix3 r;
    angle *= DEG_TO_RAD;
    r.setRows(LLVector3( cosf(angle), sinf(angle), 0.0),
              LLVector3(-sinf(angle), cosf(angle), 0.0),
              LLVector3( 0.0,         0.0,         1.0));
    
    // Global color rotation transform
    LLMatrix3 transfo = Lij_inv * r * Lij;
    colorTransform(transfo);
}

void LLImageFilter::filterGamma(F32 gamma, const LLColor3& alpha)
{
    U8 gamma_red_lut[256];
    U8 gamma_green_lut[256];
    U8 gamma_blue_lut[256];
    
    for (S32 i = 0; i < 256; i++)
    {
        F32 gamma_i = llclampf((float)(powf((float)(i)/255.0,1.0/gamma)));
        // Blend in with alpha values
        gamma_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * 255.0 * gamma_i);
        gamma_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * 255.0 * gamma_i);
        gamma_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * 255.0 * gamma_i);
    }
    
    colorCorrect(gamma_red_lut,gamma_green_lut,gamma_blue_lut);
}

void LLImageFilter::filterLinearize(F32 tail, const LLColor3& alpha)
{
    // Get the histogram
    U32* histo = getBrightnessHistogram();
    
    // Compute cumulated histogram
    U32 cumulated_histo[256];
    cumulated_histo[0] = histo[0];
    for (S32 i = 1; i < 256; i++)
    {
        cumulated_histo[i] = cumulated_histo[i-1] + histo[i];
    }
    
    // Compute min and max counts minus tail
    tail = llclampf(tail);
    S32 total = cumulated_histo[255];
    S32 min_c = (S32)((F32)(total) * tail);
    S32 max_c = (S32)((F32)(total) * (1.0 - tail));
    
    // Find min and max values
    S32 min_v = 0;
    while (cumulated_histo[min_v] < min_c)
    {
        min_v++;
    }
    S32 max_v = 255;
    while (cumulated_histo[max_v] > max_c)
    {
        max_v--;
    }
    
    // Compute linear lookup table
    U8 linear_red_lut[256];
    U8 linear_green_lut[256];
    U8 linear_blue_lut[256];
    if (max_v == min_v)
    {
        // Degenerated binary split case
        for (S32 i = 0; i < 256; i++)
        {
            U8 value_i = (i < min_v ? 0 : 255);
            // Blend in with alpha values
            linear_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * value_i);
            linear_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * value_i);
            linear_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * value_i);
        }
    }
    else
    {
        // Linearize between min and max
        F32 slope = 255.0 / (F32)(max_v - min_v);
        F32 translate = -min_v * slope;
        for (S32 i = 0; i < 256; i++)
        {
            U8 value_i = (U8)(llclampb((S32)(slope*i + translate)));
            // Blend in with alpha values
            linear_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * value_i);
            linear_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * value_i);
            linear_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * value_i);
        }
    }
    
    // Apply lookup table
    colorCorrect(linear_red_lut,linear_green_lut,linear_blue_lut);
}

void LLImageFilter::filterEqualize(S32 nb_classes, const LLColor3& alpha)
{
    // Regularize the parameter: must be between 2 and 255
    nb_classes = llmax(nb_classes,2);
    nb_classes = llclampb(nb_classes);
    
    // Get the histogram
    U32* histo = getBrightnessHistogram();
    
    // Compute cumulated histogram
    U32 cumulated_histo[256];
    cumulated_histo[0] = histo[0];
    for (S32 i = 1; i < 256; i++)
    {
        cumulated_histo[i] = cumulated_histo[i-1] + histo[i];
    }
    
    // Compute deltas
    S32 total = cumulated_histo[255];
    S32 delta_count = total / nb_classes;
    S32 current_count = delta_count;
    S32 delta_value = 256 / (nb_classes - 1);
    S32 current_value = 0;
    
    // Compute equalized lookup table
    U8 equalize_red_lut[256];
    U8 equalize_green_lut[256];
    U8 equalize_blue_lut[256];
    for (S32 i = 0; i < 256; i++)
    {
        // Blend in current_value with alpha values
        equalize_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * current_value);
        equalize_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * current_value);
        equalize_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * current_value);
        if (cumulated_histo[i] >= current_count)
        {
            current_count += delta_count;
            current_value += delta_value;
            current_value = llclampb(current_value);
        }
    }
    
    // Apply lookup table
    colorCorrect(equalize_red_lut,equalize_green_lut,equalize_blue_lut);
}

void LLImageFilter::filterColorize(const LLColor3& color, const LLColor3& alpha)
{
    U8 red_lut[256];
    U8 green_lut[256];
    U8 blue_lut[256];
    
    F32 red_composite   =  255.0 * alpha.mV[0] * color.mV[0];
    F32 green_composite =  255.0 * alpha.mV[1] * color.mV[1];
    F32 blue_composite  =  255.0 * alpha.mV[2] * color.mV[2];
    
    for (S32 i = 0; i < 256; i++)
    {
        red_lut[i]   = (U8)(llclampb((S32)((1.0 - alpha.mV[0]) * (F32)(i) + red_composite)));
        green_lut[i] = (U8)(llclampb((S32)((1.0 - alpha.mV[1]) * (F32)(i) + green_composite)));
        blue_lut[i]  = (U8)(llclampb((S32)((1.0 - alpha.mV[2]) * (F32)(i) + blue_composite)));
    }
    
    colorCorrect(red_lut,green_lut,blue_lut);
}

void LLImageFilter::filterContrast(F32 slope, const LLColor3& alpha)
{
    U8 contrast_red_lut[256];
    U8 contrast_green_lut[256];
    U8 contrast_blue_lut[256];
    
    F32 translate = 128.0 * (1.0 - slope);
    
    for (S32 i = 0; i < 256; i++)
    {
        U8 value_i = (U8)(llclampb((S32)(slope*i + translate)));
        // Blend in with alpha values
        contrast_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * value_i);
        contrast_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * value_i);
        contrast_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * value_i);
    }
    
    colorCorrect(contrast_red_lut,contrast_green_lut,contrast_blue_lut);
}

void LLImageFilter::filterBrightness(F32 add, const LLColor3& alpha)
{
    U8 brightness_red_lut[256];
    U8 brightness_green_lut[256];
    U8 brightness_blue_lut[256];
    
    S32 add_value = (S32)(add * 255.0);
    
    for (S32 i = 0; i < 256; i++)
    {
        U8 value_i = (U8)(llclampb(i + add_value));
        // Blend in with alpha values
        brightness_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * value_i);
        brightness_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * value_i);
        brightness_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * value_i);
    }
    
    colorCorrect(brightness_red_lut,brightness_green_lut,brightness_blue_lut);
}

//============================================================================
