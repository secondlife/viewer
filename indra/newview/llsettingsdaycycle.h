/**
* @file llsettingsdaycycle.h
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

#ifndef LL_SETTINGS_DAYCYCLE_H
#define LL_SETTINGS_DAYCYCLE_H

#include "llsettingsbase.h"

class LLSettingsWater;
class LLSettingsSky;

typedef boost::shared_ptr<LLSettingsWater> LLSettingsWaterPtr_t;
typedef boost::shared_ptr<LLSettingsSky> LLSettingsSkyPtr_t;

class LLSettingsDayCycle : public LLSettingsBase
{
public:
    static const std::string    SETTING_DAYLENGTH;
    static const std::string    SETTING_KEYID;
    static const std::string    SETTING_KEYNAME;
    static const std::string    SETTING_KEYKFRAME;
    static const std::string    SETTING_NAME;
    static const std::string    SETTING_TRACKS;

    static const S64            MINIMUM_DAYLENGTH;
    static const S64            MAXIMUM_DAYLENGTH;

    static const S32            TRACK_WATER;
    static const S32            TRACK_MAX;

    typedef std::map<F32, LLSettingsBase::ptr_t>    CycleTrack_t;
    typedef std::vector<CycleTrack_t>               CycleList_t;
    typedef boost::shared_ptr<LLSettingsDayCycle>   ptr_t;
    typedef std::vector<S64Seconds>                 TimeList_t;
    typedef std::vector<F32>                        KeyframeList_t;
    typedef std::pair<CycleTrack_t::iterator, CycleTrack_t::iterator> TrackBound_t;

    //---------------------------------------------------------------------
    LLSettingsDayCycle(const LLSD &data);
    virtual ~LLSettingsDayCycle() { };

    static ptr_t    buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t    buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skys, LLSD water);
    static ptr_t    buildDefaultDayCycle();
    ptr_t           buildClone();

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const { return std::string("daycycle"); }

    // Settings status 
    virtual void blend(const LLSettingsBase::ptr_t &other, F32 mix);

    static LLSD defaults();

    //---------------------------------------------------------------------
    S64Seconds getDayLength() const
    {
        return S64Seconds(mSettings[SETTING_DAYLENGTH].asInteger());
    }

    void setDayLength(S64Seconds seconds);

    KeyframeList_t              getTrackKeyframes(S32 track);
    TimeList_t                  getTrackTimes(S32 track);

    void                        setWaterAtTime(const LLSettingsWaterPtr_t &water, S64Seconds seconds);
    void                        setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, F32 keyframe);

    void                        setSkyAtTime(const LLSettingsSkyPtr_t &sky, S64Seconds seconds, S32 track);
    void                        setSkyAtKeyframe(const LLSettingsSkyPtr_t &sky, F32 keyframe, S32 track);
        //---------------------------------------------------------------------
    void                        startDayCycle();

    LLSettingsSkyPtr_t          getCurrentSky() const
    {
        return mBlendedSky;
    }

    LLSettingsWaterPtr_t        getCurrentWater() const
    {
        return mBlendedWater;
    }

protected:
    LLSettingsDayCycle();

    virtual void                updateSettings();

private:
    LLSettingsBlender::ptr_t    mSkyBlender;    // convert to [] for altitudes 
    LLSettingsBlender::ptr_t    mWaterBlender;

    LLSettingsSkyPtr_t          mBlendedSky;
    LLSettingsWaterPtr_t        mBlendedWater;

    CycleList_t                 mDayTracks;

    bool                        mHasParsed;
    F64Seconds                  mLastUpdateTime;

    F32                         secondsToKeyframe(S64Seconds seconds);
    F64Seconds                  keyframeToSeconds(F32 keyframe);

    void                        parseFromLLSD(LLSD &data);

    static CycleTrack_t::iterator   getEntryAtOrBefore(CycleTrack_t &track, F32 keyframe);
    static CycleTrack_t::iterator   getEntryAtOrAfter(CycleTrack_t &track, F32 keyframe);

    TrackBound_t                getBoundingEntries(CycleTrack_t &track, F32 keyframe);
    TrackBound_t                getBoundingEntries(CycleTrack_t &track, F64Seconds time);

    void                        onSkyTransitionDone(S32 track, const LLSettingsBlender::ptr_t &blender);
    void                        onWaterTransitionDone(const LLSettingsBlender::ptr_t &blender);

};

#endif
