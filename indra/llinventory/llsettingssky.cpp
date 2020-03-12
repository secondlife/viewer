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

#include "llsettingssky.h"
#include "indra_constants.h"
#include <algorithm>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

//=========================================================================
namespace
{
    const LLUUID IMG_BLOOM1("3c59f7fe-9dc8-47f9-8aaf-a9dd1fbc3bef");
    const LLUUID IMG_RAINBOW("11b4c57c-56b3-04ed-1f82-2004363882e4");
    const LLUUID IMG_HALO("12149143-f599-91a7-77ac-b52a3c0f59cd");
}

namespace {
    LLQuaternion convert_azimuth_and_altitude_to_quat(F32 azimuth, F32 altitude)
    {
        F32 sinTheta = sin(azimuth);
        F32 cosTheta = cos(azimuth);
        F32 sinPhi   = sin(altitude);
        F32 cosPhi   = cos(altitude);

        LLVector3 dir;
        // +x right, +z up, +y at...	
        dir.mV[0] = cosTheta * cosPhi;
        dir.mV[1] = sinTheta * cosPhi;	
        dir.mV[2] = sinPhi;

        LLVector3 axis = LLVector3::x_axis % dir;
        axis.normalize();

        F32 angle = acos(LLVector3::x_axis * dir);

        LLQuaternion quat;
        quat.setAngleAxis(angle, axis);

        return quat;
    }
}

static LLTrace::BlockTimerStatHandle FTM_BLEND_SKYVALUES("Blending Sky Environment");
static LLTrace::BlockTimerStatHandle FTM_RECALCULATE_SKYVALUES("Recalculate Sky");
static LLTrace::BlockTimerStatHandle FTM_RECALCULATE_BODIES("Recalculate Heavenly Bodies");
static LLTrace::BlockTimerStatHandle FTM_RECALCULATE_LIGHTING("Recalculate Lighting");

//=========================================================================
const std::string LLSettingsSky::SETTING_AMBIENT("ambient");
const std::string LLSettingsSky::SETTING_BLUE_DENSITY("blue_density");
const std::string LLSettingsSky::SETTING_BLUE_HORIZON("blue_horizon");
const std::string LLSettingsSky::SETTING_DENSITY_MULTIPLIER("density_multiplier");
const std::string LLSettingsSky::SETTING_DISTANCE_MULTIPLIER("distance_multiplier");
const std::string LLSettingsSky::SETTING_HAZE_DENSITY("haze_density");
const std::string LLSettingsSky::SETTING_HAZE_HORIZON("haze_horizon");

const std::string LLSettingsSky::SETTING_BLOOM_TEXTUREID("bloom_id");
const std::string LLSettingsSky::SETTING_RAINBOW_TEXTUREID("rainbow_id");
const std::string LLSettingsSky::SETTING_HALO_TEXTUREID("halo_id");
const std::string LLSettingsSky::SETTING_CLOUD_COLOR("cloud_color");
const std::string LLSettingsSky::SETTING_CLOUD_POS_DENSITY1("cloud_pos_density1");
const std::string LLSettingsSky::SETTING_CLOUD_POS_DENSITY2("cloud_pos_density2");
const std::string LLSettingsSky::SETTING_CLOUD_SCALE("cloud_scale");
const std::string LLSettingsSky::SETTING_CLOUD_SCROLL_RATE("cloud_scroll_rate");
const std::string LLSettingsSky::SETTING_CLOUD_SHADOW("cloud_shadow");
const std::string LLSettingsSky::SETTING_CLOUD_TEXTUREID("cloud_id");
const std::string LLSettingsSky::SETTING_CLOUD_VARIANCE("cloud_variance");

const std::string LLSettingsSky::SETTING_DOME_OFFSET("dome_offset");
const std::string LLSettingsSky::SETTING_DOME_RADIUS("dome_radius");
const std::string LLSettingsSky::SETTING_GAMMA("gamma");
const std::string LLSettingsSky::SETTING_GLOW("glow");

const std::string LLSettingsSky::SETTING_LIGHT_NORMAL("lightnorm");
const std::string LLSettingsSky::SETTING_MAX_Y("max_y");
const std::string LLSettingsSky::SETTING_MOON_ROTATION("moon_rotation");
const std::string LLSettingsSky::SETTING_MOON_SCALE("moon_scale");
const std::string LLSettingsSky::SETTING_MOON_TEXTUREID("moon_id");
const std::string LLSettingsSky::SETTING_MOON_BRIGHTNESS("moon_brightness");

const std::string LLSettingsSky::SETTING_STAR_BRIGHTNESS("star_brightness");
const std::string LLSettingsSky::SETTING_SUNLIGHT_COLOR("sunlight_color");
const std::string LLSettingsSky::SETTING_SUN_ROTATION("sun_rotation");
const std::string LLSettingsSky::SETTING_SUN_SCALE("sun_scale");
const std::string LLSettingsSky::SETTING_SUN_TEXTUREID("sun_id");

const std::string LLSettingsSky::SETTING_LEGACY_EAST_ANGLE("east_angle");
const std::string LLSettingsSky::SETTING_LEGACY_ENABLE_CLOUD_SCROLL("enable_cloud_scroll");
const std::string LLSettingsSky::SETTING_LEGACY_SUN_ANGLE("sun_angle");

// these are new settings for the advanced atmospherics model
const std::string LLSettingsSky::SETTING_PLANET_RADIUS("planet_radius");
const std::string LLSettingsSky::SETTING_SKY_BOTTOM_RADIUS("sky_bottom_radius");
const std::string LLSettingsSky::SETTING_SKY_TOP_RADIUS("sky_top_radius");
const std::string LLSettingsSky::SETTING_SUN_ARC_RADIANS("sun_arc_radians");

const std::string LLSettingsSky::SETTING_RAYLEIGH_CONFIG("rayleigh_config");
const std::string LLSettingsSky::SETTING_MIE_CONFIG("mie_config");
const std::string LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR("anisotropy");
const std::string LLSettingsSky::SETTING_ABSORPTION_CONFIG("absorption_config");

const std::string LLSettingsSky::KEY_DENSITY_PROFILE("density");
const std::string LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH("width");
const std::string LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM("exp_term");
const std::string LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR("exp_scale");
const std::string LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM("linear_term");
const std::string LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM("constant_term");

const std::string LLSettingsSky::SETTING_SKY_MOISTURE_LEVEL("moisture_level");
const std::string LLSettingsSky::SETTING_SKY_DROPLET_RADIUS("droplet_radius");
const std::string LLSettingsSky::SETTING_SKY_ICE_LEVEL("ice_level");

const LLUUID LLSettingsSky::DEFAULT_ASSET_ID("3ae23978-ac82-bcf3-a9cb-ba6e52dcb9ad");

static const LLUUID DEFAULT_SUN_ID("32bfbcea-24b1-fb9d-1ef9-48a28a63730f"); // dataserver
static const LLUUID DEFAULT_MOON_ID("d07f6eed-b96a-47cd-b51d-400ad4a1c428"); // dataserver
static const LLUUID DEFAULT_CLOUD_ID("1dc1368f-e8fe-f02d-a08d-9d9f11c1af6b");

const std::string LLSettingsSky::SETTING_LEGACY_HAZE("legacy_haze");

const F32 LLSettingsSky::DOME_OFFSET(0.96f);
const F32 LLSettingsSky::DOME_RADIUS(15000.f);

namespace
{

LLSettingsSky::validation_list_t legacyHazeValidationList()
{
    static LLSettingsBase::validation_list_t legacyHazeValidation;
    if (legacyHazeValidation.empty())
    {
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_AMBIENT,             false,  LLSD::TypeArray, 
            boost::bind(&LLSettingsBase::Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(3.0f)(3.0f)(3.0f)("*")))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_BLUE_DENSITY,        false,  LLSD::TypeArray, 
            boost::bind(&LLSettingsBase::Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(3.0f)(3.0f)(3.0f)("*")))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_BLUE_HORIZON,        false,  LLSD::TypeArray, 
            boost::bind(&LLSettingsBase::Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(3.0f)(3.0f)(3.0f)("*")))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_HAZE_DENSITY,        false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(5.0f)))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_HAZE_HORIZON,        false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(5.0f)))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_MULTIPLIER,  false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0001f)(2.0f)))));
        legacyHazeValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DISTANCE_MULTIPLIER, false,  LLSD::TypeReal,
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0001f)(1000.0f)))));
    }
    return legacyHazeValidation;
}

LLSettingsSky::validation_list_t rayleighValidationList()
{
    static LLSettingsBase::validation_list_t rayleighValidation;
    if (rayleighValidation.empty())
    {
        rayleighValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH,      false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(32768.0f)))));

        rayleighValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM,   false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));
        
        rayleighValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(-1.0f)(1.0f)))));

        rayleighValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));

        rayleighValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
    }
    return rayleighValidation;
}

LLSettingsSky::validation_list_t absorptionValidationList()
{
    static LLSettingsBase::validation_list_t absorptionValidation;
    if (absorptionValidation.empty())
    {
        absorptionValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH,      false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(32768.0f)))));

        absorptionValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM,   false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));
        
        absorptionValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(-1.0f)(1.0f)))));

        absorptionValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));

        absorptionValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
    }
    return absorptionValidation;
}

LLSettingsSky::validation_list_t mieValidationList()
{
    static LLSettingsBase::validation_list_t mieValidation;
    if (mieValidation.empty())
    {
        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH,      false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(32768.0f)))));

        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM,   false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));
        
        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(-1.0f)(1.0f)))));

        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(2.0f)))));

        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));

        mieValidation.push_back(LLSettingsBase::Validator(LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR, false,  LLSD::TypeReal,  
            boost::bind(&LLSettingsBase::Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
    }
    return mieValidation;
}

bool validateLegacyHaze(LLSD &value)
{
    LLSettingsSky::validation_list_t legacyHazeValidations = legacyHazeValidationList();
    llassert(value.type() == LLSD::TypeMap);
    LLSD result = LLSettingsBase::settingValidation(value, legacyHazeValidations);
    if (result["errors"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Legacy Haze Config Validation errors: " << result["errors"] << LL_ENDL;
        return false;
    }
    if (result["warnings"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Legacy Haze Config Validation warnings: " << result["warnings"] << LL_ENDL;
        return false;
    }
    return true;
}

bool validateRayleighLayers(LLSD &value)
{
    LLSettingsSky::validation_list_t rayleighValidations = rayleighValidationList();
    if (value.isArray())
    {
        bool allGood = true;
        for (LLSD::array_iterator itf = value.beginArray(); itf != value.endArray(); ++itf)
        {
            LLSD& layerConfig = (*itf);
            if (layerConfig.type() == LLSD::TypeMap)
            {
                if (!validateRayleighLayers(layerConfig))
                {
                    allGood = false;
                }
            }
            else if (layerConfig.type() == LLSD::TypeArray)
            {
                return validateRayleighLayers(layerConfig);
            }
            else
            {
                return LLSettingsBase::settingValidation(value, rayleighValidations);
            }
        }
        return allGood;
    }    
    llassert(value.type() == LLSD::TypeMap);
    LLSD result = LLSettingsBase::settingValidation(value, rayleighValidations);
    if (result["errors"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Rayleigh Config Validation errors: " << result["errors"] << LL_ENDL;
        return false;
    }
    if (result["warnings"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Rayleigh Config Validation warnings: " << result["errors"] << LL_ENDL;
        return false;
    }
    return true;
}

bool validateAbsorptionLayers(LLSD &value)
{
    LLSettingsBase::validation_list_t absorptionValidations = absorptionValidationList();
    if (value.isArray())
    {
        bool allGood = true;   
        for (LLSD::array_iterator itf = value.beginArray(); itf != value.endArray(); ++itf)
        {
            LLSD& layerConfig = (*itf);
            if (layerConfig.type() == LLSD::TypeMap)
            {
                if (!validateAbsorptionLayers(layerConfig))
                {
                    allGood = false;
                }
            }
            else if (layerConfig.type() == LLSD::TypeArray)
            {
                return validateAbsorptionLayers(layerConfig);
            }
            else
            {
                return LLSettingsBase::settingValidation(value, absorptionValidations);
            }
        }
        return allGood;
    }
    llassert(value.type() == LLSD::TypeMap);
    LLSD result = LLSettingsBase::settingValidation(value, absorptionValidations);
    if (result["errors"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Absorption Config Validation errors: " << result["errors"] << LL_ENDL;
        return false;
    }
    if (result["warnings"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Absorption Config Validation warnings: " << result["errors"] << LL_ENDL;
        return false;
    }
    return true;
}

bool validateMieLayers(LLSD &value)
{
    LLSettingsBase::validation_list_t mieValidations = mieValidationList();
    if (value.isArray())
    {
        bool allGood = true;
        for (LLSD::array_iterator itf = value.beginArray(); itf != value.endArray(); ++itf)
        {
            LLSD& layerConfig = (*itf);
            if (layerConfig.type() == LLSD::TypeMap)
            {
                if (!validateMieLayers(layerConfig))
                {
                    allGood = false;
                }
            }
            else if (layerConfig.type() == LLSD::TypeArray)
            {
                return validateMieLayers(layerConfig);
            }
            else
            {
                return LLSettingsBase::settingValidation(value, mieValidations);
            }
        }
        return allGood;
    }
    LLSD result = LLSettingsBase::settingValidation(value, mieValidations);
    if (result["errors"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Mie Config Validation errors: " << result["errors"] << LL_ENDL;
        return false;
    }
    if (result["warnings"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Mie Config Validation warnings: " << result["warnings"] << LL_ENDL;
        return false;
    }
    return true;
}

}

//=========================================================================
LLSettingsSky::LLSettingsSky(const LLSD &data) :
    LLSettingsBase(data),
    mNextSunTextureId(),
    mNextMoonTextureId(),
    mNextCloudTextureId(),
    mNextBloomTextureId(),
    mNextRainbowTextureId(),
    mNextHaloTextureId()
{
}

LLSettingsSky::LLSettingsSky():
    LLSettingsBase(),
    mNextSunTextureId(),
    mNextMoonTextureId(),
    mNextCloudTextureId(),
    mNextBloomTextureId(),
    mNextRainbowTextureId(),
    mNextHaloTextureId()
{
}

void LLSettingsSky::replaceSettings(LLSD settings)
{
    LLSettingsBase::replaceSettings(settings);
    mNextSunTextureId.setNull();
    mNextMoonTextureId.setNull();
    mNextCloudTextureId.setNull();
    mNextBloomTextureId.setNull();
    mNextRainbowTextureId.setNull();
    mNextHaloTextureId.setNull();
}

void LLSettingsSky::replaceWithSky(LLSettingsSky::ptr_t pother)
{
    replaceWith(pother);

    mNextSunTextureId = pother->mNextSunTextureId;
    mNextMoonTextureId = pother->mNextMoonTextureId;
    mNextCloudTextureId = pother->mNextCloudTextureId;
    mNextBloomTextureId = pother->mNextBloomTextureId;
    mNextRainbowTextureId = pother->mNextRainbowTextureId;
    mNextHaloTextureId = pother->mNextHaloTextureId;
}

void LLSettingsSky::blend(const LLSettingsBase::ptr_t &end, F64 blendf) 
{
    llassert(getSettingsType() == end->getSettingsType());

    LLSettingsSky::ptr_t other = PTR_NAMESPACE::dynamic_pointer_cast<LLSettingsSky>(end);
    if (other)
    {
        if (other->mSettings.has(SETTING_LEGACY_HAZE))
        {
            if (!mSettings.has(SETTING_LEGACY_HAZE) || !mSettings[SETTING_LEGACY_HAZE].has(SETTING_AMBIENT))
            {
                // Special case since SETTING_AMBIENT is both in outer and legacy maps, we prioritize legacy one
                // see getAmbientColor(), we are about to replaceSettings(), so we are free to set it
                setAmbientColor(getAmbientColor());
            }
        }
        else
        {
            if (mSettings.has(SETTING_LEGACY_HAZE) && mSettings[SETTING_LEGACY_HAZE].has(SETTING_AMBIENT))
            {
                // Special case due to ambient's duality
                // We need to match 'other's' structure for interpolation.
                // We are free to change mSettings, since we are about to reset it
                mSettings[SETTING_AMBIENT] = getAmbientColor().getValue();
                mSettings[SETTING_LEGACY_HAZE].erase(SETTING_AMBIENT);
            }
        }

        LLUUID cloud_noise_id = getCloudNoiseTextureId();
        LLUUID cloud_noise_id_next = other->getCloudNoiseTextureId();
        F64 cloud_shadow = 0;
        if (!cloud_noise_id.isNull() && cloud_noise_id_next.isNull())
        {
            // If there is no cloud texture in destination, reduce coverage to imitate disappearance
            // See LLDrawPoolWLSky::renderSkyClouds... we don't blend present texture with null
            // Note: Probably can be done by shader
            cloud_shadow = lerp(mSettings[SETTING_CLOUD_SHADOW].asReal(), (F64)0.f, blendf);
            cloud_noise_id_next = cloud_noise_id;
        }
        else if (cloud_noise_id.isNull() && !cloud_noise_id_next.isNull())
        {
            // Source has no cloud texture, reduce initial coverage to imitate appearance
            // use same texture as destination
            cloud_shadow = lerp((F64)0.f, other->mSettings[SETTING_CLOUD_SHADOW].asReal(), blendf);
            setCloudNoiseTextureId(cloud_noise_id_next);
        }
        else
        {
            cloud_shadow = lerp(mSettings[SETTING_CLOUD_SHADOW].asReal(), other->mSettings[SETTING_CLOUD_SHADOW].asReal(), blendf);
        }

        LLSD blenddata = interpolateSDMap(mSettings, other->mSettings, other->getParameterMap(), blendf);
        blenddata[SETTING_CLOUD_SHADOW] = LLSD::Real(cloud_shadow);
        replaceSettings(blenddata);
        mNextSunTextureId = other->getSunTextureId();
        mNextMoonTextureId = other->getMoonTextureId();
        mNextCloudTextureId = cloud_noise_id_next;
        mNextBloomTextureId = other->getBloomTextureId();
        mNextRainbowTextureId = other->getRainbowTextureId();
        mNextHaloTextureId = other->getHaloTextureId();
    }
    else
    {
        LL_WARNS("SETTINGS") << "Could not cast end settings to sky. No blend performed." << LL_ENDL;
    }

    setBlendFactor(blendf);
}

LLSettingsSky::stringset_t LLSettingsSky::getSkipInterpolateKeys() const
{
    static stringset_t skipSet;

    if (skipSet.empty())
    {
        skipSet = LLSettingsBase::getSkipInterpolateKeys();
        skipSet.insert(SETTING_RAYLEIGH_CONFIG);
        skipSet.insert(SETTING_MIE_CONFIG);
        skipSet.insert(SETTING_ABSORPTION_CONFIG);
        skipSet.insert(SETTING_CLOUD_SHADOW);
    }

    return skipSet;
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

LLSettingsSky::validation_list_t LLSettingsSky::getValidationList() const
{
    return LLSettingsSky::validationList();
}

LLSettingsSky::validation_list_t LLSettingsSky::validationList()
{
    static validation_list_t validation;

    if (validation.empty())
    {   // Note the use of LLSD(LLSDArray()()()...) This is due to an issue with the 
        // copy constructor for LLSDArray.  Directly binding the LLSDArray as 
        // a parameter without first wrapping it in a pure LLSD object will result 
        // in deeply nested arrays like this [[[[[[[[[[v1,v2,v3]]]]]]]]]]
        validation.push_back(Validator(SETTING_BLOOM_TEXTUREID,     true,  LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_RAINBOW_TEXTUREID,   false,  LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_HALO_TEXTUREID,      false,  LLSD::TypeUUID));

        validation.push_back(Validator(SETTING_CLOUD_COLOR,         true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(1.0f)(1.0f)(1.0f)("*")))));
        validation.push_back(Validator(SETTING_CLOUD_POS_DENSITY1,  true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(1.0f)(1.0f)(3.0f)("*")))));
        validation.push_back(Validator(SETTING_CLOUD_POS_DENSITY2,  true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(1.0f)(1.0f)(1.0f)("*")))));
        validation.push_back(Validator(SETTING_CLOUD_SCALE,         true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.001f)(3.0f)))));
        validation.push_back(Validator(SETTING_CLOUD_SCROLL_RATE,   true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(-50.0f)(-50.0f)),
                LLSD(LLSDArray(50.0f)(50.0f)))));
        validation.push_back(Validator(SETTING_CLOUD_SHADOW,        true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_CLOUD_TEXTUREID,     false, LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_CLOUD_VARIANCE,      false,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));

        validation.push_back(Validator(SETTING_DOME_OFFSET,         false, LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));
        validation.push_back(Validator(SETTING_DOME_RADIUS,         false, LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1000.0f)(2000.0f)))));
        validation.push_back(Validator(SETTING_GAMMA,               true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(20.0f)))));
        validation.push_back(Validator(SETTING_GLOW,                true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.2f)("*")(-10.0f)("*")),
                LLSD(LLSDArray(40.0f)("*")(10.0f)("*")))));
        
        validation.push_back(Validator(SETTING_MAX_Y,               true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(10000.0f)))));
        validation.push_back(Validator(SETTING_MOON_ROTATION,       true,  LLSD::TypeArray, &Validator::verifyQuaternionNormal));
        validation.push_back(Validator(SETTING_MOON_SCALE,          false, LLSD::TypeReal,
                boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.25f)(20.0f))), LLSD::Real(1.0)));
        validation.push_back(Validator(SETTING_MOON_TEXTUREID,      false, LLSD::TypeUUID));
        validation.push_back(Validator(SETTING_MOON_BRIGHTNESS,     false,  LLSD::TypeReal, 
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));

        validation.push_back(Validator(SETTING_STAR_BRIGHTNESS,     true,  LLSD::TypeReal, 
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(500.0f)))));
        validation.push_back(Validator(SETTING_SUNLIGHT_COLOR,      true,  LLSD::TypeArray, 
            boost::bind(&Validator::verifyVectorMinMax, _1,
                LLSD(LLSDArray(0.0f)(0.0f)(0.0f)("*")),
                LLSD(LLSDArray(3.0f)(3.0f)(3.0f)("*")))));
        validation.push_back(Validator(SETTING_SUN_ROTATION,        true,  LLSD::TypeArray, &Validator::verifyQuaternionNormal));
        validation.push_back(Validator(SETTING_SUN_SCALE,           false, LLSD::TypeReal,
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.25f)(20.0f))), LLSD::Real(1.0)));
        validation.push_back(Validator(SETTING_SUN_TEXTUREID, false, LLSD::TypeUUID));

        validation.push_back(Validator(SETTING_PLANET_RADIUS,       true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1000.0f)(32768.0f)))));

        validation.push_back(Validator(SETTING_SKY_BOTTOM_RADIUS,   true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1000.0f)(32768.0f)))));

        validation.push_back(Validator(SETTING_SKY_TOP_RADIUS,       true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(1000.0f)(32768.0f)))));

        validation.push_back(Validator(SETTING_SUN_ARC_RADIANS,      true,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(0.1f)))));

        validation.push_back(Validator(SETTING_SKY_MOISTURE_LEVEL,      false,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));

        validation.push_back(Validator(SETTING_SKY_DROPLET_RADIUS,      false,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(5.0f)(1000.0f)))));

        validation.push_back(Validator(SETTING_SKY_ICE_LEVEL,      false,  LLSD::TypeReal,  
            boost::bind(&Validator::verifyFloatRange, _1, LLSD(LLSDArray(0.0f)(1.0f)))));

        validation.push_back(Validator(SETTING_RAYLEIGH_CONFIG, true, LLSD::TypeArray, &validateRayleighLayers));
        validation.push_back(Validator(SETTING_ABSORPTION_CONFIG, true, LLSD::TypeArray, &validateAbsorptionLayers));
        validation.push_back(Validator(SETTING_MIE_CONFIG, true, LLSD::TypeArray, &validateMieLayers));
        validation.push_back(Validator(SETTING_LEGACY_HAZE, false, LLSD::TypeMap, &validateLegacyHaze));
    }
    return validation;
}

LLSD LLSettingsSky::createDensityProfileLayer(
    F32 width,
    F32 exponential_term,
    F32 exponential_scale_factor,
    F32 linear_term,
    F32 constant_term,
    F32 aniso_factor)
{
    LLSD dflt_layer;
    dflt_layer[SETTING_DENSITY_PROFILE_WIDTH]            = width; // 0 -> the entire atmosphere
    dflt_layer[SETTING_DENSITY_PROFILE_EXP_TERM]         = exponential_term;
    dflt_layer[SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR] = exponential_scale_factor;
    dflt_layer[SETTING_DENSITY_PROFILE_LINEAR_TERM]      = linear_term;
    dflt_layer[SETTING_DENSITY_PROFILE_CONSTANT_TERM]    = constant_term;

    if (aniso_factor != 0.0f)
    {
        dflt_layer[SETTING_MIE_ANISOTROPY_FACTOR] = aniso_factor;
    }

    return dflt_layer;
}

LLSD LLSettingsSky::createSingleLayerDensityProfile(
    F32 width,
    F32 exponential_term,
    F32 exponential_scale_factor,
    F32 linear_term,
    F32 constant_term,
    F32 aniso_factor)
{
    LLSD dflt;
    LLSD dflt_layer = createDensityProfileLayer(width, exponential_term, exponential_scale_factor, linear_term, constant_term, aniso_factor);
    dflt.append(dflt_layer);
    return dflt;
}

LLSD LLSettingsSky::rayleighConfigDefault()
{
    return createSingleLayerDensityProfile(0.0f,  1.0f, -1.0f / 8000.0f, 0.0f, 0.0f);
}

LLSD LLSettingsSky::absorptionConfigDefault()
{
// absorption (ozone) has two linear ramping zones
    LLSD dflt_absorption_layer_a = createDensityProfileLayer(25000.0f, 0.0f, 0.0f, -1.0f / 25000.0f, -2.0f / 3.0f);
    LLSD dflt_absorption_layer_b = createDensityProfileLayer(0.0f, 0.0f, 0.0f, -1.0f / 15000.0f, 8.0f / 3.0f);
    LLSD dflt_absorption;
    dflt_absorption.append(dflt_absorption_layer_a);
    dflt_absorption.append(dflt_absorption_layer_b);
    return dflt_absorption;
}

LLSD LLSettingsSky::mieConfigDefault()
{
    LLSD dflt_mie = createSingleLayerDensityProfile(0.0f,  1.0f, -1.0f / 1200.0f, 0.0f, 0.0f, 0.8f);
    return dflt_mie;
}

LLSD LLSettingsSky::defaults(const LLSettingsBase::TrackPosition& position)
{
    static LLSD dfltsetting;

    if (dfltsetting.size() == 0)
    {
        LLQuaternion sunquat;
        LLQuaternion moonquat;

        F32 azimuth  = (F_PI * position) + (80.0f * DEG_TO_RAD);
        F32 altitude = (F_PI * position);

        // give the sun and moon slightly different tracks through the sky
        // instead of positioning them at opposite poles from each other...
        sunquat  = convert_azimuth_and_altitude_to_quat(altitude,                   azimuth);
        moonquat = convert_azimuth_and_altitude_to_quat(altitude + (F_PI * 0.125f), azimuth + (F_PI * 0.125f));

        // Magic constants copied form dfltsetting.xml 
        dfltsetting[SETTING_CLOUD_COLOR]        = LLColor4(0.4099, 0.4099, 0.4099, 0.0).getValue();
        dfltsetting[SETTING_CLOUD_POS_DENSITY1] = LLColor4(1.0000, 0.5260, 1.0000, 0.0).getValue();
        dfltsetting[SETTING_CLOUD_POS_DENSITY2] = LLColor4(1.0000, 0.5260, 1.0000, 0.0).getValue();
        dfltsetting[SETTING_CLOUD_SCALE]        = LLSD::Real(0.4199);
        dfltsetting[SETTING_CLOUD_SCROLL_RATE]  = LLSDArray(0.0f)(0.0f);
        dfltsetting[SETTING_CLOUD_SHADOW]       = LLSD::Real(0.2699);
        dfltsetting[SETTING_CLOUD_VARIANCE]     = LLSD::Real(0.0);

        dfltsetting[SETTING_DOME_OFFSET]        = LLSD::Real(0.96f);
        dfltsetting[SETTING_DOME_RADIUS]        = LLSD::Real(15000.f);
        dfltsetting[SETTING_GAMMA]              = LLSD::Real(1.0);
        dfltsetting[SETTING_GLOW]               = LLColor4(5.000, 0.0010, -0.4799, 1.0).getValue();
    
        dfltsetting[SETTING_MAX_Y]              = LLSD::Real(1605);
        dfltsetting[SETTING_MOON_ROTATION]      = moonquat.getValue();
        dfltsetting[SETTING_MOON_BRIGHTNESS]    = LLSD::Real(0.5f);

        dfltsetting[SETTING_STAR_BRIGHTNESS]    = LLSD::Real(250.0000);
        dfltsetting[SETTING_SUNLIGHT_COLOR]     = LLColor4(0.7342, 0.7815, 0.8999, 0.0).getValue();
        dfltsetting[SETTING_SUN_ROTATION]       = sunquat.getValue();

        dfltsetting[SETTING_BLOOM_TEXTUREID]    = GetDefaultBloomTextureId();
        dfltsetting[SETTING_CLOUD_TEXTUREID]    = GetDefaultCloudNoiseTextureId();
        dfltsetting[SETTING_MOON_TEXTUREID]     = GetDefaultMoonTextureId();
        dfltsetting[SETTING_SUN_TEXTUREID]      = GetDefaultSunTextureId();
        dfltsetting[SETTING_RAINBOW_TEXTUREID]  = GetDefaultRainbowTextureId();
        dfltsetting[SETTING_HALO_TEXTUREID]     = GetDefaultHaloTextureId();

        dfltsetting[SETTING_TYPE] = "sky";

        // defaults are for earth...
        dfltsetting[SETTING_PLANET_RADIUS]      = 6360.0f;
        dfltsetting[SETTING_SKY_BOTTOM_RADIUS]  = 6360.0f;
        dfltsetting[SETTING_SKY_TOP_RADIUS]     = 6420.0f;
        dfltsetting[SETTING_SUN_ARC_RADIANS]    = 0.00045f;

        dfltsetting[SETTING_SKY_MOISTURE_LEVEL] = 0.0f;
        dfltsetting[SETTING_SKY_DROPLET_RADIUS] = 800.0f;
        dfltsetting[SETTING_SKY_ICE_LEVEL]      = 0.0f;

        dfltsetting[SETTING_RAYLEIGH_CONFIG]    = rayleighConfigDefault();
        dfltsetting[SETTING_MIE_CONFIG]         = mieConfigDefault();
        dfltsetting[SETTING_ABSORPTION_CONFIG]  = absorptionConfigDefault();
    }

    return dfltsetting;
}

LLSD LLSettingsSky::translateLegacyHazeSettings(const LLSD& legacy)
{
    LLSD legacyhazesettings;

// AdvancedAtmospherics TODO
// These need to be translated into density profile info in the new settings format...
// LEGACY_ATMOSPHERICS    
    if (legacy.has(SETTING_AMBIENT))
    {
        legacyhazesettings[SETTING_AMBIENT] = LLColor3(legacy[SETTING_AMBIENT]).getValue();
    }
    if (legacy.has(SETTING_BLUE_DENSITY))
    {
        legacyhazesettings[SETTING_BLUE_DENSITY] = LLColor3(legacy[SETTING_BLUE_DENSITY]).getValue();
    }
    if (legacy.has(SETTING_BLUE_HORIZON))
    {
        legacyhazesettings[SETTING_BLUE_HORIZON] = LLColor3(legacy[SETTING_BLUE_HORIZON]).getValue();
    }
    if (legacy.has(SETTING_DENSITY_MULTIPLIER))
    {
        legacyhazesettings[SETTING_DENSITY_MULTIPLIER] = LLSD::Real(legacy[SETTING_DENSITY_MULTIPLIER][0].asReal());
    }
    if (legacy.has(SETTING_DISTANCE_MULTIPLIER))
    {
        legacyhazesettings[SETTING_DISTANCE_MULTIPLIER] = LLSD::Real(legacy[SETTING_DISTANCE_MULTIPLIER][0].asReal());
    }
    if (legacy.has(SETTING_HAZE_DENSITY))
    {
        legacyhazesettings[SETTING_HAZE_DENSITY] = LLSD::Real(legacy[SETTING_HAZE_DENSITY][0].asReal());
    }
    if (legacy.has(SETTING_HAZE_HORIZON))
    {
        legacyhazesettings[SETTING_HAZE_HORIZON] = LLSD::Real(legacy[SETTING_HAZE_HORIZON][0].asReal());
    }

    return legacyhazesettings;
}

LLSD LLSettingsSky::translateLegacySettings(const LLSD& legacy)
{
    bool converted_something(false);
    LLSD newsettings(defaults());

    // Move legacy haze parameters to an inner map
    // allowing backward compat and simple conversion to legacy format
    LLSD legacyhazesettings;
    legacyhazesettings = translateLegacyHazeSettings(legacy);
    if (legacyhazesettings.size() > 0)
    {
        newsettings[SETTING_LEGACY_HAZE] = legacyhazesettings;
        converted_something |= true;
    }

    if (legacy.has(SETTING_CLOUD_COLOR))
    {
        newsettings[SETTING_CLOUD_COLOR] = LLColor3(legacy[SETTING_CLOUD_COLOR]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_CLOUD_POS_DENSITY1))
    {
        newsettings[SETTING_CLOUD_POS_DENSITY1] = LLColor3(legacy[SETTING_CLOUD_POS_DENSITY1]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_CLOUD_POS_DENSITY2))
    {
        newsettings[SETTING_CLOUD_POS_DENSITY2] = LLColor3(legacy[SETTING_CLOUD_POS_DENSITY2]).getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_CLOUD_SCALE))
    {
        newsettings[SETTING_CLOUD_SCALE] = LLSD::Real(legacy[SETTING_CLOUD_SCALE][0].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_CLOUD_SCROLL_RATE))
    {
        LLVector2 cloud_scroll(legacy[SETTING_CLOUD_SCROLL_RATE]);

        cloud_scroll -= LLVector2(10, 10);
        if (legacy.has(SETTING_LEGACY_ENABLE_CLOUD_SCROLL))
        {
            LLSD enabled = legacy[SETTING_LEGACY_ENABLE_CLOUD_SCROLL];
            if (!enabled[0].asBoolean())
                cloud_scroll.mV[0] = 0.0f;
            if (!enabled[1].asBoolean())
                cloud_scroll.mV[1] = 0.0f;
        }

        newsettings[SETTING_CLOUD_SCROLL_RATE] = cloud_scroll.getValue();
        converted_something |= true;
    }
    if (legacy.has(SETTING_CLOUD_SHADOW))
    {
        newsettings[SETTING_CLOUD_SHADOW] = LLSD::Real(legacy[SETTING_CLOUD_SHADOW][0].asReal());
        converted_something |= true;
    }
    

    if (legacy.has(SETTING_GAMMA))
    {
        newsettings[SETTING_GAMMA] = legacy[SETTING_GAMMA][0].asReal();
        converted_something |= true;
    }
    if (legacy.has(SETTING_GLOW))
    {
        newsettings[SETTING_GLOW] = LLColor3(legacy[SETTING_GLOW]).getValue();
        converted_something |= true;
    }

    if (legacy.has(SETTING_MAX_Y))
    {
        newsettings[SETTING_MAX_Y] = LLSD::Real(legacy[SETTING_MAX_Y][0].asReal());
        converted_something |= true;
    }
    if (legacy.has(SETTING_STAR_BRIGHTNESS))
    {
        newsettings[SETTING_STAR_BRIGHTNESS] = LLSD::Real(legacy[SETTING_STAR_BRIGHTNESS].asReal() * 250.0f);
        converted_something |= true;
    }
    if (legacy.has(SETTING_SUNLIGHT_COLOR))
    {
        newsettings[SETTING_SUNLIGHT_COLOR] = LLColor4(legacy[SETTING_SUNLIGHT_COLOR]).getValue();
        converted_something |= true;
    }

    if (legacy.has(SETTING_PLANET_RADIUS))
    {
        newsettings[SETTING_PLANET_RADIUS] = LLSD::Real(legacy[SETTING_PLANET_RADIUS].asReal());
        converted_something |= true;
    }

    if (legacy.has(SETTING_SKY_BOTTOM_RADIUS))
    {
        newsettings[SETTING_SKY_BOTTOM_RADIUS] = LLSD::Real(legacy[SETTING_SKY_BOTTOM_RADIUS].asReal());
        converted_something |= true;
    }

    if (legacy.has(SETTING_SKY_TOP_RADIUS))
    {
        newsettings[SETTING_SKY_TOP_RADIUS] = LLSD::Real(legacy[SETTING_SKY_TOP_RADIUS].asReal());
        converted_something |= true;
    }

    if (legacy.has(SETTING_SUN_ARC_RADIANS))
    {
        newsettings[SETTING_SUN_ARC_RADIANS] = LLSD::Real(legacy[SETTING_SUN_ARC_RADIANS].asReal());
        converted_something |= true;
    }

    if (legacy.has(SETTING_LEGACY_EAST_ANGLE) && legacy.has(SETTING_LEGACY_SUN_ANGLE))
    {
        // get counter-clockwise radian angle from clockwise legacy WL east angle...
        F32 azimuth  = -legacy[SETTING_LEGACY_EAST_ANGLE].asReal();
        F32 altitude =  legacy[SETTING_LEGACY_SUN_ANGLE].asReal();
        
        LLQuaternion sunquat  = convert_azimuth_and_altitude_to_quat(azimuth, altitude);
        // original WL moon dir was diametrically opposed to the sun dir
        LLQuaternion moonquat = convert_azimuth_and_altitude_to_quat(azimuth + F_PI, -altitude);

        newsettings[SETTING_SUN_ROTATION]  = sunquat.getValue();
        newsettings[SETTING_MOON_ROTATION] = moonquat.getValue();
        converted_something |= true;
    }

    if (!converted_something)
        return LLSD();

    return newsettings;
}

void LLSettingsSky::updateSettings()
{
    LL_RECORD_BLOCK_TIME(FTM_RECALCULATE_SKYVALUES);

    // base class clears dirty flag so as to not trigger recursive update
    LLSettingsBase::updateSettings();

    // NOTE: these functions are designed to do nothing unless a dirty bit has been set
    // so if you add new settings that are referenced by these update functions,
    // you'll need to insure that your setter updates the dirty bits as well
    calculateHeavenlyBodyPositions();
    calculateLightSettings();
}

F32 LLSettingsSky::getSunMoonGlowFactor() const
{
    return getIsSunUp()  ? 1.0f  :
           getIsMoonUp() ? getMoonBrightness() * 0.25 : 0.0f;
}

bool LLSettingsSky::getIsSunUp() const
{
    LLVector3 sunDir = getSunDirection();
    return sunDir.mV[2] >= 0.0f;
}

bool LLSettingsSky::getIsMoonUp() const
{
    LLVector3 moonDir = getMoonDirection();
    return moonDir.mV[2] >= 0.0f;
}

void LLSettingsSky::calculateHeavenlyBodyPositions()  const
{
    LLQuaternion sunq  = getSunRotation();
    LLQuaternion moonq = getMoonRotation();

    mSunDirection  = LLVector3::x_axis * sunq;
    mMoonDirection = LLVector3::x_axis * moonq;

    mSunDirection.normalize();
    mMoonDirection.normalize();

    if (mSunDirection.lengthSquared() < 0.01f)
        LL_WARNS("SETTINGS") << "Zero length sun direction. Wailing and gnashing of teeth may follow... or not." << LL_ENDL;
    if (mMoonDirection.lengthSquared() < 0.01f)
        LL_WARNS("SETTINGS") << "Zero length moon direction. Wailing and gnashing of teeth may follow... or not." << LL_ENDL;
}

LLVector3 LLSettingsSky::getLightDirection() const
{
    update();

    // is the normal from the sun or the moon
    if (getIsSunUp())
    {
        return mSunDirection;
    }
    else if (getIsMoonUp())
    {
        return mMoonDirection;
    }

    return LLVector3::z_axis_neg;
}

LLColor3 LLSettingsSky::getLightDiffuse() const
{
    update();

    // is the normal from the sun or the moon
    if (getIsSunUp())
    {
        return getSunDiffuse();
    }
    else if (getIsMoonUp())
    {
        return getMoonDiffuse();
    }

    return LLColor3::white;
}

LLColor3 LLSettingsSky::getColor(const std::string& key, const LLColor3& default_value) const
{
    if (mSettings.has(SETTING_LEGACY_HAZE) && mSettings[SETTING_LEGACY_HAZE].has(key))
    {
        return LLColor3(mSettings[SETTING_LEGACY_HAZE][key]);
    }
    if (mSettings.has(key))
    {
        return LLColor3(mSettings[key]);
    }
    return default_value;
}

F32 LLSettingsSky::getFloat(const std::string& key, F32 default_value) const
{
    if (mSettings.has(SETTING_LEGACY_HAZE) && mSettings[SETTING_LEGACY_HAZE].has(key))
    {
        return mSettings[SETTING_LEGACY_HAZE][key].asReal();
    }
    if (mSettings.has(key))
    {
        return mSettings[key].asReal();
    }
    return default_value;
}

LLColor3 LLSettingsSky::getAmbientColor() const
{
    return getColor(SETTING_AMBIENT, LLColor3(0.25f, 0.25f, 0.25f));
}

LLColor3 LLSettingsSky::getAmbientColorClamped() const
{
    LLColor3 ambient = getAmbientColor();

    F32 max_color = llmax(ambient.mV[0], ambient.mV[1], ambient.mV[2]);
    if (max_color > 1.0f)
    {
        ambient *= 1.0f/max_color;
    }

    return ambient;
}

LLColor3 LLSettingsSky::getBlueDensity() const
{
    return getColor(SETTING_BLUE_DENSITY, LLColor3(0.2447f, 0.4487f, 0.7599f));
}

LLColor3 LLSettingsSky::getBlueHorizon() const
{
    return getColor(SETTING_BLUE_HORIZON, LLColor3(0.4954f, 0.4954f, 0.6399f));
}

F32 LLSettingsSky::getHazeDensity() const
{
    return getFloat(SETTING_HAZE_DENSITY, 0.7f);
}

F32 LLSettingsSky::getHazeHorizon() const
{
    return getFloat(SETTING_HAZE_HORIZON, 0.19f);
}

F32 LLSettingsSky::getDensityMultiplier() const
{
    return getFloat(SETTING_DENSITY_MULTIPLIER, 0.0001f);
}

F32 LLSettingsSky::getDistanceMultiplier() const
{
    return getFloat(SETTING_DISTANCE_MULTIPLIER, 0.8f);
}

void LLSettingsSky::setPlanetRadius(F32 radius)
{
    mSettings[SETTING_PLANET_RADIUS] = radius;
}

void LLSettingsSky::setSkyBottomRadius(F32 radius)
{
    mSettings[SETTING_SKY_BOTTOM_RADIUS] = radius;
}

void LLSettingsSky::setSkyTopRadius(F32 radius)
{
    mSettings[SETTING_SKY_TOP_RADIUS] = radius;
}

void LLSettingsSky::setSunArcRadians(F32 radians)
{
    mSettings[SETTING_SUN_ARC_RADIANS] = radians;
}

void LLSettingsSky::setMieAnisotropy(F32 aniso_factor)
{
    getMieConfig()[SETTING_MIE_ANISOTROPY_FACTOR] = aniso_factor;
}

void LLSettingsSky::setSkyMoistureLevel(F32 moisture_level)
{
    setValue(SETTING_SKY_MOISTURE_LEVEL, moisture_level);
}

void LLSettingsSky::setSkyDropletRadius(F32 radius)
{
    setValue(SETTING_SKY_DROPLET_RADIUS,radius);
}

void LLSettingsSky::setSkyIceLevel(F32 ice_level)
{
    setValue(SETTING_SKY_ICE_LEVEL, ice_level);
}

void LLSettingsSky::setAmbientColor(const LLColor3 &val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_AMBIENT] = val.getValue();
    setDirtyFlag(true);
}

void LLSettingsSky::setBlueDensity(const LLColor3 &val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_BLUE_DENSITY] = val.getValue();
    setDirtyFlag(true);
}

void LLSettingsSky::setBlueHorizon(const LLColor3 &val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_BLUE_HORIZON] = val.getValue();
    setDirtyFlag(true);
}

void LLSettingsSky::setDensityMultiplier(F32 val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_DENSITY_MULTIPLIER] = val;
    setDirtyFlag(true);
}

void LLSettingsSky::setDistanceMultiplier(F32 val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_DISTANCE_MULTIPLIER] = val;
    setDirtyFlag(true);
}

void LLSettingsSky::setHazeDensity(F32 val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_HAZE_DENSITY] = val;
    setDirtyFlag(true);
}

void LLSettingsSky::setHazeHorizon(F32 val)
{
    mSettings[SETTING_LEGACY_HAZE][SETTING_HAZE_HORIZON] = val;
    setDirtyFlag(true);
}

// Get total from rayleigh and mie density values for normalization
LLColor3 LLSettingsSky::getTotalDensity() const
{
    LLColor3    blue_density = getBlueDensity();
    F32         haze_density = getHazeDensity();
    LLColor3 total_density = blue_density + smear(haze_density);
    return total_density;
}

// Sunlight attenuation effect (hue and brightness) due to atmosphere
// this is used later for sunlight modulation at various altitudes
LLColor3 LLSettingsSky::getLightAttenuation(F32 distance) const
{
    F32         density_multiplier = getDensityMultiplier();
    LLColor3    blue_density       = getBlueDensity();
    F32         haze_density       = getHazeDensity();
    // Approximate line integral over requested distance
    LLColor3    light_atten = (blue_density * 1.0 + smear(haze_density * 0.25f)) * density_multiplier * distance;
    return light_atten;
}

LLColor3 LLSettingsSky::getLightTransmittance(F32 distance) const
{
    LLColor3 total_density      = getTotalDensity();
    F32      density_multiplier = getDensityMultiplier();
    // Transparency (-> density) from Beer's law
    LLColor3 transmittance = componentExp(total_density * -(density_multiplier * distance));
    return transmittance;
}

// performs soft scale clip and gamma correction ala the shader implementation
// scales colors down to 0 - 1 range preserving relative ratios
LLColor3 LLSettingsSky::gammaCorrect(const LLColor3& in) const
{
    F32 gamma = getGamma();

    LLColor3 v(in);
    // scale down to 0 to 1 range preserving relative ratio (aka homegenize)
    F32 max_color = llmax(llmax(in.mV[0], in.mV[1]), in.mV[2]);
    if (max_color > 1.0f)
    {
        v *= 1.0f / max_color;
    }

    LLColor3 color = in * 2.0f;
	color = smear(1.f) - componentSaturate(color); // clamping after mul seems wrong, but prevents negative colors...
	componentPow(color, gamma);
	color = smear(1.f) - color;
    return color;
}

LLVector3 LLSettingsSky::getSunDirection() const
{
    update();
    return mSunDirection;
}

LLVector3 LLSettingsSky::getMoonDirection() const
{
    update();
    return mMoonDirection;
}

LLColor4 LLSettingsSky::getMoonAmbient() const
{
    update();
    return mMoonAmbient;
}

LLColor3 LLSettingsSky::getMoonDiffuse() const
{
    update();
    return mMoonDiffuse;
}

LLColor4 LLSettingsSky::getSunAmbient() const
{
    update();
    return mSunAmbient;
}

LLColor3 LLSettingsSky::getSunDiffuse() const
{
    update();
    return mSunDiffuse;
}

LLColor4 LLSettingsSky::getHazeColor() const
{
    update();
    return mHazeColor;
}

LLColor4 LLSettingsSky::getTotalAmbient() const
{
    update();
    return mTotalAmbient;
}

LLColor3 LLSettingsSky::getMoonlightColor() const
{
    return getSunlightColor(); //moon and sun share light color
}

void LLSettingsSky::clampColor(LLColor3& color, F32 gamma, F32 scale) const
{
    F32 max_color = llmax(color.mV[0], color.mV[1], color.mV[2]);
    if (max_color > scale)
    {
        color *= scale/max_color;
    }
    LLColor3 linear(color);
    linear *= 1.0 / scale;
    linear = smear(1.0f) - linear;
    linear = componentPow(linear, gamma);
    linear *= scale;
    color = linear;
}

void LLSettingsSky::calculateLightSettings() const
{
    // Initialize temp variables
    LLColor3    sunlight = getSunlightColor();
    LLColor3    ambient  = getAmbientColor();

    F32         cloud_shadow = getCloudShadow();
    LLVector3   lightnorm = getLightDirection();

    // Sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    F32         max_y               = getMaxY();
    LLColor3    light_atten         = getLightAttenuation(max_y);
    LLColor3    light_transmittance = getLightTransmittance(max_y);

    // and vary_sunlight will work properly with moon light
    const F32 LIMIT = FLT_EPSILON * 8.0f;

    F32 lighty = fabs(lightnorm[2]);
    if(lighty >= LIMIT)
    {
        lighty = 1.f / lighty;
    }
    lighty = llmax(LIMIT, lighty);
    componentMultBy(sunlight, componentExp((light_atten * -1.f) * lighty));
    componentMultBy(sunlight, light_transmittance);

    //increase ambient when there are more clouds
    LLColor3 tmpAmbient = ambient + (smear(1.f) - ambient) * cloud_shadow * 0.5;

    //brightness of surface both sunlight and ambient
    mSunDiffuse = sunlight;
    mSunAmbient = tmpAmbient;

    F32 haze_horizon = getHazeHorizon();
    
    sunlight *= 1.0 - cloud_shadow;
    sunlight += tmpAmbient;

    mHazeColor = getBlueHorizon() * getBlueDensity() * sunlight;
    mHazeColor += LLColor4(haze_horizon, haze_horizon, haze_horizon, haze_horizon) * getHazeDensity() * sunlight;

    F32 moon_brightness = getIsMoonUp() ? getMoonBrightness() : 0.001f;

    LLColor3 moonlight = getMoonlightColor();
    LLColor3 moonlight_b(0.66, 0.66, 1.2); // scotopic ambient value

    componentMultBy(moonlight, componentExp((light_atten * -1.f) * lighty));

    mMoonDiffuse  = componentMult(moonlight, light_transmittance) * moon_brightness;
    mMoonAmbient  = moonlight_b * 0.0125f;

    mTotalAmbient = ambient;
}

LLUUID LLSettingsSky::GetDefaultAssetId()
{
    return DEFAULT_ASSET_ID;
}

LLUUID LLSettingsSky::GetDefaultSunTextureId()
{
    return LLUUID::null;
}


LLUUID LLSettingsSky::GetBlankSunTextureId()
{
    return DEFAULT_SUN_ID;
}

LLUUID LLSettingsSky::GetDefaultMoonTextureId()
{
    return DEFAULT_MOON_ID;
}

LLUUID LLSettingsSky::GetDefaultCloudNoiseTextureId()
{
    return DEFAULT_CLOUD_ID;
}

LLUUID LLSettingsSky::GetDefaultBloomTextureId()
{
    return IMG_BLOOM1;
}

LLUUID LLSettingsSky::GetDefaultRainbowTextureId()
{
    return IMG_RAINBOW;
}

LLUUID LLSettingsSky::GetDefaultHaloTextureId()
{
    return IMG_HALO;
}

F32 LLSettingsSky::getPlanetRadius() const
{
    return mSettings[SETTING_PLANET_RADIUS].asReal();
}

F32 LLSettingsSky::getSkyMoistureLevel() const
{
    return mSettings[SETTING_SKY_MOISTURE_LEVEL].asReal();
}

F32 LLSettingsSky::getSkyDropletRadius() const
{
    return mSettings[SETTING_SKY_DROPLET_RADIUS].asReal();
}

F32 LLSettingsSky::getSkyIceLevel() const
{
    return mSettings[SETTING_SKY_ICE_LEVEL].asReal();
}

F32 LLSettingsSky::getSkyBottomRadius() const
{
    return mSettings[SETTING_SKY_BOTTOM_RADIUS].asReal();
}

F32 LLSettingsSky::getSkyTopRadius() const
{
    return mSettings[SETTING_SKY_TOP_RADIUS].asReal();
}

F32 LLSettingsSky::getSunArcRadians() const
{
    return mSettings[SETTING_SUN_ARC_RADIANS].asReal();
}

F32 LLSettingsSky::getMieAnisotropy() const
{
    return getMieConfig()[SETTING_MIE_ANISOTROPY_FACTOR].asReal();
}
 
LLSD LLSettingsSky::getRayleighConfig() const
{
    LLSD copy = *(mSettings[SETTING_RAYLEIGH_CONFIG].beginArray());
    return copy;
}

LLSD LLSettingsSky::getMieConfig() const
{
    LLSD copy = *(mSettings[SETTING_MIE_CONFIG].beginArray());
    return copy;
}

LLSD LLSettingsSky::getAbsorptionConfig() const
{
    LLSD copy = *(mSettings[SETTING_ABSORPTION_CONFIG].beginArray());
    return copy;
}
   
LLSD LLSettingsSky::getRayleighConfigs() const
{
    return mSettings[SETTING_RAYLEIGH_CONFIG];
}

LLSD LLSettingsSky::getMieConfigs() const
{
    return mSettings[SETTING_MIE_CONFIG];
}

LLSD LLSettingsSky::getAbsorptionConfigs() const
{
    return mSettings[SETTING_ABSORPTION_CONFIG];
}

void LLSettingsSky::setRayleighConfigs(const LLSD& rayleighConfig)
{
    mSettings[SETTING_RAYLEIGH_CONFIG] = rayleighConfig;
}

void LLSettingsSky::setMieConfigs(const LLSD& mieConfig)
{
    mSettings[SETTING_MIE_CONFIG] = mieConfig;
}

void LLSettingsSky::setAbsorptionConfigs(const LLSD& absorptionConfig)
{
    mSettings[SETTING_ABSORPTION_CONFIG] = absorptionConfig;
}

LLUUID LLSettingsSky::getBloomTextureId() const
{
    return mSettings[SETTING_BLOOM_TEXTUREID].asUUID();
}

LLUUID LLSettingsSky::getRainbowTextureId() const
{
    return mSettings[SETTING_RAINBOW_TEXTUREID].asUUID();
}

LLUUID LLSettingsSky::getHaloTextureId() const
{
    return mSettings[SETTING_HALO_TEXTUREID].asUUID();
}

//---------------------------------------------------------------------
LLColor3 LLSettingsSky::getCloudColor() const
{
    return LLColor3(mSettings[SETTING_CLOUD_COLOR]);
}

void LLSettingsSky::setCloudColor(const LLColor3 &val)
{
    setValue(SETTING_CLOUD_COLOR, val);
}

LLUUID LLSettingsSky::getCloudNoiseTextureId() const
{
    return mSettings[SETTING_CLOUD_TEXTUREID].asUUID();
}

void LLSettingsSky::setCloudNoiseTextureId(const LLUUID &id)
{
    setValue(SETTING_CLOUD_TEXTUREID, id);
}

LLColor3 LLSettingsSky::getCloudPosDensity1() const
{
    return LLColor3(mSettings[SETTING_CLOUD_POS_DENSITY1]);
}

void LLSettingsSky::setCloudPosDensity1(const LLColor3 &val)
{
    setValue(SETTING_CLOUD_POS_DENSITY1, val);
}

LLColor3 LLSettingsSky::getCloudPosDensity2() const
{
    return LLColor3(mSettings[SETTING_CLOUD_POS_DENSITY2]);
}

void LLSettingsSky::setCloudPosDensity2(const LLColor3 &val)
{
    setValue(SETTING_CLOUD_POS_DENSITY2, val);
}

F32 LLSettingsSky::getCloudScale() const
{
    return mSettings[SETTING_CLOUD_SCALE].asReal();
}

void LLSettingsSky::setCloudScale(F32 val)
{
    setValue(SETTING_CLOUD_SCALE, val);
}

LLVector2 LLSettingsSky::getCloudScrollRate() const
{
    return LLVector2(mSettings[SETTING_CLOUD_SCROLL_RATE]);
}

void LLSettingsSky::setCloudScrollRate(const LLVector2 &val)
{
    setValue(SETTING_CLOUD_SCROLL_RATE, val);
}

void LLSettingsSky::setCloudScrollRateX(F32 val)
{
    mSettings[SETTING_CLOUD_SCROLL_RATE][0] = val;
    setDirtyFlag(true);
}

void LLSettingsSky::setCloudScrollRateY(F32 val)
{
    mSettings[SETTING_CLOUD_SCROLL_RATE][1] = val;
    setDirtyFlag(true);
}

F32 LLSettingsSky::getCloudShadow() const
{
    return mSettings[SETTING_CLOUD_SHADOW].asReal();
}

void LLSettingsSky::setCloudShadow(F32 val)
{
    setValue(SETTING_CLOUD_SHADOW, val);
}

F32 LLSettingsSky::getCloudVariance() const
{
    return mSettings[SETTING_CLOUD_VARIANCE].asReal();
}

void LLSettingsSky::setCloudVariance(F32 val)
{
    setValue(SETTING_CLOUD_VARIANCE, val);
}

F32 LLSettingsSky::getDomeOffset() const
{
    //return mSettings[SETTING_DOME_OFFSET].asReal();
    return DOME_OFFSET;    
}

F32 LLSettingsSky::getDomeRadius() const
{
    //return mSettings[SETTING_DOME_RADIUS].asReal();
    return DOME_RADIUS;    
}

F32 LLSettingsSky::getGamma() const
{
    return mSettings[SETTING_GAMMA].asReal();
}

void LLSettingsSky::setGamma(F32 val)
{
    mSettings[SETTING_GAMMA] = LLSD::Real(val);
    setDirtyFlag(true);
}

LLColor3 LLSettingsSky::getGlow() const
{
    return LLColor3(mSettings[SETTING_GLOW]);
}

void LLSettingsSky::setGlow(const LLColor3 &val)
{
    setValue(SETTING_GLOW, val);
}

F32 LLSettingsSky::getMaxY() const
{
    return mSettings[SETTING_MAX_Y].asReal();
}

void LLSettingsSky::setMaxY(F32 val) 
{
    setValue(SETTING_MAX_Y, val);
}

LLQuaternion LLSettingsSky::getMoonRotation() const
{
    return LLQuaternion(mSettings[SETTING_MOON_ROTATION]);
}

void LLSettingsSky::setMoonRotation(const LLQuaternion &val)
{
    setValue(SETTING_MOON_ROTATION, val);
}

F32 LLSettingsSky::getMoonScale() const
{
    return mSettings[SETTING_MOON_SCALE].asReal();
}

void LLSettingsSky::setMoonScale(F32 val)
{
    setValue(SETTING_MOON_SCALE, val);
}

LLUUID LLSettingsSky::getMoonTextureId() const
{
    return mSettings[SETTING_MOON_TEXTUREID].asUUID();
}

void LLSettingsSky::setMoonTextureId(LLUUID id)
{
    setValue(SETTING_MOON_TEXTUREID, id);
}

F32  LLSettingsSky::getMoonBrightness() const
{
    return mSettings[SETTING_MOON_BRIGHTNESS].asReal();
}

void LLSettingsSky::setMoonBrightness(F32 brightness_factor)
{
    setValue(SETTING_MOON_BRIGHTNESS, brightness_factor);
}

F32 LLSettingsSky::getStarBrightness() const
{
    return mSettings[SETTING_STAR_BRIGHTNESS].asReal();
}

void LLSettingsSky::setStarBrightness(F32 val)
{
    setValue(SETTING_STAR_BRIGHTNESS, val);
}

LLColor3 LLSettingsSky::getSunlightColor() const
{
    return LLColor3(mSettings[SETTING_SUNLIGHT_COLOR]);
}

LLColor3 LLSettingsSky::getSunlightColorClamped() const
{
    LLColor3 sunlight = getSunlightColor();
    //clampColor(sunlight, getGamma(), 3.0f);

    F32 max_color = llmax(sunlight.mV[0], sunlight.mV[1], sunlight.mV[2]);
    if (max_color > 1.0f)
    {
        sunlight *= 1.0f/max_color;
    }

    return sunlight;
}

void LLSettingsSky::setSunlightColor(const LLColor3 &val)
{
    setValue(SETTING_SUNLIGHT_COLOR, val);
}

LLQuaternion LLSettingsSky::getSunRotation() const
{
    return LLQuaternion(mSettings[SETTING_SUN_ROTATION]);
}

void LLSettingsSky::setSunRotation(const LLQuaternion &val) 
{
    setValue(SETTING_SUN_ROTATION, val);
}


F32 LLSettingsSky::getSunScale() const
{
    return mSettings[SETTING_SUN_SCALE].asReal();
}

void LLSettingsSky::setSunScale(F32 val)
{
    setValue(SETTING_SUN_SCALE, val);
}

LLUUID LLSettingsSky::getSunTextureId() const
{
    return mSettings[SETTING_SUN_TEXTUREID].asUUID();
}

void LLSettingsSky::setSunTextureId(LLUUID id) 
{
    setValue(SETTING_SUN_TEXTUREID, id);
}

LLUUID LLSettingsSky::getNextSunTextureId() const
{
    return mNextSunTextureId;
}

LLUUID LLSettingsSky::getNextMoonTextureId() const
{
    return mNextMoonTextureId;
}

LLUUID LLSettingsSky::getNextCloudNoiseTextureId() const
{
    return mNextCloudTextureId;
}

LLUUID LLSettingsSky::getNextBloomTextureId() const
{
    return mNextBloomTextureId;
}

