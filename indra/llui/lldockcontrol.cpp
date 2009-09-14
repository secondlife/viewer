/** 
 * @file lldockcontrol.cpp
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

#include "lldockcontrol.h"

LLDockControl::LLDockControl(LLView* dockWidget, LLFloater* dockableFloater,
		const LLUIImagePtr& dockTongue, DocAt dockAt, bool enabled) :
	mDockWidget(dockWidget), mDockableFloater(dockableFloater), mDockTongue(
			dockTongue)
{
	mDockAt = dockAt;
	if (enabled)
	{
		on();
	}
	else
	{
		off();
	}

	if (dockWidget != NULL) {
		repositionDockable();
	}
}

LLDockControl::~LLDockControl()
{
}

void LLDockControl::setDock(LLView* dockWidget)
{
	mDockWidget = dockWidget;
	if (mDockWidget != NULL)
	{
		repositionDockable();
	}
}

void LLDockControl::repositionDockable()
{
	if (mEnabled)
	{
		calculateDockablePosition();
	}
}

void LLDockControl::calculateDockablePosition()
{
	LLRect dockRect = mDockWidget->calcScreenRect();
	LLRect rootRect = mDockableFloater->getRootView()->getRect();

	// recalculate dockable position if dock position changed
	// or root view rect changed or recalculation is forced
	if (mPrevDockRect != dockRect || mRootRect != rootRect
			|| mRecalculateDocablePosition)
	{
		LLRect dockableRect = mDockableFloater->calcScreenRect();
		S32 x = 0;
		S32 y = 0;
		switch (mDockAt)
		{
		case TOP:
			x = dockRect.getCenterX() - dockableRect.getWidth() / 2;
			y = dockRect.mTop + mDockTongue->getHeight()
					+ dockableRect.getHeight();
			if (x < rootRect.mLeft)
			{
				x = rootRect.mLeft;
			}
			if (x + dockableRect.getWidth() > rootRect.mRight)
			{
				x = rootRect.mRight - dockableRect.getWidth();
			}
			mDockTongueX = dockRect.getCenterX() - mDockTongue->getWidth() / 2;
			mDockTongueY = dockRect.mTop;
			break;
		}
		dockableRect.setLeftTopAndSize(x, y, dockableRect.getWidth(),
				dockableRect.getHeight());
		LLRect localDocableParentRect;
		mDockableFloater->getParent()->screenRectToLocal(dockableRect,
				&localDocableParentRect);
		mDockableFloater->setRect(localDocableParentRect);

		mDockableFloater->screenPointToLocal(mDockTongueX, mDockTongueY,
				&mDockTongueX, &mDockTongueY);
		mPrevDockRect = dockRect;
		mRootRect = rootRect;
		mRecalculateDocablePosition = false;
	}
}

void LLDockControl::on()
{
	mDockableFloater->setCanDrag(false);
	mEnabled = true;
	mRecalculateDocablePosition = true;
}

void LLDockControl::off()
{
	mDockableFloater->setCanDrag(true);
	mEnabled = false;
}

void LLDockControl::drawToungue()
{
	if (mEnabled)
	{
		mDockTongue->draw(mDockTongueX, mDockTongueY);
	}
}
