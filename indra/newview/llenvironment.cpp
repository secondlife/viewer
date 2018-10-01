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
#include "llagent.h"
#include "roles_constants.h"
#include "llestateinfomodel.h"

//=========================================================================
namespace
{
    const std::string KEY_ENVIRONMENT("environment");
    const std::string KEY_DAYASSET("day_asset");
    const std::string KEY_DAYCYCLE("day_cycle");
    const std::string KEY_DAYHASH("day_hash");
    const std::string KEY_DAYLENGTH("day_length");
    const std::string KEY_DAYOFFSET("day_offset");
    const std::string KEY_ISDEFAULT("is_default");
    const std::string KEY_PARCELID("parcel_id");
    const std::string KEY_REGIONID("region_id");
    const std::string KEY_TRACKALTS("track_altitudes");

    //---------------------------------------------------------------------
    LLTrace::BlockTimerStatHandle   FTM_ENVIRONMENT_UPDATE("Update Environment Tick");
    LLTrace::BlockTimerStatHandle   FTM_SHADER_PARAM_UPDATE("Update Shader Parameters");

    LLSettingsBase::Seconds         DEFAULT_UPDATE_THRESHOLD(10.0);
    const LLSettingsBase::Seconds   MINIMUM_SPANLENGTH(0.01f);

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

        return 1.0f;
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

    // Find normalized track position of given time along full length of cycle
    inline LLSettingsBase::TrackPosition convert_time_to_position(const LLSettingsBase::Seconds& time, const LLSettingsBase::Seconds& len)
    {
        LLSettingsBase::TrackPosition position = LLSettingsBase::TrackPosition(fmod((F64)time, (F64)len) / (F64)len);
        return llclamp(position, 0.0f, 1.0f);
    }

    //---------------------------------------------------------------------
    class LLTrackBlenderLoopingTime : public LLSettingsBlenderTimeDelta
    {
    public:
        LLTrackBlenderLoopingTime(const LLSettingsBase::ptr_t &target, const LLSettingsDay::ptr_t &day, S32 trackno, 
                LLSettingsBase::Seconds cyclelength, LLSettingsBase::Seconds cycleoffset, LLSettingsBase::Seconds updateThreshold) :
            LLSettingsBlenderTimeDelta(target, LLSettingsBase::ptr_t(), LLSettingsBase::ptr_t(), LLSettingsBase::Seconds(1.0)),
            mDay(day),
            mTrackNo(0),
            mCycleLength(cyclelength),
            mCycleOffset(cycleoffset)
        {
            setTimeDeltaThreshold(updateThreshold);
            // must happen prior to getBoundingEntries call...
            mTrackNo = selectTrackNumber(trackno);

            LLSettingsDay::TrackBound_t initial = getBoundingEntries(getAdjustedNow());

            mInitial = (*initial.first).second;
            mFinal = (*initial.second).second;
            mBlendSpan = getSpanTime(initial);

            setOnFinished([this](const LLSettingsBlender::ptr_t &){ onFinishedSpan(); });
        }

        void switchTrack(S32 trackno, const LLSettingsBase::TrackPosition&) override
        {
            S32 use_trackno = selectTrackNumber(trackno);

            if (use_trackno == mTrackNo)
            {   // results in no change
                return;
            }

            LLSettingsBase::ptr_t pstartsetting = mTarget->buildDerivedClone();
            mTrackNo = use_trackno;

            LLSettingsBase::Seconds now = getAdjustedNow() + LLEnvironment::TRANSITION_ALTITUDE;
            LLSettingsDay::TrackBound_t bounds = getBoundingEntries(now);

            LLSettingsBase::ptr_t pendsetting  = (*bounds.first).second->buildDerivedClone();
            LLSettingsBase::TrackPosition targetpos  = convert_time_to_position(now, mCycleLength) - (*bounds.first).first;
            LLSettingsBase::TrackPosition targetspan = get_wrapping_distance((*bounds.first).first, (*bounds.second).first);

            LLSettingsBase::BlendFactor blendf = calculateBlend(targetpos, targetspan);
            pendsetting->blend((*bounds.second).second, blendf);

            setIgnoreTimeDeltaThreshold(true); // for the next span ignore the time delta threshold.
            reset(pstartsetting, pendsetting, LLEnvironment::TRANSITION_ALTITUDE);
        }

    protected:
        S32 selectTrackNumber(S32 trackno)
        {
            if (trackno == 0)
            {   // We are dealing with the water track.  There is only ever one.
                return trackno;
            }

            for (S32 test = trackno; test != 0; --test)
            {   // Find the track below the requested one with data.
                LLSettingsDay::CycleTrack_t &track = mDay->getCycleTrack(test);

                if (!track.empty())
                    return test;
            }

            return 1;
        }

        LLSettingsDay::TrackBound_t getBoundingEntries(LLSettingsBase::Seconds time)
        {
            LLSettingsDay::CycleTrack_t &wtrack = mDay->getCycleTrack(mTrackNo);
            LLSettingsBase::TrackPosition position = convert_time_to_position(time, mCycleLength);
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
            LLSettingsBase::Seconds span = mCycleLength * get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
            if (span < MINIMUM_SPANLENGTH) // for very short spans set a minimum length.
                span = MINIMUM_SPANLENGTH;
            return span;
        }

    private:
        LLSettingsDay::ptr_t        mDay;
        S32                         mTrackNo;
        LLSettingsBase::Seconds     mCycleLength;
        LLSettingsBase::Seconds     mCycleOffset;

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

const LLUUID LLEnvironment::KNOWN_SKY_SUNRISE("4204c687-f9e4-4893-8c1b-46761c0d2021");
const LLUUID LLEnvironment::KNOWN_SKY_MIDDAY("07589e0e-8e2e-4864-8e58-07b516efd9c3");
const LLUUID LLEnvironment::KNOWN_SKY_SUNSET("8113ba47-3223-46ba-bae6-12c875091b32");
const LLUUID LLEnvironment::KNOWN_SKY_MIDNIGHT("90187088-d7f3-4656-8c27-8ba0e19e21e9");

const S32 LLEnvironment::NO_TRACK(-1);

const F32 LLEnvironment::SUN_DELTA_YAW(F_PI);   // 180deg 

//-------------------------------------------------------------------------
LLEnvironment::LLEnvironment():
    mCloudScrollDelta(),
    mCloudScrollPaused(false),
    mSelectedSky(),
    mSelectedWater(),
    mSelectedDay(),
    mSelectedEnvironment(LLEnvironment::ENV_LOCAL),
    mCurrentTrack(1)
{
}

void LLEnvironment::initSingleton()
{
    LLSettingsSky::ptr_t p_default_sky = LLSettingsVOSky::buildDefaultSky();
    LLSettingsWater::ptr_t p_default_water = LLSettingsVOWater::buildDefaultWater();

    mCurrentEnvironment = std::make_shared<DayInstance>();
    mCurrentEnvironment->setSky(p_default_sky);
    mCurrentEnvironment->setWater(p_default_water);

    mEnvironments[ENV_DEFAULT] = mCurrentEnvironment;

    requestRegion();

    gAgent.addParcelChangedCallback([this]() { onParcelChange(); });

    //TODO: This frequently results in one more request than we need.  It isn't breaking, but should be nicer.
    LLRegionInfoModel::instance().setUpdateCallback([this]() { requestRegion(); });
    gAgent.addRegionChangedCallback([this]() { requestRegion(); });

    gAgent.whenPositionChanged([this](const LLVector3 &localpos, const LLVector3d &) { onAgentPositionHasChanged(localpos); });
}

LLEnvironment::~LLEnvironment()
{
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
    settingsOut.m_mieAnisotropy = psky->getMieAnisotropy();

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

bool LLEnvironment::canAgentUpdateParcelEnvironment() const
{
    LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());

    return canAgentUpdateParcelEnvironment(parcel);
}


bool LLEnvironment::canAgentUpdateParcelEnvironment(LLParcel *parcel) const
{
    if (!parcel)
        return false;

    if (!LLEnvironment::instance().isExtendedEnvironmentEnabled())
        return false;

    if (gAgent.isGodlike())
        return true;

    if (!parcel->getRegionAllowEnvironmentOverride())
        return false;

    return LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_OPTIONS);
}

bool LLEnvironment::canAgentUpdateRegionEnvironment() const
{
    if (gAgent.isGodlike())
        return true;

    return gAgent.getRegion()->canManageEstate();
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
    if (!mCurrentEnvironment || !mCurrentEnvironment->getSky())
        return false;
    return mCurrentEnvironment->getSky()->getIsSunUp();
}

bool LLEnvironment::getIsMoonUp() const
{
    if (!mCurrentEnvironment || !mCurrentEnvironment->getSky())
        return false;
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

    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env);
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
    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env);

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

    if (settings->getSettingsType() == "daycycle")
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
    else if (settings->getSettingsType() == "sky")
    {
        fixedEnvironment_t fixedenv(std::static_pointer_cast<LLSettingsSky>(settings), LLSettingsWater::ptr_t());
        if (environment)
        {
            fixedenv.second = environment->getWater();
        }
        setEnvironment(env, fixedenv);
    }
    else if (settings->getSettingsType() == "water")
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
    setEnvironment(env, assetId, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
}


void LLEnvironment::setEnvironment(EnvSelection_t env, const LLUUID &assetId, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset)
{
    LLSettingsVOBase::getSettingsAsset(assetId,
        [this, env, daylength, dayoffset](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onSetEnvAssetLoaded(env, asset_id, settings, daylength, dayoffset, status); });
}

void LLEnvironment::onSetEnvAssetLoaded(EnvSelection_t env, LLUUID asset_id, LLSettingsBase::ptr_t settings, LLSettingsDay::Seconds daylength, LLSettingsDay::Seconds dayoffset, S32 status)
{
    if (!settings || status)
    {
        LLSD args;
        args["DESC"] = asset_id.asString();
        LLNotificationsUtil::add("FailedToFindSettings", args);
        return;
    }

    setEnvironment(env, settings);
    updateEnvironment();
}

void LLEnvironment::clearEnvironment(LLEnvironment::EnvSelection_t env)
{
    if ((env < ENV_EDIT) || (env >= ENV_DEFAULT))
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to change invalid environment selection." << LL_ENDL;
        return;
    }

    mEnvironments[env].reset();

    if (!mSignalEnvChanged.empty())
        mSignalEnvChanged(env);

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

LLVector3 LLEnvironment::getLightDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, 1);
    }
    return psky->getLightDirection();
}

LLVector3 LLEnvironment::getSunDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, 1);
    }
    return psky->getSunDirection();
}

LLVector3 LLEnvironment::getMoonDirection() const
{
    LLSettingsSky::ptr_t psky = mCurrentEnvironment->getSky();
    if (!psky)
    {
        return LLVector3(0, 0, -1);
    }
    return psky->getMoonDirection();
}

LLVector4 LLEnvironment::getLightDirectionCFR() const
{
    LLVector3 light_direction = getLightDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getSunDirectionCFR() const
{
    LLVector3 light_direction = getSunDirection();
    LLVector4 light_direction_cfr = toCFR(light_direction);
    return light_direction_cfr;
}

LLVector4 LLEnvironment::getMoonDirectionCFR() const
{
    LLVector3 light_direction = getMoonDirection();
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

LLVector4 LLEnvironment::getClampedSunNorm() const
{
    LLVector3 light_direction = getSunDirection();
    if (light_direction.mV[2] < -0.1f)
    {
        light_direction.mV[2] = -0.1f;
    }
    return toLightNorm(light_direction);
}

LLVector4 LLEnvironment::getClampedMoonNorm() const
{
    LLVector3 light_direction = getMoonDirection();
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

    mCurrentEnvironment->applyTimeDelta(delta);

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
    
    if (mCurrentEnvironment->getSky() && !mCloudScrollPaused)
    {
        LLVector2 cloud_delta = static_cast<F32>(delta_t)* (mCurrentEnvironment->getSky()->getCloudScrollRate()) / 100.0;
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

void LLEnvironment::recordEnvironment(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envinfo)
{
    if (envinfo->mParcelId == INVALID_PARCEL_ID)
    {
        // the returned info applies to an entire region.
        if (!envinfo->mDayCycle)
        {
            clearEnvironment(ENV_PARCEL);
            setEnvironment(ENV_REGION, LLSettingsDay::GetDefaultAssetId(), LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
            updateEnvironment();
        }
        else if (envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_WATER)
                 || envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_GROUND_LEVEL))
        {
            LL_WARNS("LAPRAS") << "Invalid day cycle for region" << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
            setEnvironment(ENV_REGION, LLSettingsDay::GetDefaultAssetId(), LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
            updateEnvironment();
        }
        else
        {
            LL_INFOS("LAPRAS") << "Setting Region environment" << LL_ENDL;
            setEnvironment(ENV_REGION, envinfo->mDayCycle, envinfo->mDayLength, envinfo->mDayOffset);
            mTrackAltitudes = envinfo->mAltitudes;
        }

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

        if (!envinfo->mDayCycle)
        {
            LL_WARNS("LAPRAS") << "Clearing environment on parcel #" << parcel_id << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
        }
        else if (envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_WATER)
                 || envinfo->mDayCycle->isTrackEmpty(LLSettingsDay::TRACK_GROUND_LEVEL))
        {
            LL_WARNS("LAPRAS") << "Invalid day cycle for parcel #" << parcel_id << LL_ENDL;
            clearEnvironment(ENV_PARCEL);
        }
        else
        {
            setEnvironment(ENV_PARCEL, envinfo->mDayCycle, envinfo->mDayLength, envinfo->mDayOffset);
        }
    }
    
    updateEnvironment();
}

//=========================================================================
void LLEnvironment::requestRegion(environment_apply_fn cb)
{
    requestParcel(INVALID_PARCEL_ID, cb);
}

void LLEnvironment::updateRegion(const LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLUUID &asset_id, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    if (!isExtendedEnvironmentEnabled())
    {
        LL_WARNS("ENVIRONMENT") << "attempt to apply asset id to region not supporting it." << LL_ENDL;
        LLNotificationsUtil::add("NoEnvironmentSettings");
        return;
    }

    updateParcel(INVALID_PARCEL_ID, asset_id, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, psky, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateRegion(const LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    updateParcel(INVALID_PARCEL_ID, pwater, day_length, day_offset, altitudes, cb);
}


void LLEnvironment::resetRegion(environment_apply_fn cb)
{
    resetParcel(INVALID_PARCEL_ID, cb);
}

void LLEnvironment::requestParcel(S32 parcel_id, environment_apply_fn cb)
{
    if (!isExtendedEnvironmentEnabled())
    {   /*TODO: When EEP is live on the entire grid, this can go away. */
        if (parcel_id == INVALID_PARCEL_ID)
        {
            if (!cb)
            {
                cb = [this](S32 pid, EnvironmentInfo::ptr_t envinfo)
                {
                    recordEnvironment(pid, envinfo);
                };
            }

            LLEnvironmentRequest::initiate(cb);
        }
        else if (cb)
            cb(parcel_id, EnvironmentInfo::ptr_t());
        return;
    }

    if (!cb)
    {
        cb = [this](S32 pid, EnvironmentInfo::ptr_t envinfo) { recordEnvironment(pid, envinfo); };
    }

    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroRequestEnvironment",
        [this, parcel_id, cb]() { coroRequestEnvironment(parcel_id, cb); });
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLUUID &asset_id, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        [this, parcel_id, asset_id, day_length, day_offset, altitudes, cb]() { coroUpdateEnvironment(parcel_id, NO_TRACK,
            LLSettingsDay::ptr_t(), asset_id, day_length, day_offset, altitudes, cb); });
}

void LLEnvironment::onUpdateParcelAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, S32 parcel_id, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes)
{
    if (status)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to get settings asset with id " << asset_id << "!" << LL_ENDL;
        LLNotificationsUtil::add("FailedToLoadSettingsApply");
        return;
    }

    LLSettingsDay::ptr_t pday;

    if (settings->getSettingsType() == "daycycle")
        pday = std::static_pointer_cast<LLSettingsDay>(settings);
    else
    {
        pday = createDayCycleFromEnvironment( (parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, settings);
    }

    if (!pday)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to construct day around " << asset_id << "!" << LL_ENDL;
        LLNotificationsUtil::add("FailedToBuildSettingsDay");
        return;
    }

    updateParcel(parcel_id, pday, day_length, day_offset, altitudes);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsSky::ptr_t &psky, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    LLSettingsDay::ptr_t pday = createDayCycleFromEnvironment((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, psky);
    updateParcel(parcel_id, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsWater::ptr_t &pwater, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    LLSettingsDay::ptr_t pday = createDayCycleFromEnvironment((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL, pwater);
    updateParcel(parcel_id, pday, day_length, day_offset, altitudes, cb);
}

void LLEnvironment::updateParcel(S32 parcel_id, const LLSettingsDay::ptr_t &pday, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, environment_apply_fn cb)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroUpdateEnvironment",
        [this, parcel_id, pday, day_length, day_offset, altitudes, cb]() { coroUpdateEnvironment(parcel_id, NO_TRACK,
            pday, LLUUID::null, day_length, day_offset, altitudes, cb); });
}



void LLEnvironment::resetParcel(S32 parcel_id, environment_apply_fn cb)
{
    std::string coroname =
        LLCoros::instance().launch("LLEnvironment::coroResetEnvironment",
        [this, parcel_id, cb]() { coroResetEnvironment(parcel_id, NO_TRACK, cb); });
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

//     LLSD notify;

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("WindlightCaps") << "Couldn't retrieve environment settings for " << ((parcel_id == INVALID_PARCEL_ID) ? ("region!") : ("parcel!")) << LL_ENDL;

//         std::stringstream msg;
//         msg << status.toString() << " (Code " << status.toTerseString() << ")";
//         notify = LLSD::emptyMap();
//         notify["FAIL_REASON"] = msg.str();

    }
    else
    {
        LLSD environment = result[KEY_ENVIRONMENT];
        if (environment.isDefined() && apply)
        {
            EnvironmentInfo::ptr_t envinfo = LLEnvironment::EnvironmentInfo::extract(environment);
            apply(parcel_id, envinfo);
        }
    }

//     if (!notify.isUndefined())
//     {
//         LLNotificationsUtil::add("WLRegionApplyFail", notify);
//         //LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
//     }
}

void LLEnvironment::coroUpdateEnvironment(S32 parcel_id, S32 track_no, LLSettingsDay::ptr_t pday, 
        LLUUID settings_asset, S32 day_length, S32 day_offset, LLEnvironment::altitudes_vect_t altitudes, LLEnvironment::environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

//     if (day_length < 1)
//     {
//         day_length = getEnvironmentDayLength((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL).value();
//         if ((day_length < 1) && (parcel_id != INVALID_PARCEL_ID))
//             day_length = getEnvironmentDayLength(ENV_REGION).value();
//     }
// 
//     if (day_offset < 1)
//     {
//         day_offset = getEnvironmentDayOffset((parcel_id == INVALID_PARCEL_ID) ? ENV_REGION : ENV_PARCEL).value();
//         if ((day_offset < 1) && (parcel_id != INVALID_PARCEL_ID))
//             day_offset = getEnvironmentDayOffset(ENV_REGION).value();
//     }

    LLSD body(LLSD::emptyMap());
    body[KEY_ENVIRONMENT] = LLSD::emptyMap();

    if (track_no == NO_TRACK)
    {   // day length and offset are only applicable if we are addressing the entire day cycle.
        if (day_length > 0)
            body[KEY_ENVIRONMENT][KEY_DAYLENGTH] = day_length;
        if (day_offset > 0)
            body[KEY_ENVIRONMENT][KEY_DAYOFFSET] = day_offset;

        if ((parcel_id == INVALID_PARCEL_ID) && (altitudes.size() == 3))
        {   // only test for altitude changes if we are changing the region.
            body[KEY_ENVIRONMENT][KEY_TRACKALTS] = LLSD::emptyArray();
            for (S32 i = 0; i < 3; ++i)
            {
                body[KEY_ENVIRONMENT][KEY_TRACKALTS][i] = altitudes[i];
            }
        }
    }

    if (pday)
        body[KEY_ENVIRONMENT][KEY_DAYCYCLE] = pday->getSettings();
    else if (!settings_asset.isNull())
        body[KEY_ENVIRONMENT][KEY_DAYASSET] = settings_asset;


    LL_WARNS("LAPRAS") << "Body = " << body << LL_ENDL;

    if ((parcel_id != INVALID_PARCEL_ID) || (track_no != NO_TRACK))
    {
        std::stringstream query;
        query << "?";

        if (parcel_id != INVALID_PARCEL_ID)
        {
            query << "parcelid=" << parcel_id;

            if (track_no != NO_TRACK)
                query << "&";
        }
        if (track_no != NO_TRACK)
        { 
            query << "trackno=" << track_no;
        }
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
        LLSD environment = result[KEY_ENVIRONMENT];
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

void LLEnvironment::coroResetEnvironment(S32 parcel_id, S32 track_no, environment_apply_fn apply)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ResetEnvironment", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
        return;

    if ((parcel_id != INVALID_PARCEL_ID) || (track_no != NO_TRACK))
    {
        std::stringstream query;
        query << "?";

        if (parcel_id != INVALID_PARCEL_ID)
        {
            query << "parcelid=" << parcel_id;

            if (track_no != NO_TRACK)
                query << "&";
        }
        if (track_no != NO_TRACK)
        {
            query << "trackno=" << track_no;
        }
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
       LLSD environment = result[KEY_ENVIRONMENT];
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

LLEnvironment::EnvironmentInfo::EnvironmentInfo():
    mParcelId(INVALID_PARCEL_ID),
    mRegionId(),
    mDayLength(0),
    mDayOffset(0),
    mDayHash(0),
    mDayCycle(),
    mAltitudes({ { 0.0, 0.0, 0.0, 0.0 } }),
    mIsDefault(false),
    mIsLegacy(false)
{
}

LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extract(LLSD environment)
{
    ptr_t pinfo = std::make_shared<EnvironmentInfo>();

    pinfo->mIsDefault = environment.has(KEY_ISDEFAULT) ? environment[KEY_ISDEFAULT].asBoolean() : true;
    pinfo->mParcelId = environment.has(KEY_PARCELID) ? environment[KEY_PARCELID].asInteger() : INVALID_PARCEL_ID;
    pinfo->mRegionId = environment.has(KEY_REGIONID) ? environment[KEY_REGIONID].asUUID() : LLUUID::null;
    pinfo->mIsLegacy = false;

    if (environment.has(KEY_TRACKALTS))
    {
        for (int idx = 0; idx < 3; idx++)
        {
            pinfo->mAltitudes[idx+1] = environment[KEY_TRACKALTS][idx].asReal();
        }
        pinfo->mAltitudes[0] = 0;
    }

    if (environment.has(KEY_DAYCYCLE))
    {
        pinfo->mDayCycle = LLSettingsVODay::buildFromEnvironmentMessage(environment[KEY_DAYCYCLE]);
        pinfo->mDayLength = LLSettingsDay::Seconds(environment.has(KEY_DAYLENGTH) ? environment[KEY_DAYLENGTH].asInteger() : -1);
        pinfo->mDayOffset = LLSettingsDay::Seconds(environment.has(KEY_DAYOFFSET) ? environment[KEY_DAYOFFSET].asInteger() : -1);
        pinfo->mDayHash = environment.has(KEY_DAYHASH) ? environment[KEY_DAYHASH].asInteger() : 0;
    }
    else
    {
        pinfo->mDayLength = LLEnvironment::instance().getEnvironmentDayLength(ENV_REGION);
        pinfo->mDayOffset = LLEnvironment::instance().getEnvironmentDayOffset(ENV_REGION);
    }

    if (environment.has(KEY_DAYASSET))
    {
        pinfo->mAssetId = environment[KEY_DAYASSET].asUUID();
    }

    return pinfo;
}


LLEnvironment::EnvironmentInfo::ptr_t LLEnvironment::EnvironmentInfo::extractLegacy(LLSD legacy)
{
    if (!legacy.isArray() || !legacy[0].has("regionID"))
    {
        LL_WARNS("ENVIRONMENT") << "Invalid legacy settings for environment: " << legacy << LL_ENDL;
        return ptr_t();
    }

    ptr_t pinfo = std::make_shared<EnvironmentInfo>();

    pinfo->mIsDefault = false;
    pinfo->mParcelId = INVALID_PARCEL_ID;
    pinfo->mRegionId = legacy[0]["regionID"].asUUID();
    pinfo->mIsLegacy = true;

    pinfo->mDayLength = LLSettingsDay::DEFAULT_DAYLENGTH;
    pinfo->mDayOffset = LLSettingsDay::DEFAULT_DAYOFFSET;
    pinfo->mDayCycle = LLSettingsVODay::buildFromLegacyMessage(pinfo->mRegionId, legacy[1], legacy[2], legacy[3]);
    if (pinfo->mDayCycle)
        pinfo->mDayHash = pinfo->mDayCycle->getHash();

    pinfo->mAltitudes[0] = 0;
    pinfo->mAltitudes[2] = 10001;
    pinfo->mAltitudes[3] = 10002;
    pinfo->mAltitudes[4] = 10003;

    return pinfo;
}

//=========================================================================
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

LLSettingsDay::ptr_t LLEnvironment::createDayCycleFromEnvironment(EnvSelection_t env, LLSettingsBase::ptr_t settings)
{
    std::string type(settings->getSettingsType());

    if (type == "daycycle")
        return std::static_pointer_cast<LLSettingsDay>(settings);

    if ((env != ENV_PARCEL) && (env != ENV_REGION))
    {
        LL_WARNS("ENVIRONMENT") << "May only create from parcel or region environment." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    LLSettingsDay::ptr_t day = this->getEnvironmentDay(env);
    if (!day && (env == ENV_PARCEL))
    {
        day = this->getEnvironmentDay(ENV_REGION);
    }

    if (!day)
    {
        LL_WARNS("ENVIRONMENT") << "Could not retrieve existing day settings." << LL_ENDL;
        return LLSettingsDay::ptr_t();
    }

    day = day->buildClone();

    if (type == "sky")
    {
        for (S32 idx = 1; idx < LLSettingsDay::TRACK_MAX; ++idx)
            day->clearCycleTrack(idx);
        day->setSettingsAtKeyframe(settings, 0.0f, 1);
    }
    else if (type == "water")
    {
        day->clearCycleTrack(LLSettingsDay::TRACK_WATER);
        day->setSettingsAtKeyframe(settings, 0.0f, LLSettingsDay::TRACK_WATER);
    }

    return day;
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

void LLEnvironment::DayInstance::applyTimeDelta(const LLSettingsBase::Seconds& delta)
{
    if (!mInitialized)
        initialize();

    if (mBlenderSky)
        mBlenderSky->applyTimeDelta(delta);
    if (mBlenderWater)
        mBlenderWater->applyTimeDelta(delta);
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

    bool different_sky = mSky != psky;
    
    mSky = psky;
    mSky->mReplaced |= different_sky;
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

LLSettingsBase::TrackPosition LLEnvironment::DayInstance::secondsToKeyframe(LLSettingsDay::Seconds seconds)
{
    return convert_time_to_position(seconds, mDayLength);
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
    else
    {
        mWater = LLSettingsVOWater::buildDefaultWater();
        mBlenderWater = std::make_shared<LLTrackBlenderLoopingTime>(mWater, mDayCycle, 0, 
            mDayLength, mDayOffset, DEFAULT_UPDATE_THRESHOLD);
    }

    // sky, initialize to track 1
    LLSettingsDay::CycleTrack_t &track = mDayCycle->getCycleTrack(1);

    if (track.empty())
    {
        mSky.reset();
        mBlenderSky.reset();
    }
    else
    {
        mSky = LLSettingsVOSky::buildDefaultSky();
        mBlenderSky = std::make_shared<LLTrackBlenderLoopingTime>(mSky, mDayCycle, 1, 
            mDayLength, mDayOffset, DEFAULT_UPDATE_THRESHOLD);
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

void LLEnvironment::DayTransition::applyTimeDelta(const LLSettingsBase::Seconds& delta)
{
    mNextInstance->applyTimeDelta(delta);
    DayInstance::applyTimeDelta(delta);
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
            else
                setWater(mNextInstance->getWater());
    });

    mSky = mStartSky->buildClone();
    mBlenderSky = std::make_shared<LLSettingsBlenderTimeDelta>(mSky, mStartSky, mNextInstance->getSky(), mTransitionTime);
    mBlenderSky->setOnFinished(
        [this](LLSettingsBlender::ptr_t blender) {
        mBlenderSky.reset();

        if (!mBlenderSky && !mBlenderWater)
            LLEnvironment::instance().mCurrentEnvironment = mNextInstance;
        else
            setSky(mNextInstance->getSky());
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
    mPosition = llclamp(position, 0.0f, 1.0f);

    LLSettingsDay::TrackBound_t bounds = getBoundingEntries(mPosition);

    if (bounds.first == mEndMarker)
    {   // No frames in track.
        return 0.0;
    }

    mInitial = (*bounds.first).second;
    mFinal = (*bounds.second).second;

    F64 spanLength = getSpanLength(bounds);

    F64 spanPos = ((mPosition < (*bounds.first).first) ? (mPosition + 1.0) : mPosition) - (*bounds.first).first;

    if (spanPos > spanLength)
    {
        // we are clamping position to 0-1 and spanLength is 1
        // so don't account for case of spanPos == spanLength
        spanPos = fmod(spanPos, spanLength);
    }

    F64 blendf = spanPos / spanLength;
    return LLSettingsBlender::setBlendFactor(blendf);
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
