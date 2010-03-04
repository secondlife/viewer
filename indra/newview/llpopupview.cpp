/** 
 * @file llpopupview.cpp
 * @brief Holds transient popups
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2010, Linden Research, Inc.
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

#include "llpopupview.h"

static LLRegisterPanelClassWrapper<LLPopupView> r("popup_holder");

bool view_visible_and_enabled(LLView* viewp)
{
	return viewp->getVisible() && viewp->getEnabled();
}

bool view_visible(LLView* viewp)
{
	return viewp->getVisible();
}


LLPopupView::LLPopupView()
{
}

void LLPopupView::draw()
{
	S32 screen_x, screen_y;

	// remove dead popups
	for (popup_list_t::iterator popup_it = mPopups.begin();
		popup_it != mPopups.end();)
	{
		if (!popup_it->get())
		{
			mPopups.erase(popup_it++);
		}
		else
		{
			popup_it++;
		}
	}

	// draw in reverse order (most recent is on top)
	for (popup_list_t::reverse_iterator popup_it = mPopups.rbegin();
		popup_it != mPopups.rend();)
	{
		LLView* popup = popup_it->get();

		if (popup->getVisible())
		{
			popup->localPointToScreen(0, 0, &screen_x, &screen_y);

			LLUI::pushMatrix();
			{
				LLUI::translate( (F32) screen_x, (F32) screen_y, 0.f);
				popup->draw();
			}
			LLUI::popMatrix();
		}
		++popup_it;
	}

	LLPanel::draw();
}

BOOL LLPopupView::handleMouseEvent(boost::function<BOOL(LLView*, S32, S32)> func, 
								   boost::function<bool(LLView*)> predicate, 
								   S32 x, S32 y,
								   bool close_popups)
{
	for (popup_list_t::iterator popup_it = mPopups.begin();
		popup_it != mPopups.end();)
	{
		LLView* popup = popup_it->get();
		if (!popup 
			|| !predicate(popup))
		{
			++popup_it;
			continue;
		}

		S32 popup_x, popup_y;
		if (localPointToOtherView(x, y, &popup_x, &popup_y, popup) 
			&& popup->pointInView(popup_x, popup_y))
		{
			if (func(popup, popup_x, popup_y))
			{
				return TRUE;
			}
		}

		if (close_popups)
		{
			popup_list_t::iterator cur_popup_it = popup_it++;
			mPopups.erase(cur_popup_it);
			popup->onTopLost();
		}
		else
		{
			++popup_it;
		}
	}

	return FALSE;
}


BOOL LLPopupView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!handleMouseEvent(boost::bind(&LLMouseHandler::handleMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL LLPopupView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!handleMouseEvent(boost::bind(&LLMouseHandler::handleMiddleMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL LLPopupView::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleMiddleMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (!handleMouseEvent(boost::bind(&LLMouseHandler::handleRightMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL LLPopupView::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleRightMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleDoubleClick, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleHover(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleHover, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleScrollWheel, _1, _2, _3, clicks), view_visible_and_enabled, x, y, false);
}

BOOL LLPopupView::handleToolTip(S32 x, S32 y, MASK mask)
{
	return handleMouseEvent(boost::bind(&LLMouseHandler::handleToolTip, _1, _2, _3, mask), view_visible, x, y, false);
}

void LLPopupView::addPopup(LLView* popup)
{
	if (popup)
	{
		mPopups.erase(std::find(mPopups.begin(), mPopups.end(), popup->getHandle()));
		mPopups.push_front(popup->getHandle());
	}
}

void LLPopupView::removePopup(LLView* popup)
{
	if (popup)
	{
		if (gFocusMgr.childHasKeyboardFocus(popup))
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}
		popup->onTopLost();
		mPopups.erase(std::find(mPopups.begin(), mPopups.end(), popup->getHandle()));
	}
}

void LLPopupView::clearPopups()
{
	for (popup_list_t::iterator popup_it = mPopups.begin();
		popup_it != mPopups.end();)
	{
		LLView* popup = popup_it->get();

		popup_list_t::iterator cur_popup_it = popup_it;
		++popup_it;

		mPopups.erase(cur_popup_it);
		popup->onTopLost();
	}
}

