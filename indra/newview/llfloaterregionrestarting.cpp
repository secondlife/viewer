/** 
 * @file llfloaterregionrestarting.cpp
 * @brief Shows countdown timer during region restart
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

#include "llfloaterregionrestarting.h"

#include "llfloaterreg.h"
#include "lluictrl.h"
#include "llagent.h"

static S32 mSeconds;

LLFloaterRegionRestarting::LLFloaterRegionRestarting(const LLSD& key) :
	LLFloater(key),
	LLEventTimer(1)
{
	mName = (std::string)key["NAME"];
	mSeconds = (LLSD::Integer)key["SECONDS"];
}

LLFloaterRegionRestarting::~LLFloaterRegionRestarting()
{
	mRegionChangedConnection.disconnect();
}

BOOL LLFloaterRegionRestarting::postBuild()
{
	LLStringUtil::format_map_t args;
	std::string text;

	args["[NAME]"] = mName;
	text = getString("RegionName", args);
	LLTextBox* textbox = getChild<LLTextBox>("region_name");
	textbox->setValue(text);

	refresh();

	mRegionChangedConnection = gAgent.addRegionChangedCallback(boost::bind(&LLFloaterRegionRestarting::regionChange, this));

	return TRUE;
}

void LLFloaterRegionRestarting::regionChange()
{
	close();
}

BOOL LLFloaterRegionRestarting::tick()
{
	refresh();

	return FALSE;
}

void LLFloaterRegionRestarting::refresh()
{
	LLStringUtil::format_map_t args;
	std::string text;

	args["[SECONDS]"] = llformat("%d", mSeconds);
	getChild<LLTextBox>("restart_seconds")->setValue(getString("RestartSeconds", args));

	mSeconds = mSeconds - 1;
	if(mSeconds < 0.0)
	{
		mSeconds = 0;
	}
}

void LLFloaterRegionRestarting::close()
{
	LLFloaterRegionRestarting* floaterp = LLFloaterReg::findTypedInstance<LLFloaterRegionRestarting>("region_restarting");

	if (floaterp)
	{
		floaterp->closeFloater();
	}
}

void LLFloaterRegionRestarting::updateTime(S32 time)
{
	mSeconds = time;
}
