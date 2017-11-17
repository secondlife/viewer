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

#include "llviewerprecompiledheaders.h"
#include "llviewercontrol.h"
#include "llsettingswater.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

#include "llglslshader.h"
#include "llviewershadermgr.h"

#include "llenvironment.h"

#include "llagent.h"
#include "pipeline.h"

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

const F32 LLSettingsWater::WATER_FOG_LIGHT_CLAMP(0.3f);

const LLUUID LLSettingsWater::DEFAULT_WATER_NORMAL_ID(DEFAULT_WATER_NORMAL);


//=========================================================================
LLSettingsWater::LLSettingsWater(const LLSD &data) :
    LLSettingsBase(data)
{
}

LLSettingsWater::LLSettingsWater() :
    LLSettingsBase()
{
}

//=========================================================================
LLSD LLSettingsWater::defaults()
{
    LLSD dfltsetting;

    // Magic constants copied form defaults.xml 
    dfltsetting[SETTING_BLUR_MULTIPILER] = LLSD::Real(0.04000f);
    dfltsetting[SETTING_FOG_COLOR] = LLColor3(0.0156f, 0.1490f, 0.2509f).getValue();
    dfltsetting[SETTING_FOG_DENSITY] = LLSD::Real(2.0f);
    dfltsetting[SETTING_FOG_MOD] = LLSD::Real(0.25f);
    dfltsetting[SETTING_FRESNEL_OFFSET] = LLSD::Real(0.5f);
    dfltsetting[SETTING_FRESNEL_SCALE] = LLSD::Real(0.3999);
    dfltsetting[SETTING_NORMAL_MAP] = LLSD::UUID(DEFAULT_WATER_NORMAL_ID);
    dfltsetting[SETTING_NORMAL_SCALE] = LLVector3(2.0f, 2.0f, 2.0f).getValue();
    dfltsetting[SETTING_SCALE_ABOVE] = LLSD::Real(0.0299f);
    dfltsetting[SETTING_SCALE_BELOW] = LLSD::Real(0.2000f);
    dfltsetting[SETTING_WAVE1_DIR] = LLVector2(1.04999f, -0.42000f).getValue();
    dfltsetting[SETTING_WAVE2_DIR] = LLVector2(1.10999f, -1.16000f).getValue();

    return dfltsetting;
}

LLSettingsWater::ptr_t LLSettingsWater::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(defaults());

    newsettings[SETTING_NAME] = name;


    if (oldsettings.has(SETTING_LEGACY_BLUR_MULTIPILER))
    {
        newsettings[SETTING_BLUR_MULTIPILER] = LLSD::Real(oldsettings[SETTING_LEGACY_BLUR_MULTIPILER].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_FOG_COLOR))
    {
        newsettings[SETTING_FOG_COLOR] = LLColor3(oldsettings[SETTING_LEGACY_FOG_COLOR]).getValue();
    }
    if (oldsettings.has(SETTING_LEGACY_FOG_DENSITY))
    {
        newsettings[SETTING_FOG_DENSITY] = LLSD::Real(oldsettings[SETTING_LEGACY_FOG_DENSITY]);
    }
    if (oldsettings.has(SETTING_LEGACY_FOG_MOD))
    {
        newsettings[SETTING_FOG_MOD] = LLSD::Real(oldsettings[SETTING_LEGACY_FOG_MOD].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_FRESNEL_OFFSET))
    {
        newsettings[SETTING_FRESNEL_OFFSET] = LLSD::Real(oldsettings[SETTING_LEGACY_FRESNEL_OFFSET].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_FRESNEL_SCALE))
    {
        newsettings[SETTING_FRESNEL_SCALE] = LLSD::Real(oldsettings[SETTING_LEGACY_FRESNEL_SCALE].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_NORMAL_MAP))
    {
        newsettings[SETTING_NORMAL_MAP] = LLSD::UUID(oldsettings[SETTING_LEGACY_NORMAL_MAP].asUUID());
    }
    if (oldsettings.has(SETTING_LEGACY_NORMAL_SCALE))
    {
        newsettings[SETTING_NORMAL_SCALE] = LLVector3(oldsettings[SETTING_LEGACY_NORMAL_SCALE]).getValue();
    }
    if (oldsettings.has(SETTING_LEGACY_SCALE_ABOVE))
    {
        newsettings[SETTING_SCALE_ABOVE] = LLSD::Real(oldsettings[SETTING_LEGACY_SCALE_ABOVE].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_SCALE_BELOW))
    {
        newsettings[SETTING_SCALE_BELOW] = LLSD::Real(oldsettings[SETTING_LEGACY_SCALE_BELOW].asReal());
    }
    if (oldsettings.has(SETTING_LEGACY_WAVE1_DIR))
    {
        newsettings[SETTING_WAVE1_DIR] = LLVector2(oldsettings[SETTING_LEGACY_WAVE1_DIR]).getValue();
    }
    if (oldsettings.has(SETTING_LEGACY_WAVE2_DIR))
    {
        newsettings[SETTING_WAVE2_DIR] = LLVector2(oldsettings[SETTING_LEGACY_WAVE2_DIR]).getValue();
    }

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsWater>(newsettings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

LLSettingsWater::ptr_t LLSettingsWater::buildDefaultWater()
{
    LLSD settings = LLSettingsWater::defaults();

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

LLSettingsWater::ptr_t LLSettingsWater::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

void LLSettingsWater::blend(const LLSettingsBase::ptr_t &end, F32 blendf) 
{
    LLSettingsWater::ptr_t other = boost::static_pointer_cast<LLSettingsWater>(end);
    LLSD blenddata = interpolateSDMap(mSettings, other->mSettings, blendf);
    
    replaceSettings(blenddata);
}

LLSettingsWater::validation_list_t LLSettingsWater::getValidationList() const
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

//=========================================================================

LLSettingsWater::parammapping_t LLSettingsWater::getParameterMap() const
{
    static parammapping_t param_map;

    if (param_map.empty())
    {
        param_map[SETTING_FOG_COLOR] = LLShaderMgr::WATER_FOGCOLOR;
        param_map[SETTING_FOG_DENSITY] = LLShaderMgr::WATER_FOGDENSITY;


    }
    return param_map;
}

void LLSettingsWater::applySpecial(void *ptarget)
{
    LLGLSLShader *shader = (LLGLSLShader *)ptarget;

    shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, getWaterPlane().mV);
    shader->uniform1f(LLShaderMgr::WATER_FOGKS, getWaterFogKS());

    shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, LLEnvironment::instance().getRotatedLight().mV);
    shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, LLViewerCamera::getInstance()->getOrigin().mV);
    shader->uniform1f(LLViewerShaderMgr::DISTANCE_MULTIPLIER, 0);


}

//=========================================================================
void LLSettingsWater::updateSettings()
{
//    LL_RECORD_BLOCK_TIME(FTM_UPDATE_WATERVALUES);
//    LL_INFOS("WINDLIGHT", "WATER", "EEP") << "Water Parameters are dirty.  Reticulating Splines..." << LL_ENDL;

    // base class clears dirty flag so as to not trigger recursive update
    LLSettingsBase::updateSettings();

    // only do this if we're dealing with shaders
    if (gPipeline.canUseVertexShaders())
    {
        //transform water plane to eye space
        glh::vec3f norm(0.f, 0.f, 1.f);
        glh::vec3f p(0.f, 0.f, LLEnvironment::instance().getWaterHeight() + 0.1f);

        F32 modelView[16];
        for (U32 i = 0; i < 16; i++)
        {
            modelView[i] = (F32)gGLModelView[i];
        }

        glh::matrix4f mat(modelView);
        glh::matrix4f invtrans = mat.inverse().transpose();
        glh::vec3f enorm;
        glh::vec3f ep;
        invtrans.mult_matrix_vec(norm, enorm);
        enorm.normalize();
        mat.mult_matrix_vec(p, ep);

        mWaterPlane = LLVector4(enorm.v[0], enorm.v[1], enorm.v[2], -ep.dot(enorm));

        LLVector4 light_direction = LLEnvironment::instance().getLightDirection();

        mWaterFogKS = 1.f / llmax(light_direction.mV[2], WATER_FOG_LIGHT_CLAMP);
    }

}
