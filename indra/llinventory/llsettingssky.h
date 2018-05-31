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
#include "v4coloru.h"

const F32 EARTH_RADIUS  =      6.370e6f;
const F32 SUN_RADIUS    =    695.508e6f;
const F32 SUN_DIST      = 149598.260e6f;
const F32 MOON_RADIUS   =      1.737e6f;
const F32 MOON_DIST     =    384.400e6f;

class LLSettingsSky: public LLSettingsBase
{
public:
    static const std::string SETTING_AMBIENT;
    static const std::string SETTING_BLOOM_TEXTUREID;
    static const std::string SETTING_BLUE_DENSITY;
    static const std::string SETTING_BLUE_HORIZON;
    static const std::string SETTING_DENSITY_MULTIPLIER;
    static const std::string SETTING_DISTANCE_MULTIPLIER;
    static const std::string SETTING_HAZE_DENSITY;
    static const std::string SETTING_HAZE_HORIZON;
    static const std::string SETTING_CLOUD_COLOR;
    static const std::string SETTING_CLOUD_POS_DENSITY1;
    static const std::string SETTING_CLOUD_POS_DENSITY2;
    static const std::string SETTING_CLOUD_SCALE;
    static const std::string SETTING_CLOUD_SCROLL_RATE;
    static const std::string SETTING_CLOUD_SHADOW;
    static const std::string SETTING_CLOUD_TEXTUREID;
    static const std::string SETTING_DOME_OFFSET;
    static const std::string SETTING_DOME_RADIUS;
    static const std::string SETTING_GAMMA;
    static const std::string SETTING_GLOW;    
    static const std::string SETTING_LIGHT_NORMAL;
    static const std::string SETTING_MAX_Y;
    static const std::string SETTING_MOON_ROTATION;
    static const std::string SETTING_MOON_TEXTUREID;
    static const std::string SETTING_STAR_BRIGHTNESS;
    static const std::string SETTING_SUNLIGHT_COLOR;
    static const std::string SETTING_SUN_ROTATION;
    static const std::string SETTING_SUN_TEXTUREID;

    static const std::string SETTING_PLANET_RADIUS;
    static const std::string SETTING_SKY_BOTTOM_RADIUS;
    static const std::string SETTING_SKY_TOP_RADIUS;
    static const std::string SETTING_SUN_ARC_RADIANS;
    static const std::string SETTING_MIE_ANISOTROPY_FACTOR;

    static const std::string SETTING_RAYLEIGH_CONFIG;
    static const std::string SETTING_MIE_CONFIG;
    static const std::string SETTING_ABSORPTION_CONFIG;

    static const std::string KEY_DENSITY_PROFILE;
        static const std::string SETTING_DENSITY_PROFILE_WIDTH;
        static const std::string SETTING_DENSITY_PROFILE_EXP_TERM;
        static const std::string SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR;
        static const std::string SETTING_DENSITY_PROFILE_LINEAR_TERM;
        static const std::string SETTING_DENSITY_PROFILE_CONSTANT_TERM;
        

    static const LLUUID DEFAULT_SUN_ID;
    static const LLUUID DEFAULT_MOON_ID;
    static const LLUUID DEFAULT_CLOUD_ID;

    static const std::string SETTING_LEGACY_HAZE;

    typedef std::shared_ptr<LLSettingsSky> ptr_t;

    //---------------------------------------------------------------------
    LLSettingsSky(const LLSD &data);
    virtual ~LLSettingsSky() { };

    virtual ptr_t   buildClone() = 0;

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const override { return std::string("sky"); }
    virtual LLSettingsType::type_e getSettingTypeValue() const override { return LLSettingsType::ST_SKY; }

    // Settings status 
    virtual void blend(const LLSettingsBase::ptr_t &end, F64 blendf) override;
    
    static LLSD defaults();

    F32 getPlanetRadius() const
    {
        return mSettings[SETTING_PLANET_RADIUS].asReal();
    }

    F32 getSkyBottomRadius() const
    {
        return mSettings[SETTING_SKY_BOTTOM_RADIUS].asReal();
    }

    F32 getSkyTopRadius() const
    {
        return mSettings[SETTING_SKY_TOP_RADIUS].asReal();
    }

    F32 getSunArcRadians() const
    {
        return mSettings[SETTING_SUN_ARC_RADIANS].asReal();
    }

    F32 getMieAnisotropy() const
    {
        return mSettings[SETTING_MIE_ANISOTROPY_FACTOR].asReal();
    }
    
    LLSD getRayleighConfigs() const
    {
        return mSettings[SETTING_RAYLEIGH_CONFIG];
    }

    LLSD getMieConfigs() const
    {
        return mSettings[SETTING_MIE_CONFIG];
    }

    LLSD getAbsorptionConfigs() const
    {
        return mSettings[SETTING_ABSORPTION_CONFIG];
    }

    LLUUID getBloomTextureId() const
    {
        return mSettings[SETTING_BLOOM_TEXTUREID].asUUID();
    }

    //---------------------------------------------------------------------
    LLColor3 getAmbientColor() const
    {
        return LLColor3(mSettings[SETTING_AMBIENT]);
    }

    void setAmbientColor(const LLColor3 &val)
    {
        setValue(SETTING_AMBIENT, val);
    }

    LLColor3 getCloudColor() const
    {
        return LLColor3(mSettings[SETTING_CLOUD_COLOR]);
    }

    void setCloudColor(const LLColor3 &val)
    {
        setValue(SETTING_CLOUD_COLOR, val);
    }

    LLUUID getCloudNoiseTextureId() const
    {
        return mSettings[SETTING_CLOUD_TEXTUREID].asUUID();
    }

    void setCloudNoiseTextureId(const LLUUID &id)
    {
        setValue(SETTING_CLOUD_TEXTUREID, id);
    }

    LLColor3 getCloudPosDensity1() const
    {
        return LLColor3(mSettings[SETTING_CLOUD_POS_DENSITY1]);
    }

    void setCloudPosDensity1(const LLColor3 &val)
    {
        setValue(SETTING_CLOUD_POS_DENSITY1, val);
    }

    LLColor3 getCloudPosDensity2() const
    {
        return LLColor3(mSettings[SETTING_CLOUD_POS_DENSITY2]);
    }

    void setCloudPosDensity2(const LLColor3 &val)
    {
        setValue(SETTING_CLOUD_POS_DENSITY2, val);
    }

    F32 getCloudScale() const
    {
        return mSettings[SETTING_CLOUD_SCALE].asReal();
    }

    void setCloudScale(F32 val)
    {
        setValue(SETTING_CLOUD_SCALE, val);
    }

    LLVector2 getCloudScrollRate() const
    {
        return LLVector2(mSettings[SETTING_CLOUD_SCROLL_RATE]);
    }

    void setCloudScrollRate(const LLVector2 &val)
    {
        setValue(SETTING_CLOUD_SCROLL_RATE, val);
    }

    void setCloudScrollRateX(F32 val)
    {
        mSettings[SETTING_CLOUD_SCROLL_RATE][0] = val;
        setDirtyFlag(true);
    }

    void setCloudScrollRateY(F32 val)
    {
        mSettings[SETTING_CLOUD_SCROLL_RATE][1] = val;
        setDirtyFlag(true);
    }

    F32 getCloudShadow() const
    {
        return mSettings[SETTING_CLOUD_SHADOW].asReal();
    }

    void setCloudShadow(F32 val)
    {
        setValue(SETTING_CLOUD_SHADOW, val);
    }

    
    F32 getDomeOffset() const
    {
        return DOME_OFFSET;
        //return mSettings[SETTING_DOME_OFFSET].asReal();
    }

    F32 getDomeRadius() const
    {
        return DOME_RADIUS;
        //return mSettings[SETTING_DOME_RADIUS].asReal();
    }

    F32 getGamma() const
    {
        return mSettings[SETTING_GAMMA].asReal();
    }

    void setGamma(F32 val)
    {
        mSettings[SETTING_GAMMA] = LLSD::Real(val);
        setDirtyFlag(true);
    }

    LLColor3 getGlow() const
    {
        return LLColor3(mSettings[SETTING_GLOW]);
    }

    void setGlow(const LLColor3 &val)
    {
        setValue(SETTING_GLOW, val);
    }

    F32 getMaxY() const
    {
        return mSettings[SETTING_MAX_Y].asReal();
    }

    void setMaxY(F32 val) 
    {
        setValue(SETTING_MAX_Y, val);
    }

    LLQuaternion getMoonRotation() const
    {
        return LLQuaternion(mSettings[SETTING_MOON_ROTATION]);
    }

    void setMoonRotation(const LLQuaternion &val)
    {
        setValue(SETTING_MOON_ROTATION, val);
    }

    LLUUID getMoonTextureId() const
    {
        return mSettings[SETTING_MOON_TEXTUREID].asUUID();
    }

    void setMoonTextureId(LLUUID id)
    {
        setValue(SETTING_MOON_TEXTUREID, id);
    }

    F32 getStarBrightness() const
    {
        return mSettings[SETTING_STAR_BRIGHTNESS].asReal();
    }

    void setStarBrightness(F32 val)
    {
        setValue(SETTING_STAR_BRIGHTNESS, val);
    }

    LLColor3 getSunlightColor() const
    {
        return LLColor3(mSettings[SETTING_SUNLIGHT_COLOR]);
    }

    void setSunlightColor(const LLColor3 &val)
    {
        setValue(SETTING_SUNLIGHT_COLOR, val);
    }

    LLQuaternion getSunRotation() const
    {
        return LLQuaternion(mSettings[SETTING_SUN_ROTATION]);
    }

    void setSunRotation(const LLQuaternion &val) 
    {
        setValue(SETTING_SUN_ROTATION, val);
    }

    LLUUID getSunTextureId() const
    {
        return mSettings[SETTING_SUN_TEXTUREID].asUUID();
    }

    void setSunTextureId(LLUUID id) 
    {
        setValue(SETTING_SUN_TEXTUREID, id);
    }

    //=====================================================================
    // transient properties used in animations.
    LLUUID getNextSunTextureId() const
    {
        return mNextSunTextureId;
    }

    LLUUID getNextMoonTextureId() const
    {
        return mNextMoonTextureId;
    }

    LLUUID getNextCloudNoiseTextureId() const
    {
        return mNextCloudTextureId;
    }

    //=====================================================================
    virtual void                loadTextures() { };

    //=====================================================================
    virtual validation_list_t getValidationList() const override;
    static validation_list_t validationList();

    static LLSD translateLegacySettings(const LLSD& legacy);
    static LLSD translateLegacyHazeSettings(const LLSD& legacy);

    LLColor3 getLightAttenuation(F32 distance) const;
    LLColor3 getLightTransmittance() const;
    LLColor3 gammaCorrect(const LLColor3& in) const;

// LEGACY_ATMOSPHERICS
    LLColor3 getBlueDensity() const;
    LLColor3 getBlueHorizon() const;
    F32 getHazeDensity() const;
    F32 getHazeHorizon() const;
    F32 getDensityMultiplier() const;
    F32 getDistanceMultiplier() const;

    void setBlueDensity(const LLColor3 &val);
    void setBlueHorizon(const LLColor3 &val);
    void setDensityMultiplier(F32 val);
    void setDistanceMultiplier(F32 val);
    void setHazeDensity(F32 val);
    void setHazeHorizon(F32 val);

// Internal/calculated settings
    bool getIsSunUp() const;
    bool getIsMoonUp() const;

    LLVector3 getLightDirection() const;
    LLVector3 getSunDirection() const;
    LLVector3 getMoonDirection() const;
    LLColor4U getFadeColor() const;
    LLColor4  getMoonAmbient() const;
    LLColor3  getMoonDiffuse() const;
    LLColor4  getSunAmbient() const;
    LLColor3  getSunDiffuse() const;
    LLColor4  getTotalAmbient() const;

protected:
    static const std::string SETTING_LEGACY_EAST_ANGLE;
    static const std::string SETTING_LEGACY_ENABLE_CLOUD_SCROLL;
    static const std::string SETTING_LEGACY_SUN_ANGLE;

    LLSettingsSky();

    virtual stringset_t getSlerpKeys() const override;
    virtual stringset_t getSkipInterpolateKeys() const override;

    virtual void    updateSettings() override;

private:
    mutable bool mPositionsDirty = true;
    mutable bool mLightingDirty  = true;

    static LLSD rayleighConfigDefault();
    static LLSD absorptionConfigDefault();
    static LLSD mieConfigDefault();

    void        calculateHeavnlyBodyPositions() const;
    void        calculateLightSettings() const;

    mutable LLVector3   mSunDirection;
    mutable LLVector3   mMoonDirection;
    mutable LLVector3   mLightDirection;

    static const F32 DOME_RADIUS;
    static const F32 DOME_OFFSET;

    mutable LLColor4U   mFadeColor;
    mutable LLColor4    mMoonAmbient;
    mutable LLColor3    mMoonDiffuse;
    mutable LLColor4    mSunAmbient;
    mutable LLColor3    mSunDiffuse;
    mutable LLColor4    mTotalAmbient;

    LLUUID      mNextSunTextureId;
    LLUUID      mNextMoonTextureId;
    LLUUID      mNextCloudTextureId;

    typedef std::map<std::string, S32> mapNameToUniformId_t;

    static mapNameToUniformId_t sNameToUniformMapping;
};

#endif
