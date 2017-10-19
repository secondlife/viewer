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

    static const S32            MINIMUM_DAYLENGTH;
    static const S32            MAXIMUM_DAYLENGTH;

    typedef boost::shared_ptr<LLSettingsDayCycle> ptr_t;

    //---------------------------------------------------------------------
    LLSettingsDayCycle(const LLSD &data);
    virtual ~LLSettingsDayCycle() { };

    static ptr_t    buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t    buildDefaultDayCycle();
    ptr_t           buildClone();

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const { return std::string("daycycle"); }

    // Settings status 
    ptr_t blend(const ptr_t &other, F32 mix) const;

    static LLSD defaults();

    //---------------------------------------------------------------------
    S32 getDayLength() const
    {
        return mSettings[SETTING_DAYLENGTH].asInteger();
    }

    void setDayLength(S32 seconds);

    void setWaterAt(const LLSettingsSkyPtr_t &water, S32 seconds);
    void setSkyAtOnTrack(const LLSettingsSkyPtr_t &water, S32 seconds, S32 track);
    //---------------------------------------------------------------------

protected:
    LLSettingsDayCycle();

    virtual void        updateSettings();

    typedef std::map<F32, LLSettingsBase::ptr_t>    CycleTrack_t;
    typedef std::vector<CycleTrack_t>               CycleList_t;
    typedef std::pair<CycleTrack_t::iterator, CycleTrack_t::iterator> TrackBound_t;

    CycleList_t mDayTracks;

    F32 secondsToOffset(S32 seconds);

private:
    
};

#endif
