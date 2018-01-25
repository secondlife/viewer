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
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewerregion.h"
#include "llwlhandlers.h"
#include "lltrans.h"
#include "lltrace.h"
#include "llfasttimer.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "llsky.h"

#include "llviewershadermgr.h"

#include "llparcel.h"
#include "llviewerparcelmgr.h"

#include "llsdserialize.h"
#include "lldiriterator.h"

#include "llsettingsvo.h"
#include "llnotificationsutil.h"

#include <boost/make_shared.hpp>

//define EXPORT_PRESETS 1
//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle   FTM_ENVIRONMENT_UPDATE("Update Environment Tick");
    LLTrace::BlockTimerStatHandle   FTM_SHADER_PARAM_UPDATE("Update Shader Parameters");
}

//=========================================================================
const F32Seconds LLEnvironment::TRANSITION_INSTANT(0.0f);
const F32Seconds LLEnvironment::TRANSITION_FAST(1.0f);
const F32Seconds LLEnvironment::TRANSITION_DEFAULT(5.0f);
const F32Seconds LLEnvironment::TRANSITION_SLOW(10.0f);

const F32 LLEnvironment::SUN_DELTA_YAW(F_PI);   // 180deg 
const F32 LLEnvironment::NIGHTTIME_ELEVATION_COS(LLSky::NIGHTTIME_ELEVATION_COS);

//-------------------------------------------------------------------------
LLEnvironment::LLEnvironment():
    mSelectedSky(),
    mSelectedWater(),
    mSelectedDay(),
    mSkysById(),
    mSkysByName(),
    mWaterByName(),
    mWaterById(),
    mDayCycleByName(),
    mDayCycleById(),
    mUserPrefs(),
    mSelectedEnvironment(ENV_LOCAL),
    mDayLength(LLSettingsDay::DEFAULT_DAYLENGTH),
    mDayOffset(LLSettingsDay::DEFAULT_DAYOFFSET)
{
    mSetSkys.resize(ENV_END);
    mSetWater.resize(ENV_END);
    mSetDays.resize(ENV_END);
}

void LLEnvironment::initSingleton()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsVOSky::buildDefaultSky();
    addSky(p_default_sky);
    mCurrentSky = p_default_sky;

    LLSettingsWater::ptr_t p_default_water = LLSettingsVOWater::buildDefaultWater();
    addWater(p_default_water);
    mCurrentWater = p_default_water;

    LLSettingsDay::ptr_t p_default_day = LLSettingsVODay::buildDefaultDayCycle();
    addDayCycle(p_default_day);
    mCurrentDay.reset();

    // LEGACY!
    legacyLoadAllPresets();

    requestRegionEnvironment();
    gAgent.addRegionChangedCallback(boost::bind(&LLEnvironment::onRegionChange, this));
    gAgent.addParcelChangedCallback(boost::bind(&LLEnvironment::onParcelChange, this));
}

LLEnvironment::~LLEnvironment()
{
}

void LLEnvironment::loadPreferences()
{
    mUserPrefs.load();
}

void LLEnvironment::updatePreferences()
{
    /*NOOP for now.  TODO record prefs and store.*/
}

bool LLEnvironment::canEdit() const
{
    return true;
}


LLEnvironment::connection_t LLEnvironment::setSkyListChange(const LLEnvironment::change_signal_t::slot_type& cb)
{
    return mSkyListChange.connect(cb);
}

LLEnvironment::connection_t LLEnvironment::setWaterListChange(const LLEnvironment::change_signal_t::slot_type& cb)
{
    return mWaterListChange.connect(cb);
}

LLEnvironment::connection_t LLEnvironment::setDayCycleListChange(const LLEnvironment::change_signal_t::slot_type& cb)
{
    return mDayCycleListChange.connect(cb);
}

void LLEnvironment::onRegionChange()
{
    requestRegionEnvironment();
}

void LLEnvironment::onParcelChange()
{
    S32 parcel_id(INVALID_PARCEL_ID);
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();

    if (parcel)
    {
        parcel_id = parcel->getLocalID();
    }

    requestParcel(parcel_id);
}

void LLEnvironment::requestRegionEnvironment()
{
    requestRegion();
}

void LLEnvironment::onLegacyRegionSettings(LLSD data)
{
    LLUUID regionId = data[0]["regionID"].asUUID();

    LLSettingsDay::ptr_t regionday;
    if (data[1].isUndefined())
        regionday = LLEnvironment::findDayCycleByName("Default");
    else
        regionday = LLSettingsVODay::buildFromLegacyMessage(regionId, data[1], data[2], data[3]);

    setSkyFor(ENV_REGION, LLSettingsSky::ptr_t());
    setWaterFor(ENV_REGION, LLSettingsWater::ptr_t());
    setDayFor(ENV_REGION, regionday);

    applyChosenEnvironment();
}

//-------------------------------------------------------------------------
F32 LLEnvironment::getCamHeight() const
{
    return (mCurrentSky->getDomeOffset() * mCurrentSky->getDomeRadius());
}

F32 LLEnvironment::getWaterHeight() const
{
    return gAgent.getRegion()->getWaterHeight();
}

bool LLEnvironment::getIsDayTime() const
{
    return mCurrentSky->getSunDirection().mV[2] > NIGHTTIME_ELEVATION_COS;
}

void LLEnvironment::setDayLength(S64Seconds seconds)
{
    mDayLength = seconds;
    if (mCurrentDay)
        mCurrentDay->setDayLength(mDayLength);
}

void LLEnvironment::setDayOffset(S64Seconds seconds)
{
    mDayOffset = seconds;
    if (mCurrentDay)
        mCurrentDay->setDayOffset(seconds);
}

//-------------------------------------------------------------------------
void LLEnvironment::update(const LLViewerCamera * cam)
{
    LL_RECORD_BLOCK_TIME(FTM_ENVIRONMENT_UPDATE);
    //F32Seconds now(LLDate::now().secondsSinceEpoch());
    static LLFrameTimer timer;

    F32Seconds delta(timer.getElapsedTimeAndResetF32());

    if (mBlenderSky)
        mBlenderSky->update(delta);
    if (mBlenderWater)
        mBlenderWater->update(delta);

    // update clouds, sun, and general
    updateCloudScroll();

    if (mCurrentDay)
        mCurrentDay->update();

    if (mCurrentSky)
        mCurrentSky->update();
    if (mCurrentWater)
        mCurrentWater->update();

    F32 camYaw = cam->getYaw();

    stop_glerror();

    // *TODO: potential optimization - this block may only need to be
    // executed some of the time.  For example for water shaders only.
    {
        LLVector3 lightNorm3( getLightDirection() );

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
        updateGLVariablesForSettings(shader, mCurrentWater);
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
        shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, getClampedLightDirection().mV);
        stop_glerror();
    }

    shader->uniform1f(LLShaderMgr::SCENE_LIGHT_STRENGTH, getSceneLightStrength());


}
//--------------------------------------------------------------------------
void LLEnvironment::selectSky(const std::string &name, F32Seconds transition)
{
    LLSettingsSky::ptr_t next_sky = findSkyByName(name);
    if (!next_sky)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to select sky with unknown name '" << name << "'" << LL_ENDL;
        return;
    }

    selectSky(next_sky, transition);
}

void LLEnvironment::selectSky(const LLSettingsSky::ptr_t &sky, F32Seconds transition)
{
    if (!sky)
    {
        mCurrentSky = mSelectedSky;
        mBlenderSky.reset();
        return;
    }
    mSelectedSky = sky;
    if (fabs(transition.value()) <= F_ALMOST_ZERO)
    {
        mBlenderSky.reset();
        mCurrentSky = sky;
        mCurrentSky->setDirtyFlag(true);
        mSelectedSky = sky;
    }
    else
    {
        LLSettingsSky::ptr_t skytarget = mCurrentSky->buildClone();

        mBlenderSky = boost::make_shared<LLSettingsBlender>( skytarget, mCurrentSky, sky, transition );
        mBlenderSky->setOnFinished(boost::bind(&LLEnvironment::onSkyTransitionDone, this, _1));
        mCurrentSky = skytarget;
        mSelectedSky = sky;
    }
}

void LLEnvironment::onSkyTransitionDone(const LLSettingsBlender::ptr_t &blender)
{
    mCurrentSky = mSelectedSky;
    mBlenderSky.reset();
}

void LLEnvironment::selectWater(const std::string &name, F32Seconds transition)
{
    LLSettingsWater::ptr_t next_water = findWaterByName(name);

    if (!next_water)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to select water with unknown name '" << name << "'" << LL_ENDL;
        return;
    }

    selectWater(next_water, transition);
}

void LLEnvironment::selectWater(const LLSettingsWater::ptr_t &water, F32Seconds transition)
{
    if (!water)
    {
        mCurrentWater = mSelectedWater;
        mBlenderWater.reset();
        return;
    }
    mSelectedWater = water;
    if (fabs(transition.value()) <= F_ALMOST_ZERO)
    {
        mBlenderWater.reset();
        mCurrentWater = water;
        mCurrentWater->setDirtyFlag(true);
        mSelectedWater = water;
    }
    else
    {
        LLSettingsWater::ptr_t watertarget = mCurrentWater->buildClone();

        mBlenderWater = boost::make_shared<LLSettingsBlender>(watertarget, mCurrentWater, water, transition);
        mBlenderWater->setOnFinished(boost::bind(&LLEnvironment::onWaterTransitionDone, this, _1));
        mCurrentWater = watertarget;
        mSelectedWater = water;
    }
}

void LLEnvironment::onWaterTransitionDone(const LLSettingsBlender::ptr_t &blender)
{
    mCurrentWater = mSelectedWater;
    mBlenderWater.reset();
}

void LLEnvironment::selectDayCycle(const std::string &name, F32Seconds transition)
{
    LLSettingsDay::ptr_t next_daycycle = findDayCycleByName(name);

    if (!next_daycycle)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to select daycycle with unknown name '" << name << "'" << LL_ENDL;
        return;
    }

    selectDayCycle(next_daycycle, transition);
}

void LLEnvironment::selectDayCycle(const LLSettingsDay::ptr_t &daycycle, F32Seconds transition)
{
    if (!daycycle || (daycycle == mCurrentDay))
    {
        return;
    }

    mCurrentDay = daycycle;
    mCurrentDay->setDayLength(mDayLength);
    mCurrentDay->setDayOffset(mDayOffset);

    daycycle->startDayCycle();
    selectWater(daycycle->getCurrentWater(), transition);
    selectSky(daycycle->getCurrentSky(), transition);
}


void LLEnvironment::setSelectedEnvironment(EnvSelection_t env)
{
    if (env == mSelectedEnvironment)
    {   // No action to take
        return;
    }

    mSelectedEnvironment = env;
    applyChosenEnvironment();
}

void LLEnvironment::applyChosenEnvironment()
{
    mSelectedSky.reset();
    mSelectedWater.reset();
    mSelectedDay.reset();

    for (S32 idx = mSelectedEnvironment; idx < ENV_END; ++idx)
    {
        if (mSetDays[idx] && !mSelectedSky && !mSelectedWater)
            selectDayCycle(mSetDays[idx]);
        if (mSetSkys[idx] && !mSelectedSky)
            selectSky(mSetSkys[idx]);
        if (mSetWater[idx] && !mSelectedWater)
            selectWater(mSetWater[idx]);
        if (mSelectedSky && mSelectedWater)
            return;
    }

    if (!mSelectedSky)
        selectSky("Default");
    if (!mSelectedWater)
        selectWater("Default");
}

LLSettingsSky::ptr_t LLEnvironment::getChosenSky() const
{
    for (S32 idx = mSelectedEnvironment; idx < ENV_END; ++idx)
    {
        if (mSetSkys[idx])
            return mSetSkys[idx];
    }

    return LLSettingsSky::ptr_t();
}

LLSettingsWater::ptr_t LLEnvironment::getChosenWater() const
{
    for (S32 idx = mSelectedEnvironment; idx < ENV_END; ++idx)
    {
        if (mSetWater[idx])
            return mSetWater[idx];
    }

    return LLSettingsWater::ptr_t();
}

LLSettingsDay::ptr_t LLEnvironment::getChosenDay() const
{
    for (S32 idx = mSelectedEnvironment; idx < ENV_END; ++idx)
    {
        if (mSetDays[idx])
           return mSetDays[idx];
    }

    return LLSettingsDay::ptr_t();
}

void LLEnvironment::setSkyFor(EnvSelection_t env, const LLSettingsSky::ptr_t &sky)
{
    mSetSkys[env] = sky;
}

LLSettingsSky::ptr_t LLEnvironment::getSkyFor(EnvSelection_t env) const
{
    return mSetSkys[env];
}

void LLEnvironment::setWaterFor(EnvSelection_t env, const LLSettingsWater::ptr_t &water)
{
    mSetWater[env] = water;
}

LLSettingsWater::ptr_t LLEnvironment::getWaterFor(EnvSelection_t env) const
{
    return mSetWater[env];
}

void LLEnvironment::setDayFor(EnvSelection_t env, const LLSettingsDay::ptr_t &day)
{
    mSetDays[env] = day;
}

LLSettingsDay::ptr_t LLEnvironment::getDayFor(EnvSelection_t env) const
{
    return mSetDays[env];
}

void  LLEnvironment::clearUserSettings()
{
    mSetSkys[ENV_LOCAL].reset();
    mSetWater[ENV_LOCAL].reset();
    mSetDays[ENV_LOCAL].reset();
}


LLEnvironment::list_name_id_t LLEnvironment::getSkyList() const
{
    list_name_id_t list;

    list.reserve(mSkysByName.size());

    for (namedSettingMap_t::const_iterator it = mSkysByName.begin(); it != mSkysByName.end(); ++it)
    {
        list.push_back(std::vector<name_id_t>::value_type((*it).second->getName(), (*it).second->getId()));
    }

    return list;
}

LLEnvironment::list_name_id_t LLEnvironment::getWaterList() const
{
    list_name_id_t list;

    list.reserve(mWaterByName.size());

    for (namedSettingMap_t::const_iterator it = mWaterByName.begin(); it != mWaterByName.end(); ++it)
    {
        list.push_back(std::vector<name_id_t>::value_type((*it).second->getName(), (*it).second->getId()));
    }

    return list;
}

LLEnvironment::list_name_id_t LLEnvironment::getDayCycleList() const
{
    list_name_id_t list;

    list.reserve(mDayCycleByName.size());

    for (namedSettingMap_t::const_iterator it = mDayCycleByName.begin(); it != mDayCycleByName.end(); ++it)
    {
        list.push_back(std::vector<name_id_t>::value_type((*it).second->getName(), (*it).second->getId()));
    }

    return list;
}

void LLEnvironment::addSky(const LLSettingsSky::ptr_t &sky)
{
    std::string name = sky->getValue(LLSettingsSky::SETTING_NAME).asString();

    LL_WARNS("RIDER") << "Adding sky as '" << name << "'" << LL_ENDL;

    std::pair<namedSettingMap_t::iterator, bool> result;
    result = mSkysByName.insert(namedSettingMap_t::value_type(name, sky));

    if (!result.second)
        (*(result.first)).second = sky;
    mSkyListChange();
}

void LLEnvironment::removeSky(const std::string &name)
{
    namedSettingMap_t::iterator it = mSkysByName.find(name);
    if (it != mSkysByName.end())
        mSkysByName.erase(it);
    mSkyListChange();
}

void LLEnvironment::clearAllSkys()
{
    mSkysByName.clear();
    mSkysById.clear();
    mSkyListChange();
}

void LLEnvironment::addWater(const LLSettingsWater::ptr_t &water)
{
    std::string name = water->getValue(LLSettingsWater::SETTING_NAME).asString();

    LL_WARNS("RIDER") << "Adding water as '" << name << "'" << LL_ENDL;

    std::pair<namedSettingMap_t::iterator, bool> result;
    result = mWaterByName.insert(namedSettingMap_t::value_type(name, water));

    if (!result.second)
        (*(result.first)).second = water;
    mWaterListChange();
}


void LLEnvironment::removeWater(const std::string &name)
{
    namedSettingMap_t::iterator it = mWaterByName.find(name);
    if (it != mWaterByName.end())
        mWaterByName.erase(it);
    mWaterListChange();
}

void LLEnvironment::clearAllWater()
{
    mWaterByName.clear();
    mWaterById.clear();
    mWaterListChange();
}

void LLEnvironment::addDayCycle(const LLSettingsDay::ptr_t &daycycle)
{
    std::string name = daycycle->getValue(LLSettingsDay::SETTING_NAME).asString();

    LL_WARNS("RIDER") << "Adding daycycle as '" << name << "'" << LL_ENDL;

    std::pair<namedSettingMap_t::iterator, bool> result;
    result = mDayCycleByName.insert(namedSettingMap_t::value_type(name, daycycle));

    if (!result.second)
        (*(result.first)).second = daycycle;
    mDayCycleListChange();
}

//void LLEnvironment::addDayCycle(const LLUUID &id, const LLSettingsSky::ptr_t &sky);

void LLEnvironment::removeDayCycle(const std::string &name)
{
    namedSettingMap_t::iterator it = mDayCycleByName.find(name);
    if (it != mDayCycleByName.end())
        mDayCycleByName.erase(it);
    mDayCycleListChange();
}

//void LLEnvironment::removeDayCycle(const LLUUID &id);
void LLEnvironment::clearAllDayCycles()
{
    mDayCycleByName.clear();
    mWaterById.clear();
    mDayCycleListChange();
}

LLSettingsSky::ptr_t LLEnvironment::findSkyByName(std::string name) const
{
    namedSettingMap_t::const_iterator it = mSkysByName.find(name);

    if (it == mSkysByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find sky with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsSky::ptr_t();
    }

    return boost::static_pointer_cast<LLSettingsSky>((*it).second);
}

LLSettingsWater::ptr_t LLEnvironment::findWaterByName(std::string name) const
{
    namedSettingMap_t::const_iterator it = mWaterByName.find(name);

    if (it == mWaterByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find water with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsWater::ptr_t();
    }

    return boost::static_pointer_cast<LLSettingsWater>((*it).second);
}

LLSettingsDay::ptr_t LLEnvironment::findDayCycleByName(std::string name) const
{
    namedSettingMap_t::const_iterator it = mDayCycleByName.find(name);

    if (it == mDayCycleByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find daycycle with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    return boost::static_pointer_cast<LLSettingsDay>((*it).second);
}


void LLEnvironment::selectAgentEnvironment()
{
    S64Seconds day_length(LLSettingsDay::DEFAULT_DAYLENGTH);
    S64Seconds day_offset(LLSettingsDay::DEFAULT_DAYOFFSET);
    LLSettingsDay::ptr_t pday;

    // TODO: First test if agent has local environment set.

    LLParcel *parcel = LLViewerParcelMgr::instance().getAgentParcel();
    LLViewerRegion *pRegion = gAgent.getRegion();

    if (!parcel || parcel->getUsesDefaultDayCycle() || !parcel->getParcelDayCycle())
    {
        day_length = pRegion->getDayLength();
        day_offset = pRegion->getDayOffset();
        pday = pRegion->getRegionDayCycle();
    }
    else
    {
        day_length = parcel->getDayLength();
        day_offset = parcel->getDayOffset();
        pday = parcel->getParcelDayCycle();
    }

    if (getDayLength() != day_length)
        setDayLength(day_length);

    if (getDayOffset() != day_offset)
        setDayOffset(day_offset);

    if (pday)
        selectDayCycle(pday);
    
}

void LLEnvironment::recordEnvironment(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envinfo)
{
    LL_WARNS("ENVIRONMENT") << "Have environment" << LL_ENDL;

    if (parcel_id == INVALID_PARCEL_ID)
    {
        LLViewerRegion *pRegion = gAgent.getRegion();

        if (pRegion)
        {
            pRegion->setDayLength(envinfo->mDayLength);
            pRegion->setDayOffset(envinfo->mDayOffset);
            pRegion->setIsDefaultDayCycle(envinfo->mIsDefault);
            pRegion->setRegionDayCycle(LLSettingsVODay::buildFromEnvironmentMessage(envinfo->mDaycycleData));

            /*TODO: track_altitudes*/
        }
    }
    else
    {
        LLParcel *parcel = LLViewerParcelMgr::instance().getAgentParcel();

        if (parcel->getLocalID() == parcel_id)
        {
            parcel->setDayLength(envinfo->mDayLength);
            parcel->setDayOffset(envinfo->mDayOffset);
            parcel->setUsesDefaultDayCycle(envinfo->mIsDefault);

            LLSettingsDay::ptr_t pday;
            if (!envinfo->mIsDefault)
            {
                pday = LLSettingsVODay::buildFromEnvironmentMessage(envinfo->mDaycycleData);
            }
            parcel->setParcelDayCycle(pday);

            // select parcel day
        }
    }

    selectAgentEnvironment();
}

//=========================================================================
void LLEnvironment::requestRegion()
{
    if (gAgent.getRegionCapability("ExtEnvironment").empty())
    {
        LLEnvironmentRequest::initiate();
        return;
    }

    requestParcel(INVALID_PARCEL_ID);
}

void LLEnvironment::updateRegion(LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset)
{
    if (gAgent.getRegionCapability("ExtEnvironment").empty())
    {
        LLEnvironmentApply::initiateRequest( LLSettingsVODay::convertToLegacy(pday) );
        return;
    }

    updateParcel(INVALID_PARCEL_ID, pday, day_length, day_offset);
}

void LLEnvironment::resetRegion()
{
    resetParcel(INVALID_PARCEL_ID);
}

void LLEnvironment::requestParcel(S32 parcel_id)
{
    environment_apply_fn apply = boost::bind(&LLEnvironment::recordEnvironment, this, _1, _2);

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroRequestEnvironment",
        boost::bind(&LLEnvironment::coroRequestEnvironment, this, parcel_id, apply));

}

void LLEnvironment::updateParcel(S32 parcel_id, LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset)
{
    environment_apply_fn apply = boost::bind(&LLEnvironment::recordEnvironment, this, _1, _2);

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        boost::bind(&LLEnvironment::coroUpdateEnvironment, this, parcel_id, 
        pday, day_length, day_offset, apply));

}

void LLEnvironment::resetParcel(S32 parcel_id)
{
    environment_apply_fn apply = boost::bind(&LLEnvironment::recordEnvironment, this, _1, _2);

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroResetEnvironment",
        boost::bind(&LLEnvironment::coroResetEnvironment, this, parcel_id, apply));
}

void LLEnvironment::coroRequestEnvironment(S32 parcel_id, LLEnvironment::environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    LL_WARNS("LAPRAS") << "Requesting for parcel_id=" << parcel_id << LL_ENDL;

    if (parcel_id != INVALID_PARCEL_ID)
    {
        std::stringstream query;

        query << "?parcelid=" << parcel_id;
        url += query.str();
    }

    LL_WARNS("LAPRAS") << "url=" << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);
    // results that come back may contain the new settings

    LLSD notify;

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("WindlightCaps") << "Couldn't retrieve Windlight settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        std::stringstream msg;
        msg << status.toString() << " (Code " << status.toTerseString() << ")";
        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = msg.str();

    }
    else
    {
        LLSD environment = result["environment"];
        if (environment.isDefined() && !apply.empty())
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
        //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
    }
}

void LLEnvironment::coroUpdateEnvironment(S32 parcel_id, LLSettingsDay::ptr_t pday, S32 day_length, S32 day_offset, LLEnvironment::environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    LLSD body(LLSD::emptyMap());
    body["environment"] = LLSD::emptyMap();

    if (day_length >= 0)
        body["environment"]["day_length"] = day_length;
    if (day_offset >= 0)
        body["environment"]["day_offset"] = day_offset;
    if (pday)
        body["environment"]["day_cycle"] = pday->getSettings();

    LL_WARNS("LAPRAS") << "Body = " << body << LL_ENDL;

    if (parcel_id != INVALID_PARCEL_ID)
    {
        std::stringstream query;

        query << "?parcelid=" << parcel_id;
        url += query.str();
    }

    LL_WARNS("LAPRAS") << "url=" << url << LL_ENDL;

    LLSD result = httpAdapter->putAndSuspend(httpRequest, url, body);
    // results that come back may contain the new settings

    LLSD notify;

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("WindlightCaps") << "Couldn't update Windlight settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
        LLSD environment = result["environment"];
        if (environment.isDefined() && !apply.empty())
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
        //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
    }
}

void LLEnvironment::coroResetEnvironment(S32 parcel_id, environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    if (parcel_id != INVALID_PARCEL_ID)
    {
        std::stringstream query;

        query << "?parcelid=" << parcel_id;
        url += query.str();
    }

    LL_WARNS("LAPRAS") << "url=" << url << LL_ENDL;

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, url);
    // results that come back may contain the new settings

    LLSD notify; 

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("WindlightCaps") << "Couldn't reset Windlight settings in " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
        LLSD environment = result["environment"];
        if (environment.isDefined() && !apply.empty())
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
        //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
    }

}


//=========================================================================
LLEnvironment::UserPrefs::UserPrefs():
    mUseRegionSettings(true),
    mUseDayCycle(true),
    mPersistEnvironment(false),
    mWaterPresetName(),
    mSkyPresetName(),
    mDayCycleName()
{}


void LLEnvironment::UserPrefs::load()
{
    mPersistEnvironment = gSavedSettings.getBOOL("EnvironmentPersistAcrossLogin");

    mWaterPresetName = gSavedSettings.getString("WaterPresetName");
    mSkyPresetName = gSavedSettings.getString("SkyPresetName");
    mDayCycleName = gSavedSettings.getString("DayCycleName");

    mUseRegionSettings = mPersistEnvironment ? gSavedSettings.getBOOL("UseEnvironmentFromRegion") : true;
    mUseDayCycle = mPersistEnvironment ? gSavedSettings.getBOOL("UseDayCycle") : true;
}

void LLEnvironment::UserPrefs::store()
{
    gSavedSettings.setBOOL("EnvironmentPersistAcrossLogin", mPersistEnvironment);
    if (mPersistEnvironment)
    {
        gSavedSettings.setString("WaterPresetName", getWaterPresetName());
        gSavedSettings.setString("SkyPresetName", getSkyPresetName());
        gSavedSettings.setString("DayCycleName", getDayCycleName());

        gSavedSettings.setBOOL("UseEnvironmentFromRegion", getUseRegionSettings());
        gSavedSettings.setBOOL("UseDayCycle", getUseDayCycle());
    }
}

LLEnvironment::EnvironmentInfo::EnvironmentInfo():
    mParcelId(),
    mDayLength(LLSettingsDay::DEFAULT_DAYLENGTH),
    mDayOffset(LLSettingsDay::DEFAULT_DAYOFFSET),
    mDaycycleData(),
    mAltitudes(),
    mIsDefault(false)
{
}

LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extract(LLSD environment)
{
    ptr_t pinfo = boost::make_shared<EnvironmentInfo>();

    if (environment.has("parcel_id"))
        pinfo->mParcelId = environment["parcel_id"].asUUID();
    if (environment.has("day_length"))
        pinfo->mDayLength = S64Seconds(environment["day_length"].asInteger());
    if (environment.has("day_offset"))
        pinfo->mDayOffset = S64Seconds(environment["day_offset"].asInteger());
    if (environment.has("day_cycle"))
        pinfo->mDaycycleData = environment["day_cycle"];
    if (environment.has("is_default"))
        pinfo->mIsDefault = environment["is_default"].asBoolean();
    if (environment.has("track_altitudes"))
        pinfo->mAltitudes = environment["track_altitudes"];

    return pinfo;
}

//=========================================================================
// Transitional Code.
// static
std::string LLEnvironment::getSysDir(const std::string &subdir)
{
    return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight\\"+subdir, "");
}

// static
std::string LLEnvironment::getUserDir(const std::string &subdir)
{
    return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "windlight\\"+subdir, "");
}

LLSD LLEnvironment::legacyLoadPreset(const std::string& path)
{
    llifstream xml_file;
    std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));

    xml_file.open(path.c_str());
    if (!xml_file)
    {
        return LLSD();
    }

    LLSD params_data;
    LLPointer<LLSDParser> parser = new LLSDXMLParser();
    parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED);
    xml_file.close();

    return params_data;
}

void LLEnvironment::legacyLoadAllPresets()
{
    std::string dir;
    std::string file;

    // System skies
    {
        dir = getSysDir("skies");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addSky(sky);
            }
        }
    }

    // User skies
    {
        dir = getUserDir("skies");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addSky(sky);
            }
        }
    }

    // System water
    {
        dir = getSysDir("water");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addWater(water);
            }
        }
    }

    // User water
    {
        dir = getUserDir("water");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addWater(water);
            }
        }
    }

    // System water
    {
        dir = getSysDir("days");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addDayCycle(day);

#ifdef EXPORT_PRESETS
                std::string exportfile = LLURI::escape(name) + "(new).xml";
                std::string exportpath = gDirUtilp->add(getSysDir("new"), exportfile);

                LLSD settings = day->getSettings();

                std::ofstream daycyclefile(exportpath);
                LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
                formatter->format(settings, daycyclefile, LLSDFormatter::OPTIONS_PRETTY);
                daycyclefile.close();
#endif
            }
        }
    }

    // User water
    {
        dir = getUserDir("days");
        LLDirIterator dir_iter(dir, "*.xml");
        while (dir_iter.next(file))
        {
            std::string path = gDirUtilp->add(dir, file);

            LLSD data = legacyLoadPreset(path);
            if (data)
            {
                std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

                LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addDayCycle(day);
            }
        }
    }
}
