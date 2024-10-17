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

#ifndef LL_SETTINGS_WATER_H
#define LL_SETTINGS_WATER_H

#include "llsettingsbase.h"

class LLSettingsWater : public LLSettingsBase
{
public:
    static const std::string SETTING_BLUR_MULTIPLIER;
    static const std::string SETTING_FOG_COLOR;
    static const std::string SETTING_FOG_DENSITY;
    static const std::string SETTING_FOG_MOD;
    static const std::string SETTING_FRESNEL_OFFSET;
    static const std::string SETTING_FRESNEL_SCALE;
    static const std::string SETTING_TRANSPARENT_TEXTURE;
    static const std::string SETTING_NORMAL_MAP;
    static const std::string SETTING_NORMAL_SCALE;
    static const std::string SETTING_SCALE_ABOVE;
    static const std::string SETTING_SCALE_BELOW;
    static const std::string SETTING_WAVE1_DIR;
    static const std::string SETTING_WAVE2_DIR;

    static const LLUUID DEFAULT_ASSET_ID;

    typedef PTR_NAMESPACE::shared_ptr<LLSettingsWater> ptr_t;

    //---------------------------------------------------------------------
    LLSettingsWater(const LLSD &data);
    virtual ~LLSettingsWater() { };

    virtual ptr_t   buildClone() = 0;

    //---------------------------------------------------------------------
    virtual std::string     getSettingsType() const SETTINGS_OVERRIDE { return std::string("water"); }
    virtual LLSettingsType::type_e  getSettingsTypeValue() const SETTINGS_OVERRIDE { return LLSettingsType::ST_WATER; }

    // Settings status
    virtual void blend(LLSettingsBase::ptr_t &end, F64 blendf) SETTINGS_OVERRIDE;

    virtual void replaceSettings(LLSD settings) SETTINGS_OVERRIDE;
    virtual void replaceSettings(const LLSettingsBase::ptr_t& other_water) override;
    void replaceWithWater(const LLSettingsWater::ptr_t& other);

    static LLSD defaults(const LLSettingsBase::TrackPosition& position = 0.0f);

    void loadValuesFromLLSD() override;
    void saveValuesToLLSD() override;

    //---------------------------------------------------------------------
    F32 getBlurMultiplier() const
    {
        return mBlurMultiplier;
    }

    void setBlurMultiplier(F32 val)
    {
        mBlurMultiplier = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLColor3 getWaterFogColor() const
    {
        return mWaterFogColor;
    }

    void setWaterFogColor(LLColor3 val)
    {
        mWaterFogColor = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getWaterFogDensity() const
    {
        return mWaterFogDensity;
    }

    F32 getModifiedWaterFogDensity(bool underwater) const;

    void setWaterFogDensity(F32 val)
    {
        mWaterFogDensity = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getFogMod() const
    {
        return mFogMod;
    }

    void setFogMod(F32 val)
    {
        mFogMod = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getFresnelOffset() const
    {
        return mFresnelOffset;
    }

    void setFresnelOffset(F32 val)
    {
        mFresnelOffset = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getFresnelScale() const
    {
        return mFresnelScale;
    }

    void setFresnelScale(F32 val)
    {
        mFresnelScale = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLUUID getTransparentTextureID() const
    {
        return mTransparentTextureID;
    }

    void setTransparentTextureID(LLUUID val)
    {
        mTransparentTextureID = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLUUID getNormalMapID() const
    {
        return mNormalMapID;
    }

    void setNormalMapID(LLUUID val)
    {
        mNormalMapID = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLVector3 getNormalScale() const
    {
        return mNormalScale;
    }

    void setNormalScale(LLVector3 val)
    {
        mNormalScale = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getScaleAbove() const
    {
        return mScaleAbove;
    }

    void setScaleAbove(F32 val)
    {
        mScaleAbove = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    F32 getScaleBelow() const
    {
        return mScaleBelow;
    }

    void setScaleBelow(F32 val)
    {
        mScaleBelow = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLVector2 getWave1Dir() const
    {
        return mWave1Dir;
    }

    void setWave1Dir(LLVector2 val)
    {
        mWave1Dir = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    LLVector2 getWave2Dir() const
    {
        return mWave2Dir;
    }

    void setWave2Dir(LLVector2 val)
    {
        mWave2Dir = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    //-------------------------------------------
    LLUUID getNextNormalMapID() const
    {
        return mNextNormalMapID;
    }

    LLUUID getNextTransparentTextureID() const
    {
        return mNextTransparentTextureID;
    }

    virtual validation_list_t getValidationList() const SETTINGS_OVERRIDE;
    static validation_list_t validationList();

    static LLSD         translateLegacySettings(LLSD legacy);

    virtual LLSettingsBase::ptr_t buildDerivedClone() SETTINGS_OVERRIDE { return buildClone(); }

    static LLUUID GetDefaultAssetId();
    static LLUUID GetDefaultWaterNormalAssetId();
    static LLUUID GetDefaultTransparentTextureAssetId();
    static LLUUID GetDefaultOpaqueTextureAssetId();

protected:
    static const std::string SETTING_LEGACY_BLUR_MULTIPLIER;
    static const std::string SETTING_LEGACY_FOG_COLOR;
    static const std::string SETTING_LEGACY_FOG_DENSITY;
    static const std::string SETTING_LEGACY_FOG_MOD;
    static const std::string SETTING_LEGACY_FRESNEL_OFFSET;
    static const std::string SETTING_LEGACY_FRESNEL_SCALE;
    static const std::string SETTING_LEGACY_NORMAL_MAP;
    static const std::string SETTING_LEGACY_NORMAL_SCALE;
    static const std::string SETTING_LEGACY_SCALE_ABOVE;
    static const std::string SETTING_LEGACY_SCALE_BELOW;
    static const std::string SETTING_LEGACY_WAVE1_DIR;
    static const std::string SETTING_LEGACY_WAVE2_DIR;

    LLSettingsWater();

    LLUUID    mTransparentTextureID;
    LLUUID    mNormalMapID;
    LLUUID    mNextTransparentTextureID;
    LLUUID    mNextNormalMapID;

    F32 mBlurMultiplier;
    LLColor3 mWaterFogColor;
    F32 mWaterFogDensity;
    F32 mFogMod;
    F32 mFresnelOffset;
    F32 mFresnelScale;
    LLVector3 mNormalScale;
    F32 mScaleAbove;
    F32 mScaleBelow;
    LLVector2 mWave1Dir;
    LLVector2 mWave2Dir;
};

#endif
