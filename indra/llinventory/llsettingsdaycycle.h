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

// These are alias for LLSettingsWater::ptr_t and LLSettingsSky::ptr_t respectively.
// Here for definitions only.
typedef std::shared_ptr<LLSettingsWater> LLSettingsWaterPtr_t;
typedef std::shared_ptr<LLSettingsSky> LLSettingsSkyPtr_t;

class LLSettingsDay : public LLSettingsBase
{
public:
    static const std::string    SETTING_KEYID;
    static const std::string    SETTING_KEYNAME;
    static const std::string    SETTING_KEYKFRAME;
    static const std::string    SETTING_KEYHASH;
    static const std::string    SETTING_TRACKS;
    static const std::string    SETTING_FRAMES;

    static const S64Seconds     MINIMUM_DAYLENGTH;
    static const S64Seconds     DEFAULT_DAYLENGTH;
    static const S64Seconds     MAXIMUM_DAYLENGTH;

    static const S64Seconds     MINIMUM_DAYOFFSET;
    static const S64Seconds     DEFAULT_DAYOFFSET;
    static const S64Seconds     MAXIMUM_DAYOFFSET;

    static const S32            TRACK_WATER;
    static const S32            TRACK_MAX;
    static const S32            FRAME_MAX;

    typedef std::map<F32, LLSettingsBase::ptr_t>    CycleTrack_t;
    typedef std::vector<CycleTrack_t>               CycleList_t;
    typedef std::shared_ptr<LLSettingsDay>          ptr_t;
    typedef std::vector<F32>                        KeyframeList_t;
    typedef std::pair<CycleTrack_t::iterator, CycleTrack_t::iterator> TrackBound_t;

    //---------------------------------------------------------------------
    LLSettingsDay(const LLSD &data);
    virtual ~LLSettingsDay() { };

    bool                        initialize();

    virtual ptr_t               buildClone() = 0;
    virtual LLSD                getSettings() const override;
    virtual LLSettingsType::type_e  getSettingTypeValue() const override { return LLSettingsType::ST_DAYCYCLE; }


    //---------------------------------------------------------------------
    virtual std::string         getSettingType() const override { return std::string("daycycle"); }

    // Settings status 
    virtual void                blend(const LLSettingsBase::ptr_t &other, F64 mix) override;

    static LLSD                 defaults();

    //---------------------------------------------------------------------
    KeyframeList_t              getTrackKeyframes(S32 track);
    bool                        moveTrackKeyframe(S32 track, F32 old_frame, F32 new_frame);
    bool                        removeTrackKeyframe(S32 track, F32 frame);

    void                        setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, F32 keyframe);
    LLSettingsWaterPtr_t        getWaterAtKeyframe(F32 keyframe) const;
    void                        setSkyAtKeyframe(const LLSettingsSkyPtr_t &sky, F32 keyframe, S32 track);
    LLSettingsSkyPtr_t          getSkyAtKeyframe(F32 keyframe, S32 track) const;
    void                        setSettingsAtKeyframe(const LLSettingsBase::ptr_t &settings, F32 keyframe, S32 track);
    LLSettingsBase::ptr_t       getSettingsAtKeyframe(F32 keyframe, S32 track) const;
        //---------------------------------------------------------------------
    void                        startDayCycle();

    virtual LLSettingsSkyPtr_t  getDefaultSky() const = 0;
    virtual LLSettingsWaterPtr_t getDefaultWater() const = 0;

    virtual LLSettingsSkyPtr_t  buildSky(LLSD) const = 0;
    virtual LLSettingsWaterPtr_t buildWater(LLSD) const = 0;

    virtual LLSettingsSkyPtr_t  getNamedSky(const std::string &) const = 0;
    virtual LLSettingsWaterPtr_t getNamedWater(const std::string &) const = 0;

    void    setInitialized(bool value = true) { mInitialized = value; }
    CycleTrack_t &              getCycleTrack(S32 track);

    virtual validation_list_t   getValidationList() const override;
    static validation_list_t    validationList();

protected:
    LLSettingsDay();

    virtual void                updateSettings() override;

    bool                        mInitialized;

private:
    CycleList_t                 mDayTracks;

    F64Seconds                  mLastUpdateTime;

    void                        parseFromLLSD(LLSD &data);

    static CycleTrack_t::iterator   getEntryAtOrBefore(CycleTrack_t &track, F32 keyframe);
    static CycleTrack_t::iterator   getEntryAtOrAfter(CycleTrack_t &track, F32 keyframe);

    TrackBound_t                getBoundingEntries(CycleTrack_t &track, F32 keyframe);

//     void                        onSkyTransitionDone(S32 track, const LLSettingsBlender::ptr_t &blender);
//     void                        onWaterTransitionDone(const LLSettingsBlender::ptr_t &blender);

};

#endif
