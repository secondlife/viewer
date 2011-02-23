/** 
 * @file llpopupview.cpp
 * @brief Holds transient popups
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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


LLPopupView::LLPopupView(const LLPopupView::Params& p)
: LLPanel(p)
{
	// register ourself as handler of UI popups
	LLUI::setPopupFuncs(boost::bind(&LLPopupView::addPopup, this, _1), boost::bind(&LLPopupView::removePopup, this, _1), boost::bind(&LLPopupView::clearPopups, this));
}

LLPopupView::~LLPopupView()
{
	// set empty callback function so we can't handle popups anymore
	LLUI::setPopupFuncs(LLUI::add_popup_t(), LLUI::remove_popup_t(), LLUI::clear_popups_t());
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
	BOOL handled = FALSE;

	// make a copy of list of popups, in case list is modified during mouse event handling
	popup_list_t popups(mPopups);
	for (popup_list_t::iterator popup_it = popups.begin(), popup_end = popups.end();
		popup_it != popup_end;
		++popup_it)
	{
		LLView* popup = popup_it->get();
		if (!popup 
			|| !predicate(popup))
		{
			continue;
		}

		S32 popup_x, popup_y;
		if (localPointToOtherView(x, y, &popup_x, &popup_y, popup) 
			&& popup->pointInView(popup_x, popup_y))
		{
			if (func(popup, popup_x, popup_y))
			{
				handled = TRUE;
				break;
			}
		}

		if (close_popups)
		{
			mPopups.remove(*popup_it);
			popup->onTopLost();
		}
	}

	return handled;
}


BOOL LLPopupView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true);
	if (!handled)
	{
		handled = LLPanel::handleMouseDown(x, y, mask);
	}
	return handled;
}

BOOL LLPopupView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleMouseUp(x, y, mask);
	}
	return handled;
}

BOOL LLPopupView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleMiddleMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true);
	if (!handled)
	{
		handled = LLPanel::handleMiddleMouseDown(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleMiddleMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleMiddleMouseUp(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleRightMouseDown, _1, _2, _3, mask), view_visible_and_enabled, x, y, true);
	if (!handled)
	{
		handled = LLPanel::handleRightMouseDown(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleRightMouseUp, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleRightMouseUp(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleDoubleClick, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleDoubleClick(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleHover, _1, _2, _3, mask), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleHover(x, y, mask);
	}
	return handled;	
}

BOOL LLPopupView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleScrollWheel, _1, _2, _3, clicks), view_visible_and_enabled, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleScrollWheel(x, y, clicks);
	}
	return handled;	
}

BOOL LLPopupView::handleToolTip(S32 x, S32 y, MASK mask)
{
	BOOL handled = handleMouseEvent(boost::bind(&LLMouseHandler::handleToolTip, _1, _2, _3, mask), view_visible, x, y, false);
	if (!handled)
	{
		handled = LLPanel::handleToolTip(x, y, mask);
	}
	return handled;
}

void LLPopupView::addPopup(LLView* popup)
{
	if (popup)
	{
		mPopups.remove(popup->getHandle());
		mPopups.push_front(popup->getHandle());
	}
}

void LLPopupView::removePopup(LLView* popup)
{
	if (popup)
	{
		mPopups.remove(popup->getHandle());
		popup->onTopLost();
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

