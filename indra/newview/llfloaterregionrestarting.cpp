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
#include "llagentcamera.h"
#include "llviewerwindow.h"

static S32 sSeconds;
static U32 sShakeState;

LLFloaterRegionRestarting::LLFloaterRegionRestarting(const LLSD& key) :
	LLFloater(key),
	LLEventTimer(1)
{
	mName = (std::string)key["NAME"];
	sSeconds = (LLSD::Integer)key["SECONDS"];
}

LLFloaterRegionRestarting::~LLFloaterRegionRestarting()
{
	mRegionChangedConnection.disconnect();
}

BOOL LLFloaterRegionRestarting::postBuild()
{
	mRegionChangedConnection = gAgent.addRegionChangedCallback(boost::bind(&LLFloaterRegionRestarting::regionChange, this));

	LLStringUtil::format_map_t args;
	std::string text;

	args["[NAME]"] = mName;
	text = getString("RegionName", args);
	LLTextBox* textbox = getChild<LLTextBox>("region_name");
	textbox->setValue(text);

	sShakeState = SHAKE_START;

	refresh();

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

	args["[SECONDS]"] = llformat("%d", sSeconds);
	getChild<LLTextBox>("restart_seconds")->setValue(getString("RestartSeconds", args));

	sSeconds = sSeconds - 1;
	if(sSeconds < 0.0)
	{
		sSeconds = 0;
	}
}

void LLFloaterRegionRestarting::draw()
{
	LLFloater::draw();

	const F32 SHAKE_INTERVAL = 0.03;
	const U32 SHAKE_ITERATIONS = 4;
	const F32 SHAKE_AMOUNT = 1.5;

	if(SHAKE_START == sShakeState)
	{
			mShakeTimer.setTimerExpirySec(SHAKE_INTERVAL);
			sShakeState = SHAKE_LEFT;
			mIterations = 0;
	}

	if(SHAKE_DONE != sShakeState && mShakeTimer.hasExpired())
	{
		gAgentCamera.unlockView();

		switch(sShakeState)
		{
			case SHAKE_LEFT:
				gAgentCamera.setPanLeftKey(SHAKE_AMOUNT);
				sShakeState = SHAKE_UP;
				break;

			case SHAKE_UP:
				gAgentCamera.setPanUpKey(SHAKE_AMOUNT);
				sShakeState = SHAKE_RIGHT;
				break;

			case SHAKE_RIGHT:
				gAgentCamera.setPanRightKey(SHAKE_AMOUNT);
				sShakeState = SHAKE_DOWN;
				break;

			case SHAKE_DOWN:
				gAgentCamera.setPanDownKey(SHAKE_AMOUNT);
				mIterations = mIterations + 1;
				if(SHAKE_ITERATIONS == mIterations)
				{
					sShakeState = SHAKE_DONE;
				}
				else
				{
					sShakeState = SHAKE_LEFT;
				}
				break;

			default:
				break;
		}
		mShakeTimer.setTimerExpirySec(SHAKE_INTERVAL);
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
	sSeconds = time;
	sShakeState = SHAKE_START;
}
