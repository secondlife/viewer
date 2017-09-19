/**
* @file llsettingssky.cpp
* @author optional
* @brief A base class for asset based settings groups.
*
* $LicenseInfo:2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2017, Linden Research, Inc.
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
#include "llviewercontrol.h"
#include "llsettingssky.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

#include "llglslshader.h"
#include "llviewershadermgr.h"

#include "llsky.h"

//=========================================================================
namespace
{
    const LLVector3 DUE_EAST(-1.0f, 0.0f, 0.0);

    LLTrace::BlockTimerStatHandle FTM_BLEND_ENVIRONMENT("Blending Environment Params");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_ENVIRONMENT("Update Environment Params");

}

//=========================================================================
const std::string LLSettingsSky::SETTING_AMBIENT("ambient");
const std::string LLSettingsSky::SETTING_BLOOM_TEXTUREID("bloom_id");
const std::string LLSettingsSky::SETTING_BLUE_DENSITY("blue_density");
const std::string LLSettingsSky::SETTING_BLUE_HORIZON("blue_horizon");
const std::string LLSettingsSky::SETTING_CLOUD_COLOR("cloud_color");
const std::string LLSettingsSky::SETTING_CLOUD_POS_DENSITY1("cloud_pos_density1");
const std::string LLSettingsSky::SETTING_CLOUD_POS_DENSITY2("cloud_pos_density2");
const std::string LLSettingsSky::SETTING_CLOUD_SCALE("cloud_scale");
const std::string LLSettingsSky::SETTING_CLOUD_SCROLL_RATE("cloud_scroll_rate");
const std::string LLSettingsSky::SETTING_CLOUD_SHADOW("cloud_shadow");
const std::string LLSettingsSky::SETTING_CLOUD_TEXTUREID("cloud_id");
const std::string LLSettingsSky::SETTING_DENSITY_MULTIPLIER("density_multiplier");
const std::string LLSettingsSky::SETTING_DISTANCE_MULTIPLIER("distance_multiplier");
const std::string LLSettingsSky::SETTING_DOME_OFFSET("dome_offset");
const std::string LLSettingsSky::SETTING_DOME_RADIUS("dome_radius");
const std::string LLSettingsSky::SETTING_GAMMA("gamma");
const std::string LLSettingsSky::SETTING_GLOW("glow");
const std::string LLSettingsSky::SETTING_HAZE_DENSITY("haze_density");
const std::string LLSettingsSky::SETTING_HAZE_HORIZON("haze_horizon");
const std::string LLSettingsSky::SETTING_LIGHT_NORMAL("lightnorm");
const std::string LLSettingsSky::SETTING_MAX_Y("max_y");
const std::string LLSettingsSky::SETTING_MOON_ROTATION("moon_rotation");
const std::string LLSettingsSky::SETTING_MOON_TEXTUREID("moon_id");
const std::string LLSettingsSky::SETTING_NAME("name");
const std::string LLSettingsSky::SETTING_STAR_BRIGHTNESS("star_brightness");
const std::string LLSettingsSky::SETTING_SUNLIGHT_COLOR("sunlight_color");
const std::string LLSettingsSky::SETTING_SUN_ROTATION("sun_rotation");
const std::string LLSettingsSky::SETTING_SUN_TEXUTUREID("sun_id");

const std::string LLSettingsSky::SETTING_LEGACY_EAST_ANGLE("east_angle");
const std::string LLSettingsSky::SETTING_LEGACY_ENABLE_CLOUD_SCROLL("enable_cloud_scroll");
const std::string LLSettingsSky::SETTING_LEGACY_SUN_ANGLE("sun_angle");


//=========================================================================
LLSettingsSky::LLSettingsSky(const LLSD &data) :
    LLSettingsBase(data)
{
}

LLSettingsSky::LLSettingsSky():
    LLSettingsBase()
{
}

LLSettingsSky::stringset_t LLSettingsSky::getSlerpKeys() const 
{ 
    static stringset_t slepSet;

    if (slepSet.empty())
    {
        slepSet.insert(SETTING_SUN_ROTATION);
        slepSet.insert(SETTING_MOON_ROTATION);
    }

    return slepSet;
}


LLSettingsSky::ptr_t LLSettingsSky::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(LLSD::emptyMap());

    newsettings[SETTING_NAME] = name;

    if (oldsettings.has(SETTING_AMBIENT))
    {
        newsettings[SETTING_AMBIENT] = LLColor3(oldsettings[SETTING_AMBIENT]).getValue();
    }

    if (oldsettings.has(SETTING_BLUE_DENSITY))
    {
        newsettings[SETTING_BLUE_DENSITY] = LLColor3(oldsettings[SETTING_BLUE_DENSITY]).getValue();
    }

    if (oldsettings.has(SETTING_BLUE_HORIZON))
    {
        newsettings[SETTING_BLUE_HORIZON] = LLColor3(oldsettings[SETTING_BLUE_HORIZON]).getValue();
    }

    if (oldsettings.has(SETTING_CLOUD_COLOR))
    {
        newsettings[SETTING_CLOUD_COLOR] = LLColor4(oldsettings[SETTING_CLOUD_COLOR]).getValue();
    }
    if (oldsettings.has(SETTING_SUNLIGHT_COLOR))
    {
        newsettings[SETTING_SUNLIGHT_COLOR] = LLColor4(oldsettings[SETTING_SUNLIGHT_COLOR]).getValue();
    }
    if (oldsettings.has(SETTING_CLOUD_SHADOW))
    {
        newsettings[SETTING_CLOUD_SHADOW] = LLSD::Real(oldsettings[SETTING_CLOUD_SHADOW][0].asReal());
    }

    if (oldsettings.has(SETTING_CLOUD_POS_DENSITY1))
    {
        newsettings[SETTING_CLOUD_POS_DENSITY1] = LLColor4(oldsettings[SETTING_CLOUD_POS_DENSITY1]).getValue();
    }
    if (oldsettings.has(SETTING_CLOUD_POS_DENSITY2))
    {
        newsettings[SETTING_CLOUD_POS_DENSITY2] = LLColor4(oldsettings[SETTING_CLOUD_POS_DENSITY2]).getValue();
    }
    if (oldsettings.has(SETTING_LIGHT_NORMAL))
    {
        newsettings[SETTING_LIGHT_NORMAL] = LLVector4(oldsettings[SETTING_LIGHT_NORMAL]).getValue();
    }

    if (oldsettings.has(SETTING_CLOUD_SCALE))
    {
        newsettings[SETTING_CLOUD_SCALE] = LLSD::Real(oldsettings[SETTING_CLOUD_SCALE][0].asReal());
    }

    if (oldsettings.has(SETTING_DENSITY_MULTIPLIER))
    {
        newsettings[SETTING_DENSITY_MULTIPLIER] = LLSD::Real(oldsettings[SETTING_DENSITY_MULTIPLIER][0].asReal());
    }
    if (oldsettings.has(SETTING_DISTANCE_MULTIPLIER))
    {
        newsettings[SETTING_DISTANCE_MULTIPLIER] = LLSD::Real(oldsettings[SETTING_DISTANCE_MULTIPLIER][0].asReal());
    }
    if (oldsettings.has(SETTING_HAZE_DENSITY))
    {
        newsettings[SETTING_HAZE_DENSITY] = LLSD::Real(oldsettings[SETTING_HAZE_DENSITY][0].asReal());
    }
    if (oldsettings.has(SETTING_HAZE_HORIZON))
    {
        newsettings[SETTING_HAZE_HORIZON] = LLSD::Real(oldsettings[SETTING_HAZE_HORIZON][0].asReal());
    }
    if (oldsettings.has(SETTING_MAX_Y))
    {
        newsettings[SETTING_MAX_Y] = LLSD::Real(oldsettings[SETTING_MAX_Y][0].asReal());
    }
    if (oldsettings.has(SETTING_STAR_BRIGHTNESS))
    {
        newsettings[SETTING_STAR_BRIGHTNESS] = LLSD::Real(oldsettings[SETTING_STAR_BRIGHTNESS].asReal());
    }

    if (oldsettings.has(SETTING_GLOW))
    {
        newsettings[SETTING_GLOW] = LLColor3(oldsettings[SETTING_GLOW]).getValue();
    }

    if (oldsettings.has(SETTING_GAMMA))
    {
        newsettings[SETTING_GAMMA] = LLSD::Real(oldsettings[SETTING_GAMMA][0].asReal());
    }

    if (oldsettings.has(SETTING_CLOUD_SCROLL_RATE))
    {
        LLVector2 cloud_scroll(oldsettings[SETTING_CLOUD_SCROLL_RATE]);

        if (oldsettings.has(SETTING_LEGACY_ENABLE_CLOUD_SCROLL))
        {
            LLSD enabled = oldsettings[SETTING_LEGACY_ENABLE_CLOUD_SCROLL];
            if (!enabled[0].asBoolean())
                cloud_scroll.mV[0] = 0.0f;
            if (!enabled[1].asBoolean())
                cloud_scroll.mV[1] = 0.0f;
        }

        newsettings[SETTING_CLOUD_SCROLL_RATE] = cloud_scroll.getValue();
    }


    if (oldsettings.has(SETTING_LEGACY_EAST_ANGLE) && oldsettings.has(SETTING_LEGACY_SUN_ANGLE))
    {   // convert the east and sun angles into a quaternion.
        F32 east = oldsettings[SETTING_LEGACY_EAST_ANGLE].asReal();
        F32 azimuth = oldsettings[SETTING_LEGACY_SUN_ANGLE].asReal();

        LLQuaternion sunquat;
        sunquat.setEulerAngles(azimuth, 0.0, east);
//         // set the sun direction from SunAngle and EastAngle
//         F32 sinTheta = sin(east);
//         F32 cosTheta = cos(east);
// 
//         F32 sinPhi = sin(azimuth);
//         F32 cosPhi = cos(azimuth);
// 
//         LLVector4 sunDir;
//         sunDir.mV[0] = -sinTheta * cosPhi;
//         sunDir.mV[1] = sinPhi;
//         sunDir.mV[2] = cosTheta * cosPhi;
//         sunDir.mV[3] = 0;
// 
//         LLQuaternion sunquat = LLQuaternion(0.1, sunDir);   // small rotation around axis
        LLQuaternion moonquat = ~sunquat;

        newsettings[SETTING_SUN_ROTATION] = sunquat.getValue();
        newsettings[SETTING_MOON_ROTATION] = moonquat.getValue();
    }

    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsSky>(newsettings);
    skyp->update();

    return skyp;    
}

LLSettingsSky::ptr_t LLSettingsSky::buildDefaultSky()
{
    LLSD settings = LLSettingsSky::defaults();

    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsSky>(settings);
    skyp->update();

    return skyp;
}


// Settings status 

LLSettingsSky::ptr_t LLSettingsSky::blend(const LLSettingsSky::ptr_t &other, F32 mix) const
{
    LL_RECORD_BLOCK_TIME(FTM_BLEND_ENVIRONMENT);
    LL_INFOS("WINDLIGHT", "SKY", "EEP") << "Blending new sky settings object." << LL_ENDL;

    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsSky>(mSettings);
    // the settings in the initial constructor are references tho this' settings block.  
    // They will be replaced in the following lerp
    skyp->lerpSettings(*other, mix);

    return skyp;
}


LLSD LLSettingsSky::defaults()
{
    LLSD dfltsetting;

    
    LLQuaternion sunquat;
    sunquat.setEulerAngles(1.39626, 0.0, 0.0); // 80deg Azumith/0deg East
    LLQuaternion moonquat = ~sunquat;

    // Magic constants copied form dfltsetting.xml 
    dfltsetting[SETTING_AMBIENT]            = LLColor3::white.getValue();
    dfltsetting[SETTING_BLUE_DENSITY]       = LLColor3(0.2447, 0.4487, 0.7599).getValue();
    dfltsetting[SETTING_BLUE_HORIZON]       = LLColor3(0.4954, 0.4954, 0.6399).getValue();
    dfltsetting[SETTING_CLOUD_COLOR]        = LLColor3(0.4099, 0.4099, 0.4099).getValue();
    dfltsetting[SETTING_CLOUD_POS_DENSITY1] = LLColor3(1.0000, 0.5260, 1.0000).getValue();
    dfltsetting[SETTING_CLOUD_POS_DENSITY2] = LLColor3(1.0000, 0.5260, 1.0000).getValue();
    dfltsetting[SETTING_CLOUD_SCALE]        = LLSD::Real(0.4199);
    dfltsetting[SETTING_CLOUD_SCROLL_RATE]  = LLSDArray(10.1999)(10.0109);
    dfltsetting[SETTING_CLOUD_SHADOW]       = LLColor3(0.2699, 0.0000, 0.0000).getValue();
    dfltsetting[SETTING_DENSITY_MULTIPLIER] = LLSD::Real(0.0001);
    dfltsetting[SETTING_DISTANCE_MULTIPLIER] = LLSD::Real(0.8000);
    dfltsetting[SETTING_DOME_OFFSET]        = LLSD::Real(1.0);
    dfltsetting[SETTING_DOME_RADIUS]        = LLSD::Real(0.0);
    dfltsetting[SETTING_GAMMA]              = LLSD::Real(1.0000);
    dfltsetting[SETTING_GLOW]               = LLColor3(5.000, 0.0010, -0.4799).getValue();   // *RIDER: This is really weird for a color... TODO: check if right.
    dfltsetting[SETTING_HAZE_DENSITY]       = LLSD::Real(0.6999);
    dfltsetting[SETTING_HAZE_HORIZON]       = LLSD::Real(0.1899);
    dfltsetting[SETTING_LIGHT_NORMAL]       = LLVector4(0.0000, 0.9126, -0.4086, 0.0000).getValue();
    dfltsetting[SETTING_MAX_Y]              = LLSD::Real(1605);
    dfltsetting[SETTING_MOON_ROTATION]      = moonquat.getValue();
    dfltsetting[SETTING_NAME]               = std::string("_default_");
    dfltsetting[SETTING_STAR_BRIGHTNESS]    = LLSD::Real(0.0000);
    dfltsetting[SETTING_SUNLIGHT_COLOR]     = LLColor3(0.7342, 0.7815, 0.8999).getValue();
    dfltsetting[SETTING_SUN_ROTATION]       = sunquat.getValue();

    dfltsetting[SETTING_BLOOM_TEXTUREID]    = LLUUID::null;
    dfltsetting[SETTING_CLOUD_TEXTUREID]    = LLUUID::null;
    dfltsetting[SETTING_MOON_TEXTUREID]     = IMG_SUN; // gMoonTextureID;   // These two are returned by the login... wow!
    dfltsetting[SETTING_SUN_TEXUTUREID]     = IMG_MOON; // gSunTextureID;

    return dfltsetting;
}

void LLSettingsSky::updateSettings()
{
    LL_RECORD_BLOCK_TIME(FTM_UPDATE_ENVIRONMENT);
    LL_INFOS("WINDLIGHT", "SKY", "EEP") << "WL Parameters are dirty.  Reticulating Splines..." << LL_ENDL;

    // base class clears dirty flag so as to not trigger recursive update
    LLSettingsBase::updateSettings();

    calculateHeavnlyBodyPositions();
    calculateLightSettings();
}

void LLSettingsSky::calculateHeavnlyBodyPositions()
{
    mSunDirection = DUE_EAST * getSunRotation();
    mSunDirection.normalize();
    mMoonDirection = DUE_EAST * getMoonRotation();
    mMoonDirection.normalize();


    // is the normal from the sun or the moon
    if (mSunDirection.mV[1] >= 0.0)
    {
        mLightDirection = mSunDirection;
    }
    else if (mSunDirection.mV[1] < 0.0 && mSunDirection.mV[1] > LLSky::NIGHTTIME_ELEVATION_COS)
    {
        // clamp v1 to 0 so sun never points up and causes weirdness on some machines
        LLVector3 vec(mSunDirection);
        vec.mV[1] = 0.0;
        vec.normVec();
        mLightDirection = vec;
    }
    else
    {
        mLightDirection = mMoonDirection;
    }

    // calculate the clamp lightnorm for sky (to prevent ugly banding in sky
    // when haze goes below the horizon
    mLightDirectionClamped = mSunDirection;

    if (mLightDirectionClamped.mV[1] < -0.1f)
    {
        mLightDirectionClamped.mV[1] = -0.1f;
    }
}

void LLSettingsSky::calculateLightSettings()
{
    LLColor3 vary_HazeColor;
    LLColor3 vary_SunlightColor;
    LLColor3 vary_AmbientColor;
    {
        // Initialize temp variables
        LLColor3 sunlight = getSunlightColor();

        // Fetch these once...
        F32 haze_density = getHazeDensity();
        F32 haze_horizon = getHazeHorizon();
        F32 density_multiplier = getDensityMultiplier();
        F32 max_y = getMaxY();
        F32 gamma = getGama();
        F32 cloud_shadow = getCloudShadow();
        LLColor3 blue_density = getBlueDensity();
        LLColor3 blue_horizon = getBlueHorizon();
        LLColor3 ambient = getAmbientColor();


        // Sunlight attenuation effect (hue and brightness) due to atmosphere
        // this is used later for sunlight modulation at various altitudes
        LLColor3 light_atten =  
            (blue_density * 1.0 + smear(haze_density * 0.25f)) * (density_multiplier * max_y);

        // Calculate relative weights
        LLColor3 temp2(0.f, 0.f, 0.f);
        LLColor3 temp1 = blue_density + smear(haze_density);
        LLColor3 blue_weight = componentDiv(blue_density, temp1);
        LLColor3 haze_weight = componentDiv(smear(haze_density), temp1);

        // Compute sunlight from P & lightnorm (for long rays like sky)
        /// USE only lightnorm.
        // temp2[1] = llmax(0.f, llmax(0.f, Pn[1]) * 1.0f + lightnorm[1] );

        // and vary_sunlight will work properly with moon light
        F32 lighty = mLightDirection[1];
        if (lighty < LLSky::NIGHTTIME_ELEVATION_COS)
        {
            lighty = -lighty;
        }

        temp2.mV[1] = llmax(0.f, lighty);
        if (temp2.mV[1] > 0.f)
        {
            temp2.mV[1] = 1.f / temp2.mV[1];
        }
        componentMultBy(sunlight, componentExp((light_atten * -1.f) * temp2.mV[1]));

        // Distance
        temp2.mV[2] = density_multiplier;

        // Transparency (-> temp1)
        temp1 = componentExp((temp1 * -1.f) * temp2.mV[2]);

        // vary_AtmosAttenuation = temp1; 

        //increase ambient when there are more clouds
        LLColor3 tmpAmbient = ambient + (smear(1.f) - ambient) * cloud_shadow * 0.5f;

        //haze color
        vary_HazeColor =
            (blue_horizon * blue_weight * (sunlight*(1.f - cloud_shadow) + tmpAmbient)
            + componentMult(haze_horizon * haze_weight, sunlight*(1.f - cloud_shadow) * temp2.mV[0] + tmpAmbient)
            );

        //brightness of surface both sunlight and ambient
        vary_SunlightColor = componentMult(sunlight, temp1) * 1.f;
        vary_SunlightColor.clamp();
        vary_SunlightColor = smear(1.0f) - vary_SunlightColor;
        vary_SunlightColor = componentPow(vary_SunlightColor, gamma);
        vary_SunlightColor = smear(1.0f) - vary_SunlightColor;
        vary_AmbientColor = componentMult(tmpAmbient, temp1) * 0.5;
        vary_AmbientColor.clamp();
        vary_AmbientColor = smear(1.0f) - vary_AmbientColor;
        vary_AmbientColor = componentPow(vary_AmbientColor, gamma);
        vary_AmbientColor = smear(1.0f) - vary_AmbientColor;

        componentMultBy(vary_HazeColor, LLColor3(1.f, 1.f, 1.f) - temp1);

    }

    float dp = getSunDirection() * LLVector3(0, 0, 1.f); // a dot b
    if (dp < 0)
    {
        dp = 0;
    }

    // Since WL scales everything by 2, there should always be at least a 2:1 brightness ratio
    // between sunlight and point lights in windlight to normalize point lights.
    F32 sun_dynamic_range = std::max(gSavedSettings.getF32("RenderSunDynamicRange"), 0.0001f);
    
    mSceneLightStrength = 2.0f * (1.0f + sun_dynamic_range * dp);

    mSunDiffuse = vary_SunlightColor;
    mSunAmbient = vary_AmbientColor;
    mMoonDiffuse = vary_SunlightColor;
    mMoonAmbient = vary_AmbientColor;

    mTotalAmbient = vary_AmbientColor;
    mTotalAmbient.setAlpha(1);

    mFadeColor = mTotalAmbient + (mSunDiffuse + mMoonDiffuse) * 0.5f;
    mFadeColor.setAlpha(0);

}


LLSettingsSky::stringset_t LLSettingsSky::getSkipApplyKeys() const
{

    static stringset_t skip_apply_set;

    if (skip_apply_set.empty())
    {
        skip_apply_set.insert(SETTING_GAMMA);
        skip_apply_set.insert(SETTING_MOON_ROTATION);
        skip_apply_set.insert(SETTING_SUN_ROTATION);
        skip_apply_set.insert(SETTING_NAME);
        skip_apply_set.insert(SETTING_STAR_BRIGHTNESS);
        skip_apply_set.insert(SETTING_CLOUD_SCROLL_RATE);
        skip_apply_set.insert(SETTING_LIGHT_NORMAL);
        skip_apply_set.insert(SETTING_CLOUD_POS_DENSITY1);
    }

    return skip_apply_set;
}

void LLSettingsSky::applySpecial(void *ptarget)
{
    LLGLSLShader *shader = (LLGLSLShader *)ptarget;

    if (shader->mShaderGroup == LLGLSLShader::SG_SKY)
    {
        shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, mLightDirectionClamped.mV);
    }

    shader->uniform1f(LLShaderMgr::SCENE_LIGHT_STRENGTH, mSceneLightStrength);
    
    shader->uniform4f(LLShaderMgr::GAMMA, getGama(), 0.0, 0.0, 1.0);
}

//-------------------------------------------------------------------------
// const std::string LLSettingsSky::SETTING_DENSITY_MULTIPLIER("density_multiplier");
// const std::string LLSettingsSky::SETTING_LIGHT_NORMAL("lightnorm");
// const std::string LLSettingsSky::SETTING_NAME("name");
