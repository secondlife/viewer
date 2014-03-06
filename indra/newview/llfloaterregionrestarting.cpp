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

	const F32 SHAKE_INTERVAL = 0.025;
	const F32 SHAKE_TOTAL_DURATION = 1.8; // the length of the default alert tone for this
	const F32 SHAKE_INITIAL_MAGNITUDE = 1.5;
	const F32 SHAKE_HORIZONTAL_BIAS = 0.25;
	F32 time_shaking;
	
	if(SHAKE_START == sShakeState)
	{
			mShakeTimer.setTimerExpirySec(SHAKE_INTERVAL);
			sShakeState = SHAKE_LEFT;
			mShakeIterations = 0;
			mShakeMagnitude = SHAKE_INITIAL_MAGNITUDE;
	}

	if(SHAKE_DONE != sShakeState && mShakeTimer.hasExpired())
	{
		gAgentCamera.unlockView();

		switch(sShakeState)
		{
			case SHAKE_LEFT:
				gAgentCamera.setPanLeftKey(mShakeMagnitude * SHAKE_HORIZONTAL_BIAS);
				sShakeState = SHAKE_UP;
				break;

			case SHAKE_UP:
				gAgentCamera.setPanUpKey(mShakeMagnitude);
				sShakeState = SHAKE_RIGHT;
				break;

			case SHAKE_RIGHT:
				gAgentCamera.setPanRightKey(mShakeMagnitude * SHAKE_HORIZONTAL_BIAS);
				sShakeState = SHAKE_DOWN;
				break;

			case SHAKE_DOWN:
				gAgentCamera.setPanDownKey(mShakeMagnitude);
				mShakeIterations++;
				time_shaking = SHAKE_INTERVAL * (mShakeIterations * 4 /* left, up, right, down */);
				if(SHAKE_TOTAL_DURATION <= time_shaking)
				{
					sShakeState = SHAKE_DONE;
					mShakeMagnitude = 0.0;
				}
				else
				{
					sShakeState = SHAKE_LEFT;
					F32 percent_done_shaking = (SHAKE_TOTAL_DURATION - time_shaking) / SHAKE_TOTAL_DURATION;
					mShakeMagnitude = SHAKE_INITIAL_MAGNITUDE * (percent_done_shaking * percent_done_shaking); // exponential decay
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
