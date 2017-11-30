/**
* @file llsettingsvo.cpp
* @author Rider Linden
* @brief Subclasses for viewer specific settings behaviors.
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
#include "llsettingsvo.h"

#include "pipeline.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

#include "llglslshader.h"
#include "llviewershadermgr.h"

#include "llenvironment.h"
#include "llsky.h"

//=========================================================================
LLSettingsVOSky::LLSettingsVOSky(const LLSD &data):
    LLSettingsSky(data)
{
}

LLSettingsVOSky::LLSettingsVOSky():
    LLSettingsSky()
{
}

//-------------------------------------------------------------------------
LLSettingsSky::ptr_t LLSettingsVOSky::buildFromLegacyPreset(const std::string &name, const LLSD &legacy)
{

    LLSD newsettings = LLSettingsSky::translateLegacySettings(legacy);

    newsettings[SETTING_NAME] = name;

    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsVOSky>(newsettings);

    if (skyp->validate())
        return skyp;

    return LLSettingsSky::ptr_t();
}

LLSettingsSky::ptr_t LLSettingsVOSky::buildDefaultSky()
{
    LLSD settings = LLSettingsSky::defaults();
    settings[SETTING_NAME] = std::string("_default_");


    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsVOSky>(settings);
    if (skyp->validate())
        return skyp;

    return LLSettingsSky::ptr_t();
}

LLSettingsSky::ptr_t LLSettingsVOSky::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsVOSky>(settings);

    if (skyp->validate())
        return skyp;

    return LLSettingsSky::ptr_t();
}

//-------------------------------------------------------------------------
void LLSettingsVOSky::updateSettings()
{
    LLSettingsSky::updateSettings();

    LLVector3 sun_direction = getSunDirection();
    LLVector3 moon_direction = getMoonDirection();

    // set direction (in CRF) and don't allow overriding
    LLVector3 crf_sunDirection(sun_direction.mV[2], sun_direction.mV[0], sun_direction.mV[1]);
    LLVector3 crf_moonDirection(moon_direction.mV[2], moon_direction.mV[0], moon_direction.mV[1]);

    gSky.setSunDirection(crf_sunDirection, crf_moonDirection);
}

void LLSettingsVOSky::applySpecial(void *ptarget)
{
    LLGLSLShader *shader = (LLGLSLShader *)ptarget;

    shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, getClampedLightDirection().mV);

    shader->uniform4f(LLShaderMgr::GAMMA, getGamma(), 0.0, 0.0, 1.0);

    {
        LLVector4 vect_c_p_d1(mSettings[SETTING_CLOUD_POS_DENSITY1]);
        vect_c_p_d1 += LLVector4(LLEnvironment::instance().getCloudScrollDelta());
        shader->uniform4fv(LLShaderMgr::CLOUD_POS_DENSITY1, 1, vect_c_p_d1.mV);
    }
}

LLSettingsSky::parammapping_t LLSettingsVOSky::getParameterMap() const
{
    static parammapping_t param_map;

    if (param_map.empty())
    {
        param_map[SETTING_AMBIENT] = LLShaderMgr::AMBIENT;
        param_map[SETTING_BLUE_DENSITY] = LLShaderMgr::BLUE_DENSITY;
        param_map[SETTING_BLUE_HORIZON] = LLShaderMgr::BLUE_HORIZON;
        param_map[SETTING_CLOUD_COLOR] = LLShaderMgr::CLOUD_COLOR;

        param_map[SETTING_CLOUD_POS_DENSITY2] = LLShaderMgr::CLOUD_POS_DENSITY2;
        param_map[SETTING_CLOUD_SCALE] = LLShaderMgr::CLOUD_SCALE;
        param_map[SETTING_CLOUD_SHADOW] = LLShaderMgr::CLOUD_SHADOW;
        param_map[SETTING_DENSITY_MULTIPLIER] = LLShaderMgr::DENSITY_MULTIPLIER;
        param_map[SETTING_DISTANCE_MULTIPLIER] = LLShaderMgr::DISTANCE_MULTIPLIER;
        param_map[SETTING_GLOW] = LLShaderMgr::GLOW;
        param_map[SETTING_HAZE_DENSITY] = LLShaderMgr::HAZE_DENSITY;
        param_map[SETTING_HAZE_HORIZON] = LLShaderMgr::HAZE_HORIZON;
        param_map[SETTING_MAX_Y] = LLShaderMgr::MAX_Y;
        param_map[SETTING_SUNLIGHT_COLOR] = LLShaderMgr::SUNLIGHT_COLOR;
    }

    return param_map;
}

//=========================================================================
const F32 LLSettingsVOWater::WATER_FOG_LIGHT_CLAMP(0.3f);

//-------------------------------------------------------------------------
LLSettingsVOWater::LLSettingsVOWater(const LLSD &data) :
    LLSettingsWater(data)
{

}

LLSettingsVOWater::LLSettingsVOWater() :
    LLSettingsWater()
{

}

//-------------------------------------------------------------------------
LLSettingsWater::ptr_t LLSettingsVOWater::buildFromLegacyPreset(const std::string &name, const LLSD &legacy)
{
    LLSD newsettings(LLSettingsWater::translateLegacySettings(legacy));

    newsettings[SETTING_NAME] = name; 

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsVOWater>(newsettings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

LLSettingsWater::ptr_t LLSettingsVOWater::buildDefaultWater()
{
    LLSD settings = LLSettingsWater::defaults();
    settings[SETTING_NAME] = std::string("_default_");

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsVOWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

LLSettingsWater::ptr_t LLSettingsVOWater::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsVOWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

//-------------------------------------------------------------------------
void LLSettingsVOWater::applySpecial(void *ptarget)
{
    LLGLSLShader *shader = (LLGLSLShader *)ptarget;

    shader->uniform4fv(LLShaderMgr::WATER_WATERPLANE, 1, getWaterPlane().mV);
    shader->uniform1f(LLShaderMgr::WATER_FOGKS, getWaterFogKS());

    shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, LLEnvironment::instance().getRotatedLight().mV);
    shader->uniform3fv(LLShaderMgr::WL_CAMPOSLOCAL, 1, LLViewerCamera::getInstance()->getOrigin().mV);
    shader->uniform1f(LLViewerShaderMgr::DISTANCE_MULTIPLIER, 0);
}

void LLSettingsVOWater::updateSettings()
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

LLSettingsWater::parammapping_t LLSettingsVOWater::getParameterMap() const
{
    static parammapping_t param_map;

    if (param_map.empty())
    {
        param_map[SETTING_FOG_COLOR] = LLShaderMgr::WATER_FOGCOLOR;
        param_map[SETTING_FOG_DENSITY] = LLShaderMgr::WATER_FOGDENSITY;
    }
    return param_map;
}

//=========================================================================
LLSettingsVODay::LLSettingsVODay(const LLSD &data):
    LLSettingsDay(data)
{}

LLSettingsVODay::LLSettingsVODay():
    LLSettingsDay()
{}

//-------------------------------------------------------------------------
LLSettingsDay::ptr_t LLSettingsVODay::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(defaults());

    newsettings[SETTING_NAME] = name;
    newsettings[SETTING_DAYLENGTH] = static_cast<S32>(MINIMUM_DAYLENGTH);

    LLSD watertrack = LLSDArray(
        LLSDMap(SETTING_KEYKFRAME, LLSD::Real(0.0f))
        (SETTING_KEYNAME, "Default"));

    LLSD skytrack = LLSD::emptyArray();

    for (LLSD::array_const_iterator it = oldsettings.beginArray(); it != oldsettings.endArray(); ++it)
    {
        LLSD entry = LLSDMap(SETTING_KEYKFRAME, (*it)[0].asReal())
            (SETTING_KEYNAME, (*it)[1].asString());
        skytrack.append(entry);
    }

    newsettings[SETTING_TRACKS] = LLSDArray(watertrack)(skytrack);

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsVODay>(newsettings);

    if (dayp->validate())
    {
        dayp->initialize();
        return dayp;
    }

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::ptr_t LLSettingsVODay::buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skydefs, LLSD waterdef)
{
    LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPreset("Region", waterdef);
    LLEnvironment::namedSettingMap_t skys;

    for (LLSD::map_iterator itm = skydefs.beginMap(); itm != skydefs.endMap(); ++itm)
    {
        std::string name = (*itm).first;
        LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPreset(name, (*itm).second);

        skys[name] = sky;
        LL_WARNS("WindlightCaps") << "created region sky '" << name << "'" << LL_ENDL;
    }

    LLSettingsDay::ptr_t dayp = buildFromLegacyPreset("Region (legacy)", daycycle);

    dayp->setWaterAtKeyframe(water, 0.0f);

    for (LLSD::array_iterator ita = daycycle.beginArray(); ita != daycycle.endArray(); ++ita)
    {
        F32 frame = (*ita)[0].asReal();
        std::string name = (*ita)[1].asString();

        LLEnvironment::namedSettingMap_t::iterator it = skys.find(name);

        if (it == skys.end())
            continue;
        dayp->setSkyAtKeyframe(boost::static_pointer_cast<LLSettingsSky>((*it).second), frame, 1);

        LL_WARNS("WindlightCaps") << "Added '" << name << "' to region day cycle at " << frame << LL_ENDL;
    }

    dayp->setInitialized();

    if (dayp->validate())
    {
        return dayp;
    }

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::ptr_t LLSettingsVODay::buildDefaultDayCycle()
{
    LLSD settings = LLSettingsDay::defaults();
    settings[SETTING_NAME] = std::string("_default_");

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsVODay>(settings);

    if (dayp->validate())
    {
        dayp->initialize();
        return dayp;
    }

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::ptr_t LLSettingsVODay::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsVODay>(settings);

    return dayp;
}

LLSettingsSkyPtr_t  LLSettingsVODay::getDefaultSky() const
{
    return LLSettingsVOSky::buildDefaultSky();
}

LLSettingsWaterPtr_t LLSettingsVODay::getDefaultWater() const
{
    return LLSettingsVOWater::buildDefaultWater();
}

LLSettingsSkyPtr_t LLSettingsVODay::buildSky(LLSD settings) const
{
    LLSettingsSky::ptr_t skyp = boost::make_shared<LLSettingsVOSky>(settings);

    if (skyp->validate())
        return skyp;

    return LLSettingsSky::ptr_t();
}

LLSettingsWaterPtr_t LLSettingsVODay::buildWater(LLSD settings) const
{
    LLSettingsWater::ptr_t waterp = boost::make_shared<LLSettingsVOWater>(settings);

    if (waterp->validate())
        return waterp;

    return LLSettingsWater::ptr_t();
}

LLSettingsSkyPtr_t LLSettingsVODay::getNamedSky(const std::string &name) const
{
    return LLEnvironment::instance().findSkyByName(name);
}

LLSettingsWaterPtr_t LLSettingsVODay::getNamedWater(const std::string &name) const
{
    return LLEnvironment::instance().findWaterByName(name);
}
