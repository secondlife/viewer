/** 
 * @file llsavedsettingsglue.cpp
 * @author James Cook
 * @brief LLSavedSettingsGlue class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llsavedsettingsglue.h"

#include "lluictrl.h"

#include "llviewercontrol.h"

void LLSavedSettingsGlue::setBOOL(LLUICtrl* ctrl, const std::string& name)
{
    gSavedSettings.setBOOL(name, ctrl->getValue().asBoolean());
}

void LLSavedSettingsGlue::setS32(LLUICtrl* ctrl, const std::string& name)
{
    gSavedSettings.setS32(name, ctrl->getValue().asInteger());
}

void LLSavedSettingsGlue::setF32(LLUICtrl* ctrl, const std::string& name)
{
    gSavedSettings.setF32(name, (F32)ctrl->getValue().asReal());
}

void LLSavedSettingsGlue::setU32(LLUICtrl* ctrl, const std::string& name)
{
    gSavedSettings.setU32(name, (U32)ctrl->getValue().asInteger());
}

void LLSavedSettingsGlue::setString(LLUICtrl* ctrl, const std::string& name)
{
    gSavedSettings.setString(name, ctrl->getValue().asString());
}
