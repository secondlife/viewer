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
typedef PTR_NAMESPACE::shared_ptr<LLSettingsWater> LLSettingsWaterPtr_t;
typedef PTR_NAMESPACE::shared_ptr<LLSettingsSky> LLSettingsSkyPtr_t;

class LLSettingsDay : public LLSettingsBase
{
public:
    // 32-bit as LLSD only supports that width at present
    typedef S32Seconds Seconds;

    static const std::string    SETTING_KEYID;
    static const std::string    SETTING_KEYNAME;
    static const std::string    SETTING_KEYKFRAME;
    static const std::string    SETTING_KEYHASH;
    static const std::string    SETTING_TRACKS;
    static const std::string    SETTING_FRAMES;

    static const Seconds MINIMUM_DAYLENGTH;
    static const Seconds DEFAULT_DAYLENGTH;
    static const Seconds MAXIMUM_DAYLENGTH;

    static const Seconds MINIMUM_DAYOFFSET;
    static const Seconds DEFAULT_DAYOFFSET;
    static const Seconds MAXIMUM_DAYOFFSET;

    static const S32     TRACK_WATER;
    static const S32     TRACK_GROUND_LEVEL;
    static const S32     TRACK_MAX;
    static const S32     FRAME_MAX;

    static const F32     DEFAULT_FRAME_SLOP_FACTOR;

    static const LLUUID DEFAULT_ASSET_ID;

    typedef std::map<LLSettingsBase::TrackPosition, LLSettingsBase::ptr_t>  CycleTrack_t;
    typedef std::vector<CycleTrack_t>                                       CycleList_t;
    typedef PTR_NAMESPACE::shared_ptr<LLSettingsDay>                        ptr_t;
    typedef PTR_NAMESPACE::weak_ptr<LLSettingsDay>                          wptr_t;
    typedef std::vector<LLSettingsBase::TrackPosition>                      KeyframeList_t;
    typedef std::pair<CycleTrack_t::iterator, CycleTrack_t::iterator>       TrackBound_t;

    //---------------------------------------------------------------------
    LLSettingsDay(const LLSD &data);
    virtual ~LLSettingsDay() { };

    bool                        initialize(bool validate_frames = false);

    virtual ptr_t               buildClone() const = 0;
    virtual ptr_t               buildDeepCloneAndUncompress() const = 0;
    virtual LLSD                getSettings() const SETTINGS_OVERRIDE;
    virtual LLSettingsType::type_e  getSettingsTypeValue() const SETTINGS_OVERRIDE { return LLSettingsType::ST_DAYCYCLE; }


    //---------------------------------------------------------------------
    virtual std::string         getSettingsType() const SETTINGS_OVERRIDE { return std::string("daycycle"); }

    // Settings status 
    virtual void                blend(const LLSettingsBase::ptr_t &other, F64 mix) SETTINGS_OVERRIDE;

    static LLSD                 defaults();

    //---------------------------------------------------------------------
    KeyframeList_t              getTrackKeyframes(S32 track);
    bool                        moveTrackKeyframe(S32 track, const LLSettingsBase::TrackPosition& old_frame, const LLSettingsBase::TrackPosition& new_frame);
    bool                        removeTrackKeyframe(S32 track, const LLSettingsBase::TrackPosition& frame);

    void                        setWaterAtKeyframe(const LLSettingsWaterPtr_t &water, const LLSettingsBase::TrackPosition& keyframe);
    LLSettingsWaterPtr_t        getWaterAtKeyframe(const LLSettingsBase::TrackPosition& keyframe) const;
    void                        setSkyAtKeyframe(const LLSettingsSkyPtr_t &sky, const LLSettingsBase::TrackPosition& keyframe, S32 track);
    LLSettingsSkyPtr_t          getSkyAtKeyframe(const LLSettingsBase::TrackPosition& keyframe, S32 track) const;
    void                        setSettingsAtKeyframe(const LLSettingsBase::ptr_t &settings, const LLSettingsBase::TrackPosition& keyframe, S32 track);
    LLSettingsBase::ptr_t       getSettingsAtKeyframe(const LLSettingsBase::TrackPosition& keyframe, S32 track) const;
    CycleTrack_t::value_type    getSettingsNearKeyframe(const LLSettingsBase::TrackPosition &keyframe, S32 track, F32 fudge) const;

        //---------------------------------------------------------------------
    void                        startDayCycle();

    virtual LLSettingsSkyPtr_t  getDefaultSky() const = 0;
    virtual LLSettingsWaterPtr_t getDefaultWater() const = 0;

    virtual LLSettingsSkyPtr_t  buildSky(LLSD) const = 0;
    virtual LLSettingsWaterPtr_t buildWater(LLSD) const = 0;

    void                        setInitialized(bool value = true) { mInitialized = value; }
    CycleTrack_t &              getCycleTrack(S32 track);
    const CycleTrack_t &        getCycleTrackConst(S32 track) const;
    bool                        clearCycleTrack(S32 track);
    bool                        replaceCycleTrack(S32 track, const CycleTrack_t &source);
    bool                        isTrackEmpty(S32 track) const;

    virtual validation_list_t   getValidationList() const SETTINGS_OVERRIDE;
    static validation_list_t    validationList();

    virtual LLSettingsBase::ptr_t buildDerivedClone() const SETTINGS_OVERRIDE { return buildClone(); }
	
    LLSettingsBase::TrackPosition getUpperBoundFrame(S32 track, const LLSettingsBase::TrackPosition& keyframe);
    LLSettingsBase::TrackPosition getLowerBoundFrame(S32 track, const LLSettingsBase::TrackPosition& keyframe);

    static LLUUID GetDefaultAssetId();

protected:
    LLSettingsDay();

    virtual void                updateSettings() SETTINGS_OVERRIDE;

    bool                        mInitialized;

private:
    CycleList_t                 mDayTracks;

    LLSettingsBase::Seconds     mLastUpdateTime;

    void                        parseFromLLSD(LLSD &data);

    static CycleTrack_t::iterator   getEntryAtOrBefore(CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe);
    static CycleTrack_t::iterator   getEntryAtOrAfter(CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe);
    TrackBound_t                    getBoundingEntries(CycleTrack_t &track, const LLSettingsBase::TrackPosition& keyframe);
};

#endif
