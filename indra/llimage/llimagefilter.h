/** 
 * @file llimagefilter.h
 * @brief Simple Image Filtering. See https://wiki.lindenlab.com/wiki/SL_Viewer_Image_Filters for complete documentation.
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

#include "llsd.h"
#include "llimage.h"

class LLImageRaw;
class LLColor4U;
class LLColor3;
class LLMatrix3;

typedef enum e_stencil_blend_mode
{
    STENCIL_BLEND_MODE_BLEND = 0,
    STENCIL_BLEND_MODE_ADD   = 1,
    STENCIL_BLEND_MODE_ABACK = 2,
    STENCIL_BLEND_MODE_FADE  = 3
} EStencilBlendMode;

typedef enum e_stencil_shape
{
    STENCIL_SHAPE_UNIFORM    = 0,
    STENCIL_SHAPE_GRADIENT   = 1,
    STENCIL_SHAPE_VIGNETTE   = 2,
    STENCIL_SHAPE_SCAN_LINES = 3
} EStencilShape;

typedef enum e_screen_mode
{
    SCREEN_MODE_2DSINE   = 0,
    SCREEN_MODE_LINE     = 1
} EScreenMode;

//============================================================================
// LLImageFilter 
//============================================================================

class LLImageFilter
{
public:
    LLImageFilter(const std::string& file_path);
    ~LLImageFilter();
    
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
    void filterBrightness(F32 add, const LLColor3& alpha);      // Change brightness according to add: > 0 brighter, < 0 darker
    
    // Filter Primitives
    void colorTransform(const LLMatrix3 &transform);
    void colorCorrect(const U8* lut_red, const U8* lut_green, const U8* lut_blue);
    void filterScreen(EScreenMode mode, const F32 wave_length, const F32 angle);
    void blendStencil(F32 alpha, U8* pixel, U8 red, U8 green, U8 blue);
    void convolve(const LLMatrix3 &kernel, bool normalize, bool abs_value);

    // Procedural Stencils
    void setStencil(EStencilShape shape, EStencilBlendMode mode, F32 min, F32 max, F32* params);
    F32 getStencilAlpha(S32 i, S32 j);

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
    
    // Current Stencil Settings
    EStencilBlendMode mStencilBlendMode;
    EStencilShape mStencilShape;
    F32 mStencilMin;
    F32 mStencilMax;
    
    S32 mStencilCenterX;
    S32 mStencilCenterY;
    S32 mStencilWidth;
    F32 mStencilGamma;
    
    F32 mStencilWavelength;
    F32 mStencilSine;
    F32 mStencilCosine;
    
    F32 mStencilStartX;
    F32 mStencilStartY;
    F32 mStencilGradX;
    F32 mStencilGradY;
    F32 mStencilGradN;
};


#endif
