/** 
 * @file lldockablefloater.cpp
 * @brief Creates a panel of a specific kind for a toast
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "linden_common.h"

#include "lldockablefloater.h"
#include "llfloaterreg.h"

//static
LLHandle<LLFloater> LLDockableFloater::sInstanceHandle;

//static
void LLDockableFloater::init(LLDockableFloater* thiz)
{
	thiz->setDocked(thiz->mDockControl.get() != NULL
			&& thiz->mDockControl.get()->isDockVisible());
	thiz->resetInstance();

	// all dockable floaters should have close, dock and minimize buttons
	thiz->setCanClose(TRUE);
	thiz->setCanDock(true);
	thiz->setCanMinimize(TRUE);
	thiz->setOverlapsScreenChannel(false);
	thiz->mForceDocking = false;
}

LLDockableFloater::LLDockableFloater(LLDockControl* dockControl,
		const LLSD& key, const Params& params) :
	LLFloater(key, params), mDockControl(dockControl), mUniqueDocking(true)
{
	init(this);
	mUseTongue = true;
}

LLDockableFloater::LLDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
		const LLSD& key, const Params& params) :
	LLFloater(key, params), mDockControl(dockControl), mUniqueDocking(uniqueDocking)
{
	init(this);
	mUseTongue = true;
}

LLDockableFloater::LLDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
		bool useTongue, const LLSD& key, const Params& params) :
	LLFloater(key, params), mDockControl(dockControl), mUseTongue(useTongue), mUniqueDocking(uniqueDocking)
{
	init(this);
}

LLDockableFloater::~LLDockableFloater()
{
}

BOOL LLDockableFloater::postBuild()
{
	// Remember we should force docking when the floater is opened for the first time
	if (mIsDockedStateForcedCallback != NULL && mIsDockedStateForcedCallback())
	{
		mForceDocking = true;
	}

	mDockTongue = LLUI::getUIImage("Flyout_Pointer");
	LLFloater::setDocked(true);
	return LLView::postBuild();
}

//static
void LLDockableFloater::toggleInstance(const LLSD& sdname)
{
	LLSD key;
	std::string name = sdname.asString();

	LLDockableFloater* instance =
			dynamic_cast<LLDockableFloater*> (LLFloaterReg::findInstance(name));
	// if floater closed or docked
	if (instance == NULL || (instance && instance->isDocked()))
	{
		LLFloaterReg::toggleInstance(name, key);
		// restore button toggle state
		if (instance != NULL)
		{
			instance->storeVisibilityControl();
		}
	}
	// if floater undocked
	else if (instance != NULL)
	{
		instance->setMinimized(FALSE);
		if (instance->getVisible())
		{
			instance->setVisible(FALSE);
		}
		else
		{
			instance->setVisible(TRUE);
			gFloaterView->bringToFront(instance);
		}
	}
}

void LLDockableFloater::resetInstance()
{
	if (mUniqueDocking && sInstanceHandle.get() != this)
	{
		if (sInstanceHandle.get() != NULL && sInstanceHandle.get()->isDocked())
		{
			sInstanceHandle.get()->setVisible(FALSE);
		}
		sInstanceHandle = getHandle();
	}
}

void LLDockableFloater::setVisible(BOOL visible)
{
	// Force docking if requested
	if (visible && mForceDocking)
	{
		setCanDock(true);
		setDocked(true);
		mForceDocking = false;
	}

	if(visible && isDocked())
	{
		resetInstance();
	}

	if (visible && mDockControl.get() != NULL)
	{
		mDockControl.get()->repositionDockable();
	}

	if (visible)
	{
		LLFloater::setFrontmost(getAutoFocus());
	}
	LLFloater::setVisible(visible);
}

void LLDockableFloater::setMinimized(BOOL minimize)
{
	if(minimize && isDocked())
	{
		// minimizing a docked floater just hides it
		setVisible(FALSE);
	}
	else
	{
		LLFloater::setMinimized(minimize);
	}
}

LLView * LLDockableFloater::getDockWidget()
{
	LLView * res = NULL;
	if (getDockControl() != NULL) {
		res = getDockControl()->getDock();
	}

	return res;
}

void LLDockableFloater::onDockHidden()
{
	setCanDock(FALSE);
}

void LLDockableFloater::onDockShown()
{
	if (!isMinimized())
	{
		setCanDock(TRUE);
	}
}

void LLDockableFloater::setDocked(bool docked, bool pop_on_undock)
{
	if (mDockControl.get() != NULL && mDockControl.get()->isDockVisible())
	{
		if (docked)
		{
			resetInstance();
			mDockControl.get()->on();
		}
		else
		{
			mDockControl.get()->off();
		}

		if (!docked && pop_on_undock)
		{
			// visually pop up a little bit to emphasize the undocking
			translate(0, UNDOCK_LEAP_HEIGHT);
		}
	}

	LLFloater::setDocked(docked, pop_on_undock);
}

void LLDockableFloater::draw()
{
	if (mDockControl.get() != NULL)
	{
		mDockControl.get()->repositionDockable();
		if (isDocked())
		{
			mDockControl.get()->drawToungue();
		}
	}
	LLFloater::draw();
}

void LLDockableFloater::setDockControl(LLDockControl* dockControl)
{
	mDockControl.reset(dockControl);
	setDocked(isDocked());
}

const LLUIImagePtr& LLDockableFloater::getDockTongue(LLDockControl::DocAt dock_side)
{
	switch(dock_side)
	{
	case LLDockControl::LEFT:
		mDockTongue = LLUI::getUIImage("Flyout_Left");
		break;
	case LLDockControl::RIGHT:
		mDockTongue = LLUI::getUIImage("Flyout_Right");
		break;
	default:
		mDockTongue = LLUI::getUIImage("Flyout_Pointer");
		break;
	}

	return mDockTongue;
}

LLDockControl* LLDockableFloater::getDockControl()
{
	return mDockControl.get();
}
