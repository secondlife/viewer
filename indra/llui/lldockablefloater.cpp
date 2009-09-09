/** 
 * @file lldockablefloater.cpp
 * @brief Creates a panel of a specific kind for a toast
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "lldockablefloater.h"

LLDockableFloater::LLDockableFloater(LLDockControl* dockControl,
		const LLSD& key, const Params& params) :
	LLFloater(key, params), mDockControl(dockControl)
{
}

LLDockableFloater::~LLDockableFloater()
{
}

BOOL LLDockableFloater::postBuild()
{
	mDockTongue = LLUI::getUIImage("windows/Flyout_Pointer.png");
	LLFloater::setDocked(true);
	return LLView::postBuild();
}

void LLDockableFloater::setDocked(bool docked, bool pop_on_undock)
{
	if (mDockControl.get() != NULL)
	{
		if (docked)
		{
			mDockControl.get()->on();
		}
		else
		{
			mDockControl.get()->off();
		}
	}

	if (!docked && pop_on_undock)
	{
		// visually pop up a little bit to emphasize the undocking
		translate(0, UNDOCK_LEAP_HEIGHT);
	}

	LLFloater::setDocked(docked, pop_on_undock);
}

void LLDockableFloater::draw()
{
	if (mDockControl.get() != NULL)
	{
		mDockControl.get()->repositionDockable();
		mDockControl.get()->drawToungue();
	}
	LLFloater::draw();
}

void LLDockableFloater::setDockControl(LLDockControl* dockControl)
{
	mDockControl.reset(dockControl);
}
const LLUIImagePtr& LLDockableFloater::getDockTongue()
{
	return mDockTongue;
}

LLDockControl* LLDockableFloater::getDockControl()
{
	return mDockControl.get();
}
