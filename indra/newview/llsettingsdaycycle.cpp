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

//=========================================================================
namespace
{
    LLTrace::BlockTimerStatHandle FTM_BLEND_WATERVALUES("Blending Water Environment");
    LLTrace::BlockTimerStatHandle FTM_UPDATE_WATERVALUES("Update Water Environment");
}

//=========================================================================


//=========================================================================
LLSettingsDayCycle::LLSettingsDayCycle(const LLSD &data) :
    LLSettingsBase(data)
{
}

LLSettingsDayCycle::LLSettingsDayCycle() :
    LLSettingsBase()
{
}

//=========================================================================
LLSD LLSettingsDayCycle::defaults()
{
    LLSD dfltsetting;

    
    return dfltsetting;
}


LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildFromLegacyPreset(const std::string &name, const LLSD &oldsettings)
{
    LLSD newsettings(defaults());

    //newsettings[SETTING_NAME] = name;



    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(newsettings);

    return dayp;
}

LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildDefaultDayCycle()
{
    LLSD settings = LLSettingsDayCycle::defaults();

    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(settings);

    return dayp;
}

LLSettingsDayCycle::ptr_t LLSettingsDayCycle::buildClone()
{
    LLSD settings = cloneSettings();

    LLSettingsDayCycle::ptr_t dayp = boost::make_shared<LLSettingsDayCycle>(settings);

    return dayp;
}

//=========================================================================
void LLSettingsDayCycle::updateSettings()
{

}

//=========================================================================
