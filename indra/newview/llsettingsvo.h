/**
* @file llsettingsvo.h
* @author Rider Linden
* @brief Subclasses for viewer specific settings behaviors.
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

#ifndef LL_SETTINGS_VO_H
#define LL_SETTINGS_VO_H

#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llsettingswater.h"
#include "llsettingsdaycycle.h"

class LLSettingsVOSky : public LLSettingsSky
{
public:
    LLSettingsVOSky(const LLSD &data);

    static ptr_t    buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t    buildDefaultSky();
    virtual ptr_t   buildClone();

protected:
    LLSettingsVOSky();

    virtual void updateSettings();

    virtual void        applySpecial(void *);

    virtual parammapping_t getParameterMap() const;

};

//=========================================================================
class LLSettingsVOWater : public LLSettingsWater
{
public:
    LLSettingsVOWater(const LLSD &data);

    static ptr_t    buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t    buildDefaultWater();
    virtual ptr_t   buildClone();

protected:
    LLSettingsVOWater();

    virtual void updateSettings();
    virtual void applySpecial(void *);

    virtual parammapping_t getParameterMap() const;


private:
    static const F32 WATER_FOG_LIGHT_CLAMP;

};

//=========================================================================
class LLSettingsVODay : public LLSettingsDay
{
public:
    LLSettingsVODay(const LLSD &data);

    static ptr_t    buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings);
    static ptr_t    buildFromLegacyMessage(const LLUUID &regionId, LLSD daycycle, LLSD skys, LLSD water);
    static ptr_t    buildDefaultDayCycle();
    virtual ptr_t   buildClone();

    virtual LLSettingsSkyPtr_t      getDefaultSky() const;
    virtual LLSettingsWaterPtr_t    getDefaultWater() const;
    virtual LLSettingsSkyPtr_t      buildSky(LLSD) const;
    virtual LLSettingsWaterPtr_t    buildWater(LLSD) const;
    virtual LLSettingsSkyPtr_t      getNamedSky(const std::string &) const;
    virtual LLSettingsWaterPtr_t    getNamedWater(const std::string &) const;

protected:
    LLSettingsVODay();
};


#endif
