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

//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment");

    inline F32 get_wrapping_distance(F32 begin, F32 end)
    {
        return 1.0 - fabs((begin + 1.0) - end);
    }

    LLSettingsDayCycle::CycleTrack_t::iterator get_wrapping_atafter(LLSettingsDayCycle::CycleTrack_t &collection, F32 key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDayCycle::CycleTrack_t::iterator it = collection.upper_bound(key);

        if (it == collection.end())
        {   // wrap around
            it = collection.begin();
        }

        return it;
    }

    LLSettingsDayCycle::CycleTrack_t::iterator get_wrapping_atbefore(LLSettingsDayCycle::CycleTrack_t &collection, F32 key)
    {
        if (collection.empty())
            return collection.end();

        LLSettingsDayCycle::CycleTrack_t::iterator it = collection.lower_bound(key);

        if (it == collection.end())
        {   // all offsets are lower, take the last one.
            --it; // we know the range is not empty
        }
        else if ((*it).first > key)
        {   // the offset we are interested in is smaller than the found.
            if (it == collection.begin())
                it = collection.end();
            --it;
        }

        return it;
    }


}

//=========================================================================
const std::string LLSettingsDayCycle::SETTING_DAYLENGTH("day_length");
const std::string LLSettingsDayCycle::SETTING_KEYID("key_id");
const std::string LLSettingsDayCycle::SETTING_KEYNAME("key_name");
const std::string LLSettingsDayCycle::SETTING_KEYOFFSET("key_offset");
const std::string LLSettingsDayCycle::SETTING_NAME("name");
const std::string LLSettingsDayCycle::SETTING_TRACKS("tracks");

const S32 LLSettingsDayCycle::MINIMUM_DAYLENGTH( 14400); // 4 hours
const S32 LLSettingsDayCycle::MAXIMUM_DAYLENGTH(604800); // 7 days

const S32 LLSettingsDayCycle::TRACK_WATER(0);   // water track is 0
const S32 LLSettingsDayCycle::TRACK_MAX(5);     // 5 tracks, 4 skys, 1 water

//=========================================================================
LLSettingsDayCycle::LLSettingsDayCycle(const LLSD &data) :
    LLSettingsBase(data)
{
    mDayTracks.resize(TRACK_MAX);
}

LLSettingsDayCycle::LLSettingsDayCycle() :
    LLSettingsBase()
{
    mDayTracks.resize(TRACK_MAX);
}

//=========================================================================
LLSD LLSettingsDayCycle::defaults()
{
    LLSD dfltsetting;

    dfltsetting[SETTING_NAME] = "_default_";
    dfltsetting[SETTING_DAYLENGTH] = MINIMUM_DAYLENGTH;
    dfltsetting[SETTING_TRACKS] = LLSDArray(
        LLSDArray(LLSDMap(SETTING_KEYOFFSET, LLSD::Real(0.0f))(SETTING_KEYNAME, "_default_"))
        (LLSDMap(SETTING_KEYOFFSET, LLSD::Real(0.0f))(SETTING_KEYNAME, "_default_")));
    
    return dfltsetting;
}

LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(defaults());

    newsettings[SETTING_NAME] = name;
    newsettings[SETTING_DAYLENGTH] = MINIMUM_DAYLENGTH;

    LLSD watertrack = LLSDArray( 
        LLSDMap ( SETTING_KEYOFFSET, LLSD::Real(0.0f) )
                ( SETTING_KEYNAME, "Default" ));

    LLSD skytrack = LLSD::emptyArray();

    for (LLSD::array_const_iterator it = oldsettings.beginArray(); it != oldsettings.endArray(); ++it)
    {
        LLSD entry = LLSDMap(SETTING_KEYOFFSET, (*it)[0].asReal())
            (SETTING_KEYNAME, (*it)[1].asString());
        skytrack.append(entry);
    }

    newsettings[SETTING_TRACKS] = LLSDArray(watertrack)(skytrack);

    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(newsettings);

    return dayp;
}

LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildDefaultDayCycle()
{
    LLSD settings = LLSettingsDayCycle::defaults();

    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(settings);

    return dayp;
}

void LLSettingsDayCycle::parseFromLLSD(LLSD &data)
{
    LLEnvironment &environment(LLEnvironment::instance());
    LLSD tracks = data[SETTING_TRACKS];

    for (S32 i = 0; (i < tracks.size()) && (i < TRACK_MAX); ++i)
    {
        mDayTracks[i].clear();
        LLSD curtrack = tracks[i];
        for (LLSD::array_const_iterator it = curtrack.beginArray(); it != curtrack.endArray(); ++it)
        {
            F32 offset = (*it)[SETTING_KEYOFFSET].asReal();
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
                mDayTracks[i][offset] = setting;
        }
    }
}


LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(settings);

    return dayp;
}

LLSettingsBase::ptr_t LLSettingsDayCycle::blend(const LLSettingsBase::ptr_t &other, F32 mix) const
{
    LL_ERRS("DAYCYCLE") << "Day cycles are not blendable!" << LL_ENDL;
    return LLSettingsBase::ptr_t();
}

//=========================================================================
F32 LLSettingsDayCycle::secondsToOffset(S32 seconds)
{
    S32 daylength = getDayLength();

    return static_cast<F32>(seconds % daylength) / static_cast<F32>(daylength);
}

S32 LLSettingsDayCycle::offsetToSeconds(F32 offset)
{
    S32 daylength = getDayLength();

    return static_cast<S32>(offset * static_cast<F32>(daylength));
}

//=========================================================================
void LLSettingsDayCycle::updateSettings()
{

}

//=========================================================================
void LLSettingsDayCycle::setDayLength(S32 seconds)
{
    seconds = llclamp(seconds, MINIMUM_DAYLENGTH, MAXIMUM_DAYLENGTH);

    setValue(SETTING_DAYLENGTH, seconds);
}

LLSettingsDayCycle::OffsetList_t LLSettingsDayCycle::getTrackOffsets(S32 trackno)
{
    if ((trackno < 1) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return OffsetList_t();
    }

    OffsetList_t offsets;
    CycleTrack_t &track = mDayTracks[trackno];

    offsets.reserve(track.size());

    for (CycleTrack_t::iterator it = track.begin(); it != track.end(); ++it)
    {
        offsets.push_back((*it).first);
    }

    return offsets;
}

LLSettingsDayCycle::TimeList_t LLSettingsDayCycle::getTrackTimes(S32 trackno)
{
    OffsetList_t offsets = getTrackOffsets(trackno);

    if (offsets.empty())
        return TimeList_t();

    TimeList_t times;

    times.reserve(offsets.size());
    for (OffsetList_t::iterator it = offsets.begin(); it != offsets.end(); ++it)
    {
        times.push_back(offsetToSeconds(*it));
    }

    return times;
}

void LLSettingsDayCycle::setWaterAtTime(const LLSettingsWaterPtr_t &water, S32 seconds)
{
    F32 offset = secondsToOffset(seconds);
    setWaterAtOffset(water, offset);
}

void LLSettingsDayCycle::setWaterAtOffset(const LLSettingsWaterPtr_t &water, F32 offset)
{
    mDayTracks[TRACK_WATER][offset] = water;
    setDirtyFlag(true);
}


void LLSettingsDayCycle::setSkyAtOnTrack(const LLSettingsSkyPtr_t &sky, S32 seconds, S32 track)
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }
    F32 offset = secondsToOffset(seconds);

    mDayTracks[track][offset] = sky;
    setDirtyFlag(true);

}

LLSettingsDayCycle::TrackBound_t LLSettingsDayCycle::getBoundingEntries(CycleTrack_t &track, F32 offset)
{
    return TrackBound_t(get_wrapping_atbefore(track, offset), get_wrapping_atafter(track, offset));
}

LLSettingsBase::ptr_t LLSettingsDayCycle::getBlendedEntry(CycleTrack_t &track, F32 offset)
{
    TrackBound_t bounds = getBoundingEntries(track, offset);

    if (bounds.first == track.end())
        return LLSettingsBase::ptr_t(); // Track is empty nothing to blend.

    if (bounds.first == bounds.second)
    {   // Single entry.  Nothing to blend
        return (*bounds.first).second;
    }

    F32 blendf = get_wrapping_distance((*bounds.first).first, offset) / get_wrapping_distance((*bounds.first).first, (*bounds.second).first);

    LLSettingsBase::ptr_t base = (*bounds.first).second;
    return base->blend((*bounds.second).second, blendf);
}

//=========================================================================
