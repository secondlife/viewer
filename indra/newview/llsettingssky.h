/**
* @file llsettingssky.h
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

#ifndef LL_SETTINGS_SKY_H
#define LL_SETTINGS_SKY_H

#include "llsettingsbase.h"

class LLSettingsSky: public LLSettingsBase
{
public:
    static const std::string SETTING_AMBIENT;
    static const std::string SETTING_BLOOM_TEXTUREID;
    static const std::string SETTING_BLUE_DENSITY;
    static const std::string SETTING_BLUE_HORIZON;
    static const std::string SETTING_CLOUD_COLOR;
    static const std::string SETTING_CLOUD_POS_DENSITY1;
    static const std::string SETTING_CLOUD_POS_DENSITY2;
    static const std::string SETTING_CLOUD_SCALE;
    static const std::string SETTING_CLOUD_SCROLL_RATE;
    static const std::string SETTING_CLOUD_SHADOW;
    static const std::string SETTING_CLOUD_TEXTUREID;
    static const std::string SETTING_DENSITY_MULTIPLIER;
    static const std::string SETTING_DISTANCE_MULTIPLIER;
    static const std::string SETTING_DOME_OFFSET;
    static const std::string SETTING_DOME_RADIUS;
    static const std::string SETTING_GAMMA;
    static const std::string SETTING_GLOW;
    static const std::string SETTING_HAZE_DENSITY;
    static const std::string SETTING_HAZE_HORIZON;
    static const std::string SETTING_LIGHT_NORMAL;
    static const std::string SETTING_MAX_Y;
    static const std::string SETTING_MOON_ROTATION;
    static const std::string SETTING_MOON_TEXTUREID;
    static const std::string SETTING_NAME;
    static const std::string SETTING_STAR_BRIGHTNESS;
    static const std::string SETTING_SUNLIGHT_COLOR;
    static const std::string SETTING_SUN_ROTATION;
    static const std::string SETTING_SUN_TEXUTUREID;

    static const std::string SETTING_LEGACY_EAST_ANGLE;
    static const std::string SETTING_LEGACY_ENABLE_CLOUD_SCROLL;
    static const std::string SETTING_LEGACY_SUN_ANGLE;

    typedef boost::shared_ptr<LLSettingsSky> ptr_t;

    //---------------------------------------------------------------------
    LLSettingsSky(const LLSD &data);
    virtual ~LLSettingsSky() { };

    static ptr_t buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t buildDefaultSky();

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const { return std::string("sky"); }

    // Settings status 
    ptr_t blend(const ptr_t &other, F32 mix) const;
    
    static LLSD defaults();

    //---------------------------------------------------------------------
    LLColor3 getAmbientColor() const
    {
        return LLColor3(mSettings[SETTING_AMBIENT]);
    }

    LLUUID getBloomTextureId() const
    {
        return mSettings[SETTING_BLOOM_TEXTUREID].asUUID();
    }

    LLColor3 getBlueDensity() const
    {
        return LLColor3(mSettings[SETTING_BLUE_DENSITY]);
    }

    LLColor3 getBlueHorizon() const
    {
        return LLColor3(mSettings[SETTING_BLUE_HORIZON]);
    }

    LLColor3 getCloudColor() const
    {
        return mSettings[SETTING_CLOUD_COLOR].asReal();
    }

    LLUUID getCloudNoiseTextureId() const
    {
        return mSettings[SETTING_CLOUD_TEXTUREID].asUUID();
    }

    LLColor3 getCloudPosDensity1() const
    {
        return mSettings[SETTING_CLOUD_POS_DENSITY1].asReal();
    }

    LLColor3 getCloudPosDensity2() const
    {
        return mSettings[SETTING_CLOUD_POS_DENSITY2].asReal();
    }

    F32 getCloudScale() const
    {
        return mSettings[SETTING_CLOUD_SCALE].asReal();
    }

    LLVector2 getCloudScrollRate() const
    {
        return LLVector2(mSettings[SETTING_CLOUD_SCROLL_RATE]);
    }

    F32 getCloudShadow() const
    {
        return mSettings[SETTING_CLOUD_SHADOW].asReal();
    }

    F32 getDensityMultiplier() const
    {
        return mSettings[SETTING_DENSITY_MULTIPLIER].asReal();
    }

    F32 getDistanceMultiplier() const
    {
        return mSettings[SETTING_DISTANCE_MULTIPLIER].asReal();
    }

    F32 getDomeOffset() const
    {
        return mSettings[SETTING_DOME_OFFSET].asReal();
    }

    F32 getDomeRadius() const
    {
        return mSettings[SETTING_DOME_RADIUS].asReal();
    }

    F32 getGama() const
    {
        return mSettings[SETTING_GAMMA].asReal();
    }

    LLColor3 getGlow() const
    {
        return mSettings[SETTING_GLOW].asReal();
    }

    F32 getHazeDensity() const
    {
        return mSettings[SETTING_HAZE_DENSITY].asReal();
    }

    F32 getHazeHorizon() const
    {
        return mSettings[SETTING_HAZE_HORIZON].asReal();
    }

    F32 getMaxY() const
    {
        return mSettings[SETTING_MAX_Y].asReal();
    }

    LLQuaternion getMoonRotation() const
    {
        return LLQuaternion(mSettings[SETTING_MOON_ROTATION]);
    }

    LLUUID getMoonTextureId() const
    {
        return mSettings[SETTING_MOON_TEXTUREID].asUUID();
    }

    F32 getStarBrightness() const
    {
        return mSettings[SETTING_STAR_BRIGHTNESS].asReal();
    }

    LLColor3 getSunlightColor() const
    {
        return LLColor3(mSettings[SETTING_SUNLIGHT_COLOR]);
    }

    LLQuaternion getSunRotation() const
    {
        return LLQuaternion(mSettings[SETTING_SUN_ROTATION]);
    }

    LLUUID getSunTextureId() const
    {
        return mSettings[SETTING_SUN_TEXUTUREID].asUUID();
    }


    // Internal/calculated settings
    LLVector3 getLightDirection() const
    {
        update();
        return mLightDirection;
    };

    LLVector3 getLightDirectionClamped() const
    {
        update();
        return mLightDirectionClamped;
    };

    F32 getSceneLightStrength() const
    {
        update();
        return mSceneLightStrength;
    }

    LLVector3   getSunDirection() const
    {
        update();
        return mSunDirection;
    }

    LLVector3   getMoonDirection() const
    {
        update();
        return mMoonDirection;
    }

    LLColor3    getSunDiffuse() const
    {
        update();
        return mSunDiffuse;
    }

    LLColor3    getSunAmbient() const
    {
        update();
        return mSunAmbient;
    }

    LLColor3    getMoonDiffuse() const
    {
        update();
        return mMoonDiffuse;
    }

    LLColor3    getMoonAmbient() const
    {
        update();
        return mMoonAmbient;
    }

    LLColor4    getTotalAmbient() const
    {
        update();
        return mTotalAmbient;
    }

    LLColor4    getFadeColor() const
    {
        update();
        return mFadeColor;
    }

protected:
    LLSettingsSky();

    virtual stringset_t getSlerpKeys() const;

    virtual void        updateSettings();

    virtual stringset_t getSkipApplyKeys() const;
    virtual void        applySpecial(void *);

private:
    void        calculateHeavnlyBodyPositions();
    void        calculateLightSettings();

    LLVector3   mSunDirection;
    LLVector3   mMoonDirection;
    F32         mSceneLightStrength;
    LLVector3   mLightDirection;
    LLVector3   mLightDirectionClamped;

    LLColor3    mSunDiffuse;
    LLColor3    mSunAmbient;
    LLColor3    mMoonDiffuse;
    LLColor3    mMoonAmbient;        
    
    LLColor4    mTotalAmbient;
    LLColor4    mFadeColor;
};

#endif
