/** 
 * @file llimagefilter.h
 * @brief Simple Image Filtering.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLIMAGEFILTER_H
#define LL_LLIMAGEFILTER_H

#include "llimage.h"

class LLImageRaw;
class LLColor4U;
class LLColor3;
class LLMatrix3;

typedef enum e_vignette_mode
{
	VIGNETTE_MODE_NONE  = 0,
	VIGNETTE_MODE_BLEND = 1,
	VIGNETTE_MODE_FADE  = 2
} EVignetteMode;

typedef enum e_vignette_type
{
	VIGNETTE_TYPE_CENTER = 0,
	VIGNETTE_TYPE_LINES  = 1
} EVignetteType;

typedef enum e_screen_mode
{
	SCREEN_MODE_2DSINE   = 0,
	SCREEN_MODE_LINE     = 1
} EScreenMode;

//============================================================================
// Image Filter 

class LLImageFilter
{
public:
    LLImageFilter();
    ~LLImageFilter();
    
    void loadFromFile(const std::string& file_path);
    void executeFilter(LLPointer<LLImageRaw> raw_image);
    
private:
    // Filter Operations : Transforms
    void filterGrayScale();                         // Convert to grayscale
    void filterSepia();                             // Convert to sepia
    void filterSaturate(F32 saturation);            // < 1.0 desaturates, > 1.0 saturates
    void filterRotate(F32 angle);                   // Rotates hue according to angle, angle in degrees
    
    // Filter Operations : Color Corrections
    // When specified, the LLColor3 alpha parameter indicates the intensity of the effect for each color channel
    // acting in effect as an alpha blending factor different for each channel. For instance (1.0,0.0,0.0) will apply
    // the effect only to the Red channel. Intermediate values blends the effect with the source color.
    void filterGamma(F32 gamma, const LLColor3& alpha);         // Apply gamma to each channel
    void filterLinearize(F32 tail, const LLColor3& alpha);      // Use histogram to linearize constrast between min and max values minus tail
    void filterEqualize(S32 nb_classes, const LLColor3& alpha); // Use histogram to equalize constrast between nb_classes throughout the image
    void filterColorize(const LLColor3& color, const LLColor3& alpha);  // Colorize with color and alpha per channel
    void filterContrast(F32 slope, const LLColor3& alpha);      // Change contrast according to slope: > 1.0 more contrast, < 1.0 less contrast
    void filterBrightness(S32 add, const LLColor3& alpha);      // Change brightness according to add: > 0 brighter, < 0 darker
    
    // Filter Primitives
    void colorTransform(const LLMatrix3 &transform);
    void colorCorrect(const U8* lut_red, const U8* lut_green, const U8* lut_blue);
    void filterScreen(EScreenMode mode, const S32 wave_length, const F32 angle);

    // Procedural Stencils
    void setVignette(EVignetteMode mode, EVignetteType type, F32 gamma, F32 min);
    F32 getVignetteAlpha(S32 i, S32 j);

    // Histograms
    U32* getBrightnessHistogram();
    void computeHistograms();

    LLSD mFilterData;
    LLPointer<LLImageRaw> mImage;

    // Histograms (if we ever happen to need them)
    U32 *mHistoRed;
    U32 *mHistoGreen;
    U32 *mHistoBlue;
    U32 *mHistoBrightness;
    
    // Vignette filtering
    EVignetteMode mVignetteMode;
    EVignetteType mVignetteType;
    S32 mVignetteCenterX;
    S32 mVignetteCenterY;
    S32 mVignetteWidth;
    F32 mVignetteGamma;
    F32 mVignetteMin;
};


#endif
