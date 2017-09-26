/**
 * @file llenvmanager.cpp
 * @brief Implementation of classes managing WindLight and water settings.
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

#include "llenvironment.h"

#include "llagent.h"
#include "lldaycyclemanager.h"
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewerregion.h"
#include "llwaterparammanager.h"
#include "llwlhandlers.h"
#include "llwlparammanager.h"
#include "lltrans.h"
#include "lltrace.h"
#include "llfasttimer.h"
#include "llviewercamera.h"
#include "pipeline.h"

//=========================================================================
const F32 LLEnvironment::SUN_DELTA_YAW(F_PI);   // 180deg 

namespace
{
    LLTrace::BlockTimerStatHandle   FTM_ENVIRONMENT_UPDATE("Update Environment Tick");
    LLTrace::BlockTimerStatHandle   FTM_SHADER_PARAM_UPDATE("Update Shader Parameters");
}

//-------------------------------------------------------------------------
LLEnvironment::LLEnvironment():
    mCurrentSky(),
    mSkysById(),
    mSkysByName()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsSky::buildDefaultSky();
    addSky(p_default_sky);
    mCurrentSky = p_default_sky;
}

LLEnvironment::~LLEnvironment()
{
}

//-------------------------------------------------------------------------
void LLEnvironment::update(const LLViewerCamera * cam)
{
    LL_RECORD_BLOCK_TIME(FTM_ENVIRONMENT_UPDATE);

    // update clouds, sun, and general
    updateCloudScroll();
    mCurrentSky->update();

//     // update only if running
//     if (mAnimator.getIsRunning())
//     {
//         mAnimator.update(mCurParams);
//     }

    LLVector3 lightdir = mCurrentSky->getLightDirection();
    // update the shaders and the menu

    F32 camYaw = cam->getYaw();

    stop_glerror();

    // *TODO: potential optimization - this block may only need to be
    // executed some of the time.  For example for water shaders only.
    {
        LLVector3 lightNorm3(mCurrentSky->getLightDirection());

        lightNorm3 *= LLQuaternion(-(camYaw + SUN_DELTA_YAW), LLVector3(0.f, 1.f, 0.f));
        mRotatedLight = LLVector4(lightNorm3, 0.f);

        LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
        end_shaders = LLViewerShaderMgr::instance()->endShaders();
        for (shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter)
        {
            if ((shaders_iter->mProgramObject != 0)
                && (gPipeline.canUseWindLightShaders()
                || shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER))
            {
                shaders_iter->mUniformsDirty = TRUE;
            }
        }
    }
}

void LLEnvironment::updateCloudScroll()
{
    // This is a function of the environment rather than the sky, since it should 
    // persist through sky transitions.
    static LLTimer s_cloud_timer;

    F64 delta_t = s_cloud_timer.getElapsedTimeAndResetF64();
    
    LLVector2 cloud_delta = static_cast<F32>(delta_t)* (mCurrentSky->getCloudScrollRate() - LLVector2(10.0, 10.0)) / 100.0;
    mCloudScrollDelta += cloud_delta;

//     {
//         LLVector2 v2(mCurrentSky->getCloudScrollRate());
//         static F32 xoffset(0.f);
//         static F32 yoffset(0.f);
// 
//         xoffset += F32(delta_t * (v2[0] - 10.f) / 100.f);
//         yoffset += F32(delta_t * (v2[1] - 10.f) / 100.f);
// 
//         LL_WARNS("RIDER") << "offset " << mCloudScrollDelta << " vs (" << xoffset << ", " << yoffset << ")" << LL_ENDL;
//     }


}

void LLEnvironment::updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting)
{
    LL_RECORD_BLOCK_TIME(FTM_SHADER_PARAM_UPDATE);

    //_WARNS("RIDER") << "----------------------------------------------------------------" << LL_ENDL;
    LLSettingsBase::parammapping_t params = psetting->getParameterMap();
    for (LLSettingsBase::parammapping_t::iterator it = params.begin(); it != params.end(); ++it)
    {
        if (!psetting->mSettings.has((*it).first))
            continue;

        LLSD value = psetting->mSettings[(*it).first];
        LLSD::Type setting_type = value.type();

        stop_glerror();
        switch (setting_type)
        {
        case LLSD::TypeInteger:
            shader->uniform1i((*it).second, value.asInteger());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;
        case LLSD::TypeReal:
            shader->uniform1f((*it).second, value.asReal());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeBoolean:
            shader->uniform1i((*it).second, value.asBoolean() ? 1 : 0);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeArray:
        {
            LLVector4 vect4(value);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << vect4 << LL_ENDL;
            shader->uniform4fv((*it).second, 1, vect4.mV);

            break;
        }

        //  case LLSD::TypeMap:
        //  case LLSD::TypeString:
        //  case LLSD::TypeUUID:
        //  case LLSD::TypeURI:
        //  case LLSD::TypeBinary:
        //  case LLSD::TypeDate:
        default:
            break;
        }
        stop_glerror();
    }
    //_WARNS("RIDER") << "----------------------------------------------------------------" << LL_ENDL;

    psetting->applySpecial(shader);

    if (LLPipeline::sRenderDeferred && !LLPipeline::sReflectionRender && !LLPipeline::sUnderWaterRender)
    {
        shader->uniform1f(LLShaderMgr::GLOBAL_GAMMA, 2.2);
    }
    else 
    {
        shader->uniform1f(LLShaderMgr::GLOBAL_GAMMA, 1.0);
    }

}

void LLEnvironment::updateShaderUniforms(LLGLSLShader *shader)
{

    if (gPipeline.canUseWindLightShaders())
    {
        updateGLVariablesForSettings(shader, mCurrentSky);
    }

    if (shader->mShaderGroup == LLGLSLShader::SG_DEFAULT)
    {
        stop_glerror();
        shader->uniform4fv(LLShaderMgr::LIGHTNORM, 1, mRotatedLight.mV);
        shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, LLViewerCamera::getInstance()->getOrigin().mV);
        stop_glerror();
    }
    else if (shader->mShaderGroup == LLGLSLShader::SG_SKY)
    {
        stop_glerror();
        shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, mCurrentSky->getLightDirectionClamped().mV);
        stop_glerror();
    }

    shader->uniform1f(LLShaderMgr::SCENE_LIGHT_STRENGTH, mCurrentSky->getSceneLightStrength());

//     {
//         LLVector4 cloud_scroll(mCloudScroll[0], mCloudScroll[1], 0.0, 0.0);
// //         val.mV[0] = F32(i->second[0].asReal()) + mCloudScrollXOffset;
// //         val.mV[1] = F32(i->second[1].asReal()) + mCloudScrollYOffset;
// //         val.mV[2] = (F32)i->second[2].asReal();
// //         val.mV[3] = (F32)i->second[3].asReal();
// 
//         stop_glerror();
//         shader->uniform4fv(LLSettingsSky::SETTING_CLOUD_POS_DENSITY1, 1, cloud_scroll.mV);
//         stop_glerror();
//     }


}
//--------------------------------------------------------------------------

void LLEnvironment::addSky(const LLSettingsSky::ptr_t &sky)
{
    std::string name = sky->getValue(LLSettingsSky::SETTING_NAME).asString();

    std::pair<NamedSkyMap_t::iterator, bool> result;
    result = mSkysByName.insert(NamedSkyMap_t::value_type(name, sky));

    if (!result.second)
        (*(result.first)).second = sky;
}

void LLEnvironment::addSky(const LLUUID &id, const LLSettingsSky::ptr_t &sky)
{
    //     std::string name = sky->getValue(LLSettingsSky::SETTING_NAME).asString();
    // 
    //     std::pair<NamedSkyMap_t::iterator, bool> result;
    //     result = mSkysByName.insert(NamedSkyMap_t::value_type(name, sky));
    // 
    //     if (!result.second)
    //         (*(result.first)).second = sky;
}

void LLEnvironment::removeSky(const std::string &name)
{
    NamedSkyMap_t::iterator it = mSkysByName.find(name);
    if (it != mSkysByName.end())
        mSkysByName.erase(it);
}

void LLEnvironment::removeSky(const LLUUID &id)
{

}

void LLEnvironment::clearAllSkys()
{
    mSkysByName.clear();
    mSkysById.clear();
}

