/**
* @file llsettingsdaycycle.cpp
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
#include "llsettingsdaycycle.h"
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

#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llenvironment.h"

#include "llworld.h"

//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment");

    inline F32 get_wrapping_distance(F32 begin, F32 end)
    {
        if (begin < end)
        {
            return end - begin;
        }
        else if (begin > end)
        {
            return 1.0 - (begin - end);
        }

        return 0;
    }

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atafter(LLSettingsDay::CycleTrack_t &collection, F32 key)
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

    LLSettingsDay::CycleTrack_t::iterator get_wrapping_atbefore(LLSettingsDay::CycleTrack_t &collection, F32 key)
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


}

//=========================================================================
const std::string LLSettingsDay::SETTING_DAYLENGTH("day_length");
const std::string LLSettingsDay::SETTING_KEYID("key_id");
const std::string LLSettingsDay::SETTING_KEYNAME("key_name");
const std::string LLSettingsDay::SETTING_KEYKFRAME("key_keyframe");
const std::string LLSettingsDay::SETTING_TRACKS("tracks");

//const S64 LLSettingsDayCycle::MINIMUM_DAYLENGTH(  300); // 5 mins

const S64 LLSettingsDay::MINIMUM_DAYLENGTH( 14400); // 4 hours
const S64 LLSettingsDay::MAXIMUM_DAYLENGTH(604800); // 7 days

const S32 LLSettingsDay::TRACK_WATER(0);   // water track is 0
const S32 LLSettingsDay::TRACK_MAX(5);     // 5 tracks, 4 skys, 1 water
const S32 LLSettingsDay::FRAME_MAX(56);

//=========================================================================
LLSettingsDay::LLSettingsDay(const LLSD &data) :
    LLSettingsBase(data),
    mHasParsed(false)
{
    mDayTracks.resize(TRACK_MAX);
}

LLSettingsDay::LLSettingsDay() :
    LLSettingsBase(),
    mHasParsed(false)
{
    mDayTracks.resize(TRACK_MAX);
}

//=========================================================================
LLSD LLSettingsDay::defaults()
{
    LLSD dfltsetting;

    dfltsetting[SETTING_NAME] = "_default_";
    dfltsetting[SETTING_DAYLENGTH] = static_cast<S32>(MINIMUM_DAYLENGTH);
    dfltsetting[SETTING_TRACKS] = LLSDArray(
        LLSDArray(LLSDMap(SETTING_KEYKFRAME, LLSD::Real(0.0f))(SETTING_KEYNAME, "_default_"))
        (LLSDMap(SETTING_KEYKFRAME, LLSD::Real(0.0f))(SETTING_KEYNAME, "_default_")));

    return dfltsetting;
}

LLSettingsDay::ptr_t LLSettingsDay::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(defaults());

    newsettings[SETTING_NAME] = name;
    newsettings[SETTING_DAYLENGTH] = static_cast<S32>(MINIMUM_DAYLENGTH);

    LLSD watertrack = LLSDArray( 
        LLSDMap ( SETTING_KEYKFRAME, LLSD::Real(0.0f) )
                ( SETTING_KEYNAME, "Default" ));

    LLSD skytrack = LLSD::emptyArray();

    for (LLSD::array_const_iterator it = oldsettings.beginArray(); it != oldsettings.endArray(); ++it)
    {
        LLSD entry = LLSDMap(SETTING_KEYKFRAME, (*it)[0].asReal())
            (SETTING_KEYNAME, (*it)[1].asString());
        skytrack.append(entry);
    }

    newsettings[SETTING_TRACKS] = LLSDArray(watertrack)(skytrack);

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsDay>(newsettings);
    dayp->parseFromLLSD(dayp->mSettings);

    if (dayp->validate())
        return dayp;

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::ptr_t LLSettingsDay::buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skydefs, LLSD waterdef)
{
    LLSettingsWater::ptr_t water = LLSettingsWater::buildFromLegacyPreset("Region", waterdef);
    LLEnvironment::namedSettingMap_t skys;

    for (LLSD::map_iterator itm = skydefs.beginMap(); itm != skydefs.endMap(); ++itm)
    {
        std::string name = (*itm).first;
        LLSettingsSky::ptr_t sky = LLSettingsSky::buildFromLegacyPreset(name, (*itm).second);

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

    dayp->mHasParsed = true;

    if (dayp->validate())
        return dayp;

    return LLSettingsDay::ptr_t();
}

LLSettingsDay::ptr_t LLSettingsDay::buildDefaultDayCycle()
{
    LLSD settings = LLSettingsDay::defaults();

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsDay>(settings);
    dayp->parseFromLLSD(dayp->mSettings);

    if (dayp->validate())
        return dayp;

    return LLSettingsDay::ptr_t();
}

void LLSettingsDay::parseFromLLSD(LLSD &data)
{
    LLEnvironment &environment(LLEnvironment::instance());
    LLSD tracks = data[SETTING_TRACKS];

    for (S32 i = 0; (i < tracks.size()) && (i < TRACK_MAX); ++i)
    {
        mDayTracks[i].clear();
        LLSD curtrack = tracks[i];
        for (LLSD::array_const_iterator it = curtrack.beginArray(); it != curtrack.endArray(); ++it)
        {
            F32 keyframe = (*it)[SETTING_KEYKFRAME].asReal();
            keyframe = llclamp(keyframe, 0.0f, 1.0f);
            LLSettingsBase::ptr_t setting;

            if ((*it).has(SETTING_KEYNAME))
            {
                if (i == TRACK_WATER)
                    setting = environment.findWaterByName((*it)[SETTING_KEYNAME]);
                else
                    setting = environment.findSkyByName((*it)[SETTING_KEYNAME]);
            }
            else if ((*it).has(SETTING_KEYID))
            {

            }

            if (setting)
                mDayTracks[i][keyframe] = setting;
        }
    }
    mHasParsed = true;
}


LLSettingsDay::ptr_t LLSettingsDay::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsDay::ptr_t dayp = boost::make_shared<LLSettingsDay>(settings);

    return dayp;
}

void LLSettingsDay::blend(const LLSettingsBase::ptr_t &other, F32 mix)
{
    LL_ERRS("DAYCYCLE") << "Day cycles are not blendable!" << LL_ENDL;
}

namespace
{
    bool validateDayCycleTrack(LLSD &value)
    {
        // Trim extra tracks.
        while (value.size() > LLSettingsDay::TRACK_MAX)
        {
            value.erase(value.size() - 1);
        }

        for (LLSD::array_iterator track = value.beginArray(); track != value.endArray(); ++track)
        {
            S32 index = 0;
            while (index < (*track).size())
            {
                if (index >= LLSettingsDay::FRAME_MAX)
                {
                    (*track).erase(index);
                    continue;
                }

                if (!(*track)[index].has(LLSettingsDay::SETTING_KEYKFRAME) ||
                        !(*track)[index][LLSettingsDay::SETTING_KEYKFRAME].isReal())
                {
                    (*track).erase(index);
                    continue;
                }

                if (!(*track)[index].has(LLSettingsDay::SETTING_KEYNAME) &&
                    !(*track)[index].has(LLSettingsDay::SETTING_KEYID))
                {
                    (*track).erase(index);
                    continue;
                }

                F32 frame = (*track)[index][LLSettingsDay::SETTING_KEYKFRAME].asReal();
                if ((frame < 0.0) || (frame > 1.0))
                {
                    frame = llclamp(frame, 0.0f, 1.0f);
                    (*track)[index][LLSettingsDay::SETTING_KEYKFRAME] = frame;
                }
                ++index;
            }

        }
        return true;
    }
}

LLSettingsDay::validation_list_t LLSettingsDay::getValidationList() const
{
    static validation_list_t validation;

    if (validation.empty())
    {
        validation.push_back(Validator(SETTING_TRACKS, true, LLSD::TypeArray, 
            &validateDayCycleTrack));
        validation.push_back(Validator(SETTING_DAYLENGTH, false, LLSD::TypeInteger,
            boost::bind(&Validator::verifyIntegerRange, _1, 
                LLSD(LLSDArray(LLSD::Integer(MINIMUM_DAYLENGTH))(LLSD::Integer(MAXIMUM_DAYLENGTH))))));
    }

    return validation;
}

//=========================================================================
F32 LLSettingsDay::secondsToKeyframe(S64Seconds seconds)
{
    S64Seconds daylength = getDayLength();

    return llclamp(static_cast<F32>(seconds.value() % daylength.value()) / static_cast<F32>(daylength.value()), 0.0f, 1.0f);
}

F64Seconds LLSettingsDay::keyframeToSeconds(F32 keyframe)
{
    S64Seconds daylength = getDayLength();

    return F64Seconds(static_cast<S32>(keyframe * static_cast<F32>(daylength.value())));
}

//=========================================================================
void LLSettingsDay::startDayCycle()
{
    F64Seconds now(LLDate::now().secondsSinceEpoch());

    if (!mHasParsed)
        parseFromLLSD(mSettings);

    // water
    if (mDayTracks[0].empty())
    {
        mBlendedWater.reset();
        mWaterBlender.reset();
    }
    else if (mDayTracks[0].size() == 1)
    {
        mBlendedWater = boost::static_pointer_cast<LLSettingsWater>((*(mDayTracks[0].begin())).second);
        mWaterBlender.reset();
    }
    else
    {
        TrackBound_t bounds = getBoundingEntries(mDayTracks[0], now);

        F64Seconds timespan = F64Seconds( getDayLength() * get_wrapping_distance((*bounds.first).first, (*bounds.second).first));

        mBlendedWater = LLSettingsWater::buildDefaultWater();
        mWaterBlender = boost::make_shared<LLSettingsBlender>(mBlendedWater,
            (*bounds.first).second, (*bounds.second).second, timespan);
        mWaterBlender->setOnFinished(boost::bind(&LLSettingsDay::onWaterTransitionDone, this, _1));
    }

    // sky
    if (mDayTracks[1].empty())
    {
        mBlendedSky.reset();
        mSkyBlender.reset();
    }
    else if (mDayTracks[1].size() == 1)
    {
        mBlendedSky = boost::static_pointer_cast<LLSettingsSky>( (*(mDayTracks[1].begin())).second);
        mSkyBlender.reset();
    }
    else
    {
        TrackBound_t bounds = getBoundingEntries(mDayTracks[1], now);
        F64Seconds timespan = F64Seconds(getDayLength() * get_wrapping_distance((*bounds.first).first, (*bounds.second).first));

        mBlendedSky = LLSettingsSky::buildDefaultSky();
        mSkyBlender = boost::make_shared<LLSettingsBlender>(mBlendedSky,
            (*bounds.first).second, (*bounds.second).second, timespan);
        mSkyBlender->setOnFinished(boost::bind(&LLSettingsDay::onSkyTransitionDone, this, 1, _1));
    }
}


void LLSettingsDay::updateSettings()
{
    static LLFrameTimer timer;


    F64Seconds delta(timer.getElapsedTimeAndResetF32());

    if (mSkyBlender)
        mSkyBlender->update(delta);
    if (mWaterBlender)
        mWaterBlender->update(delta);

#if 0
    //F64Seconds time_now(LLWorld::instance().getSpaceTimeUSec());
    F64Seconds time_now(LLDate::now().secondsSinceEpoch());

    // base class clears dirty flag so as to not trigger recursive update
    LLSettingsBase::updateSettings();

    if (!mBlendedWater)
    {
        mBlendedWater = LLEnvironment::instance().getCurrentWater()->buildClone();
        LLEnvironment::instance().selectWater(mBlendedWater);
    }

    if (!mBlendedSky)
    {
        mBlendedSky = LLEnvironment::instance().getCurrentSky()->buildClone();
        LLEnvironment::instance().selectSky(mBlendedSky);
    }


    if ((time_now < mLastUpdateTime) || ((time_now - mLastUpdateTime) > static_cast<F64Seconds>(0.1)))
    {
        F64Seconds daylength = static_cast<F64Seconds>(getDayLength());
        F32 frame = fmod(time_now.value(), daylength.value()) / daylength.value();

        CycleList_t::iterator itTrack = mDayTracks.begin();
        TrackBound_t bounds = getBoundingEntries(*itTrack, frame);

        mBlendedWater->replaceSettings((*bounds.first).second->getSettings());
        if (bounds.first != bounds.second)
        {
            F32 blendf = get_wrapping_distance((*bounds.first).first, frame) / get_wrapping_distance((*bounds.first).first, (*bounds.second).first);

            mBlendedWater->blend((*bounds.second).second, blendf);
        }

        ++itTrack;
        bounds = getBoundingEntries(*itTrack, frame);

        //_WARNS("RIDER") << "Sky blending: frame=" << frame << " start=" << F64Seconds((*bounds.first).first) << " end=" << F64Seconds((*bounds.second).first) << LL_ENDL;

        mBlendedSky->replaceSettings((*bounds.first).second->getSettings());
        if (bounds.first != bounds.second)
        {
            F32 blendf = get_wrapping_distance((*bounds.first).first, frame) / get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
            //_WARNS("RIDER") << "Distance=" << get_wrapping_distance((*bounds.first).first, frame) << "/" << get_wrapping_distance((*bounds.first).first, (*bounds.second).first) << " Blend factor=" << blendf << LL_ENDL;

            mBlendedSky->blend((*bounds.second).second, blendf);
        }

        mLastUpdateTime = time_now;
    }

    // Always mark the day cycle as dirty.So that the blend check can be handled.
    setDirtyFlag(true);
#endif
}

//=========================================================================
void LLSettingsDay::setDayLength(S64Seconds seconds)
{
    S32 val = llclamp(seconds.value(), MINIMUM_DAYLENGTH, MAXIMUM_DAYLENGTH);

    setValue(SETTING_DAYLENGTH, val);
}

LLSettingsDay::KeyframeList_t LLSettingsDay::getTrackKeyframes(S32 trackno)
{
    if ((trackno < 1) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return KeyframeList_t();
    }

    KeyframeList_t keyframes;
    CycleTrack_t &track = mDayTracks[trackno];

    keyframes.reserve(track.size());

    for (CycleTrack_t::iterator it = track.begin(); it != track.end(); ++it)
    {
        keyframes.push_back((*it).first);
    }

    return keyframes;
}

LLSettingsDay::TimeList_t LLSettingsDay::getTrackTimes(S32 trackno)
{
    KeyframeList_t keyframes = getTrackKeyframes(trackno);

    if (keyframes.empty())
        return TimeList_t();

    TimeList_t times;

    times.reserve(keyframes.size());
    for (KeyframeList_t::iterator it = keyframes.begin(); it != keyframes.end(); ++it)
    {
        times.push_back(keyframeToSeconds(*it));
    }

    return times;
}

void LLSettingsDay::setWaterAtTime(const LLSettingsWaterPtr_t &water, S64Seconds seconds)
{
    F32 keyframe = secondsToKeyframe(seconds);
    setWaterAtKeyframe(water, keyframe);
}

void LLSettingsDay::setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, F32 keyframe)
{
    mDayTracks[TRACK_WATER][llclamp(keyframe, 0.0f, 1.0f)] = water;
    setDirtyFlag(true);
}


void LLSettingsDay::setSkyAtTime(const LLSettingsSkyPtr_t &sky, S64Seconds seconds, S32 track)
{
    F32 keyframe = secondsToKeyframe(seconds);
    setSkyAtKeyframe(sky, keyframe, track);
}

void LLSettingsDay::setSkyAtKeyframe(const LLSettingsSkyPtr_t &sky, F32 keyframe, S32 track)
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }

    mDayTracks[track][llclamp(keyframe, 0.0f, 1.0f)] = sky;
    setDirtyFlag(true);
}

LLSettingsDay::TrackBound_t LLSettingsDay::getBoundingEntries(LLSettingsDay::CycleTrack_t &track, F32 keyframe)
{
    return TrackBound_t(get_wrapping_atbefore(track, keyframe), get_wrapping_atafter(track, keyframe));
}

LLSettingsDay::TrackBound_t LLSettingsDay::getBoundingEntries(LLSettingsDay::CycleTrack_t &track, F64Seconds time)
{
    F32 frame = secondsToKeyframe(time);

    return getBoundingEntries(track, frame);
}

//=========================================================================
void LLSettingsDay::onSkyTransitionDone(S32 track, const LLSettingsBlender::ptr_t &blender)
{
    F64Seconds now(LLDate::now().secondsSinceEpoch());
    TrackBound_t bounds = getBoundingEntries(mDayTracks[track], now);

    F32 distance = get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
    F64Seconds timespan = F64Seconds(distance * getDayLength());

    LL_WARNS("RIDER") << "New sky blender. now=" << now <<
        " start=" << (*bounds.first).first << " end=" << (*bounds.second).first <<
        " span=" << timespan << LL_ENDL;

    mSkyBlender = boost::make_shared<LLSettingsBlender>(mBlendedSky,
        (*bounds.first).second, (*bounds.second).second, timespan);
    mSkyBlender->setOnFinished(boost::bind(&LLSettingsDay::onSkyTransitionDone, this, track, _1));
}

void LLSettingsDay::onWaterTransitionDone(const LLSettingsBlender::ptr_t &blender)
{
    F64Seconds now(LLDate::now().secondsSinceEpoch());
    TrackBound_t bounds = getBoundingEntries(mDayTracks[0], now);

    F32 distance = get_wrapping_distance((*bounds.first).first, (*bounds.second).first);
    F64Seconds timespan = F64Seconds(distance * getDayLength());

    mWaterBlender = boost::make_shared<LLSettingsBlender>(mBlendedWater,
        (*bounds.first).second, (*bounds.second).second, timespan);
    mWaterBlender->setOnFinished(boost::bind(&LLSettingsDay::onWaterTransitionDone, this, _1));
}
