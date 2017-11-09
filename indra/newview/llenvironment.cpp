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

#include "llsdserialize.h"
#include "lldiriterator.h"

#include <boost/make_shared.hpp>
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
    mSelectedDayCycle(),
    mSkysById(),
    mSkysByName(),
    mWaterByName(),
    mWaterById(),
    mDayCycleByName(),
    mDayCycleById(),
    mUserPrefs()
{
}

void LLEnvironment::initSingleton()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsSky::buildDefaultSky();
    addSky(p_default_sky);
    mCurrentSky = p_default_sky;

    LLSettingsWater::ptr_t p_default_water = LLSettingsWater::buildDefaultWater();
    addWater(p_default_water);
    mCurrentWater = p_default_water;

    LLSettingsDayCycle::ptr_t p_default_day = LLSettingsDayCycle::buildDefaultDayCycle();
    addDayCycle(p_default_day);
    mCurrentDayCycle.reset();

    applyAllSelected();

    // LEGACY!
    legacyLoadAllPresets();

    LLEnvironmentRequest::initiate();
    gAgent.addRegionChangedCallback(boost::bind(&LLEnvironment::onRegionChange, this));
}

LLEnvironment::~LLEnvironment()
{
}

void LLEnvironment::loadPreferences()
{
    mUserPrefs.load();
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
    LLEnvironmentRequest::initiate();
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

    if (mCurrentDayCycle)
        mCurrentDayCycle->update();

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

void LLEnvironment::applySky(const LLSettingsSky::ptr_t &sky)
{
#if 0
    if (sky)
    {
        mCurrentSky = sky;
    }
    else
    {
        mCurrentSky = mCurrentSky;
    }
#endif
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

void LLEnvironment::applyWater(const LLSettingsWater::ptr_t water)
{
#if 0
    if (water)
    {
        mCurrentWater = water;
    }
    else
    {
        mCurrentWater = mCurrentWater;
    }
#endif
}

void LLEnvironment::selectDayCycle(const std::string &name, F32Seconds transition)
{
    LLSettingsDayCycle::ptr_t next_daycycle = findDayCycleByName(name);

    if (!next_daycycle)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to select daycycle with unknown name '" << name << "'" << LL_ENDL;
        return;
    }

    selectDayCycle(next_daycycle, transition);
}

void LLEnvironment::selectDayCycle(const LLSettingsDayCycle::ptr_t &daycycle, F32Seconds transition)
{
    if (!daycycle)
    {
        return;
    }

    mCurrentDayCycle = daycycle;

    daycycle->startDayCycle();
    selectWater(daycycle->getCurrentWater(), transition);
    selectSky(daycycle->getCurrentSky(), transition);
}

void LLEnvironment::applyDayCycle(const LLSettingsDayCycle::ptr_t &daycycle)
{
#if 0
    if (daycycle)
    {
        mCurrentDayCycle = daycycle;
    }
    else
    {
        mCurrentDayCycle = mCurrentDayCycle;
    }
#endif
}

void LLEnvironment::clearAllSelected()
{
#if 0
    if (mCurrentSky != mCurrentSky)
        selectSky();
    if (mCurrentWater != mCurrentWater)
        selectWater();
    if (mCurrentDayCycle != mCurrentDayCycle)
        selectDayCycle();
#endif
}

void LLEnvironment::applyAllSelected()
{
    if (mCurrentSky != mCurrentSky)
        applySky();
    if (mCurrentWater != mCurrentWater)
        applyWater();
    if (mCurrentDayCycle != mCurrentDayCycle)
        applyDayCycle();
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

// void LLEnvironment::addSky(const LLUUID &id, const LLSettingsSky::ptr_t &sky)
// {
//     //     std::string name = sky->getValue(LLSettingsSky::SETTING_NAME).asString();
//     // 
//     //     std::pair<NamedSkyMap_t::iterator, bool> result;
//     //     result = mSkysByName.insert(NamedSkyMap_t::value_type(name, sky));
//     // 
//     //     if (!result.second)
//     //         (*(result.first)).second = sky;
// }

void LLEnvironment::removeSky(const std::string &name)
{
    namedSettingMap_t::iterator it = mSkysByName.find(name);
    if (it != mSkysByName.end())
        mSkysByName.erase(it);
    mSkyListChange();
}

// void LLEnvironment::removeSky(const LLUUID &id)
// {
// 
// }

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

//void LLEnvironment::addWater(const LLUUID &id, const LLSettingsSky::ptr_t &sky);

void LLEnvironment::removeWater(const std::string &name)
{
    namedSettingMap_t::iterator it = mWaterByName.find(name);
    if (it != mWaterByName.end())
        mWaterByName.erase(it);
    mWaterListChange();
}

//void LLEnvironment::removeWater(const LLUUID &id);
void LLEnvironment::clearAllWater()
{
    mWaterByName.clear();
    mWaterById.clear();
    mWaterListChange();
}

void LLEnvironment::addDayCycle(const LLSettingsDayCycle::ptr_t &daycycle)
{
    std::string name = daycycle->getValue(LLSettingsDayCycle::SETTING_NAME).asString();

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

LLSettingsDayCycle::ptr_t LLEnvironment::findDayCycleByName(std::string name) const
{
    namedSettingMap_t::const_iterator it = mDayCycleByName.find(name);

    if (it == mDayCycleByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find daycycle with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsDayCycle::ptr_t();
    }

    return boost::static_pointer_cast<LLSettingsDayCycle>((*it).second);
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

//=========================================================================
// Transitional Code.
void LLEnvironment::onLegacyRegionSettings(LLSD data)
{
    LLUUID regionId = data[0]["regionID"].asUUID();

    LLSettingsDayCycle::ptr_t regionday;
    if (data[1].isUndefined())
        regionday = LLEnvironment::findDayCycleByName("Default");
    else
        regionday = LLSettingsDayCycle::buildFromLegacyMessage(regionId, data[1], data[2], data[3]);

    selectDayCycle(regionday, TRANSITION_DEFAULT);
}

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

                LLSettingsSky::ptr_t sky = LLSettingsSky::buildFromLegacyPreset(name, data);
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

                LLSettingsSky::ptr_t sky = LLSettingsSky::buildFromLegacyPreset(name, data);
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

                LLSettingsWater::ptr_t water = LLSettingsWater::buildFromLegacyPreset(name, data);
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

                LLSettingsWater::ptr_t water = LLSettingsWater::buildFromLegacyPreset(name, data);
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

                LLSettingsDayCycle::ptr_t day = LLSettingsDayCycle::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addDayCycle(day);
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

                LLSettingsDayCycle::ptr_t day = LLSettingsDayCycle::buildFromLegacyPreset(name, data);
                LLEnvironment::instance().addDayCycle(day);
            }
        }
    }
}
