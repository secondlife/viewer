/** 
 * @file llimagefilter.cpp
 * @brief Simple Image Filtering.
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

//---------------------------------------------------------------------------
// LLImageFilter
//---------------------------------------------------------------------------

LLImageFilter::LLImageFilter() :
    mFilterData(LLSD::emptyArray()),
    mImage(NULL),
    mHistoRed(NULL),
    mHistoGreen(NULL),
    mHistoBlue(NULL),
    mHistoBrightness(NULL),
    mVignetteMode(VIGNETTE_MODE_NONE),
    mVignetteGamma(1.0),
    mVignetteMin(0.0)
{
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
 * Rename vignette to stencil
 * Separate shape from mode
 * Add shapes : uniform and gradients
 * Add modes
 * Add stencil (min,max) range
 * Suppress alpha from colorcorrect and use uniform alpha instead
 * Refactor stencil composition in the filter primitives
 
 <array>
 <string>stencil</string>
 <string>shape</string>
 <string>blend_mode</string>
 <real>min</real>
 <real>max</real>
 <real>param1</real>
 <real>param2</real>
 <real>param3</real>
 <real>param4</real>
 </array>
 
 vignette : center_x, center_y, width, feather
 sine : wavelength, angle
 flat
 gradient : start_x, start_y, end_x, end_y

 * Document all the admissible names in the wiki
 
 "        Apply the filter <name> to the input images using the optional <param> value. Admissible names:\n"
 "        - 'grayscale' converts to grayscale (no param).\n"
 "        - 'sepia' converts to sepia (no param).\n"
 "        - 'saturate' changes color saturation according to <param>: < 1.0 will desaturate, > 1.0 will saturate.\n"
 "        - 'rotate' rotates the color hue according to <param> (in degree, positive value only).\n"
 "        - 'gamma' applies gamma curve <param> to all channels: > 1.0 will darken, < 1.0 will lighten.\n"
 "        - 'colorize' applies a red tint to the image using <param> as an alpha (transparency between 0.0 and 1.0) value.\n"
 "        - 'contrast' modifies the contrast according to <param> : > 1.0 will enhance the contrast, <1.0 will flatten it.\n"
 "        - 'brighten' adds <param> light to the image (<param> between 0 and 255).\n"
 "        - 'darken' substracts <param> light to the image (<param> between 0 and 255).\n"
 "        - 'linearize' optimizes the contrast using the brightness histogram. <param> is the fraction (between 0.0 and 1.0) of discarded tail of the histogram.\n"
 "        - 'posterize' redistributes the colors between <param> classes per channel (<param> between 2 and 255).\n"
 "        - 'newsscreen' applies a 2D sine screening to the red channel and output to black and white.\n"
 "        - 'horizontalscreen' applies a horizontal screening to the red channel and output to black and white.\n"
 "        - 'verticalscreen' applies a vertical screening to the red channel and output to black and white.\n"
 "        - 'slantedscreen' applies a 45 degrees slanted screening to the red channel and output to black and white.\n"
 "        - Any other value will be interpreted as a file name describing a sequence of filters and parameters to be applied to the input images.\n"

 "        Apply a circular central vignette <name> to the filter using the optional <feather> and <min> values. Admissible names:\n"
 "        - 'blend' : the filter is applied with full intensity in the center and blends with the image to the periphery.\n"
 "        - 'fade' : the filter is applied with full intensity in the center and fades to black to the periphery.\n"
 */

//============================================================================
// Load filter from file
//============================================================================

void LLImageFilter::loadFromFile(const std::string& file_path)
{
	//std::cout << "Loading filter settings from : " << file_path << std::endl;
	llifstream filter_xml(file_path);
	if (filter_xml.is_open())
	{
		// Load and parse the file
		LLPointer<LLSDParser> parser = new LLSDXMLParser();
		parser->parse(filter_xml, mFilterData, LLSDSerialize::SIZE_UNLIMITED);
		filter_xml.close();
	}
	else
	{
        // File couldn't be open, reset the filter data
		mFilterData = LLSD();
	}
}

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
        
        // Execute the filter described on this line
        if (filter_name == "blend")
        {
            setVignette(VIGNETTE_MODE_BLEND,VIGNETTE_TYPE_CENTER,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
        }
        else if (filter_name == "fade")
        {
            setVignette(VIGNETTE_MODE_FADE,VIGNETTE_TYPE_CENTER,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
        }
        else if (filter_name == "lines")
        {
            setVignette(VIGNETTE_MODE_BLEND,VIGNETTE_TYPE_LINES,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
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
            filterBrightness((S32)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "darken")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            filterBrightness((S32)(-mFilterData[i][1].asReal()),color);
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
            filterScreen(mode,(S32)(mFilterData[i][2].asReal()),(F32)(mFilterData[i][3].asReal()));
        }
    }
}

//============================================================================
// Filter Primitives
//============================================================================

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
            if (mVignetteMode == VIGNETTE_MODE_NONE)
            {
                dst_data[VRED]   = lut_red[dst_data[VRED]];
                dst_data[VGREEN] = lut_green[dst_data[VGREEN]];
                dst_data[VBLUE]  = lut_blue[dst_data[VBLUE]];
            }
            else
            {
                F32 alpha = getVignetteAlpha(i,j);
                if (mVignetteMode == VIGNETTE_MODE_BLEND)
                {
                    // Blends with the source image on the edges
                    F32 inv_alpha = 1.0 - alpha;
                    dst_data[VRED]   = inv_alpha * dst_data[VRED]  + alpha * lut_red[dst_data[VRED]];
                    dst_data[VGREEN] = inv_alpha * dst_data[VGREEN] + alpha * lut_green[dst_data[VGREEN]];
                    dst_data[VBLUE]  = inv_alpha * dst_data[VBLUE]  + alpha * lut_blue[dst_data[VBLUE]];
                }
                else // VIGNETTE_MODE_FADE
                {
                    // Fade to black on the edges
                    dst_data[VRED]   = alpha * lut_red[dst_data[VRED]];
                    dst_data[VGREEN] = alpha * lut_green[dst_data[VGREEN]];
                    dst_data[VBLUE]  = alpha * lut_blue[dst_data[VBLUE]];
                }
            }
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
            LLVector3 src((F32)(dst_data[VRED]),(F32)(dst_data[VGREEN]),(F32)(dst_data[VBLUE]));
            LLVector3 dst = src * transform;
            dst.clamp(0.0f,255.0f);
            if (mVignetteMode == VIGNETTE_MODE_NONE)
            {
                dst_data[VRED]   = dst.mV[VRED];
                dst_data[VGREEN] = dst.mV[VGREEN];
                dst_data[VBLUE]  = dst.mV[VBLUE];
            }
            else
            {
                F32 alpha = getVignetteAlpha(i,j);
                if (mVignetteMode == VIGNETTE_MODE_BLEND)
                {
                    // Blends with the source image on the edges
                    F32 inv_alpha = 1.0 - alpha;
                    dst_data[VRED]   = inv_alpha * src.mV[VRED]   + alpha * dst.mV[VRED];
                    dst_data[VGREEN] = inv_alpha * src.mV[VGREEN] + alpha * dst.mV[VGREEN];
                    dst_data[VBLUE]  = inv_alpha * src.mV[VBLUE]  + alpha * dst.mV[VBLUE];
                }
                else // VIGNETTE_MODE_FADE
                {
                    // Fade to black on the edges
                    dst_data[VRED]   = alpha * dst.mV[VRED];
                    dst_data[VGREEN] = alpha * dst.mV[VGREEN];
                    dst_data[VBLUE]  = alpha * dst.mV[VBLUE];
                }
            }
            dst_data += components;
        }
	}
}

void LLImageFilter::filterScreen(EScreenMode mode, const S32 wave_length, const F32 angle)
{
	const S32 components = mImage->getComponents();
	llassert( components >= 1 && components <= 4 );
    
	S32 width  = mImage->getWidth();
    S32 height = mImage->getHeight();
    
    F32 sin = sinf(angle*DEG_TO_RAD);
    F32 cos = cosf(angle*DEG_TO_RAD);
    
	U8* dst_data = mImage->getData();
	for (S32 j = 0; j < height; j++)
	{
        for (S32 i = 0; i < width; i++)
        {
            F32 value = 0.0;
            F32 d = 0.0;
            switch (mode)
            {
                case SCREEN_MODE_2DSINE:
                    value = (sinf(2*F_PI*i/wave_length)*sinf(2*F_PI*j/wave_length)+1.0)*255.0/2.0;
                    break;
                case SCREEN_MODE_LINE:
                    d = sin*i - cos*j;
                    value = (sinf(2*F_PI*d/wave_length)+1.0)*255.0/2.0;
                    break;
            }
            U8 dst_value = (dst_data[VRED] >= (U8)(value) ? 255 : 0);
            
            if (mVignetteMode == VIGNETTE_MODE_NONE)
            {
                dst_data[VRED]   = dst_value;
                dst_data[VGREEN] = dst_value;
                dst_data[VBLUE]  = dst_value;
            }
            else
            {
                F32 alpha = getVignetteAlpha(i,j);
                if (mVignetteMode == VIGNETTE_MODE_BLEND)
                {
                    // Blends with the source image on the edges
                    F32 inv_alpha = 1.0 - alpha;
                    dst_data[VRED]   = inv_alpha * dst_data[VRED]   + alpha * dst_value;
                    dst_data[VGREEN] = inv_alpha * dst_data[VGREEN] + alpha * dst_value;
                    dst_data[VBLUE]  = inv_alpha * dst_data[VBLUE]  + alpha * dst_value;
                }
                else // VIGNETTE_MODE_FADE
                {
                    // Fade to black on the edges
                    dst_data[VRED]   = alpha * dst_value;
                    dst_data[VGREEN] = alpha * dst_value;
                    dst_data[VBLUE]  = alpha * dst_value;
                }
            }
            dst_data += components;
        }
	}
}

//============================================================================
// Procedural Stencils
//============================================================================

void LLImageFilter::setVignette(EVignetteMode mode, EVignetteType type, F32 gamma, F32 min)
{
    mVignetteMode = mode;
    mVignetteType = type;
    mVignetteGamma = gamma;
    mVignetteMin = llclampf(min);
    // We always center the vignette on the image and fits it in the image smallest dimension
    mVignetteCenterX = mImage->getWidth()/2;
    mVignetteCenterY = mImage->getHeight()/2;
    mVignetteWidth = llmin(mImage->getWidth()/2,mImage->getHeight()/2);
}

F32 LLImageFilter::getVignetteAlpha(S32 i, S32 j)
{
    F32 alpha = 1.0;
    if (mVignetteType == VIGNETTE_TYPE_CENTER)
    {
        // alpha is a modified gaussian value, with a center and fading in a circular pattern toward the edges
        // The gamma parameter controls the intensity of the drop down from alpha 1.0 (center) to 0.0
        F32 d_center_square = (i - mVignetteCenterX)*(i - mVignetteCenterX) + (j - mVignetteCenterY)*(j - mVignetteCenterY);
        alpha = powf(F_E, -(powf((d_center_square/(mVignetteWidth*mVignetteWidth)),mVignetteGamma)/2.0f));
    }
    else if (mVignetteType == VIGNETTE_TYPE_LINES)
    {
        // alpha varies according to a squared sine function vertically.
        // gamma is interpreted as the wavelength (in pixels) of the sine in that case.
        alpha = (sinf(2*F_PI*j/mVignetteGamma) > 0.0 ? 1.0 : 0.0);
    }
    // We rescale alpha between min and 1.0 so to avoid complete fading if so desired.
    return (mVignetteMin + alpha * (1.0 - mVignetteMin));
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
        F32 gamma_i = llclampf((float)(powf((float)(i)/255.0,gamma)));
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

void LLImageFilter::filterBrightness(S32 add, const LLColor3& alpha)
{
    U8 brightness_red_lut[256];
    U8 brightness_green_lut[256];
    U8 brightness_blue_lut[256];
    
    for (S32 i = 0; i < 256; i++)
    {
        U8 value_i = (U8)(llclampb((S32)((S32)(i) + add)));
        // Blend in with alpha values
        brightness_red_lut[i]   = (U8)((1.0 - alpha.mV[0]) * (float)(i) + alpha.mV[0] * value_i);
        brightness_green_lut[i] = (U8)((1.0 - alpha.mV[1]) * (float)(i) + alpha.mV[1] * value_i);
        brightness_blue_lut[i]  = (U8)((1.0 - alpha.mV[2]) * (float)(i) + alpha.mV[2] * value_i);
    }
    
    colorCorrect(brightness_red_lut,brightness_green_lut,brightness_blue_lut);
}

//============================================================================
