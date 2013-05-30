/** 
 * @file lltoggleablemenu.cpp
 * @brief Menu toggled by a button press
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "lltoggleablemenu.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLToggleableMenu> r("toggleable_menu");

LLToggleableMenu::LLToggleableMenu(const LLToggleableMenu::Params& p)
:	LLMenuGL(p),
	mButtonRect(),
	mVisibilityChangeSignal(NULL),
	mClosedByButtonClick(false)
{
}

LLToggleableMenu::~LLToggleableMenu()
{
	delete mVisibilityChangeSignal;
}

boost::signals2::connection LLToggleableMenu::setVisibilityChangeCallback(const commit_signal_t::slot_type& cb)
{
	if (!mVisibilityChangeSignal) mVisibilityChangeSignal = new commit_signal_t();
	return mVisibilityChangeSignal->connect(cb);
}

// virtual
void LLToggleableMenu::handleVisibilityChange (BOOL curVisibilityIn)
{
	S32 x,y;
	LLUI::getMousePositionLocal(LLUI::getRootView(), &x, &y);

	// STORM-1879: also check MouseCapture to see if the button was really
        // clicked (otherwise the VisibilityChange was triggered via keyboard shortcut)
	if (!curVisibilityIn && mButtonRect.pointInRect(x, y) && gFocusMgr.getMouseCapture())
	{
		mClosedByButtonClick = true;
	}

	if (mVisibilityChangeSignal)
	{
		(*mVisibilityChangeSignal)(this,
				LLSD().with("visibility", curVisibilityIn).with("closed_by_button_click", mClosedByButtonClick));
	}
}

void LLToggleableMenu::setButtonRect(const LLRect& rect, LLView* current_view)
{
	LLRect screen;
	current_view->localRectToScreen(rect, &screen);
	mButtonRect = screen;
}

void LLToggleableMenu::setButtonRect(LLView* current_view)
{
	LLRect rect = current_view->getLocalRect();
	setButtonRect(rect, current_view);
}

bool LLToggleableMenu::toggleVisibility() 
{
	if (mClosedByButtonClick)
	{
		mClosedByButtonClick = false;
		return false;
	}

	if (getVisible())
	{
		setVisible(FALSE);
		mClosedByButtonClick = false;
		return false;
	}

	return true;
}

bool LLToggleableMenu::addChild(LLView* view, S32 tab_group)
{
	return addContextChild(view, tab_group);
}
