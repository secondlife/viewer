/** 
 * @file lldockcontrol.cpp
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
				// assume that parent for all dockable flaoters
				// is the root view
				LLRect dockParentRect =
						mDockWidget->getRootView()->calcScreenRect();
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
		// unique docking used with dock tongue, so add tongue height to the Y coordinate
		if (use_tongue)
		{
			y += mDockTongue->getHeight();

			if ( y > rootRect.mTop)
			{
				y = rootRect.mTop;
			}
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
		// unique docking used with dock tongue, so add tongue height to the Y coordinate
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

	S32 max_available_height = rootRect.getHeight() - mDockTongueY - mDockTongue->getHeight();

	// A floater should be shrunk so it doesn't cover a part of its docking tongue and
	// there is a space between a dockable floater and a control to which it is docked.
	if (use_tongue && dockableRect.getHeight() >= max_available_height)
	{
		dockableRect.setLeftTopAndSize(x, y, dockableRect.getWidth(), max_available_height);
		mDockableFloater->reshape(dockableRect.getWidth(), dockableRect.getHeight());
	}
	else
	{
		// move dockable
		dockableRect.setLeftTopAndSize(x, y, dockableRect.getWidth(),
				dockableRect.getHeight());
	}
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

