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

/*
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
*/
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
    LLSD mFilterData;
};


#endif
