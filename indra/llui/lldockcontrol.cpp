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
#include "lldockablefloater.h"

LLDockControl::LLDockControl(LLView* dockWidget, LLFloater* dockableFloater,
		const LLUIImagePtr& dockTongue, DocAt dockAt, get_allowed_rect_callback_t get_allowed_rect_callback) :
		mDockWidget(dockWidget),
		mDockableFloater(dockableFloater),
		mDockTongue(dockTongue),
		mDockTongueX(0),
		mDockTongueY(0)
{
	mDockAt = dockAt;

	if (dockableFloater->isDocked())
	{
		on();
	}
	else
	{
		off();
	}

	if (!(get_allowed_rect_callback))
	{
		mGetAllowedRectCallback = boost::bind(&LLDockControl::getAllowedRect, this, _1);
	}
	else
	{
		mGetAllowedRectCallback = get_allowed_rect_callback;
	}

	if (dockWidget != NULL) 
	{
		repositionDockable();
	}

	if (mDockWidget != NULL)
	{
		mDockWidgetVisible = isDockVisible();
	}
	else
	{
		mDockWidgetVisible = false;
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
		mDockWidgetVisible = isDockVisible();
	}
	else
	{
		mDockWidgetVisible = false;
	}
}

void LLDockControl::getAllowedRect(LLRect& rect)
{
	rect = mDockableFloater->getRootView()->getRect();
}

void LLDockControl::repositionDockable()
{
	LLRect dockRect = mDockWidget->calcScreenRect();
	LLRect rootRect;
	mGetAllowedRectCallback(rootRect);

	// recalculate dockable position if dock position changed, dock visibility changed,
	// root view rect changed or recalculation is forced
	if (mPrevDockRect != dockRect  || mDockWidgetVisible != isDockVisible()
			|| mRootRect != rootRect || mRecalculateDocablePosition)
	{
		// undock dockable and off() if dock not visible
		if (!isDockVisible())
		{
			mDockableFloater->setDocked(false);
			// force off() since dockable may not have dockControll at this time
			off();
			LLDockableFloater* dockable_floater =
					dynamic_cast<LLDockableFloater*> (mDockableFloater);
			if(dockable_floater != NULL)
			{
				dockable_floater->onDockHidden();
			}
		}
		else
		{
			if(mEnabled)
			{
				moveDockable();
			}
			LLDockableFloater* dockable_floater =
					dynamic_cast<LLDockableFloater*> (mDockableFloater);
			if(dockable_floater != NULL)
			{
				dockable_floater->onDockShown();
			}
		}

		mPrevDockRect = dockRect;
		mRootRect = rootRect;
		mRecalculateDocablePosition = false;
		mDockWidgetVisible = isDockVisible();
	}
}

bool LLDockControl::isDockVisible()
{
	bool res = true;

	if (mDockWidget != NULL)
	{
		//we should check all hierarchy
		res = mDockWidget->isInVisibleChain();
		if (res)
		{
			LLRect dockRect = mDockWidget->calcScreenRect();

			switch (mDockAt)
			{
			case LEFT: // to keep compiler happy
				break;
			case BOTTOM:
			case TOP:
			{
				// check is dock inside parent rect
				LLRect dockParentRect =
						mDockWidget->getParent()->calcScreenRect();
				if (dockRect.mRight <= dockParentRect.mLeft
						|| dockRect.mLeft >= dockParentRect.mRight)
				{
					res = false;
				}
				break;
			}
			default:
				break;
			}
		}
	}

	return res;
}

void LLDockControl::moveDockable()
{
	// calculate new dockable position
	LLRect dockRect = mDockWidget->calcScreenRect();
	LLRect rootRect;
	mGetAllowedRectCallback(rootRect);

	bool use_tongue = false;
	LLDockableFloater* dockable_floater =
			dynamic_cast<LLDockableFloater*> (mDockableFloater);
	if (dockable_floater != NULL)
	{
		use_tongue = dockable_floater->getUseTongue();
	}

	LLRect dockableRect = mDockableFloater->calcScreenRect();
	S32 x = 0;
	S32 y = 0;
	LLRect dockParentRect;
	switch (mDockAt)
	{
	case LEFT:
		x = dockRect.mLeft;
		y = dockRect.mTop + mDockTongue->getHeight() + dockableRect.getHeight();
		// check is dockable inside root view rect
		if (x < rootRect.mLeft)
		{
			x = rootRect.mLeft;
		}
		if (x + dockableRect.getWidth() > rootRect.mRight)
		{
			x = rootRect.mRight - dockableRect.getWidth();
		}
		
		mDockTongueX = x + dockableRect.getWidth()/2 - mDockTongue->getWidth() / 2;
		
		mDockTongueY = dockRect.mTop;
		break;

	case TOP:
		x = dockRect.getCenterX() - dockableRect.getWidth() / 2;
		y = dockRect.mTop + dockableRect.getHeight();
		// unique docking used with dock tongue, so add tongue height o the Y coordinate
		if (use_tongue)
		{
			y += mDockTongue->getHeight();
		}

		// check is dockable inside root view rect
		if (x < rootRect.mLeft)
		{
			x = rootRect.mLeft;
		}
		if (x + dockableRect.getWidth() > rootRect.mRight)
		{
			x = rootRect.mRight - dockableRect.getWidth();
		}


		// calculate dock tongue position
		dockParentRect = mDockWidget->getParent()->calcScreenRect();
		if (dockRect.getCenterX() < dockParentRect.mLeft)
		{
			mDockTongueX = dockParentRect.mLeft - mDockTongue->getWidth() / 2;
		}
		else if (dockRect.getCenterX() > dockParentRect.mRight)
		{
			mDockTongueX = dockParentRect.mRight - mDockTongue->getWidth() / 2;;
		}
		else
		{
			mDockTongueX = dockRect.getCenterX() - mDockTongue->getWidth() / 2;
		}
		mDockTongueY = dockRect.mTop;

		break;
	case BOTTOM:
		x = dockRect.getCenterX() - dockableRect.getWidth() / 2;
		y = dockRect.mBottom;
		// unique docking used with dock tongue, so add tongue height o the Y coordinate
		if (use_tongue)
		{
			y -= mDockTongue->getHeight();
		}

		// check is dockable inside root view rect
		if (x < rootRect.mLeft)
		{
			x = rootRect.mLeft;
		}
		if (x + dockableRect.getWidth() > rootRect.mRight)
		{
			x = rootRect.mRight - dockableRect.getWidth();
		}

		// calculate dock tongue position
		dockParentRect = mDockWidget->getParent()->calcScreenRect();
		if (dockRect.getCenterX() < dockParentRect.mLeft)
		{
			mDockTongueX = dockParentRect.mLeft - mDockTongue->getWidth() / 2;
		}
		else if (dockRect.getCenterX() > dockParentRect.mRight)
		{
			mDockTongueX = dockParentRect.mRight - mDockTongue->getWidth() / 2;;
		}
		else
		{
			mDockTongueX = dockRect.getCenterX() - mDockTongue->getWidth() / 2;
		}
		mDockTongueY = dockRect.mBottom - mDockTongue->getHeight();

		break;
	}

	// move dockable
	dockableRect.setLeftTopAndSize(x, y, dockableRect.getWidth(),
			dockableRect.getHeight());
	LLRect localDocableParentRect;
	mDockableFloater->getParent()->screenRectToLocal(dockableRect,
			&localDocableParentRect);
	mDockableFloater->setRect(localDocableParentRect);

	mDockableFloater->screenPointToLocal(mDockTongueX, mDockTongueY,
			&mDockTongueX, &mDockTongueY);

}

void LLDockControl::on()
{
	 if (isDockVisible())
	{
		mEnabled = true;
		mRecalculateDocablePosition = true;
	}
}

void LLDockControl::off()
{
	mEnabled = false;
}

void LLDockControl::forceRecalculatePosition()
{
	mRecalculateDocablePosition = true;
}

void LLDockControl::drawToungue()
{
	bool use_tongue = false;
	LLDockableFloater* dockable_floater =
			dynamic_cast<LLDockableFloater*> (mDockableFloater);
	if (dockable_floater != NULL)
	{
		use_tongue = dockable_floater->getUseTongue();
	}

	if (mEnabled && use_tongue)
	{
		mDockTongue->draw(mDockTongueX, mDockTongueY);
	}
}

