/**
* @file llsettingswater.h
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

#include "llsettingswater.h"
#include <algorithm>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"
#include "indra_constants.h"
#include <boost/bind.hpp>

const std::string LLSettingsWater::SETTING_BLUR_MULTIPLIER("blur_multiplier");
const std::string LLSettingsWater::SETTING_FOG_COLOR("water_fog_color");
const std::string LLSettingsWater::SETTING_FOG_DENSITY("water_fog_density");
const std::string LLSettingsWater::SETTING_FOG_MOD("underwater_fog_mod");
const std::string LLSettingsWater::SETTING_FRESNEL_OFFSET("fresnel_offset");
const std::string LLSettingsWater::SETTING_FRESNEL_SCALE("fresnel_scale");
const std::string LLSettingsWater::SETTING_TRANSPARENT_TEXTURE("transparent_texture");
const std::string LLSettingsWater::SETTING_NORMAL_MAP("normal_map");
const std::string LLSettingsWater::SETTING_NORMAL_SCALE("normal_scale");
const std::string LLSettingsWater::SETTING_SCALE_ABOVE("scale_above");
const std::string LLSettingsWater::SETTING_SCALE_BELOW("scale_below");
const std::string LLSettingsWater::SETTING_WAVE1_DIR("wave1_direction");
const std::string LLSettingsWater::SETTING_WAVE2_DIR("wave2_direction");

const std::string LLSettingsWater::SETTING_LEGACY_BLUR_MULTIPLIER("blurMultiplier");
const std::string LLSettingsWater::SETTING_LEGACY_FOG_COLOR("waterFogColor");
const std::string LLSettingsWater::SETTING_LEGACY_FOG_DENSITY("waterFogDensity");
const std::string LLSettingsWater::SETTING_LEGACY_FOG_MOD("underWaterFogMod");
const std::string LLSettingsWater::SETTING_LEGACY_FRESNEL_OFFSET("fresnelOffset");
const std::string LLSettingsWater::SETTING_LEGACY_FRESNEL_SCALE("fresnelScale");
const std::string LLSettingsWater::SETTING_LEGACY_NORMAL_MAP("normalMap");
const std::string LLSettingsWater::SETTING_LEGACY_NORMAL_SCALE("normScale");
const std::string LLSettingsWater::SETTING_LEGACY_SCALE_ABOVE("scaleAbove");
const std::string LLSettingsWater::SETTING_LEGACY_SCALE_BELOW("scaleBelow");
const std::string LLSettingsWater::SETTING_LEGACY_WAVE1_DIR("wave1Dir");
const std::string LLSettingsWater::SETTING_LEGACY_WAVE2_DIR("wave2Dir");

const LLUUID LLSettingsWater::DEFAULT_ASSET_ID("59d1a851-47e7-0e5f-1ed7-6b715154f41a");

static const LLUUID DEFAULT_TRANSPARENT_WATER_TEXTURE("2bfd3884-7e27-69b9-ba3a-3e673f680004");
static const LLUUID DEFAULT_OPAQUE_WATER_TEXTURE("43c32285-d658-1793-c123-bf86315de055");

//=========================================================================
LLSettingsWater::LLSettingsWater(const LLSD &data) :
    LLSettingsBase(data),
    mNextNormalMapID(),
    mNextTransparentTextureID()
{
    loadValuesFromLLSD();
}

LLSettingsWater::LLSettingsWater() :
    LLSettingsBase(),
    mNextNormalMapID(),
    mNextTransparentTextureID()
{
    replaceSettings(defaults());
}

//=========================================================================
LLSD LLSettingsWater::defaults(const LLSettingsBase::TrackPosition& position)
{
    static LLSD dfltsetting;

    if (dfltsetting.size() == 0)
    {
        // give the normal scale offset some variability over track time...
        F32 normal_scale_offset = (position * 0.5f) - 0.25f;

        // Magic constants copied form defaults.xml
        dfltsetting[SETTING_BLUR_MULTIPLIER] = LLSD::Real(0.04000f);
        dfltsetting[SETTING_FOG_COLOR] = LLColor3(0.0156f, 0.1490f, 0.2509f).getValue();
        dfltsetting[SETTING_FOG_DENSITY] = LLSD::Real(2.0f);
        dfltsetting[SETTING_FOG_MOD] = LLSD::Real(0.25f);
        dfltsetting[SETTING_FRESNEL_OFFSET] = LLSD::Real(0.5f);
        dfltsetting[SETTING_FRESNEL_SCALE] = LLSD::Real(0.3999);
        dfltsetting[SETTING_TRANSPARENT_TEXTURE] = GetDefaultTransparentTextureAssetId();
        dfltsetting[SETTING_NORMAL_MAP] = GetDefaultWaterNormalAssetId();
        dfltsetting[SETTING_NORMAL_SCALE] = LLVector3(2.0f + normal_scale_offset, 2.0f + normal_scale_offset, 2.0f + normal_scale_offset).getValue();
        dfltsetting[SETTING_SCALE_ABOVE] = LLSD::Real(0.0299f);
        dfltsetting[SETTING_SCALE_BELOW] = LLSD::Real(0.2000f);
        dfltsetting[SETTING_WAVE1_DIR] = LLVector2(1.04999f, -0.42000f).getValue();
        dfltsetting[SETTING_WAVE2_DIR] = LLVector2(1.10999f, -1.16000f).getValue();

        dfltsetting[SETTING_TYPE] = "water";
    }

    return dfltsetting;
}

void LLSettingsWater::loadValuesFromLLSD()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_ENVIRONMENT;

    LLSettingsBase::loadValuesFromLLSD();

    LLSD& settings = getSettings();

    mBlurMultiplier = (F32)settings[SETTING_BLUR_MULTIPLIER].asReal();
    mWaterFogColor = LLColor3(settings[SETTING_FOG_COLOR]);
    mWaterFogDensity = (F32)settings[SETTING_FOG_DENSITY].asReal();
    mFogMod = (F32)settings[SETTING_FOG_MOD].asReal();
    mFresnelOffset = (F32)settings[SETTING_FRESNEL_OFFSET].asReal();
    mFresnelScale = (F32)settings[SETTING_FRESNEL_SCALE].asReal();
    mNormalScale = LLVector3(settings[SETTING_NORMAL_SCALE]);
    mScaleAbove = (F32)settings[SETTING_SCALE_ABOVE].asReal();
    mScaleBelow = (F32)settings[SETTING_SCALE_BELOW].asReal();
    mWave1Dir = LLVector2(settings[SETTING_WAVE1_DIR]);
    mWave2Dir = LLVector2(settings[SETTING_WAVE2_DIR]);

    mNormalMapID = settings[SETTING_NORMAL_MAP].asUUID();
    mTransparentTextureID = settings[SETTING_TRANSPARENT_TEXTURE].asUUID();
}

void LLSettingsWater::saveValuesToLLSD()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_ENVIRONMENT;

    LLSettingsBase::saveValuesToLLSD();

    LLSD & settings = getSettings();
    settings[SETTING_BLUR_MULTIPLIER] = LLSD::Real(mBlurMultiplier);
    settings[SETTING_FOG_COLOR] = mWaterFogColor.getValue();
    settings[SETTING_FOG_DENSITY] = LLSD::Real(mWaterFogDensity);
    settings[SETTING_FOG_MOD] = LLSD::Real(mFogMod);
    settings[SETTING_FRESNEL_OFFSET] = LLSD::Real(mFresnelOffset);
    settings[SETTING_FRESNEL_SCALE] = LLSD::Real(mFresnelScale);
    settings[SETTING_NORMAL_SCALE] = mNormalScale.getValue();
    settings[SETTING_SCALE_ABOVE] = LLSD::Real(mScaleAbove);
    settings[SETTING_SCALE_BELOW] = LLSD::Real(mScaleBelow);
    settings[SETTING_WAVE1_DIR] = mWave1Dir.getValue();
    settings[SETTING_WAVE2_DIR] = mWave2Dir.getValue();

    settings[SETTING_NORMAL_MAP] = mNormalMapID;
    settings[SETTING_TRANSPARENT_TEXTURE] = mTransparentTextureID;
}

LLSD LLSettingsWater::translateLegacySettings(LLSD legacy)
{
    bool converted_something(false);
    LLSD newsettings(defaults());

    if (legacy.has(SETTING_LEGACY_BLUR_MULTIPLIER))
    {
        newsettings[SETTING_BLUR_MULTIPLIER] = LLSD::Real(legacy[SETTING_LEGACY_BLUR_MULTIPLIER].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_FOG_COLOR))
    {
        newsettings[SETTING_FOG_COLOR] = LLColor3(legacy[SETTING_LEGACY_FOG_COLOR]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_FOG_DENSITY))
    {
        newsettings[SETTING_FOG_DENSITY] = LLSD::Real(legacy[SETTING_LEGACY_FOG_DENSITY]);
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_FOG_MOD))
    {
        newsettings[SETTING_FOG_MOD] = LLSD::Real(legacy[SETTING_LEGACY_FOG_MOD].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_FRESNEL_OFFSET))
    {
        newsettings[SETTING_FRESNEL_OFFSET] = LLSD::Real(legacy[SETTING_LEGACY_FRESNEL_OFFSET].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_FRESNEL_SCALE))
    {
        newsettings[SETTING_FRESNEL_SCALE] = LLSD::Real(legacy[SETTING_LEGACY_FRESNEL_SCALE].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_NORMAL_MAP))
    {
        newsettings[SETTING_NORMAL_MAP] = LLSD::UUID(legacy[SETTING_LEGACY_NORMAL_MAP].asUUID());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_NORMAL_SCALE))
    {
        newsettings[SETTING_NORMAL_SCALE] = LLVector3(legacy[SETTING_LEGACY_NORMAL_SCALE]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_SCALE_ABOVE))
    {
        newsettings[SETTING_SCALE_ABOVE] = LLSD::Real(legacy[SETTING_LEGACY_SCALE_ABOVE].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_SCALE_BELOW))
    {
        newsettings[SETTING_SCALE_BELOW] = LLSD::Real(legacy[SETTING_LEGACY_SCALE_BELOW].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_WAVE1_DIR))
    {
        newsettings[SETTING_WAVE1_DIR] = LLVector2(legacy[SETTING_LEGACY_WAVE1_DIR]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_LEGACY_WAVE2_DIR))
    {
        newsettings[SETTING_WAVE2_DIR] = LLVector2(legacy[SETTING_LEGACY_WAVE2_DIR]).getValue();
        converted_something |= true;
    }

    if (!converted_something)
        return LLSD();
    return newsettings;
}

void LLSettingsWater::blend(LLSettingsBase::ptr_t &end, F64 blendf)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_ENVIRONMENT;
    LLSettingsWater::ptr_t other = PTR_NAMESPACE::static_pointer_cast<LLSettingsWater>(end);
    if (other)
    {
        mSettingFlags |= other->mSettingFlags;

        mBlurMultiplier = lerp(mBlurMultiplier, other->mBlurMultiplier, (F32)blendf);
        lerpColor(mWaterFogColor, other->mWaterFogColor, (F32)blendf);
        mWaterFogDensity = lerp(mWaterFogDensity, other->mWaterFogDensity, (F32)blendf);
        mFogMod = lerp(mFogMod, other->mFogMod, (F32)blendf);
        mFresnelOffset = lerp(mFresnelOffset, other->mFresnelOffset, (F32)blendf);
        mFresnelScale = lerp(mFresnelScale, other->mFresnelScale, (F32)blendf);
        lerpVector3(mNormalScale, other->mNormalScale, (F32)blendf);
        mScaleAbove = lerp(mScaleAbove, other->mScaleAbove, (F32)blendf);
        mScaleBelow = lerp(mScaleBelow, other->mScaleBelow, (F32)blendf);
        lerpVector2(mWave1Dir, other->mWave1Dir, (F32)blendf);
        lerpVector2(mWave2Dir, other->mWave2Dir, (F32)blendf);

        setDirtyFlag(true);
        setReplaced();
        setLLSDDirty();

        mNextNormalMapID = other->getNormalMapID();
        mNextTransparentTextureID = other->getTransparentTextureID();
    }
    else
    {
        LL_WARNS("SETTINGS") << "Could not cast end settings to water. No blend performed." << LL_ENDL;
    }
    setBlendFactor(blendf);
}

void LLSettingsWater::replaceSettings(LLSD settings)
{
    LLSettingsBase::replaceSettings(settings);
    mNextNormalMapID.setNull();
    mNextTransparentTextureID.setNull();
}

void LLSettingsWater::replaceSettings(const LLSettingsBase::ptr_t& other_water)
{
    LLSettingsBase::replaceSettings(other_water);

    llassert(getSettingsType() == other_water->getSettingsType());

    LLSettingsWater::ptr_t other = PTR_NAMESPACE::dynamic_pointer_cast<LLSettingsWater>(other_water);

    mBlurMultiplier = other->mBlurMultiplier;
    mWaterFogColor = other->mWaterFogColor;
    mWaterFogDensity = other->mWaterFogDensity;
    mFogMod = other->mFogMod;
    mFresnelOffset = other->mFresnelOffset;
    mFresnelScale = other->mFresnelScale;
    mNormalScale = other->mNormalScale;
    mScaleAbove = other->mScaleAbove;
    mScaleBelow = other->mScaleBelow;
    mWave1Dir = other->mWave1Dir;
    mWave2Dir = other->mWave2Dir;

    mNormalMapID = other->mNormalMapID;
    mTransparentTextureID = other->mTransparentTextureID;

    mNextNormalMapID.setNull();
    mNextTransparentTextureID.setNull();
}

void LLSettingsWater::replaceWithWater(const LLSettingsWater::ptr_t& other)
{
    replaceWith(other);

    mNextNormalMapID = other->mNextNormalMapID;
    mNextTransparentTextureID = other->mNextTransparentTextureID;
}

LLSettingsWater::validation_list_t LLSettingsWater::getValidationList() const
{
    return LLSettingsWater::validationList();
}

LLSettingsWater::validation_list_t LLSettingsWater::validationList()
{
    static validation_list_t validation;

    if (validation.empty())
    {
        validation.push_back(Validator(SETTING_BLUR_MULTIPLIER, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(-0.5f, 0.5f))));
        validation.push_back(Validator(SETTING_FOG_COLOR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1, _2,
                llsd::array(0.0f, 0.0f, 0.0f, 1.0f),
                llsd::array(1.0f, 1.0f, 1.0f, 1.0f))));
        validation.push_back(Validator(SETTING_FOG_DENSITY, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.001f, 100.0f))));
        validation.push_back(Validator(SETTING_FOG_MOD, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.0f, 20.0f))));
        validation.push_back(Validator(SETTING_FRESNEL_OFFSET, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.0f, 1.0f))));
        validation.push_back(Validator(SETTING_FRESNEL_SCALE, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.0f, 1.0f))));
        validation.push_back(Validator(SETTING_NORMAL_MAP, true, LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_NORMAL_SCALE, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1, _2,
                llsd::array(0.0f, 0.0f, 0.0f),
                llsd::array(10.0f, 10.0f, 10.0f))));
        validation.push_back(Validator(SETTING_SCALE_ABOVE, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.0f, 3.0f))));
        validation.push_back(Validator(SETTING_SCALE_BELOW, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, _2, llsd::array(0.0f, 3.0f))));
        validation.push_back(Validator(SETTING_WAVE1_DIR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1, _2,
                llsd::array(-20.0f, -20.0f),
                llsd::array(20.0f, 20.0f))));
        validation.push_back(Validator(SETTING_WAVE2_DIR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1, _2,
                llsd::array(-20.0f, -20.0f),
                llsd::array(20.0f, 20.0f))));
    }

    return validation;
}

LLUUID LLSettingsWater::GetDefaultAssetId()
{
    return DEFAULT_ASSET_ID;
}

LLUUID LLSettingsWater::GetDefaultWaterNormalAssetId()
{
    return DEFAULT_WATER_NORMAL;
}

LLUUID LLSettingsWater::GetDefaultTransparentTextureAssetId()
{
    return DEFAULT_TRANSPARENT_WATER_TEXTURE;
}

LLUUID LLSettingsWater::GetDefaultOpaqueTextureAssetId()
{
    return DEFAULT_OPAQUE_WATER_TEXTURE;
}

F32 LLSettingsWater::getModifiedWaterFogDensity(bool underwater) const
{
    F32 fog_density = getWaterFogDensity();
    F32 underwater_fog_mod = getFogMod();
    if (underwater && underwater_fog_mod > 0.0f)
    {
        underwater_fog_mod = llclamp(underwater_fog_mod, 0.0f, 10.0f);
        // BUG-233797/BUG-233798 -ve underwater fog density can cause (unrecoverable) blackout.
        // raising a negative number to a non-integral power results in a non-real result (which is NaN for our purposes)
        // Two methods were tested, number 2 is being used:
        // 1) Force the fog_mod to be integral. The effect is unlikely to be nice, but it is better than blackness.
        // In this method a few of the combinations are "usable" but the water colour is effectively inverted (blue becomes yellow)
        // this seems to be unlikely to be a desirable use case for the majority.
        // 2) Force density to be an arbitrary non-negative (i.e. 1) when underwater and modifier is not an integer (1 was aribtrarily chosen as it gives at least some notion of fog in the transition)
        // This is more restrictive, effectively forcing a density under certain conditions, but allowing the range of #1 and avoiding blackness in other cases
        // at the cost of overriding the fog density.
        if(fog_density < 0.0f && underwater_fog_mod != (F32)llround(underwater_fog_mod) )
        {
            fog_density = 1.0f;
        }
        fog_density = pow(fog_density, underwater_fog_mod);
    }
    return fog_density;
}
