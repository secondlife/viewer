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
    static const std::string SETTING_RAINBOW_TEXTUREID;
    static const std::string SETTING_HALO_TEXTUREID;
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
    static const std::string SETTING_CLOUD_VARIANCE;

    static const std::string SETTING_DOME_OFFSET;
    static const std::string SETTING_DOME_RADIUS;
    static const std::string SETTING_GAMMA;
    static const std::string SETTING_GLOW;
    static const std::string SETTING_LIGHT_NORMAL;
    static const std::string SETTING_MAX_Y;
    static const std::string SETTING_MOON_ROTATION;
    static const std::string SETTING_MOON_SCALE;
    static const std::string SETTING_MOON_TEXTUREID;
    static const std::string SETTING_MOON_BRIGHTNESS;

    static const std::string SETTING_STAR_BRIGHTNESS;
    static const std::string SETTING_SUNLIGHT_COLOR;
    static const std::string SETTING_SUN_ROTATION;
    static const std::string SETTING_SUN_SCALE;
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

    static const std::string SETTING_SKY_MOISTURE_LEVEL;
    static const std::string SETTING_SKY_DROPLET_RADIUS;
    static const std::string SETTING_SKY_ICE_LEVEL;

    static const std::string SETTING_REFLECTION_PROBE_AMBIANCE;

    static const std::string SETTING_LEGACY_HAZE;

    static const LLUUID DEFAULT_ASSET_ID;

    static const F32 DEFAULT_AUTO_ADJUST_PROBE_AMBIANCE;
    static F32 sAutoAdjustProbeAmbiance;

    typedef PTR_NAMESPACE::shared_ptr<LLSettingsSky> ptr_t;

    //---------------------------------------------------------------------
    LLSettingsSky(const LLSD &data);
    virtual ~LLSettingsSky() { };

    virtual ptr_t   buildClone() = 0;

    //---------------------------------------------------------------------
    virtual std::string getSettingsType() const SETTINGS_OVERRIDE { return std::string("sky"); }
    virtual LLSettingsType::type_e getSettingsTypeValue() const SETTINGS_OVERRIDE { return LLSettingsType::ST_SKY; }

    // Settings status
    virtual void blend(LLSettingsBase::ptr_t &end, F64 blendf) SETTINGS_OVERRIDE;

    virtual void replaceSettings(LLSD settings) SETTINGS_OVERRIDE;
    virtual void replaceSettings(const LLSettingsBase::ptr_t& other_sky) override;

    void replaceWithSky(const LLSettingsSky::ptr_t& pother);
    static LLSD defaults(const LLSettingsBase::TrackPosition& position = 0.0f);

    void loadValuesFromLLSD() override;
    void saveValuesToLLSD() override;

    F32 getPlanetRadius() const;
    F32 getSkyBottomRadius() const;
    F32 getSkyTopRadius() const;
    F32 getSunArcRadians() const;
    F32 getMieAnisotropy() const;

    F32 getSkyMoistureLevel() const;
    F32 getSkyDropletRadius() const;
    F32 getSkyIceLevel() const;

    // get the probe ambiance setting as stored in the sky settings asset
    // auto_adjust - if true and canAutoAdjust() is true, return 1.0
    F32 getReflectionProbeAmbiance(bool auto_adjust = false) const;

    // Return first (only) profile layer represented in LLSD
    LLSD getRayleighConfig() const;
    LLSD getMieConfig() const;
    LLSD getAbsorptionConfig() const;

    // Return entire LLSDArray of profile layers represented in LLSD
    LLSD getRayleighConfigs() const;
    LLSD getMieConfigs() const;
    LLSD getAbsorptionConfigs() const;

    LLUUID getBloomTextureId() const;
    LLUUID getRainbowTextureId() const;
    LLUUID getHaloTextureId() const;

    void setRayleighConfigs(const LLSD& rayleighConfig);
    void setMieConfigs(const LLSD& mieConfig);
    void setAbsorptionConfigs(const LLSD& absorptionConfig);

    void setPlanetRadius(F32 radius);
    void setSkyBottomRadius(F32 radius);
    void setSkyTopRadius(F32 radius);
    void setSunArcRadians(F32 radians);
    void setMieAnisotropy(F32 aniso_factor);

    void setSkyMoistureLevel(F32 moisture_level);
    void setSkyDropletRadius(F32 radius);
    void setSkyIceLevel(F32 ice_level);

    void setReflectionProbeAmbiance(F32 ambiance);

    //---------------------------------------------------------------------
    LLColor3 getAmbientColor() const;
    void setAmbientColor(const LLColor3 &val);

    LLColor3 getCloudColor() const;
    void setCloudColor(const LLColor3 &val);

    LLUUID getCloudNoiseTextureId() const;
    void setCloudNoiseTextureId(const LLUUID &id);

    LLColor3 getCloudPosDensity1() const;
    void setCloudPosDensity1(const LLColor3 &val);

    LLColor3 getCloudPosDensity2() const;
    void setCloudPosDensity2(const LLColor3 &val);

    F32 getCloudScale() const;
    void setCloudScale(F32 val);

    LLVector2 getCloudScrollRate() const;
    void setCloudScrollRate(const LLVector2 &val);

    void setCloudScrollRateX(F32 val);
    void setCloudScrollRateY(F32 val);

    F32 getCloudShadow() const;
    void setCloudShadow(F32 val);

    F32 getCloudVariance() const;
    void setCloudVariance(F32 val);

    F32 getDomeOffset() const;
    F32 getDomeRadius() const;

    F32 getGamma() const;

    F32 getHDRMin(bool auto_adjust = false) const;
    F32 getHDRMax(bool auto_adjust = false) const;
    F32 getHDROffset(bool auto_adjust = false) const;
    F32 getTonemapMix(bool auto_adjust = false) const;
    void setTonemapMix(F32 mix);

    void setGamma(F32 val);

    LLColor3 getGlow() const;
    void setGlow(const LLColor3 &val);

    F32 getMaxY() const;

    void setMaxY(F32 val);

    LLQuaternion getMoonRotation() const;
    void setMoonRotation(const LLQuaternion &val);

    F32 getMoonScale() const;
    void setMoonScale(F32 val);

    LLUUID getMoonTextureId() const;
    void setMoonTextureId(LLUUID id);

    F32  getMoonBrightness() const;
    void setMoonBrightness(F32 brightness_factor);

    F32 getStarBrightness() const;
    void setStarBrightness(F32 val);

    LLColor3 getSunlightColor() const;
    void setSunlightColor(const LLColor3 &val);

    LLQuaternion getSunRotation() const;
    void setSunRotation(const LLQuaternion &val) ;

    F32 getSunScale() const;
    void setSunScale(F32 val);

    LLUUID getSunTextureId() const;
    void setSunTextureId(LLUUID id);

    //=====================================================================
    // transient properties used in animations.
    LLUUID getNextSunTextureId() const;
    LLUUID getNextMoonTextureId() const;
    LLUUID getNextCloudNoiseTextureId() const;
    LLUUID getNextBloomTextureId() const;

    //=====================================================================
    virtual void loadTextures() { };

    //=====================================================================
    virtual validation_list_t getValidationList() const SETTINGS_OVERRIDE;
    static validation_list_t validationList();

    static LLSD translateLegacySettings(const LLSD& legacy);

// LEGACY_ATMOSPHERICS
    static LLSD translateLegacyHazeSettings(const LLSD& legacy);

    LLColor3 getLightAttenuation(F32 distance) const;
    LLColor3 getLightTransmittance(F32 distance) const;
    LLColor3 getLightTransmittanceFast(const LLColor3& total_density, const F32 density_multiplier, const F32 distance) const;
    LLColor3 getTotalDensity() const;
    LLColor3 gammaCorrect(const LLColor3& in,const F32 &gamma) const;

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

    // determines how much the haze glow effect occurs in rendering
    F32 getSunMoonGlowFactor() const;

    LLVector3 getLightDirection() const;
    LLColor3  getLightDiffuse() const;

    LLVector3 getSunDirection() const;
    LLVector3 getMoonDirection() const;

    // color based on brightness
    LLColor3  getMoonlightColor() const;

    LLColor4  getMoonAmbient() const;
    LLColor3  getMoonDiffuse() const;
    LLColor4  getSunAmbient() const;
    LLColor3  getSunDiffuse() const;
    LLColor4  getTotalAmbient() const;
    LLColor4  getHazeColor() const;

    LLColor3 getSunlightColorClamped() const;
    LLColor3 getAmbientColorClamped() const;

    virtual LLSettingsBase::ptr_t buildDerivedClone() SETTINGS_OVERRIDE { return buildClone(); }

    static LLUUID GetDefaultAssetId();
    static LLUUID GetDefaultSunTextureId();
    static LLUUID GetBlankSunTextureId();
    static LLUUID GetDefaultMoonTextureId();
    static LLUUID GetDefaultCloudNoiseTextureId();
    static LLUUID GetDefaultBloomTextureId();
    static LLUUID GetDefaultRainbowTextureId();
    static LLUUID GetDefaultHaloTextureId();

    static LLSD createDensityProfileLayer(
                    F32 width,
                    F32 exponential_term,
                    F32 exponential_scale_factor,
                    F32 linear_term,
                    F32 constant_term,
                    F32 aniso_factor = 0.0f);

    static LLSD createSingleLayerDensityProfile(
                    F32 width,
                    F32 exponential_term,
                    F32 exponential_scale_factor,
                    F32 linear_term,
                    F32 constant_term,
                    F32 aniso_factor = 0.0f);

    virtual void        updateSettings() SETTINGS_OVERRIDE;

    // if true, this sky is a candidate for auto-adjustment
    bool canAutoAdjust() const;

protected:
    static const std::string SETTING_LEGACY_EAST_ANGLE;
    static const std::string SETTING_LEGACY_ENABLE_CLOUD_SCROLL;
    static const std::string SETTING_LEGACY_SUN_ANGLE;

    LLSettingsSky();

    virtual stringset_t getSlerpKeys() const SETTINGS_OVERRIDE;
    virtual stringset_t getSkipInterpolateKeys() const SETTINGS_OVERRIDE;

    LLUUID      mSunTextureId;
    LLUUID      mMoonTextureId;
    LLUUID      mCloudTextureId;
    LLUUID      mBloomTextureId;
    LLUUID      mRainbowTextureId;
    LLUUID      mHaloTextureId;
    LLUUID      mNextSunTextureId;
    LLUUID      mNextMoonTextureId;
    LLUUID      mNextCloudTextureId;
    LLUUID      mNextBloomTextureId;
    LLUUID      mNextRainbowTextureId;
    LLUUID      mNextHaloTextureId;

    bool mCanAutoAdjust;
    LLQuaternion mSunRotation;
    LLQuaternion mMoonRotation;
    LLColor3 mSunlightColor;
    LLColor3 mGlow;
    F32 mReflectionProbeAmbiance;
    F32 mSunScale;
    F32 mStarBrightness;
    F32 mMoonBrightness;
    F32 mMoonScale;
    F32 mMaxY;
    F32 mGamma;
    F32 mCloudVariance;
    F32 mCloudShadow;
    F32 mCloudScale;
    F32 mTonemapMix;
    F32 mHDROffset;
    F32 mHDRMax;
    F32 mHDRMin;
    LLVector2 mScrollRate;
    LLColor3 mCloudPosDensity1;
    LLColor3 mCloudPosDensity2;
    LLColor3 mCloudColor;
    LLSD mAbsorptionConfigs;
    LLSD mMieConfigs;
    LLSD mRayleighConfigs;
    F32 mSunArcRadians;
    F32 mSkyTopRadius;
    F32 mSkyBottomRadius;
    F32 mSkyMoistureLevel;
    F32 mSkyDropletRadius;
    F32 mSkyIceLevel;
    F32 mPlanetRadius;

    F32 mHazeHorizon;
    F32 mHazeDensity;
    F32 mDistanceMultiplier;
    F32 mDensityMultiplier;
    LLColor3 mBlueHorizon;
    LLColor3 mBlueDensity;
    LLColor3 mAmbientColor;

    bool mHasLegacyHaze;
    bool mLegacyHazeHorizon;
    bool mLegacyHazeDensity;
    bool mLegacyDistanceMultiplier;
    bool mLegacyDensityMultiplier;
    bool mLegacyBlueHorizon;
    bool mLegacyBlueDensity;
    bool mLegacyAmbientColor;

private:
    static LLSD rayleighConfigDefault();
    static LLSD absorptionConfigDefault();
    static LLSD mieConfigDefault();

    LLColor3 getColor(const std::string& key, const LLColor3& default_value);
    F32      getFloat(const std::string& key, F32 default_value);

    void        calculateHeavenlyBodyPositions() const;
    void        calculateLightSettings() const;
    static void clampColor(LLColor3& color, F32 gamma, const F32 scale = 1.0f);

    mutable LLVector3   mSunDirection;
    mutable LLVector3   mMoonDirection;
    mutable LLVector3   mLightDirection;

    static const F32 DOME_RADIUS;
    static const F32 DOME_OFFSET;

    mutable LLColor4    mMoonAmbient;
    mutable LLColor3    mMoonDiffuse;
    mutable LLColor4    mSunAmbient;
    mutable LLColor3    mSunDiffuse;
    mutable LLColor4    mTotalAmbient;
    mutable LLColor4    mHazeColor;

    typedef std::map<std::string, S32> mapNameToUniformId_t;

    static mapNameToUniformId_t sNameToUniformMapping;
};

#endif
