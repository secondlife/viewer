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
#include "llsettingswater.h"
//=========================================================================

LLSkySettingsAdapter::LLSkySettingsAdapter():
    mWLGamma(1.0f, LLSettingsSky::SETTING_GAMMA),
    
    // Lighting
    mLightnorm(LLColor4(0.f, 0.707f, -0.707f, 1.f), LLSettingsSky::SETTING_LIGHT_NORMAL),
    mSunlight(LLColor4(0.5f, 0.5f, 0.5f, 1.0f), LLSettingsSky::SETTING_SUNLIGHT_COLOR, "WLSunlight"),
    
    mGlow(LLColor4(18.0f, 0.0f, -0.01f, 1.0f), LLSettingsSky::SETTING_GLOW),
    // Clouds
    mCloudColor(LLColor4(0.5f, 0.5f, 0.5f, 1.0f), LLSettingsSky::SETTING_CLOUD_COLOR, "WLCloudColor"),
    mCloudMain(LLColor4(0.5f, 0.5f, 0.125f, 1.0f), LLSettingsSky::SETTING_CLOUD_POS_DENSITY1),
    mCloudCoverage(0.0f, LLSettingsSky::SETTING_CLOUD_SHADOW),
    mCloudDetail(LLColor4(0.0f, 0.0f, 0.0f, 1.0f), LLSettingsSky::SETTING_CLOUD_POS_DENSITY2),    
    mCloudScale(0.42f, LLSettingsSky::SETTING_CLOUD_SCALE)
{

}

LLWatterSettingsAdapter::LLWatterSettingsAdapter():
    mFogColor(LLColor4((22.f / 255.f), (43.f / 255.f), (54.f / 255.f), (0.0f)), LLSettingsWater::SETTING_FOG_COLOR, "WaterFogColor"),
    mFogDensity(4, LLSettingsWater::SETTING_FOG_DENSITY, 2),
    mUnderWaterFogMod(0.25, LLSettingsWater::SETTING_FOG_MOD),
    mNormalScale(LLVector3(2.f, 2.f, 2.f), LLSettingsWater::SETTING_NORMAL_SCALE),
    mFresnelScale(0.5f, LLSettingsWater::SETTING_FRESNEL_SCALE),
    mFresnelOffset(0.4f, LLSettingsWater::SETTING_FRESNEL_OFFSET),
    mScaleAbove(0.025f, LLSettingsWater::SETTING_SCALE_ABOVE),
    mScaleBelow(0.2f, LLSettingsWater::SETTING_SCALE_BELOW),
    mBlurMultiplier(0.1f, LLSettingsWater::SETTING_BLUR_MULTIPILER),
    mWave1Dir(LLVector2(0.5f, 0.5f), LLSettingsWater::SETTING_WAVE1_DIR),
    mWave2Dir(LLVector2(0.5f, 0.5f), LLSettingsWater::SETTING_WAVE2_DIR)

{

}
