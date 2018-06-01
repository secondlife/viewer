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

#include "llsettingsdaycycle.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include "lltrace.h"
#include "llfasttimer.h"
#include "v3colorutil.h"

#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llframetimer.h"
//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment");

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
const std::string LLSettingsDay::SETTING_KEYID("key_id");
const std::string LLSettingsDay::SETTING_KEYNAME("key_name");
const std::string LLSettingsDay::SETTING_KEYKFRAME("key_keyframe");
const std::string LLSettingsDay::SETTING_KEYHASH("key_hash");
const std::string LLSettingsDay::SETTING_TRACKS("tracks");
const std::string LLSettingsDay::SETTING_FRAMES("frames");

const LLSettingsDay::Seconds LLSettingsDay::MINIMUM_DAYLENGTH(300); // 5 mins
const LLSettingsDay::Seconds LLSettingsDay::DEFAULT_DAYLENGTH(14400); // 4 hours
const LLSettingsDay::Seconds LLSettingsDay::MAXIMUM_DAYLENGTH(604800); // 7 days

const LLSettingsDay::Seconds LLSettingsDay::MINIMUM_DAYOFFSET(0);
const LLSettingsDay::Seconds LLSettingsDay::DEFAULT_DAYOFFSET(57600);  // +16 hours == -8 hours (SLT time offset)
const LLSettingsDay::Seconds LLSettingsDay::MAXIMUM_DAYOFFSET(86400);  // 24 hours

const S32 LLSettingsDay::TRACK_WATER(0);   // water track is 0
const S32 LLSettingsDay::TRACK_MAX(5);     // 5 tracks, 4 skys, 1 water
const S32 LLSettingsDay::FRAME_MAX(56);

//=========================================================================
LLSettingsDay::LLSettingsDay(const LLSD &data) :
    LLSettingsBase(data),
    mInitialized(false)
{
    mDayTracks.resize(TRACK_MAX);
}

LLSettingsDay::LLSettingsDay() :
    LLSettingsBase(),
    mInitialized(false)
{
    mDayTracks.resize(TRACK_MAX);
}

//=========================================================================
LLSD LLSettingsDay::getSettings() const
{
    LLSD settings(LLSD::emptyMap());

    if (mSettings.has(SETTING_NAME))
        settings[SETTING_NAME] = mSettings[SETTING_NAME];

    if (mSettings.has(SETTING_ID))
        settings[SETTING_ID] = mSettings[SETTING_ID];

    settings[SETTING_TYPE] = getSettingType();

    std::map<std::string, LLSettingsBase::ptr_t> in_use;

    LLSD tracks(LLSD::emptyArray());
    
    for (auto &track: mDayTracks)
    {
        LLSD trackout(LLSD::emptyArray());

        for (auto &frame: track)
        {
            F32 frame_time = frame.first;
            LLSettingsBase::ptr_t data = frame.second;
            size_t datahash = data->getHash();

            std::stringstream keyname;
            keyname << datahash;

            trackout.append(LLSD(LLSDMap(SETTING_KEYKFRAME, LLSD::Real(frame_time))(SETTING_KEYNAME, keyname.str())));
            in_use[keyname.str()] = data;
        }
        tracks.append(trackout);
    }
    settings[SETTING_TRACKS] = tracks;

    LLSD frames(LLSD::emptyMap());
    for (auto &used_frame: in_use)
    {
        LLSD framesettings = llsd_clone(used_frame.second->getSettings(),
            LLSDMap("*", true)(SETTING_NAME, false)(SETTING_ID, false)(SETTING_HASH, false));

        frames[used_frame.first] = framesettings;
    }
    settings[SETTING_FRAMES] = frames;

    return settings;
}

bool LLSettingsDay::initialize()
{
    LLSD tracks = mSettings[SETTING_TRACKS];
    LLSD frames = mSettings[SETTING_FRAMES];

    std::map<std::string, LLSettingsBase::ptr_t> used;

    for (LLSD::map_const_iterator itFrame = frames.beginMap(); itFrame != frames.endMap(); ++itFrame)
    {
        std::string name = (*itFrame).first;
        LLSD data = (*itFrame).second;
        LLSettingsBase::ptr_t keyframe;

        if (data[SETTING_TYPE].asString() == "sky")
        {
            keyframe = buildSky(data);
        }
        else if (data[SETTING_TYPE].asString() == "water")
        {
            keyframe = buildWater(data);
        }
        else
        {
            LL_WARNS("DAYCYCLE") << "Unknown child setting type '" << data[SETTING_TYPE].asString() << "' named '" << name << "'" << LL_ENDL;
        }
        if (!keyframe)
        {
            LL_WARNS("DAYCYCLE") << "Invalid frame data" << LL_ENDL;
            continue;
        }

        used[name] = keyframe;
    }

    bool haswater(false);
    bool hassky(false);

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
                {
                    setting = used[(*it)[SETTING_KEYNAME]];
                    if (!setting)
                        setting = getNamedWater((*it)[SETTING_KEYNAME]);
                    if (setting && setting->getSettingType() != "water")
                    {
                        LL_WARNS("DAYCYCLE") << "Water track referencing " << setting->getSettingType() << " frame at " << keyframe << "." << LL_ENDL;
                        setting.reset();
                    }
                }
                else
                {
                    setting = used[(*it)[SETTING_KEYNAME]];
                    if (!setting)
                        setting = getNamedSky((*it)[SETTING_KEYNAME]);
                    if (setting && setting->getSettingType() != "sky")
                    {
                        LL_WARNS("DAYCYCLE") << "Sky track #" << i << " referencing " << setting->getSettingType() << " frame at " << keyframe << "." << LL_ENDL;
                        setting.reset();
                    }
                }
            }

            if (setting)
            {
                if (i == TRACK_WATER)
                    haswater |= true;
                else
                    hassky |= true;
                mDayTracks[i][keyframe] = setting;
            }
        }
    }

    if (!haswater || !hassky)
    {
        LL_WARNS("DAYCYCLE") << "Must have at least one water and one sky frame!" << LL_ENDL;
        return false;
    }
    // these are no longer needed and just take up space now.
    mSettings.erase(SETTING_TRACKS);
    mSettings.erase(SETTING_FRAMES);

    mInitialized = true;
    return true;
}


//=========================================================================
LLSD LLSettingsDay::defaults()
{
    LLSD dfltsetting;

    dfltsetting[SETTING_NAME] = "_default_";

    LLSD waterTrack;
    waterTrack[SETTING_KEYKFRAME] = 0.0f;
    waterTrack[SETTING_KEYNAME]   = "_default_";

    LLSD skyTrack;
    skyTrack[SETTING_KEYKFRAME] = 0.0f;
    skyTrack[SETTING_KEYNAME]   = "_default_";

    LLSD tracks;
    tracks.append(LLSDArray(waterTrack));
    tracks.append(LLSDArray(skyTrack));

    dfltsetting[SETTING_TRACKS] = tracks;

    LLSD frames(LLSD::emptyMap());

    frames["water:_defaults_"] = LLSettingsWater::defaults();
    frames["sky:_defaults_"] = LLSettingsSky::defaults();

    dfltsetting[SETTING_FRAMES] = frames;

    dfltsetting[SETTING_TYPE] = "daycycle";

    return dfltsetting;
}

void LLSettingsDay::blend(const LLSettingsBase::ptr_t &other, F64 mix)
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

        S32 framecount(0);

        for (LLSD::array_iterator track = value.beginArray(); track != value.endArray(); ++track)
        {
            S32 index = 0;
            while (index < (*track).size())
            {
                LLSD& elem = (*track)[index];

                ++framecount;
                if (index >= LLSettingsDay::FRAME_MAX)
                {
                    (*track).erase(index);
                    continue;
                }

                if (!elem.has(LLSettingsDay::SETTING_KEYKFRAME))
                {
                    (*track).erase(index);
                    continue;
                }

                if (!elem[LLSettingsDay::SETTING_KEYKFRAME].isReal())
                {
                    (*track).erase(index);
                    continue;
                }

                if (!elem.has(LLSettingsDay::SETTING_KEYNAME) &&
                    !elem.has(LLSettingsDay::SETTING_KEYID))
                {
                    (*track).erase(index);
                    continue;
                }

                F32 frame = elem[LLSettingsDay::SETTING_KEYKFRAME].asReal();
                if ((frame < 0.0) || (frame > 1.0))
                {
                    frame = llclamp(frame, 0.0f, 1.0f);
                    elem[LLSettingsDay::SETTING_KEYKFRAME] = frame;
                }
                ++index;
            }

        }

        int waterTracks = value[0].size();
        int skyTracks   = framecount - waterTracks;

        if (waterTracks < 1)
        {
            LL_WARNS("SETTINGS") << "Missing water track" << LL_ENDL;
            return false;
        }

        if (skyTracks < 1)
        {
            LL_WARNS("SETTINGS") << "Missing sky tracks" << LL_ENDL;
            return false;
        }
        return true;
    }

    bool validateDayCycleFrames(LLSD &value)
    {
        bool hasSky(false);
        bool hasWater(false);

        for (LLSD::map_iterator itf = value.beginMap(); itf != value.endMap(); ++itf)
        {
            LLSD frame = (*itf).second;

            std::string ftype = frame[LLSettingsBase::SETTING_TYPE];
            if (ftype == "sky")
            {
                LLSettingsSky::validation_list_t valid_sky = LLSettingsSky::validationList();
                LLSD res_sky = LLSettingsBase::settingValidation(frame, valid_sky);
                
                if (res_sky["success"].asInteger() == 0)
                {
                    LL_WARNS("SETTINGS") << "Sky setting named '" << (*itf).first << "' validation failed!: " << res_sky << LL_ENDL;
                    LL_WARNS("SETTINGS") << "Sky: " << frame << LL_ENDL;
                    continue;
                }
                hasSky |= true;
            }
            else if (ftype == "water")
            {
                LLSettingsWater::validation_list_t valid_h2o = LLSettingsWater::validationList();
                LLSD res_h2o = LLSettingsBase::settingValidation(frame, valid_h2o);
                if (res_h2o["success"].asInteger() == 0)
                {
                    LL_WARNS("SETTINGS") << "Water setting named '" << (*itf).first << "' validation failed!: " << res_h2o << LL_ENDL;
                    LL_WARNS("SETTINGS") << "Water: " << frame << LL_ENDL;
                    continue;
                }
                hasWater |= true;
            }
            else
            {
                LL_WARNS("SETTINGS") << "Unknown settings block of type '" << ftype << "' named '" << (*itf).first << "'" << LL_ENDL;
                return false;
            }
        }

        if (!hasSky)
        {
            LL_WARNS("SETTINGS") << "No skies defined." << LL_ENDL;
            return false;
        }

        if (!hasWater)
        {
            LL_WARNS("SETTINGS") << "No waters defined." << LL_ENDL;
            return false;
        }

        return true;
    }
}

LLSettingsDay::validation_list_t LLSettingsDay::getValidationList() const
{
    return LLSettingsDay::validationList();
}

LLSettingsDay::validation_list_t LLSettingsDay::validationList()
{
    static validation_list_t validation;

    if (validation.empty())
    {
        validation.push_back(Validator(SETTING_TRACKS, true, LLSD::TypeArray, 
            &validateDayCycleTrack));
        validation.push_back(Validator(SETTING_FRAMES, true, LLSD::TypeMap, 
            &validateDayCycleFrames));
    }

    return validation;
}

LLSettingsDay::CycleTrack_t &LLSettingsDay::getCycleTrack(S32 track)
{
    static CycleTrack_t emptyTrack;
    if (mDayTracks.size() <= track)
        return emptyTrack;

    return mDayTracks[track];
}

//=========================================================================
void LLSettingsDay::startDayCycle()
{
    LLSettingsBase::Seconds now(LLDate::now().secondsSinceEpoch());

    if (!mInitialized)
    {
        LL_WARNS("DAYCYCLE") << "Attempt to start day cycle on uninitialized object." << LL_ENDL;
        return;
    }

}


void LLSettingsDay::updateSettings()
{
}

//=========================================================================
LLSettingsDay::KeyframeList_t LLSettingsDay::getTrackKeyframes(S32 trackno)
{
    if ((trackno < 0) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return KeyframeList_t();
    }

    KeyframeList_t keyframes;
    CycleTrack_t &track = mDayTracks[trackno];

    keyframes.reserve(track.size());

    for (auto &frame: track)
    {
        keyframes.push_back(frame.first);
    }

    return keyframes;
}

bool LLSettingsDay::moveTrackKeyframe(S32 trackno, F32 old_frame, F32 new_frame)
{
    if ((trackno < 0) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return false;
    }

    if (old_frame == new_frame)
    {
        return false;
    }

    CycleTrack_t &track = mDayTracks[trackno];
    CycleTrack_t::iterator iter = track.find(old_frame);
    if (iter != track.end())
    {
        LLSettingsBase::ptr_t base = iter->second;
        track.erase(iter);
        track[llclamp(new_frame, 0.0f, 1.0f)] = base;
        return true;
    }

    return false;

}

bool LLSettingsDay::removeTrackKeyframe(S32 trackno, F32 frame)
{
    if ((trackno < 0) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return false;
    }

    CycleTrack_t &track = mDayTracks[trackno];
    CycleTrack_t::iterator iter = track.find(frame);
    if (iter != track.end())
    {
        LLSettingsBase::ptr_t base = iter->second;
        track.erase(iter);
        return true;
    }

    return false;
}

void LLSettingsDay::setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, F32 keyframe)
{
    setSettingsAtKeyframe(water, keyframe, TRACK_WATER);
}

LLSettingsWater::ptr_t LLSettingsDay::getWaterAtKeyframe(F32 keyframe) const
{
    return std::dynamic_pointer_cast<LLSettingsWater>(getSettingsAtKeyframe(keyframe, TRACK_WATER));
}

void LLSettingsDay::setSkyAtKeyframe(const LLSettingsSky::ptr_t &sky, F32 keyframe, S32 track)
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }

    setSettingsAtKeyframe(sky, keyframe, track);
}

LLSettingsSky::ptr_t LLSettingsDay::getSkyAtKeyframe(F32 keyframe, S32 track) const
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return LLSettingsSky::ptr_t();
    }

    return std::dynamic_pointer_cast<LLSettingsSky>(getSettingsAtKeyframe(keyframe, track));
}

void LLSettingsDay::setSettingsAtKeyframe(const LLSettingsBase::ptr_t &settings, F32 keyframe, S32 track)
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }

    mDayTracks[track][llclamp(keyframe, 0.0f, 1.0f)] = settings;
    setDirtyFlag(true);
}

LLSettingsBase::ptr_t LLSettingsDay::getSettingsAtKeyframe(F32 keyframe, S32 track) const
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return LLSettingsBase::ptr_t();
    }

    // todo: better way to identify keyframes?
    CycleTrack_t::const_iterator iter = mDayTracks[track].find(keyframe);
    if (iter != mDayTracks[track].end())
    {
        return iter->second;
    }

    return LLSettingsBase::ptr_t();
}

F32 LLSettingsDay::getUpperBoundFrame(S32 track, F32 keyframe)
{
    return get_wrapping_atafter(mDayTracks[track], keyframe)->first;
}

F32 LLSettingsDay::getLowerBoundFrame(S32 track, F32 keyframe)
{
    return get_wrapping_atbefore(mDayTracks[track], keyframe)->first;
}

LLSettingsDay::TrackBound_t LLSettingsDay::getBoundingEntries(LLSettingsDay::CycleTrack_t &track, F32 keyframe)
{
    return TrackBound_t(get_wrapping_atbefore(track, keyframe), get_wrapping_atafter(track, keyframe));
}

//=========================================================================
