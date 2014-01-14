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
    mFilterData(LLSD::emptyArray())
{
}

LLImageFilter::~LLImageFilter()
{
}

/*
 " -f, --filter <name> [<param>]\n"
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
 " -v, --vignette <name> [<feather> <min>]\n"
 "        Apply a circular central vignette <name> to the filter using the optional <feather> and <min> values. Admissible names:\n"
 "        - 'blend' : the filter is applied with full intensity in the center and blends with the image to the periphery.\n"
 "        - 'fade' : the filter is applied with full intensity in the center and fades to black to the periphery.\n"

 // Set the vignette if any
 if (vignette_name == "blend")
 {
 raw_image->setVignette(VIGNETTE_MODE_BLEND,VIGNETTE_TYPE_CENTER,(float)(vignette_param_1),(float)(vignette_param_2));
 }
 else if (vignette_name == "fade")
 {
 raw_image->setVignette(VIGNETTE_MODE_FADE,VIGNETTE_TYPE_CENTER,(float)(vignette_param_1),(float)(vignette_param_2));
 }
 
 // Apply filter if any
 if (filter_name == "sepia")
 {
 raw_image->filterSepia();
 }
 else if (filter_name == "grayscale")
 {
 raw_image->filterGrayScale();
 }
 else if (filter_name == "saturate")
 {
 raw_image->filterSaturate((float)(filter_param));
 }
 else if (filter_name == "rotate")
 {
 raw_image->filterRotate((float)(filter_param));
 }
 else if (filter_name == "gamma")
 {
 raw_image->filterGamma((float)(filter_param),LLColor3::white);
 }
 else if (filter_name == "colorize")
 {
 // For testing, we just colorize in the red channel, modulate by the alpha passed as a parameter
 LLColor3 color(1.0,0.0,0.0);
 LLColor3 alpha((F32)(filter_param),0.0,0.0);
 raw_image->filterColorize(color,alpha);
 }
 else if (filter_name == "contrast")
 {
 raw_image->filterContrast((float)(filter_param),LLColor3::white);
 }
 else if (filter_name == "brighten")
 {
 raw_image->filterBrightness((S32)(filter_param),LLColor3::white);
 }
 else if (filter_name == "darken")
 {
 raw_image->filterBrightness((S32)(-filter_param),LLColor3::white);
 }
 else if (filter_name == "linearize")
 {
 raw_image->filterLinearize((float)(filter_param),LLColor3::white);
 }
 else if (filter_name == "posterize")
 {
 raw_image->filterEqualize((S32)(filter_param),LLColor3::white);
 }
 else if (filter_name == "newsscreen")
 {
 raw_image->filterScreen(SCREEN_MODE_2DSINE,(S32)(filter_param),0.0);
 }
 else if (filter_name == "horizontalscreen")
 {
 raw_image->filterScreen(SCREEN_MODE_LINE,(S32)(filter_param),0.0);
 }
 else if (filter_name == "verticalscreen")
 {
 raw_image->filterScreen(SCREEN_MODE_LINE,(S32)(filter_param),90.0);
 }
 else if (filter_name == "slantedscreen")
 {
 raw_image->filterScreen(SCREEN_MODE_LINE,(S32)(filter_param),45.0);
 }
 
 */

// Load filter from file
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

// Apply the filter data to the image passed as parameter
void LLImageFilter::executeFilter(LLPointer<LLImageRaw> raw_image)
{
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
            raw_image->setVignette(VIGNETTE_MODE_BLEND,VIGNETTE_TYPE_CENTER,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
        }
        else if (filter_name == "fade")
        {
            raw_image->setVignette(VIGNETTE_MODE_FADE,VIGNETTE_TYPE_CENTER,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
        }
        else if (filter_name == "lines")
        {
            raw_image->setVignette(VIGNETTE_MODE_BLEND,VIGNETTE_TYPE_LINES,(float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()));
        }
        else if (filter_name == "sepia")
        {
            raw_image->filterSepia();
        }
        else if (filter_name == "grayscale")
        {
            raw_image->filterGrayScale();
        }
        else if (filter_name == "saturate")
        {
            raw_image->filterSaturate((float)(mFilterData[i][1].asReal()));
        }
        else if (filter_name == "rotate")
        {
            raw_image->filterRotate((float)(mFilterData[i][1].asReal()));
        }
        else if (filter_name == "gamma")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterGamma((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "colorize")
        {
            LLColor3 color((float)(mFilterData[i][1].asReal()),(float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()));
            LLColor3 alpha((F32)(mFilterData[i][4].asReal()),(float)(mFilterData[i][5].asReal()),(float)(mFilterData[i][6].asReal()));
            raw_image->filterColorize(color,alpha);
        }
        else if (filter_name == "contrast")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterContrast((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "brighten")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterBrightness((S32)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "darken")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterBrightness((S32)(-mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "linearize")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterLinearize((float)(mFilterData[i][1].asReal()),color);
        }
        else if (filter_name == "posterize")
        {
            LLColor3 color((float)(mFilterData[i][2].asReal()),(float)(mFilterData[i][3].asReal()),(float)(mFilterData[i][4].asReal()));
            raw_image->filterEqualize((S32)(mFilterData[i][1].asReal()),color);
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
            raw_image->filterScreen(mode,(S32)(mFilterData[i][2].asReal()),(F32)(mFilterData[i][3].asReal()));
        }
    }
}


//============================================================================
