/**
 * @file llenvadapters.cpp
 * @brief Declaration of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llenvadapters.h"

#include "llsettingssky.h"
//=========================================================================

LLSkySettingsAdapter::LLSkySettingsAdapter():
    mWLGamma(1.0f, LLSettingsSky::SETTING_GAMMA),
    mBlueHorizon(LLColor4(0.25f, 0.25f, 1.0f, 1.0f), LLSettingsSky::SETTING_BLUE_HORIZON, "WLBlueHorizon"),
    mHazeDensity(1.0f, LLSettingsSky::SETTING_HAZE_DENSITY),
    mBlueDensity(LLColor4(0.25f, 0.25f, 0.25f, 1.0f), LLSettingsSky::SETTING_BLUE_DENSITY, "WLBlueDensity"),
    mDensityMult(1.0f, LLSettingsSky::SETTING_DENSITY_MULTIPLIER, 1000),
    mHazeHorizon(1.0f, LLSettingsSky::SETTING_HAZE_HORIZON),
    mMaxAlt(4000.0f, LLSettingsSky::SETTING_MAX_Y),
    // Lighting
    mLightnorm(LLColor4(0.f, 0.707f, -0.707f, 1.f), LLSettingsSky::SETTING_LIGHT_NORMAL),
    mSunlight(LLColor4(0.5f, 0.5f, 0.5f, 1.0f), LLSettingsSky::SETTING_SUNLIGHT_COLOR, "WLSunlight"),
    mAmbient(LLColor4(0.5f, 0.75f, 1.0f, 1.19f), LLSettingsSky::SETTING_AMBIENT, "WLAmbient"),
    mGlow(LLColor4(18.0f, 0.0f, -0.01f, 1.0f), LLSettingsSky::SETTING_GLOW),
    // Clouds
    mCloudColor(LLColor4(0.5f, 0.5f, 0.5f, 1.0f), LLSettingsSky::SETTING_CLOUD_COLOR, "WLCloudColor"),
    mCloudMain(LLColor4(0.5f, 0.5f, 0.125f, 1.0f), LLSettingsSky::SETTING_CLOUD_POS_DENSITY1),
    mCloudCoverage(0.0f, LLSettingsSky::SETTING_CLOUD_SHADOW),
    mCloudDetail(LLColor4(0.0f, 0.0f, 0.0f, 1.0f), LLSettingsSky::SETTING_CLOUD_POS_DENSITY2),
    mDistanceMult(1.0f, LLSettingsSky::SETTING_DISTANCE_MULTIPLIER),
    mCloudScale(0.42f, LLSettingsSky::SETTING_CLOUD_SCALE)
{

}
