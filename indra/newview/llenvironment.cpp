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

#include <algorithm>

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

#include "llregioninfomodel.h"

#include <boost/make_shared.hpp>

#include "llatmosphere.h"

//define EXPORT_PRESETS 1
//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle   FTM_ENVIRONMENT_UPDATE("Update Environment Tick");
    LLTrace::BlockTimerStatHandle   FTM_SHADER_PARAM_UPDATE("Update Shader Parameters");

    //---------------------------------------------------------------------
    inline LLSettingsBase::TrackPosition get_wrapping_distance(LLSettingsBase::TrackPosition begin, LLSettingsBase::TrackPosition end)
    {
        if (begin < end)
        {
            return end - begin;
        }
        else if (begin > end)
        {
            return LLSettingsBase::TrackPosition(1.0) - (begin - end);
        }

        return 0;
    }

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atafter(LLSettingsDay::CycleTrack_t &collection, const LLSettingsBase::TrackPosition& key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDay::CycleTrack_t::iterator it = collection.upper_bound(key);

        if (it == collection.end())
        {   // wrap around
            it = collection.begin();
        }

        return it;
    }

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atbefore(LLSettingsDay::CycleTrack_t &collection, const LLSettingsBase::TrackPosition& key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDay::CycleTrack_t::iterator it = collection.lower_bound(key);

        if (it == collection.end())
        {   // all keyframes are lower, take the last one.
            --it; // we know the range is not empty
        }
        else if ((*it).first > key)
        {   // the keyframe we are interested in is smaller than the found.
            if (it == collection.begin())
                it = collection.end();
            --it;
        }

        return it;
    }

    LLSettingsDay::TrackBound_t get_bounding_entries(LLSettingsDay::CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe)
    {
        return LLSettingsDay::TrackBound_t(get_wrapping_atbefore(track, keyframe), get_wrapping_atafter(track, keyframe));
    }

    //---------------------------------------------------------------------
    class LLTrackBlenderLoopingTime : public LLSettingsBlenderTimeDelta
    {
    public:
        LLTrackBlenderLoopingTime(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno, LLSettingsBase::Seconds cyclelength, LLSettingsBase::Seconds cycleoffset) :
            LLSettingsBlenderTimeDelta(target, LLSettingsBase::ptr_t(), LLSettingsBase::ptr_t(), LLSettingsBase::Seconds(1.0)),
            mDay(day),
            mTrackNo(0),
            mCycleLength(cyclelength),
            mCycleOffset(cycleoffset)
        {
            LLSettingsDay::TrackBound_t initial = getBoundingEntries(getAdjustedNow());

            mInitial = (*initial.first).second;
            mFinal = (*initial.second).second;
            mBlendSpan = getSpanTime(initial);

            mTrackNo = selectTrackNumber(trackno);

            setOnFinished([this](const LLSettingsBlender::ptr_t &){ onFinishedSpan(); });
        }


        void switchTrack(S32 trackno, const LLSettingsBase::BlendFactor&) override
        {
            S32 use_trackno = selectTrackNumber(trackno);

            if (use_trackno == mTrackNo)
            {   // results in no change
                return;
            }

            mTrackTransitionStart = mTarget->buildDerivedClone();
            mTrackNo = use_trackno;

            LLSettingsBase::Seconds now = getAdjustedNow() + LLEnvironment::TRANSITION_ALTITUDE;
            LLSettingsDay::TrackBound_t bounds = getBoundingEntries(now);

            LLSettingsBase::ptr_t pendsetting  = (*bounds.first).second->buildDerivedClone();
            LLSettingsBase::TrackPosition targetpos  = convertTimeToPosition(now) - (*bounds.first).first;
            LLSettingsBase::TrackPosition targetspan = get_wrapping_distance((*bounds.first).first, (*bounds.second).first);

            LLSettingsBase::BlendFactor blendf = calculateBlend(targetpos, targetspan);
            pendsetting->blend((*bounds.second).second, blendf);

            reset(mTrackTransitionStart, pendsetting, LLEnvironment::TRANSITION_ALTITUDE);
        }

    protected:
        S32 selectTrackNumber(S32 trackno)
        {
            if (trackno == 0)
            {   // We are dealing with the water track.  There is only ever one.
                return 0;
            }

            for (S32 test = trackno; test == 0; --test)
            {   // Find the track below the requested one with data.
                LLSettingsDay::CycleTrack_t &track = mDay->getCycleTrack(mTrackNo);

                if (!track.empty())
                    return test;
            }

            return 1;
        }

        LLSettingsDay::TrackBound_t getBoundingEntries(LLSettingsBase::Seconds time)
        {
            LLSettingsDay::CycleTrack_t &wtrack = mDay->getCycleTrack(mTrackNo);
            F64 position = convertTimeToPosition(time);

            LLSettingsDay::TrackBound_t bounds = get_bounding_entries(wtrack, position);
            return bounds;
        }

        LLSettingsBase::Seconds getAdjustedNow() const
        {
            LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());

            return (now + mCycleOffset);
        }

        LLSettingsBase::Seconds getSpanTime(const LLSettingsDay::TrackBound_t &bounds) const
        {
            return mCycleLength * get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
        }

        LLSettingsBase::TrackPosition convertTimeToPosition(const LLSettingsBase::Seconds& time)
        {
            F64 position = fmod((F64)time, (F64)mCycleLength) / (F64)mCycleLength;
            return llclamp(position, 0.0, 1.0);
        }

    private:
        LLSettingsDay::ptr_t        mDay;
        S32                         mTrackNo;
        LLSettingsBase::Seconds     mCycleLength;
        LLSettingsBase::Seconds     mCycleOffset;
        LLSettingsBase::ptr_t       mTrackTransitionStart;

        void onFinishedSpan()
        {
            LLSettingsDay::TrackBound_t next = getBoundingEntries(getAdjustedNow());
            LLSettingsBase::Seconds nextspan = getSpanTime(next);
            reset((*next.first).second, (*next.second).second, nextspan);
        }
    };

}

//=========================================================================
const F32Seconds LLEnvironment::TRANSITION_INSTANT(0.0f);
const F32Seconds LLEnvironment::TRANSITION_FAST(1.0f);
const F32Seconds LLEnvironment::TRANSITION_DEFAULT(5.0f);
const F32Seconds LLEnvironment::TRANSITION_SLOW(10.0f);
const F32Seconds LLEnvironment::TRANSITION_ALTITUDE(5.0f);

//*TODO* Change these when available on Agni (these are Damballah asset IDs).
const LLUUID LLEnvironment::KNOWN_SKY_SUNRISE("645d7475-19d6-d05c-6eb2-29eeacf76e06");
const LLUUID LLEnvironment::KNOWN_SKY_MIDDAY("68f5a7ec-2785-d9d8-be7c-cca93976759a");
const LLUUID LLEnvironment::KNOWN_SKY_SUNSET("06420773-757b-4b7c-a1f9-85fceb2f7bd4");
const LLUUID LLEnvironment::KNOWN_SKY_MIDNIGHT("c13658f3-91b8-d559-3a12-b11ce3940c4c");

const F32 LLEnvironment::SUN_DELTA_YAW(F_PI);   // 180deg 

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
    mSelectedEnvironment(LLEnvironment::ENV_LOCAL),
    mCurrentTrack(1)
{
}

void LLEnvironment::initSingleton()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsVOSky::buildDefaultSky();
    addSky(p_default_sky);

    LLSettingsWater::ptr_t p_default_water = LLSettingsVOWater::buildDefaultWater();
    addWater(p_default_water);

    LLSettingsDay::ptr_t p_default_day = LLSettingsVODay::buildDefaultDayCycle();
    addDayCycle(p_default_day);

    mCurrentEnvironment = std::make_shared<DayInstance>();
    mCurrentEnvironment->setSky(p_default_sky);
    mCurrentEnvironment->setWater(p_default_water);

    mEnvironments[ENV_DEFAULT] = mCurrentEnvironment;

    // LEGACY!
    legacyLoadAllPresets();

    requestRegionEnvironment();

    LLRegionInfoModel::instance().setUpdateCallback([this]() { onParcelChange(); });
    gAgent.addParcelChangedCallback([this]() { onParcelChange(); });

    //TODO: This frequently results in one more request than we need.  It isn't breaking, but should be nicer.
    gAgent.addRegionChangedCallback([this]() { requestRegionEnvironment(); });

    gAgent.whenPositionChanged([this](const LLVector3 &localpos, const LLVector3d &) { onAgentPositionHasChanged(localpos); });
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

void LLEnvironment::getAtmosphericModelSettings(AtmosphericModelSettings& settingsOut, const LLSettingsSky::ptr_t &psky)
{
    settingsOut.m_skyBottomRadius   = psky->getSkyBottomRadius();
    settingsOut.m_skyTopRadius      = psky->getSkyTopRadius();
    settingsOut.m_sunArcRadians     = psky->getSunArcRadians();
    settingsOut.m_mieAnisotropy     = psky->getMieAnisotropy();

    LLSD rayleigh = psky->getRayleighConfigs();
    settingsOut.m_rayleighProfile.clear();
    for (LLSD::array_iterator itf = rayleigh.beginArray(); itf != rayleigh.endArray(); ++itf)
    {
        atmosphere::DensityProfileLayer layer;
        LLSD& layerConfig = (*itf);
        layer.constant_term     = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
        layer.exp_scale         = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
        layer.exp_term          = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
        layer.linear_term       = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
        layer.width             = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();
        settingsOut.m_rayleighProfile.push_back(layer);
    }

    LLSD mie = psky->getMieConfigs();
    settingsOut.m_mieProfile.clear();
    for (LLSD::array_iterator itf = mie.beginArray(); itf != mie.endArray(); ++itf)
    {
        atmosphere::DensityProfileLayer layer;
        LLSD& layerConfig = (*itf);
        layer.constant_term     = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
        layer.exp_scale         = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
        layer.exp_term          = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
        layer.linear_term       = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
        layer.width             = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();
        settingsOut.m_mieProfile.push_back(layer);
    }

    LLSD absorption = psky->getAbsorptionConfigs();
    settingsOut.m_absorptionProfile.clear();
    for (LLSD::array_iterator itf = absorption.beginArray(); itf != absorption.endArray(); ++itf)
    {
        atmosphere::DensityProfileLayer layer;
        LLSD& layerConfig = (*itf);
        layer.constant_term     = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
        layer.exp_scale         = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
        layer.exp_term          = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
        layer.linear_term       = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
        layer.width             = layerConfig[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();
        settingsOut.m_absorptionProfile.push_back(layer);
    }
}

bool LLEnvironment::isExtendedEnvironmentEnabled() const
{
    return !gAgent.getRegionCapability("ExtEnvironment").empty();
}

bool LLEnvironment::isInventoryEnabled() const
{
    return (!gAgent.getRegionCapability("UpdateSettingsAgentInventory").empty() &&
        !gAgent.getRegionCapability("UpdateSettingsTaskInventory").empty());
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

    clearEnvironment(ENV_PARCEL);
    setEnvironment(ENV_REGION, regionday, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);

    updateEnvironment();
}

//-------------------------------------------------------------------------
F32 LLEnvironment::getCamHeight() const
{
    return (mCurrentEnvironment->getSky()->getDomeOffset() * mCurrentEnvironment->getSky()->getDomeRadius());
}

F32 LLEnvironment::getWaterHeight() const
{
    return gAgent.getRegion()->getWaterHeight();
}

bool LLEnvironment::getIsSunUp() const
{
    return mCurrentEnvironment->getSky()->getIsSunUp();
}

bool LLEnvironment::getIsMoonUp() const
{
    return mCurrentEnvironment->getSky()->getIsMoonUp();
}

//-------------------------------------------------------------------------
void LLEnvironment::setSelectedEnvironment(LLEnvironment::EnvSelection_t env, LLSettingsBase::Seconds transition, bool forced)
{
    mSelectedEnvironment = env;
    updateEnvironment(transition, forced);
}

bool LLEnvironment::hasEnvironment(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT) || (!mEnvironments[env]))
    {
        return false;
    }

    return true;
}

LLEnvironment::DayInstance::ptr_t LLEnvironment::getEnvironmentInstance(LLEnvironment::EnvSelection_t env, bool create /*= false*/)
{
    DayInstance::ptr_t environment = mEnvironments[env];
//    if (!environment && create)
    if (create)
    {
        environment = std::make_shared<DayInstance>();
        mEnvironments[env] = environment;
    }

    return environment;
}


void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, const LLSettingsDay::ptr_t &pday, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {   
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection." << LL_ENDL;
        return;
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env, true);

    environment->clear();
    environment->setDay(pday, daylength, dayoffset);
    environment->setSkyTrack(mCurrentTrack);
    environment->animate();
    /*TODO: readjust environment*/
}


void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, LLEnvironment::fixedEnvironment_t fixed)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection." << LL_ENDL;
        return;
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env, true);

    environment->clear();
    environment->setSky((fixed.first) ? fixed.first : mEnvironments[ENV_DEFAULT]->getSky());
    environment->setWater((fixed.second) ? fixed.second : mEnvironments[ENV_DEFAULT]->getWater());

    /*TODO: readjust environment*/
}

void LLEnvironment::setEnvironment(LLEnvironment::EnvSelection_t env, const LLSettingsBase::ptr_t &settings)
{
    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (env == ENV_DEFAULT)
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to set default environment. Not allowed." << LL_ENDL;
        return;
    }

    if (!settings)
    {
        clearEnvironment(env);
        return;
    }

    if (settings->getSettingType() == "daycycle")
    {
        LLSettingsDay::Seconds daylength(LLSettingsDay::DEFAULT_DAYLENGTH);
        LLSettingsDay::Seconds dayoffset(LLSettingsDay::DEFAULT_DAYOFFSET);
        if (environment)
        {
            daylength = environment->getDayLength();
            dayoffset = environment->getDayOffset();
        }
        setEnvironment(env, std::static_pointer_cast<LLSettingsDay>(settings), daylength, dayoffset);
    }
    else if (settings->getSettingType() == "sky")
    {
        fixedEnvironment_t fixedenv(std::static_pointer_cast<LLSettingsSky>(settings), LLSettingsWater::ptr_t());
        if (environment)
        {
            fixedenv.second = environment->getWater();
        }
        setEnvironment(env, fixedenv);
    }
    else if (settings->getSettingType() == "water")
    {
        fixedEnvironment_t fixedenv(LLSettingsSky::ptr_t(), std::static_pointer_cast<LLSettingsWater>(settings));
        if (environment)
        {
            fixedenv.first = environment->getSky();
        }
        setEnvironment(env, fixedenv);
    }
}

void LLEnvironment::setEnvironment(EnvSelection_t env, const LLUUID &assetId)
{
    LLSettingsVOBase::getSettingsAsset(assetId,
        [this, env](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onSetEnvAssetLoaded(env, asset_id, settings, status); });

}

void LLEnvironment::onSetEnvAssetLoaded(EnvSelection_t env, LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (!settings || status)
    {
        LLSD args;
        args["DESC"] = asset_id.asString();
        LLNotificationsUtil::add("FailedToFindSettings", args);
        return;
    }

    setEnvironment(env, settings);
}

void LLEnvironment::clearEnvironment(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection." << LL_ENDL;
        return;
    }

    mEnvironments[env].reset();
    /*TODO: readjust environment*/
}

LLSettingsDay::ptr_t LLEnvironment::getEnvironmentDay(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return environment->getDayCycle();

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::Seconds LLEnvironment::getEnvironmentDayLength(EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection." << LL_ENDL;
        return LLSettingsDay::Seconds(0);
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return environment->getDayLength();

    return LLSettingsDay::Seconds(0);
}

LLSettingsDay::Seconds LLEnvironment::getEnvironmentDayOffset(EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection." << LL_ENDL;
        return LLSettingsDay::Seconds(0);
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);
    if (environment)
        return environment->getDayOffset();

    return LLSettingsDay::Seconds(0);
}


LLEnvironment::fixedEnvironment_t LLEnvironment::getEnvironmentFixed(LLEnvironment::EnvSelection_t env)
{
    if (env == ENV_CURRENT)
    {
        fixedEnvironment_t fixed;
        for (S32 idx = mSelectedEnvironment; idx < ENV_END; ++idx)
        {
            if (fixed.first && fixed.second)
                break;

            if (idx == ENV_EDIT)
                continue;   // skip the edit environment.

            DayInstance::ptr_t environment = getEnvironmentInstance(static_cast<EnvSelection_t>(idx));
            if (environment)
            {
                if (!fixed.first)
                    fixed.first = environment->getSky();
                if (!fixed.second)
                    fixed.second = environment->getWater();
            }
        }

        if (!fixed.first || !fixed.second)
            LL_WARNS("ENVIRONMENT") << "Can not construct complete fixed environment.  Missing Sky and/or Water." << LL_ENDL;

        return fixed;
    }

    if ((env < ENV_EDIT) || (env > ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to retrieve invalid environment selection." << LL_ENDL;
        return fixedEnvironment_t();
    }

    DayInstance::ptr_t environment = getEnvironmentInstance(env);

    if (environment)
        return fixedEnvironment_t(environment->getSky(), environment->getWater());

    return fixedEnvironment_t();
}

LLEnvironment::DayInstance::ptr_t LLEnvironment::getSelectedEnvironmentInstance()
{
    for (S32 idx = mSelectedEnvironment; idx < ENV_DEFAULT; ++idx)
    {
        if (mEnvironments[idx])
            return mEnvironments[idx];
    }

    return mEnvironments[ENV_DEFAULT];
}


void LLEnvironment::updateEnvironment(LLSettingsBase::Seconds transition, bool forced)
{
    DayInstance::ptr_t pinstance = getSelectedEnvironmentInstance();

    if ((mCurrentEnvironment != pinstance) || forced)
    {
        DayInstance::ptr_t trans = std::make_shared<DayTransition>(
            mCurrentEnvironment->getSky(), mCurrentEnvironment->getWater(), pinstance, transition);

        trans->animate();

        mCurrentEnvironment = trans;
    }
}

LLVector3 LLEnvironment::getLightDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, 1);
    }
    return psky->getLightDirection();
}

LLVector4 LLEnvironment::toCFR(const LLVector3 vec) const
{
    LLVector4 vec_cfr(vec.mV[1], vec.mV[0], vec.mV[2], 0.0f);
    return vec_cfr;
}

LLVector4 LLEnvironment::toLightNorm(const LLVector3 vec) const
{
    LLVector4 vec_ogl(vec.mV[1], vec.mV[2], vec.mV[0], 0.0f);
    return vec_ogl;
}

LLVector4 LLEnvironment::getLightDirectionCFR() const
{
    LLVector3 light_direction = getLightDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getClampedLightNorm() const
{
    LLVector3 light_direction = getLightDirection();
    if (light_direction.mV[2] < -0.1f)
    {
        light_direction.mV[2] = -0.1f;
    }
    return toLightNorm(light_direction);
}

LLVector4 LLEnvironment::getRotatedLightNorm() const
{
    LLVector3 light_direction = getLightDirection();
    light_direction *= LLQuaternion(-mLastCamYaw, LLVector3(0.f, 1.f, 0.f));
    return toLightNorm(light_direction);
}

//-------------------------------------------------------------------------
void LLEnvironment::update(const LLViewerCamera * cam)
{
    LL_RECORD_BLOCK_TIME(FTM_ENVIRONMENT_UPDATE);
    //F32Seconds now(LLDate::now().secondsSinceEpoch());
    static LLFrameTimer timer;

    F32Seconds delta(timer.getElapsedTimeAndResetF32());

    mCurrentEnvironment->update(delta);

    // update clouds, sun, and general
    updateCloudScroll();

    // cache this for use in rotating the rotated light vec for shader param updates later...
    mLastCamYaw = cam->getYaw() + SUN_DELTA_YAW;

    stop_glerror();

    // *TODO: potential optimization - this block may only need to be
    // executed some of the time.  For example for water shaders only.
    {
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
    
    if (mCurrentEnvironment->getSky())
    {
        LLVector2 cloud_delta = static_cast<F32>(delta_t)* (mCurrentEnvironment->getSky()->getCloudScrollRate() - LLVector2(10.0, 10.0)) / 100.0;
        mCloudScrollDelta += cloud_delta;
    }

}

void LLEnvironment::updateGLVariablesForSettings(LLGLSLShader *shader, const LLSettingsBase::ptr_t &psetting)
{
    LL_RECORD_BLOCK_TIME(FTM_SHADER_PARAM_UPDATE);

    //_WARNS("RIDER") << "----------------------------------------------------------------" << LL_ENDL;
    LLSettingsBase::parammapping_t params = psetting->getParameterMap();
    for (auto &it: params)
    {
        LLSD value;
        
        bool found_in_settings = psetting->mSettings.has(it.first);
        bool found_in_legacy_settings = !found_in_settings && psetting->mSettings.has(LLSettingsSky::SETTING_LEGACY_HAZE) && psetting->mSettings[LLSettingsSky::SETTING_LEGACY_HAZE].has(it.first);

        if (!found_in_settings && !found_in_legacy_settings)
            continue;

        if (found_in_settings)
        {
            value = psetting->mSettings[it.first];
        }
        else if (found_in_legacy_settings)
        {
            value = psetting->mSettings[LLSettingsSky::SETTING_LEGACY_HAZE][it.first];
        }

        LLSD::Type setting_type = value.type();
        stop_glerror();
        switch (setting_type)
        {
        case LLSD::TypeInteger:
            shader->uniform1i(it.second, value.asInteger());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;
        case LLSD::TypeReal:
            shader->uniform1f(it.second, value.asReal());
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeBoolean:
            shader->uniform1i(it.second, value.asBoolean() ? 1 : 0);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << value << LL_ENDL;
            break;

        case LLSD::TypeArray:
        {
            LLVector4 vect4(value);
            //_WARNS("RIDER") << "pushing '" << (*it).first << "' as " << vect4 << LL_ENDL;
            shader->uniform4fv(it.second, 1, vect4.mV);

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
        updateGLVariablesForSettings(shader, mCurrentEnvironment->getWater());
        updateGLVariablesForSettings(shader, mCurrentEnvironment->getSky());
    }    
}

LLEnvironment::list_name_id_t LLEnvironment::getSkyList() const
{
    list_name_id_t list;

    list.reserve(mSkysByName.size());

    for (auto &it: mSkysByName)
    {
        list.push_back(std::vector<name_id_t>::value_type(it.second->getName(), it.second->getId()));
    }

    return list;
}

LLEnvironment::list_name_id_t LLEnvironment::getWaterList() const
{
    list_name_id_t list;

    list.reserve(mWaterByName.size());

    for (auto &it : mWaterByName)
    {
        list.push_back(std::vector<name_id_t>::value_type(it.second->getName(), it.second->getId()));
    }

    return list;
}

LLEnvironment::list_name_id_t LLEnvironment::getDayCycleList() const
{
    list_name_id_t list;

    list.reserve(mDayCycleByName.size());

    for (auto &it: mDayCycleByName)
    {
        list.push_back(std::vector<name_id_t>::value_type(it.second->getName(), it.second->getId()));
    }

    return list;
}

void LLEnvironment::addSky(const LLSettingsSky::ptr_t &sky)
{
    std::string name = sky->getValue(LLSettingsSky::SETTING_NAME).asString();

    LL_WARNS("RIDER") << "Adding sky as '" << name << "'" << LL_ENDL;

    auto result = mSkysByName.insert(namedSettingMap_t::value_type(name, sky)); // auto should be: std::pair<namedSettingMap_t::iterator, bool> 

    if (!result.second)
        (*(result.first)).second = sky;
    mSkyListChange();
}

void LLEnvironment::removeSky(const std::string &name)
{
    auto it = mSkysByName.find(name);
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

    auto result = mWaterByName.insert(namedSettingMap_t::value_type(name, water));

    if (!result.second)
        (*(result.first)).second = water;
    mWaterListChange();
}


void LLEnvironment::removeWater(const std::string &name)
{
    auto it = mWaterByName.find(name);
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

    auto result = mDayCycleByName.insert(namedSettingMap_t::value_type(name, daycycle));

    if (!result.second)
        (*(result.first)).second = daycycle;
    mDayCycleListChange();
}

//void LLEnvironment::addDayCycle(const LLUUID &id, const LLSettingsSky::ptr_t &sky);

void LLEnvironment::removeDayCycle(const std::string &name)
{
    auto it = mDayCycleByName.find(name);
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
    auto it = mSkysByName.find(name);

    if (it == mSkysByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find sky with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsSky::ptr_t();
    }

    return std::static_pointer_cast<LLSettingsSky>((*it).second);
}

LLSettingsWater::ptr_t LLEnvironment::findWaterByName(std::string name) const
{
    auto it = mWaterByName.find(name);

    if (it == mWaterByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find water with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsWater::ptr_t();
    }

    return std::static_pointer_cast<LLSettingsWater>((*it).second);
}

LLSettingsDay::ptr_t LLEnvironment::findDayCycleByName(std::string name) const
{
    auto it = mDayCycleByName.find(name);

    if (it == mDayCycleByName.end())
    {
        LL_WARNS("ENVIRONMENT") << "Unable to find daycycle with unknown name '" << name << "'" << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    return std::static_pointer_cast<LLSettingsDay>((*it).second);
}


void LLEnvironment::recordEnvironment(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envinfo)
{
    LL_WARNS("ENVIRONMENT") << "Have environment" << LL_ENDL;

    LLSettingsDay::ptr_t pday = LLSettingsVODay::buildFromEnvironmentMessage(envinfo->mDaycycleData);

    LL_WARNS("ENVIRONMENT") << "serverhash=" << envinfo->mDayHash << " viewerhash=" << pday->getHash() << LL_ENDL;

    if (envinfo->mParcelId == INVALID_PARCEL_ID)
    {   // the returned info applies to an entire region.
        LL_WARNS("LAPRAS") << "Setting Region environment" << LL_ENDL;
        setEnvironment(ENV_REGION, pday, envinfo->mDayLength, envinfo->mDayOffset);
        if (parcel_id != INVALID_PARCEL_ID)
        {
            LL_WARNS("LAPRAS") << "Had requested parcel environment #" << parcel_id << " but got region." << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
        }

        mTrackAltitudes = envinfo->mAltitudes;

        LL_WARNS("LAPRAS") << "Altitudes set to {" << mTrackAltitudes[0] << ", "<< mTrackAltitudes[1] << ", " << mTrackAltitudes[2] << ", " << mTrackAltitudes[3] << LL_ENDL;
    }
    else
    {
        LLParcel *parcel = LLViewerParcelMgr::instance().getAgentParcel();

        LL_WARNS("LAPRAS") << "Have parcel environment #" << envinfo->mParcelId << LL_ENDL;
        if (parcel && (parcel->getLocalID() != parcel_id))
        {
            LL_WARNS("ENVIRONMENT") << "Requested parcel #" << parcel_id << " agent is on " << parcel->getLocalID() << LL_ENDL;
            return;
        }

        setEnvironment(ENV_PARCEL, pday, envinfo->mDayLength, envinfo->mDayOffset);
    }
    
    /*TODO: track_altitudes*/
    updateEnvironment();
}

//=========================================================================
void LLEnvironment::requestRegion()
{
    if (!isExtendedEnvironmentEnabled())
    {
        LLEnvironmentRequest::initiate();
        return;
    }

    requestParcel(INVALID_PARCEL_ID);
}

void LLEnvironment::updateRegion(LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset)
{
    if (!isExtendedEnvironmentEnabled())
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
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroRequestEnvironment",
        boost::bind(&LLEnvironment::coroRequestEnvironment, this, parcel_id,
        [this](S32 pid, EnvironmentInfo::ptr_t envinfo) { recordEnvironment(pid, envinfo); }));
}

void LLEnvironment::updateParcel(S32 parcel_id, LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        boost::bind(&LLEnvironment::coroUpdateEnvironment, this, parcel_id, 
        pday, day_length, day_offset, 
        [this](S32 pid, EnvironmentInfo::ptr_t envinfo) { recordEnvironment(pid, envinfo); }));
}

void LLEnvironment::resetParcel(S32 parcel_id)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroResetEnvironment",
        boost::bind(&LLEnvironment::coroResetEnvironment, this, parcel_id, 
        [this](S32 pid, EnvironmentInfo::ptr_t envinfo) { recordEnvironment(pid, envinfo); }));
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
        if (environment.isDefined() && apply)
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

    LLSD result = httpAdapter->putAndSuspend(httpRequest, url, body);
    // results that come back may contain the new settings

    LLSD notify;

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    LL_WARNS("LAPRAS") << "success=" << result["success"] << LL_ENDL;
    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("WindlightCaps") << "Couldn't update Windlight settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
        LLSD environment = result["environment"];
        if (environment.isDefined() && apply)
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

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, url);
    // results that come back may contain the new settings

    LLSD notify; 

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    LL_WARNS("LAPRAS") << "success=" << result["success"] << LL_ENDL;
    if ((!status) || !result["success"].asBoolean())
    {
        LL_WARNS("WindlightCaps") << "Couldn't reset Windlight settings in " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

        notify = LLSD::emptyMap();
        notify["FAIL_REASON"] = result["message"].asString();
    }
    else
    {
        LLSD environment = result["environment"];
        if (environment.isDefined() && apply)
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
    mParcelId(INVALID_PARCEL_ID),
    mRegionId(),
    mDayLength(0),
    mDayOffset(0),
    mDayHash(0),
    mDaycycleData(),
    mAltitudes({ { 0.0, 0.0, 0.0, 0.0 } }),
    mIsDefault(false),
    mIsRegion(false)
{
}

LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extract(LLSD environment)
{
    ptr_t pinfo = std::make_shared<EnvironmentInfo>();

    if (environment.has("parcel_id"))
        pinfo->mParcelId = environment["parcel_id"].asInteger();
    if (environment.has("region_id"))
        pinfo->mRegionId = environment["region_id"].asUUID();
    if (environment.has("day_length"))
        pinfo->mDayLength = LLSettingsDay::Seconds(environment["day_length"].asInteger());
    if (environment.has("day_offset"))
        pinfo->mDayOffset = LLSettingsDay::Seconds(environment["day_offset"].asInteger());
    if (environment.has("day_hash"))
        pinfo->mDayHash = environment["day_hash"].asInteger();
    if (environment.has("day_cycle"))
        pinfo->mDaycycleData = environment["day_cycle"];
    if (environment.has("is_default"))
        pinfo->mIsDefault = environment["is_default"].asBoolean();
    if (environment.has("track_altitudes"))
    {
        LL_WARNS("LAPRAS") << "track_altitudes=" << environment["track_altitudes"] << LL_ENDL;

        /*LAPRAS: TODO: Fix the simulator message.  Shouldn't be 5, just 4*/
        int idx = 1; 
        for (F32 &altitude : pinfo->mAltitudes)
        {
            altitude = environment["track_altitudes"][idx++].asReal();
        }
    }

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

LLSettingsWater::ptr_t LLEnvironment::createWaterFromLegacyPreset(const std::string filename)
{
    std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(filename), true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPresetFile(name, path);
    return water;
}

LLSettingsSky::ptr_t LLEnvironment::createSkyFromLegacyPreset(const std::string filename)
{
    std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(filename), true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPresetFile(name, path);
    return sky;

}

LLSettingsDay::ptr_t LLEnvironment::createDayCycleFromLegacyPreset(const std::string filename)
{
    std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(filename), true));
    std::string path(gDirUtilp->getDirName(filename));

    LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPresetFile(name, path);
    return day;
}


void LLEnvironment::legacyLoadAllPresets()
{
//     std::string dir;
//     std::string file;
// 
//     // System skies
//     {
//         dir = getSysDir("skies");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPreset(name, data);
//                 LLEnvironment::instance().addSky(sky);
//             }
//         }
//     }
// 
//     // User skies
//     {
//         dir = getUserDir("skies");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsSky::ptr_t sky = LLSettingsVOSky::buildFromLegacyPreset(name, data);
//                 LLEnvironment::instance().addSky(sky);
//             }
//         }
//     }
// 
//     // System water
//     {
//         dir = getSysDir("water");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPreset(name, data);
//                 LLEnvironment::instance().addWater(water);
//             }
//         }
//     }
// 
//     // User water
//     {
//         dir = getUserDir("water");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsWater::ptr_t water = LLSettingsVOWater::buildFromLegacyPreset(name, data);
//                 LLEnvironment::instance().addWater(water);
//             }
//         }
//     }
// 
//     // System Days
//     {
//         dir = getSysDir("days");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPreset(name, data);
//                 /*if (day->validate())
//                 {
//                     LL_INFOS() << "Adding Day Cycle " << name << "." << LL_ENDL;
//                     LLEnvironment::instance().addDayCycle(day);
//                 }
//                 else
//                 {
//                     LL_WARNS() << "Day Cycle " << name << " was not valid. Ignoring." << LL_ENDL;
//                 }*/
//                 LL_INFOS() << "Adding Day Cycle " << name << "." << LL_ENDL;
//                 LLEnvironment::instance().addDayCycle(day);
// #ifdef EXPORT_PRESETS
//                 std::string exportfile = LLURI::escape(name) + "(new).xml";
//                 std::string exportpath = gDirUtilp->add(getSysDir("new"), exportfile);
// 
//                 LLSD settings = day->getSettings();
// 
//                 std::ofstream daycyclefile(exportpath);
//                 LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
//                 formatter->format(settings, daycyclefile, LLSDFormatter::OPTIONS_PRETTY);
//                 daycyclefile.close();
// #endif
//             }
//         }
//     }
// 
//     // User Days
//     {
//         dir = getUserDir("days");
//         LLDirIterator dir_iter(dir, "*.xml");
//         while (dir_iter.next(file))
//         {
//             std::string path = gDirUtilp->add(dir, file);
// 
//             LLSD data = legacyLoadPreset(path);
//             if (data)
//             {
//                 std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));
// 
//                 LLSettingsDay::ptr_t day = LLSettingsVODay::buildFromLegacyPreset(name, data);
//                 LLEnvironment::instance().addDayCycle(day);
//             }
//         }
//     }
}

void LLEnvironment::onAgentPositionHasChanged(const LLVector3 &localpos)
{
    S32 trackno = calculateSkyTrackForAltitude(localpos.mV[VZ]);
    if (trackno == mCurrentTrack)
        return;

    LL_WARNS("LAPRAS") << "Wants to switch to track #" << trackno << LL_ENDL;

    mCurrentTrack = trackno;
    for (S32 env = ENV_LOCAL; env < ENV_DEFAULT; ++env)
    {
        if (mEnvironments[env])
            mEnvironments[env]->setSkyTrack(mCurrentTrack);
    }
}

S32 LLEnvironment::calculateSkyTrackForAltitude(F64 altitude)
{
    auto it = std::find_if_not(mTrackAltitudes.begin(), mTrackAltitudes.end(), [altitude](F32 test) { return altitude > test; });
    
    if (it == mTrackAltitudes.begin())
        return 1;
    else if (it == mTrackAltitudes.end())
        return 4;

    return std::min(static_cast<S32>(std::distance(mTrackAltitudes.begin(), it)), 4);
}


//=========================================================================
LLEnvironment::DayInstance::DayInstance() :
    mDayCycle(),
    mSky(),
    mWater(),
    mDayLength(LLSettingsDay::DEFAULT_DAYLENGTH),
    mDayOffset(LLSettingsDay::DEFAULT_DAYOFFSET),
    mBlenderSky(),
    mBlenderWater(),
    mInitialized(false),
    mType(TYPE_INVALID),
    mSkyTrack(1)
{ }

void LLEnvironment::DayInstance::update(LLSettingsBase::Seconds delta)
{
    if (!mInitialized)
        initialize();

    if (mBlenderSky)
        mBlenderSky->update(delta.value());
    if (mBlenderWater)
        mBlenderWater->update(delta.value());

//     if (mSky)
//         mSky->update();
//     if (mWater)
//         mWater->update();
}

void LLEnvironment::DayInstance::setDay(const LLSettingsDay::ptr_t &pday, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset)
{
    if (mType == TYPE_FIXED)
        LL_WARNS("ENVIRONMENT") << "Fixed day instance changed to Cycled" << LL_ENDL;
    mType = TYPE_CYCLED;
    mInitialized = false;

    mDayCycle = pday;
    mDayLength = daylength;
    mDayOffset = dayoffset;

    mBlenderSky.reset();
    mBlenderWater.reset();

    mSky = LLSettingsVOSky::buildDefaultSky();
    mWater = LLSettingsVOWater::buildDefaultWater();

    animate();
}


void LLEnvironment::DayInstance::setSky(const LLSettingsSky::ptr_t &psky)
{
    if (mType == TYPE_CYCLED)
        LL_WARNS("ENVIRONMENT") << "Cycled day instance changed to FIXED" << LL_ENDL;
    mType = TYPE_FIXED;
    mInitialized = false;

    mSky = psky;
    mSky->update();
    mBlenderSky.reset();

    if (gAtmosphere)
    {
        AtmosphericModelSettings settings;
        LLEnvironment::getAtmosphericModelSettings(settings, psky);
        gAtmosphere->configureAtmosphericModel(settings);
    }
}

void LLEnvironment::DayInstance::setWater(const LLSettingsWater::ptr_t &pwater)
{
    if (mType == TYPE_CYCLED)
        LL_WARNS("ENVIRONMENT") << "Cycled day instance changed to FIXED" << LL_ENDL;
    mType = TYPE_FIXED;
    mInitialized = false;

    mWater = pwater;
    mWater->update();
    mBlenderWater.reset();
}

void LLEnvironment::DayInstance::initialize()
{
    mInitialized = true;

    if (!mWater)
        mWater = LLSettingsVOWater::buildDefaultWater();
    if (!mSky)
        mSky = LLSettingsVOSky::buildDefaultSky();
}

void LLEnvironment::DayInstance::clear()
{
    mType = TYPE_INVALID;
    mDayCycle.reset();
    mSky.reset();
    mWater.reset();
    mDayLength = LLSettingsDay::DEFAULT_DAYLENGTH;
    mDayOffset = LLSettingsDay::DEFAULT_DAYOFFSET;
    mBlenderSky.reset();
    mBlenderWater.reset();
    mSkyTrack = 1;
}

void LLEnvironment::DayInstance::setSkyTrack(S32 trackno)
{
    mSkyTrack = trackno;
    if (mBlenderSky)
    {
        mBlenderSky->switchTrack(trackno, 0.0);
    }
}


void LLEnvironment::DayInstance::setBlenders(const LLSettingsBlender::ptr_t &skyblend, const LLSettingsBlender::ptr_t &waterblend)
{
    mBlenderSky = skyblend;
    mBlenderWater = waterblend;
}

F64 LLEnvironment::DayInstance::secondsToKeyframe(LLSettingsDay::Seconds seconds)
{
    F64 frame = static_cast<F64>(seconds.value() % mDayLength.value()) / static_cast<F64>(mDayLength.value());

    return llclamp(frame, 0.0, 1.0);
}

void LLEnvironment::DayInstance::animate()
{
    LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());

    now += mDayOffset;

    if (!mDayCycle)
        return;

    LLSettingsDay::CycleTrack_t &wtrack = mDayCycle->getCycleTrack(0);

    if (wtrack.empty())
    {
        mWater.reset();
        mBlenderWater.reset();
    }
//     else if (wtrack.size() == 1)
//     {
//         mWater = std::static_pointer_cast<LLSettingsWater>((*(wtrack.begin())).second);
//         mBlenderWater.reset();
//     }
    else
    {
        mWater = LLSettingsVOWater::buildDefaultWater();
        mBlenderWater = std::make_shared<LLTrackBlenderLoopingTime>(mWater, mDayCycle, 0, mDayLength, mDayOffset);
    }

    // sky, initalize to track 1
    LLSettingsDay::CycleTrack_t &track = mDayCycle->getCycleTrack(1);

    if (track.empty())
    {
        mSky.reset();
        mBlenderSky.reset();
    }
//     else if (track.size() == 1)
//     {
//         mSky = std::static_pointer_cast<LLSettingsSky>((*(track.begin())).second);
//         mBlenderSky.reset();
//     }
    else
    {
        mSky = LLSettingsVOSky::buildDefaultSky();
        mBlenderSky = std::make_shared<LLTrackBlenderLoopingTime>(mSky, mDayCycle, 1, mDayLength, mDayOffset);
        mBlenderSky->switchTrack(mSkyTrack, 0.0);
    }
}

//-------------------------------------------------------------------------
LLEnvironment::DayTransition::DayTransition(const LLSettingsSky::ptr_t &skystart,
    const LLSettingsWater::ptr_t &waterstart, LLEnvironment::DayInstance::ptr_t &end, LLSettingsDay::Seconds time) :
    DayInstance(),
    mStartSky(skystart),
    mStartWater(waterstart),
    mNextInstance(end),
    mTransitionTime(time)
{
    
}

void LLEnvironment::DayTransition::update(LLSettingsBase::Seconds delta)
{
    mNextInstance->update(delta);
    DayInstance::update(delta);
}

void LLEnvironment::DayTransition::animate() 
{
    mNextInstance->animate();

    mWater = mStartWater->buildClone();
    mBlenderWater = std::make_shared<LLSettingsBlenderTimeDelta>(mWater, mStartWater, mNextInstance->getWater(), mTransitionTime);
    mBlenderWater->setOnFinished(
        [this](LLSettingsBlender::ptr_t blender) { 
            mBlenderWater.reset();

            if (!mBlenderSky && !mBlenderWater)
                LLEnvironment::instance().mCurrentEnvironment = mNextInstance;
    });

    mSky = mStartSky->buildClone();
    mBlenderSky = std::make_shared<LLSettingsBlenderTimeDelta>(mSky, mStartSky, mNextInstance->getSky(), mTransitionTime);
    mBlenderSky->setOnFinished(
        [this](LLSettingsBlender::ptr_t blender) {
        mBlenderSky.reset();

        if (!mBlenderSky && !mBlenderWater)
            LLEnvironment::instance().mCurrentEnvironment = mNextInstance;
    });
}

//=========================================================================
LLTrackBlenderLoopingManual::LLTrackBlenderLoopingManual(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno) :
        LLSettingsBlender(target, LLSettingsBase::ptr_t(), LLSettingsBase::ptr_t()),
        mDay(day),
        mTrackNo(trackno),
        mPosition(0.0)
{
    LLSettingsDay::TrackBound_t initial = getBoundingEntries(mPosition);

    if (initial.first != mEndMarker)
    {   // No frames in track
        mInitial = (*initial.first).second;
        mFinal = (*initial.second).second;

        LLSD initSettings = mInitial->getSettings();
        mTarget->replaceSettings(initSettings);
    }
}

LLSettingsBase::BlendFactor LLTrackBlenderLoopingManual::setPosition(const LLSettingsBase::TrackPosition& position)
{
    mPosition = llclamp(position, 0.0, 1.0);

    LLSettingsDay::TrackBound_t bounds = getBoundingEntries(mPosition);

    if (bounds.first == mEndMarker)
    {   // No frames in track.
        return 0.0;
    }

    mInitial = (*bounds.first).second;
    mFinal = (*bounds.second).second;

    F64 spanLength = getSpanLength(bounds);

    F64 spanPos = ((mPosition < (*bounds.first).first) ? (mPosition + 1.0) : mPosition) - (*bounds.first).first;

    F64 blendf = fmod(spanPos, spanLength) / spanLength;
    return LLSettingsBlender::setPosition(blendf);
}

void LLTrackBlenderLoopingManual::switchTrack(S32 trackno, const LLSettingsBase::TrackPosition& position)
{
    mTrackNo = trackno;

    LLSettingsBase::TrackPosition useposition = (position < 0.0) ? mPosition : position;

    setPosition(useposition);
}

LLSettingsDay::TrackBound_t LLTrackBlenderLoopingManual::getBoundingEntries(F64 position)
{
    LLSettingsDay::CycleTrack_t &wtrack = mDay->getCycleTrack(mTrackNo);

    mEndMarker = wtrack.end();

    LLSettingsDay::TrackBound_t bounds = get_bounding_entries(wtrack, position);
    return bounds;
}

F64 LLTrackBlenderLoopingManual::getSpanLength(const LLSettingsDay::TrackBound_t &bounds) const
{
    return get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
}
