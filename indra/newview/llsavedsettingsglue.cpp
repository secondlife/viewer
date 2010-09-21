/** 
 * @file llsavedsettingsglue.cpp
 * @author James Cook
 * @brief LLSavedSettingsGlue class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
