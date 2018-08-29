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
#include <boost/make_shared.hpp>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"
#include "indra_constants.h"

//=========================================================================
namespace
{
     LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment");
     LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment");
}

//=========================================================================
const std::string LLSettingsWater::SETTING_BLUR_MULTIPILER("blur_multiplier");
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

const std::string LLSettingsWater::SETTING_LEGACY_BLUR_MULTIPILER("blurMultiplier");
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

const LLUUID LLSettingsWater::DEFAULT_ASSET_ID("ce4cfe94-700a-292c-7c22-a2d9201bd661");

static const LLUUID DEFAULT_TRANSPARENT_WATER_TEXTURE("2bfd3884-7e27-69b9-ba3a-3e673f680004");
static const LLUUID DEFAULT_OPAQUE_WATER_TEXTURE("43c32285-d658-1793-c123-bf86315de055");

//=========================================================================
LLSettingsWater::LLSettingsWater(const LLSD &data) :
    LLSettingsBase(data),
    mNextNormalMapID()
{
}

LLSettingsWater::LLSettingsWater() :
    LLSettingsBase(),
    mNextNormalMapID()
{
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
        dfltsetting[SETTING_BLUR_MULTIPILER] = LLSD::Real(0.04000f);
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

LLSD LLSettingsWater::translateLegacySettings(LLSD legacy)
{
    LLSD newsettings(defaults());

    if (legacy.has(SETTING_LEGACY_BLUR_MULTIPILER))
    {
        newsettings[SETTING_BLUR_MULTIPILER] = LLSD::Real(legacy[SETTING_LEGACY_BLUR_MULTIPILER].asReal());
    }
    if (legacy.has(SETTING_LEGACY_FOG_COLOR))
    {
        newsettings[SETTING_FOG_COLOR] = LLColor3(legacy[SETTING_LEGACY_FOG_COLOR]).getValue();
    }
    if (legacy.has(SETTING_LEGACY_FOG_DENSITY))
    {
        newsettings[SETTING_FOG_DENSITY] = LLSD::Real(legacy[SETTING_LEGACY_FOG_DENSITY]);
    }
    if (legacy.has(SETTING_LEGACY_FOG_MOD))
    {
        newsettings[SETTING_FOG_MOD] = LLSD::Real(legacy[SETTING_LEGACY_FOG_MOD].asReal());
    }
    if (legacy.has(SETTING_LEGACY_FRESNEL_OFFSET))
    {
        newsettings[SETTING_FRESNEL_OFFSET] = LLSD::Real(legacy[SETTING_LEGACY_FRESNEL_OFFSET].asReal());
    }
    if (legacy.has(SETTING_LEGACY_FRESNEL_SCALE))
    {
        newsettings[SETTING_FRESNEL_SCALE] = LLSD::Real(legacy[SETTING_LEGACY_FRESNEL_SCALE].asReal());
    }
    if (legacy.has(SETTING_LEGACY_NORMAL_MAP))
    {
        newsettings[SETTING_NORMAL_MAP] = LLSD::UUID(legacy[SETTING_LEGACY_NORMAL_MAP].asUUID());
    }
    if (legacy.has(SETTING_LEGACY_NORMAL_SCALE))
    {
        newsettings[SETTING_NORMAL_SCALE] = LLVector3(legacy[SETTING_LEGACY_NORMAL_SCALE]).getValue();
    }
    if (legacy.has(SETTING_LEGACY_SCALE_ABOVE))
    {
        newsettings[SETTING_SCALE_ABOVE] = LLSD::Real(legacy[SETTING_LEGACY_SCALE_ABOVE].asReal());
    }
    if (legacy.has(SETTING_LEGACY_SCALE_BELOW))
    {
        newsettings[SETTING_SCALE_BELOW] = LLSD::Real(legacy[SETTING_LEGACY_SCALE_BELOW].asReal());
    }
    if (legacy.has(SETTING_LEGACY_WAVE1_DIR))
    {
        newsettings[SETTING_WAVE1_DIR] = LLVector2(legacy[SETTING_LEGACY_WAVE1_DIR]).getValue();
    }
    if (legacy.has(SETTING_LEGACY_WAVE2_DIR))
    {
        newsettings[SETTING_WAVE2_DIR] = LLVector2(legacy[SETTING_LEGACY_WAVE2_DIR]).getValue();
    }

    return newsettings;
}

void LLSettingsWater::blend(const LLSettingsBase::ptr_t &end, F64 blendf) 
{
    LLSettingsWater::ptr_t other = PTR_NAMESPACE::static_pointer_cast<LLSettingsWater>(end);
    if (other)
    {
        LLSD blenddata = interpolateSDMap(mSettings, other->mSettings, blendf);
        replaceSettings(blenddata);
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

LLSettingsWater::validation_list_t LLSettingsWater::getValidationList() const
{
    return LLSettingsWater::validationList();
}

LLSettingsWater::validation_list_t LLSettingsWater::validationList()
{
    static validation_list_t validation;

    if (validation.empty())
    {   // Note the use of LLSD(LLSDArray()()()...) This is due to an issue with the 
        // copy constructor for LLSDArray.  Directly binding the LLSDArray as 
        // a parameter without first wrapping it in a pure LLSD object will result 
        // in deeply nested arrays like this [[[[[[[[[[v1,v2,v3]]]]]]]]]]

        validation.push_back(Validator(SETTING_BLUR_MULTIPILER, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(0.16f)))));
        validation.push_back(Validator(SETTING_FOG_COLOR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)(1.0f)),
                LLSD(LLSDArray(1.0f)(1.0f)(1.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_FOG_DENSITY, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1.0f)(1024.0f)))));
        validation.push_back(Validator(SETTING_FOG_MOD, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1.0f)(1024.0f)))));
        validation.push_back(Validator(SETTING_FRESNEL_OFFSET, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_FRESNEL_SCALE, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_NORMAL_MAP, true, LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_NORMAL_SCALE, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)),
                LLSD(LLSDArray(10.0f)(10.0f)(10.0f)))));
        validation.push_back(Validator(SETTING_SCALE_ABOVE, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_SCALE_BELOW, true, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_WAVE1_DIR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(-4.0f)(-4.0f)),
                LLSD(LLSDArray(4.0f)(4.0f)))));
        validation.push_back(Validator(SETTING_WAVE2_DIR, true, LLSD::TypeArray,
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(-4.0f)(-4.0f)),
                LLSD(LLSDArray(4.0f)(4.0f)))));
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
