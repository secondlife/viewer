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
#include "llerror.h"
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
    LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment Day");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment Day");

    template<typename T>
    inline T get_wrapping_distance(T begin, T end)
    {
        if (begin < end)
        {
            return end - begin;
        }
        else if (begin > end)
        {
            return T(1.0) - (begin - end);
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


}

//=========================================================================
const std::string LLSettingsDay::SETTING_KEYID("key_id");
const std::string LLSettingsDay::SETTING_KEYNAME("key_name");
const std::string LLSettingsDay::SETTING_KEYKFRAME("key_keyframe");
const std::string LLSettingsDay::SETTING_KEYHASH("key_hash");
const std::string LLSettingsDay::SETTING_TRACKS("tracks");
const std::string LLSettingsDay::SETTING_FRAMES("frames");

const LLSettingsDay::Seconds LLSettingsDay::MINIMUM_DAYLENGTH(14400);  // 4 hours
const LLSettingsDay::Seconds LLSettingsDay::DEFAULT_DAYLENGTH(14400);  // 4 hours
const LLSettingsDay::Seconds LLSettingsDay::MAXIMUM_DAYLENGTH(604800); // 7 days

const LLSettingsDay::Seconds LLSettingsDay::MINIMUM_DAYOFFSET(0);
const LLSettingsDay::Seconds LLSettingsDay::DEFAULT_DAYOFFSET(57600);  // +16 hours == -8 hours (SLT time offset)
const LLSettingsDay::Seconds LLSettingsDay::MAXIMUM_DAYOFFSET(86400);  // 24 hours

const S32 LLSettingsDay::TRACK_WATER(0);   // water track is 0
const S32 LLSettingsDay::TRACK_GROUND_LEVEL(1);
const S32 LLSettingsDay::TRACK_MAX(5);     // 5 tracks, 4 skys, 1 water
const S32 LLSettingsDay::FRAME_MAX(56);

const F32 LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR(0.02501f);

const LLUUID LLSettingsDay::DEFAULT_ASSET_ID("5646d39e-d3d7-6aff-ed71-30fc87d64a91");

// Minimum value to prevent multislider in edit floaters from eating up frames that 'encroach' on one another's space
static const F32 DEFAULT_MULTISLIDER_INCREMENT(0.005f);
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

    if (mSettings.has(SETTING_ASSETID))
        settings[SETTING_ASSETID] = mSettings[SETTING_ASSETID];

    settings[SETTING_TYPE] = getSettingsType();

    std::map<std::string, LLSettingsBase::ptr_t> in_use;

    LLSD tracks(LLSD::emptyArray());
    
    for (CycleList_t::const_iterator itTrack = mDayTracks.begin(); itTrack != mDayTracks.end(); ++itTrack)
    {
        LLSD trackout(LLSD::emptyArray());

        for (CycleTrack_t::const_iterator itFrame = (*itTrack).begin(); itFrame != (*itTrack).end(); ++itFrame)
        {
            F32 frame = (*itFrame).first;
            LLSettingsBase::ptr_t data = (*itFrame).second;
            size_t datahash = data->getHash();

            std::stringstream keyname;
            keyname << datahash;

            trackout.append(LLSD(LLSDMap(SETTING_KEYKFRAME, LLSD::Real(frame))(SETTING_KEYNAME, keyname.str())));
            in_use[keyname.str()] = data;
        }
        tracks.append(trackout);
    }
    settings[SETTING_TRACKS] = tracks;

    LLSD frames(LLSD::emptyMap());
    for (std::map<std::string, LLSettingsBase::ptr_t>::iterator itFrame = in_use.begin(); itFrame != in_use.end(); ++itFrame)
    {
        LLSD framesettings = llsd_clone((*itFrame).second->getSettings(),
            LLSDMap("*", true)(SETTING_NAME, false)(SETTING_ID, false)(SETTING_HASH, false));

        frames[(*itFrame).first] = framesettings;
    }
    settings[SETTING_FRAMES] = frames;

    return settings;
}

bool LLSettingsDay::initialize(bool validate_frames)
{
    LLSD tracks = mSettings[SETTING_TRACKS];
    LLSD frames = mSettings[SETTING_FRAMES];

    // save for later... 
    LLUUID assetid;
    if (mSettings.has(SETTING_ASSETID))
    {
        assetid = mSettings[SETTING_ASSETID].asUUID();
    }

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
            LLSettingsBase::TrackPosition keyframe = LLSettingsBase::TrackPosition((*it)[SETTING_KEYKFRAME].asReal());
            keyframe = llclamp(keyframe, 0.0f, 1.0f);
            LLSettingsBase::ptr_t setting;

            
            if ((*it).has(SETTING_KEYNAME))
            {
                std::string key_name = (*it)[SETTING_KEYNAME];
                if (i == TRACK_WATER)
                {
                    setting = used[key_name];
                    if (setting && setting->getSettingsType() != "water")
                    {
                        LL_WARNS("DAYCYCLE") << "Water track referencing " << setting->getSettingsType() << " frame at " << keyframe << "." << LL_ENDL;
                        setting.reset();
                    }
                }
                else
                {
                    setting = used[key_name];
                    if (setting && setting->getSettingsType() != "sky")
                    {
                        LL_WARNS("DAYCYCLE") << "Sky track #" << i << " referencing " << setting->getSettingsType() << " frame at " << keyframe << "." << LL_ENDL;
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

                if (validate_frames && mDayTracks[i].size() > 0)
                {
                    // check if we hit close to anything in the list
                    LLSettingsDay::CycleTrack_t::value_type frame = getSettingsNearKeyframe(keyframe, i, DEFAULT_FRAME_SLOP_FACTOR);
                    if (frame.second)
                    {
                        // figure out direction of search
                        LLSettingsBase::TrackPosition found = frame.first;
                        LLSettingsBase::TrackPosition new_frame = keyframe;
                        F32 total_frame_shift = 0;
                        // We consider frame DEFAULT_FRAME_SLOP_FACTOR away as still encroaching, so add minimum increment
                        F32 move_factor = DEFAULT_FRAME_SLOP_FACTOR + DEFAULT_MULTISLIDER_INCREMENT;
                        bool move_forward = true;
                        if ((new_frame < found && (found - new_frame) <= DEFAULT_FRAME_SLOP_FACTOR)
                            || (new_frame > found && (new_frame - found) > DEFAULT_FRAME_SLOP_FACTOR))
                        {
                            move_forward = false;
                        }

                        if (move_forward)
                        {
                            CycleTrack_t::iterator iter = mDayTracks[i].find(found);
                            new_frame = found; // for total_frame_shift
                            while (total_frame_shift < 1)
                            {
                                // calculate shifted position from previous found point
                                total_frame_shift += move_factor + (found >= new_frame ? found : found + 1) - new_frame;
                                new_frame = found + move_factor;
                                if (new_frame > 1) new_frame--;

                                // we know that current point is too close, go for next one
                                iter++;
                                if (iter == mDayTracks[i].end())
                                {
                                    iter = mDayTracks[i].begin();
                                }

                                if (((iter->first >= (new_frame - DEFAULT_MULTISLIDER_INCREMENT)) && ((new_frame + DEFAULT_FRAME_SLOP_FACTOR) >= iter->first))
                                    || ((iter->first < new_frame) && ((new_frame + DEFAULT_FRAME_SLOP_FACTOR) >= (iter->first + 1))))
                                {
                                    // we are encroaching at new point as well
                                    found = iter->first;
                                }
                                else // (new_frame + DEFAULT_FRAME_SLOP_FACTOR < iter->first)
                                {
                                    //we found clear spot
                                    break;
                                }
                            }
                        }
                        else
                        {
                            CycleTrack_t::reverse_iterator iter = mDayTracks[i].rbegin();
                            while (iter->first != found)
                            {
                                iter++;
                            }
                            new_frame = found; // for total_frame_shift
                            while (total_frame_shift < 1)
                            {
                                // calculate shifted position from current found point
                                total_frame_shift += move_factor + new_frame - (found <= new_frame ? found : found - 1);
                                new_frame = found - move_factor;
                                if (new_frame < 0) new_frame++;

                                // we know that current point is too close, go for next one
                                iter++;
                                if (iter == mDayTracks[i].rend())
                                {
                                    iter = mDayTracks[i].rbegin();
                                }

                                if ((iter->first <= (new_frame + DEFAULT_MULTISLIDER_INCREMENT) && (new_frame - DEFAULT_FRAME_SLOP_FACTOR) <= iter->first)
                                    || ((iter->first > new_frame) && ((new_frame - DEFAULT_FRAME_SLOP_FACTOR) <= (iter->first - 1))))
                                {
                                    // we are encroaching at new point as well
                                    found = iter->first;
                                }
                                else // (new_frame - DEFAULT_FRAME_SLOP_FACTOR > iter->first)
                                {
                                    //we found clear spot
                                    break;
                                }
                            }


                        }

                        if (total_frame_shift >= 1)
                        {
                            LL_WARNS("SETTINGS") << "Could not fix frame position, adding as is to position: " << keyframe << LL_ENDL;
                        }
                        else
                        {
                            // Mark as new position
                            keyframe = new_frame;
                        }
                    }
                }
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

    if (!assetid.isNull())
    {
        mSettings[SETTING_ASSETID] = assetid;
    }

    mInitialized = true;
    return true;
}


//=========================================================================
LLSD LLSettingsDay::defaults()
{
    static LLSD dfltsetting;

    if (dfltsetting.size() == 0)
    {
        dfltsetting[SETTING_NAME] = DEFAULT_SETTINGS_NAME;
        dfltsetting[SETTING_TYPE] = "daycycle";

        LLSD frames(LLSD::emptyMap());
        LLSD waterTrack;
        LLSD skyTrack;

    
        const U32 FRAME_COUNT = 8;
        const F32 FRAME_STEP  = 1.0f / F32(FRAME_COUNT);
        F32 time = 0.0f;
        for (U32 i = 0; i < FRAME_COUNT; i++)
        {
            std::string name(DEFAULT_SETTINGS_NAME);
            name += ('a' + i);

            std::string water_frame_name("water:");
            std::string sky_frame_name("sky:");

            water_frame_name += name;
            sky_frame_name   += name;

            waterTrack[SETTING_KEYKFRAME] = time;
            waterTrack[SETTING_KEYNAME]   = water_frame_name;

            skyTrack[SETTING_KEYKFRAME] = time;
            skyTrack[SETTING_KEYNAME]   = sky_frame_name;

            frames[water_frame_name] = LLSettingsWater::defaults(time);
            frames[sky_frame_name]   = LLSettingsSky::defaults(time);

            time += FRAME_STEP;
        }

        LLSD tracks;
        tracks.append(LLSDArray(waterTrack));
        tracks.append(LLSDArray(skyTrack));

        dfltsetting[SETTING_TRACKS] = tracks;
        dfltsetting[SETTING_FRAMES] = frames;
    }

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

                LLSettingsBase::TrackPosition frame = elem[LLSettingsDay::SETTING_KEYKFRAME].asReal();
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

LLSettingsDay::CycleTrack_t& LLSettingsDay::getCycleTrack(S32 track)
{
    static CycleTrack_t emptyTrack;
    if (mDayTracks.size() <= track)
        return emptyTrack;

    return mDayTracks[track];
}

const LLSettingsDay::CycleTrack_t& LLSettingsDay::getCycleTrackConst(S32 track) const
{
    static CycleTrack_t emptyTrack;
    if (mDayTracks.size() <= track)
        return emptyTrack;

    return mDayTracks[track];
}

bool LLSettingsDay::clearCycleTrack(S32 track)
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to clear track (#" << track << ") out of range!" << LL_ENDL;
        return false;
    }
    mDayTracks[track].clear();
    clearAssetId();
    setDirtyFlag(true);
    return true;
}

bool LLSettingsDay::replaceCycleTrack(S32 track, const CycleTrack_t &source)
{
    if (source.empty())
    {
        LL_WARNS("DAYCYCLE") << "Attempt to copy an empty track." << LL_ENDL;
        return false;
    }

    {
        LLSettingsBase::ptr_t   first((*source.begin()).second);
        std::string setting_type = first->getSettingsType();

        if (((setting_type == "water") && (track != 0)) ||
            ((setting_type == "sky") && (track == 0)))
        {
            LL_WARNS("DAYCYCLE") << "Attempt to copy track missmatch" << LL_ENDL;
            return false;
        }
    }

    if (!clearCycleTrack(track))
        return false;

    mDayTracks[track] = source;
    return true;
}


bool LLSettingsDay::isTrackEmpty(S32 track) const
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to test track (#" << track << ") out of range!" << LL_ENDL;
        return true;
    }

    return mDayTracks[track].empty();
}

//=========================================================================
void LLSettingsDay::startDayCycle()
{
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

    for (CycleTrack_t::iterator it = track.begin(); it != track.end(); ++it)
    {
        keyframes.push_back((*it).first);
    }

    return keyframes;
}

bool LLSettingsDay::moveTrackKeyframe(S32 trackno, const LLSettingsBase::TrackPosition& old_frame, const LLSettingsBase::TrackPosition& new_frame)
{
    if ((trackno < 0) || (trackno >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt get track (#" << trackno << ") out of range!" << LL_ENDL;
        return false;
    }

    if (llabs(old_frame - new_frame) < F_APPROXIMATELY_ZERO)
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
        track[new_frame] = base;
        return true;
    }

    return false;

}

bool LLSettingsDay::removeTrackKeyframe(S32 trackno, const LLSettingsBase::TrackPosition& frame)
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

void LLSettingsDay::setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, const LLSettingsBase::TrackPosition& keyframe)
{
    setSettingsAtKeyframe(water, keyframe, TRACK_WATER);
}

LLSettingsWater::ptr_t LLSettingsDay::getWaterAtKeyframe(const LLSettingsBase::TrackPosition& keyframe) const
{
    LLSettingsBase* p = getSettingsAtKeyframe(keyframe, TRACK_WATER).get();
    return LLSettingsWater::ptr_t((LLSettingsWater*)p);
}

void LLSettingsDay::setSkyAtKeyframe(const LLSettingsSky::ptr_t &sky, const LLSettingsBase::TrackPosition& keyframe, S32 track)
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }

    setSettingsAtKeyframe(sky, keyframe, track);
}

LLSettingsSky::ptr_t LLSettingsDay::getSkyAtKeyframe(const LLSettingsBase::TrackPosition& keyframe, S32 track) const
{
    if ((track < 1) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set sky track (#" << track << ") out of range!" << LL_ENDL;
        return LLSettingsSky::ptr_t();
    }

    return PTR_NAMESPACE::dynamic_pointer_cast<LLSettingsSky>(getSettingsAtKeyframe(keyframe, track));
}

void LLSettingsDay::setSettingsAtKeyframe(const LLSettingsBase::ptr_t &settings, const LLSettingsBase::TrackPosition& keyframe, S32 track)
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to set track (#" << track << ") out of range!" << LL_ENDL;
        return;
    }

    std::string type = settings->getSettingsType();
    if ((track == TRACK_WATER) && (type != "water"))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to add frame of type '" << type << "' to water track!" << LL_ENDL;
        llassert(type == "water");
        return;
    }
    else if ((track != TRACK_WATER) && (type != "sky"))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to add frame of type '" << type << "' to sky track!" << LL_ENDL;
        llassert(type == "sky");
        return;
    }

    mDayTracks[track][llclamp(keyframe, 0.0f, 1.0f)] = settings;
    setDirtyFlag(true);
}

LLSettingsBase::ptr_t LLSettingsDay::getSettingsAtKeyframe(const LLSettingsBase::TrackPosition& keyframe, S32 track) const
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

LLSettingsDay::CycleTrack_t::value_type LLSettingsDay::getSettingsNearKeyframe(const LLSettingsBase::TrackPosition &keyframe, S32 track, F32 fudge) const
{
    if ((track < 0) || (track >= TRACK_MAX))
    {
        LL_WARNS("DAYCYCLE") << "Attempt to get track (#" << track << ") out of range!" << LL_ENDL;
        return CycleTrack_t::value_type(TrackPosition(INVALID_TRACKPOS), LLSettingsBase::ptr_t());
    }

    if (mDayTracks[track].empty())
    {
        LL_INFOS("DAYCYCLE") << "Empty track" << LL_ENDL;
        return CycleTrack_t::value_type(TrackPosition(INVALID_TRACKPOS), LLSettingsBase::ptr_t());
    }

    TrackPosition startframe(keyframe - fudge);
    if (startframe < 0.0f)
        startframe = 1.0f + startframe;

    LLSettingsDay::CycleTrack_t collection = const_cast<CycleTrack_t &>(mDayTracks[track]);
    CycleTrack_t::iterator it = get_wrapping_atafter(collection, startframe);

    F32 dist = get_wrapping_distance(startframe, (*it).first);

    CycleTrack_t::iterator next_it = std::next(it);
    if ((dist <= DEFAULT_MULTISLIDER_INCREMENT) && next_it != collection.end())
        return (*next_it);
    else if (dist <= (fudge * 2.0f))
        return (*it);

    return CycleTrack_t::value_type(TrackPosition(INVALID_TRACKPOS), LLSettingsBase::ptr_t());
}

LLSettingsBase::TrackPosition LLSettingsDay::getUpperBoundFrame(S32 track, const LLSettingsBase::TrackPosition& keyframe)
{
    return get_wrapping_atafter(mDayTracks[track], keyframe)->first;
}

LLSettingsBase::TrackPosition LLSettingsDay::getLowerBoundFrame(S32 track, const LLSettingsBase::TrackPosition& keyframe)
{
    return get_wrapping_atbefore(mDayTracks[track], keyframe)->first;
}

LLSettingsDay::TrackBound_t LLSettingsDay::getBoundingEntries(LLSettingsDay::CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe)
{
    return TrackBound_t(get_wrapping_atbefore(track, keyframe), get_wrapping_atafter(track, keyframe));
}

LLUUID LLSettingsDay::GetDefaultAssetId()
{
    return DEFAULT_ASSET_ID;
}

//=========================================================================
