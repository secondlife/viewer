/** 
 * @file lltabcontainer.cpp
 * @brief LLTabContainer class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "lltabcontainer.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llresizehandle.h"
#include "lltextbox.h"
#include "llcriticaldamp.h"
#include "lluictrlfactory.h"
#include "lltabcontainervertical.h"


const F32 SCROLL_STEP_TIME = 0.4f;
const F32 SCROLL_DELAY_TIME = 0.5f;
const S32 TAB_PADDING = 15;
const S32 TABCNTR_TAB_MIN_WIDTH = 60;
const S32 TABCNTR_VERT_TAB_MIN_WIDTH = 100;
const S32 TABCNTR_TAB_MAX_WIDTH = 150;
const S32 TABCNTR_TAB_PARTIAL_WIDTH = 12;	// When tabs are parially obscured, how much can you still see.
const S32 TABCNTR_TAB_HEIGHT = 16;
const S32 TABCNTR_ARROW_BTN_SIZE = 16;
const S32 TABCNTR_BUTTON_PANEL_OVERLAP = 1;  // how many pixels the tab buttons and tab panels overlap.
const S32 TABCNTR_TAB_H_PAD = 4;

const S32 TABCNTR_CLOSE_BTN_SIZE = 16;
const S32 TABCNTR_HEADER_HEIGHT = LLPANEL_BORDER_WIDTH + TABCNTR_CLOSE_BTN_SIZE;

const S32 TABCNTRV_CLOSE_BTN_SIZE = 16;
const S32 TABCNTRV_HEADER_HEIGHT = LLPANEL_BORDER_WIDTH + TABCNTRV_CLOSE_BTN_SIZE;
//const S32 TABCNTRV_TAB_WIDTH = 100;
const S32 TABCNTRV_ARROW_BTN_SIZE = 16;
const S32 TABCNTRV_PAD = 0;



LLTabContainer::LLTabContainer(const LLString& name, const LLRect& rect, TabPosition pos,
							   BOOL bordered, BOOL is_vertical )
	: 
	LLPanel(name, rect, bordered),
	mCurrentTabIdx(-1),
	mTabsHidden(FALSE),
	mScrolled(FALSE),
	mScrollPos(0),
	mScrollPosPixels(0),
	mMaxScrollPos(0),
	mCloseCallback( NULL ),
	mCallbackUserdata( NULL ),
	mTitleBox(NULL),
	mTopBorderHeight(LLPANEL_BORDER_WIDTH),
	mTabPosition(pos),
	mLockedTabCount(0),
	mMinTabWidth(TABCNTR_TAB_MIN_WIDTH),
	mMaxTabWidth(TABCNTR_TAB_MAX_WIDTH),
	mPrevArrowBtn(NULL),
	mNextArrowBtn(NULL),
	mIsVertical(is_vertical),
	// Horizontal Specific
	mJumpPrevArrowBtn(NULL),
	mJumpNextArrowBtn(NULL),
	mRightTabBtnOffset(0),
	mTotalTabWidth(0)
{ 
	//RN: HACK to support default min width for legacy vertical tab containers
	if (mIsVertical)
	{
		mMinTabWidth = TABCNTR_VERT_TAB_MIN_WIDTH;
	}
	setMouseOpaque(FALSE);
	initButtons( );
	mDragAndDropDelayTimer.stop();
}

LLTabContainer::~LLTabContainer()
{
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
}

//virtual
void LLTabContainer::setValue(const LLSD& value)
{
	selectTab((S32) value.asInteger());
}

//virtual
EWidgetType LLTabContainer::getWidgetType() const
{
	return WIDGET_TYPE_TAB_CONTAINER;
}

//virtual
LLString LLTabContainer::getWidgetTag() const
{
	return LL_TAB_CONTAINER_COMMON_TAG;
}

//virtual
void LLTabContainer::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape( width, height, called_from_parent );
	updateMaxScrollPos();
}

//virtual
LLView* LLTabContainer::getChildByName(const LLString& name, BOOL recurse) const
{
	tuple_list_t::const_iterator itor;
	for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
	{
		LLPanel *panel = (*itor)->mTabPanel;
		if (panel->getName() == name)
		{
			return panel;
		}
	}
	if (recurse)
	{
		for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
		{
			LLPanel *panel = (*itor)->mTabPanel;
			LLView *child = panel->getChild<LLView>(name, recurse);
			if (child)
			{
				return child;
			}
		}
	}
	return LLView::getChildByName(name, recurse);
}

// virtual
void LLTabContainer::draw()
{
	S32 target_pixel_scroll = 0;
	S32 cur_scroll_pos = mIsVertical ? 0 : getScrollPos();
	if (cur_scroll_pos > 0)
	{
		S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			if (cur_scroll_pos == 0)
			{
				break;
			}
			target_pixel_scroll += (*iter)->mButton->getRect().getWidth();
			cur_scroll_pos--;
		}

		// Show part of the tab to the left of what is fully visible
		target_pixel_scroll -= TABCNTR_TAB_PARTIAL_WIDTH;
		// clamp so that rightmost tab never leaves right side of screen
		target_pixel_scroll = llmin(mTotalTabWidth - available_width_with_arrows, target_pixel_scroll);
	}

	setScrollPosPixels((S32)lerp((F32)getScrollPosPixels(), (F32)target_pixel_scroll, LLCriticalDamp::getInterpolant(0.08f)));
	if( getVisible() )
	{
		BOOL has_scroll_arrows = (mMaxScrollPos > 0) || (mScrollPosPixels > 0);
		if (!mIsVertical)
		{
			mJumpPrevArrowBtn->setVisible( has_scroll_arrows );
			mJumpNextArrowBtn->setVisible( has_scroll_arrows );
		}
		mPrevArrowBtn->setVisible( has_scroll_arrows );
		mNextArrowBtn->setVisible( has_scroll_arrows );

		S32 left = 0, top = 0;
		if (mIsVertical)
		{
			top = getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1 - (has_scroll_arrows ? TABCNTRV_ARROW_BTN_SIZE : 0);
			top += getScrollPosPixels();
		}
		else
		{
			// Set the leftmost position of the tab buttons.
			left = LLPANEL_BORDER_WIDTH + (has_scroll_arrows ? (TABCNTR_ARROW_BTN_SIZE * 2) : TABCNTR_TAB_H_PAD);
			left -= getScrollPosPixels();
		}
		
		// Hide all the buttons
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}

		LLPanel::draw();

		// if tabs are hidden, don't draw them and leave them in the invisible state
		if (!getTabsHidden())
		{
			// Show all the buttons
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;
				tuple->mButton->setVisible( TRUE );
			}

			// Draw some of the buttons...
			LLRect clip_rect = getLocalRect();
			if (has_scroll_arrows)
			{
				// ...but clip them.
				if (mIsVertical)
				{
					clip_rect.mBottom = mNextArrowBtn->getRect().mTop + 3*TABCNTRV_PAD;
					clip_rect.mTop = mPrevArrowBtn->getRect().mBottom - 3*TABCNTRV_PAD;
				}
				else
				{
					clip_rect.mLeft = mPrevArrowBtn->getRect().mRight;
					clip_rect.mRight = mNextArrowBtn->getRect().mLeft;
				}
			}
			LLLocalClipRect clip(clip_rect);

			S32 max_scroll_visible = getTabCount() - getMaxScrollPos() + getScrollPos();
			S32 idx = 0;
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;

				tuple->mButton->translate( left ? left - tuple->mButton->getRect().mLeft : 0,
										   top ? top - tuple->mButton->getRect().mTop : 0 );
				if (top) top -= BTN_HEIGHT + TABCNTRV_PAD;
				if (left) left += tuple->mButton->getRect().getWidth();

				if (!mIsVertical)
				{
					if( idx < getScrollPos() )
					{
						if( tuple->mButton->getFlashing() )
						{
							mPrevArrowBtn->setFlashing( TRUE );
						}
					}
					else if( max_scroll_visible < idx )
					{
						if( tuple->mButton->getFlashing() )
						{
							mNextArrowBtn->setFlashing( TRUE );
						}
					}
				}
				LLUI::pushMatrix();
				{
					LLUI::translate((F32)tuple->mButton->getRect().mLeft, (F32)tuple->mButton->getRect().mBottom, 0.f);
					tuple->mButton->draw();
				}
				LLUI::popMatrix();

				idx++;
			}


			if( mIsVertical && has_scroll_arrows )
			{
				// Redraw the arrows so that they appears on top.
				glPushMatrix();
				glTranslatef((F32)mPrevArrowBtn->getRect().mLeft, (F32)mPrevArrowBtn->getRect().mBottom, 0.f);
				mPrevArrowBtn->draw();
				glPopMatrix();

				glPushMatrix();
				glTranslatef((F32)mNextArrowBtn->getRect().mLeft, (F32)mNextArrowBtn->getRect().mBottom, 0.f);
				mNextArrowBtn->draw();
				glPopMatrix();
			}
		}

		mPrevArrowBtn->setFlashing(FALSE);
		mNextArrowBtn->setFlashing(FALSE);
	}
}


// virtual
BOOL LLTabContainer::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);

	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn&& mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseDown( x, y, mask );
	}

	if (getTabCount() > 0)
	{
		LLTabTuple* firsttuple = getTab(0);
		LLRect tab_rect;
		if (mIsVertical)
		{
			tab_rect = LLRect(firsttuple->mButton->getRect().mLeft,
							  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - TABCNTRV_PAD : mPrevArrowBtn->getRect().mTop,
							  firsttuple->mButton->getRect().mRight,
							  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + TABCNTRV_PAD : mNextArrowBtn->getRect().mBottom );
		}
		else
		{
			tab_rect = LLRect(has_scroll_arrows ? mPrevArrowBtn->getRect().mRight : mJumpPrevArrowBtn->getRect().mLeft,
							  firsttuple->mButton->getRect().mTop,
							  has_scroll_arrows ? mNextArrowBtn->getRect().mLeft : mJumpNextArrowBtn->getRect().mRight,
							  firsttuple->mButton->getRect().mBottom );
		}
		if( tab_rect.pointInRect( x, y ) )
		{
			LLButton* tab_button = getTab(getCurrentPanelIndex())->mButton;
			gFocusMgr.setMouseCapture(this);
			gFocusMgr.setKeyboardFocus(tab_button);
		}
	}
	return handled;
}

// virtual
BOOL LLTabContainer::handleHover( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);

	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleHover(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleHover(x, y, mask);
	}

	commitHoveredButton(x, y);
	return handled;
}

// virtual
BOOL LLTabContainer::handleMouseUp( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);

	if (has_scroll_arrows)
	{
		if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpPrevArrowBtn->getRect().mBottom;
			handled = mJumpPrevArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
		else if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x,	y))
		{
			S32	local_x	= x	- mJumpNextArrowBtn->getRect().mLeft;
			S32	local_y	= y	- mJumpNextArrowBtn->getRect().mBottom;
			handled = mJumpNextArrowBtn->handleMouseUp(local_x,	local_y, mask);
		}
		else if (mPrevArrowBtn && mPrevArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mPrevArrowBtn->getRect().mLeft;
			S32 local_y = y - mPrevArrowBtn->getRect().mBottom;
			handled = mPrevArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
		else if (mNextArrowBtn && mNextArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mNextArrowBtn->getRect().mLeft;
			S32 local_y = y - mNextArrowBtn->getRect().mBottom;
			handled = mNextArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseUp( x, y, mask );
	}

	commitHoveredButton(x, y);
	LLPanel* cur_panel = getCurrentPanel();
	if (hasMouseCapture())
	{
		if (cur_panel)
		{
			if (!cur_panel->focusFirstItem(FALSE))
			{
				// if nothing in the panel gets focus, make sure the new tab does
				// otherwise the last tab might keep focus
				getTab(getCurrentPanelIndex())->mButton->setFocus(TRUE);
			}
		}
		gFocusMgr.setMouseCapture(NULL);
	}
	return handled;
}

// virtual
BOOL LLTabContainer::handleToolTip( S32 x, S32 y, LLString& msg, LLRect* sticky_rect )
{
	BOOL handled = LLPanel::handleToolTip( x, y, msg, sticky_rect );
	if (!handled && getTabCount() > 0) 
	{
		LLTabTuple* firsttuple = getTab(0);

		BOOL has_scroll_arrows = (getMaxScrollPos() > 0);
		LLRect clip;
		if (mIsVertical)
		{
			clip = LLRect(firsttuple->mButton->getRect().mLeft,
						  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - TABCNTRV_PAD : mPrevArrowBtn->getRect().mTop,
						  firsttuple->mButton->getRect().mRight,
						  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + TABCNTRV_PAD : mNextArrowBtn->getRect().mBottom );
		}
		else
		{
			clip = LLRect(has_scroll_arrows ? mPrevArrowBtn->getRect().mRight : mJumpPrevArrowBtn->getRect().mLeft,
						  firsttuple->mButton->getRect().mTop,
						  has_scroll_arrows ? mNextArrowBtn->getRect().mLeft : mJumpNextArrowBtn->getRect().mRight,
						  firsttuple->mButton->getRect().mBottom );
		}

		if( clip.pointInRect( x, y ) )
		{
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;
				tuple->mButton->setVisible( TRUE );
				S32 local_x = x - tuple->mButton->getRect().mLeft;
				S32 local_y = y - tuple->mButton->getRect().mBottom;
				handled = tuple->mButton->handleToolTip( local_x, local_y, msg, sticky_rect );
				if( handled )
				{
					break;
				}
			}
		}

		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}
	}
	return handled;
}

// virtual
BOOL LLTabContainer::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if (!getEnabled()) return FALSE;

	if (!gFocusMgr.childHasKeyboardFocus(this)) return FALSE;

	BOOL handled = FALSE;
	if (key == KEY_LEFT && mask == MASK_ALT)
	{
		selectPrevTab();
		handled = TRUE;
	}
	else if (key == KEY_RIGHT && mask == MASK_ALT)
	{
		selectNextTab();
		handled = TRUE;
	}

	if (handled)
	{
		if (getCurrentPanel())
		{
			getCurrentPanel()->setFocus(TRUE);
		}
	}

	if (!gFocusMgr.childHasKeyboardFocus(getCurrentPanel()))
	{
		// if child has focus, but not the current panel, focus is on a button
		if (mIsVertical)
		{
			switch(key)
			{
			  case KEY_UP:
				selectPrevTab();
				handled = TRUE;
				break;
			  case KEY_DOWN:
				selectNextTab();
				handled = TRUE;
				break;
			  case KEY_LEFT:
				handled = TRUE;
				break;
			  case KEY_RIGHT:
				if (getTabPosition() == LEFT && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  default:
				break;
			}
		}
		else
		{
			switch(key)
			{
			  case KEY_UP:
				if (getTabPosition() == BOTTOM && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  case KEY_DOWN:
				if (getTabPosition() == TOP && getCurrentPanel())
				{
					getCurrentPanel()->setFocus(TRUE);
				}
				handled = TRUE;
				break;
			  case KEY_LEFT:
				selectPrevTab();
				handled = TRUE;
				break;
			  case KEY_RIGHT:
				selectNextTab();
				handled = TRUE;
				break;
			  default:
				break;
			}
		}
	}
	return handled;
}

// virtual
LLXMLNodePtr LLTabContainer::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML();
	node->createChild("tab_position", TRUE)->setStringValue((getTabPosition() == TOP ? "top" : "bottom"));
	return node;
}

// virtual
BOOL LLTabContainer::handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,	EDragAndDropType type, void* cargo_data, EAcceptance *accept, LLString	&tooltip)
{
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);

	if( mDragAndDropDelayTimer.getElapsedTimeF32() > SCROLL_DELAY_TIME )
	{
		if (has_scroll_arrows)
		{
			if (mJumpPrevArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mJumpPrevArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mJumpPrevArrowBtn->getRect().mBottom;
				mJumpPrevArrowBtn->handleHover(local_x,	local_y, mask);
			}
			if (mJumpNextArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mJumpNextArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mJumpNextArrowBtn->getRect().mBottom;
				mJumpNextArrowBtn->handleHover(local_x,	local_y, mask);
			}
			if (mPrevArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mPrevArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mPrevArrowBtn->getRect().mBottom;
				mPrevArrowBtn->handleHover(local_x,	local_y, mask);
			}
			else if	(mNextArrowBtn->getRect().pointInRect(x, y))
			{
				S32	local_x	= x	- mNextArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mNextArrowBtn->getRect().mBottom;
				mNextArrowBtn->handleHover(local_x, local_y, mask);
			}
		}

		for(tuple_list_t::iterator iter	= mTabList.begin();	iter !=	 mTabList.end(); ++iter)
		{
			LLTabTuple*	tuple =	*iter;
			tuple->mButton->setVisible(	TRUE );
			S32	local_x	= x	- tuple->mButton->getRect().mLeft;
			S32	local_y	= y	- tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) &&  tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
				mDragAndDropDelayTimer.stop();
			}
		}
	}

	return LLView::handleDragAndDrop(x,	y, mask, drop, type, cargo_data,  accept, tooltip);
}

void LLTabContainer::addTabPanel(LLPanel* child, 
								 const LLString& label, 
								 BOOL select, 
								 void (*on_tab_clicked)(void*, bool), 
								 void* userdata,
								 S32 indent,
								 BOOL placeholder,
								 eInsertionPoint insertion_point)
{
	if (child->getParent() == this)
	{
		// already a child of mine
		return;
	}
	const LLFontGL* font = gResMgr->getRes( mIsVertical ? LLFONT_SANSSERIF : LLFONT_SANSSERIF_SMALL );

	// Store the original label for possible xml export.
	child->setLabel(label);
	LLString trimmed_label = label;
	LLString::trim(trimmed_label);

	S32 button_width = mMinTabWidth;
	if (!mIsVertical)
	{
		button_width = llclamp(font->getWidth(trimmed_label) + TAB_PADDING, mMinTabWidth, mMaxTabWidth);
	}
	
	// Tab panel
	S32 tab_panel_top;
	S32 tab_panel_bottom;
	if( getTabPosition() == LLTabContainer::TOP )
	{
		S32 tab_height = mIsVertical ? BTN_HEIGHT : TABCNTR_TAB_HEIGHT;
		tab_panel_top = getRect().getHeight() - getTopBorderHeight() - (tab_height - TABCNTR_BUTTON_PANEL_OVERLAP);	
		tab_panel_bottom = LLPANEL_BORDER_WIDTH;
	}
	else
	{
		tab_panel_top = getRect().getHeight() - getTopBorderHeight();
		tab_panel_bottom = (TABCNTR_TAB_HEIGHT - TABCNTR_BUTTON_PANEL_OVERLAP);  // Run to the edge, covering up the border
	}
	
	LLRect tab_panel_rect;
	if (mIsVertical)
	{
		tab_panel_rect = LLRect(mMinTabWidth + (LLPANEL_BORDER_WIDTH * 2) + TABCNTRV_PAD, 
								getRect().getHeight() - LLPANEL_BORDER_WIDTH,
								getRect().getWidth() - LLPANEL_BORDER_WIDTH,
								LLPANEL_BORDER_WIDTH);
	}
	else
	{
		tab_panel_rect = LLRect(LLPANEL_BORDER_WIDTH, 
								tab_panel_top,
								getRect().getWidth()-LLPANEL_BORDER_WIDTH,
								tab_panel_bottom );
	}
	child->setFollowsAll();
	child->translate( tab_panel_rect.mLeft - child->getRect().mLeft, tab_panel_rect.mBottom - child->getRect().mBottom);
	child->reshape( tab_panel_rect.getWidth(), tab_panel_rect.getHeight(), TRUE );
	child->setBackgroundVisible( FALSE );  // No need to overdraw
	// add this child later

	child->setVisible( FALSE );  // Will be made visible when selected

	mTotalTabWidth += button_width;

	// Tab button
	LLRect btn_rect;  // Note: btn_rect.mLeft is just a dummy.  Will be updated in draw().
	LLString tab_img;
	LLString tab_selected_img;
	S32 tab_fudge = 1;		//  To make new tab art look better, nudge buttons up 1 pel

	if (mIsVertical)
	{
		btn_rect.setLeftTopAndSize(TABCNTRV_PAD + LLPANEL_BORDER_WIDTH + 2,	// JC - Fudge factor
								   (getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1) - ((BTN_HEIGHT + TABCNTRV_PAD) * getTabCount()),
								   mMinTabWidth,
								   BTN_HEIGHT);
	}
	else if( getTabPosition() == LLTabContainer::TOP )
	{
		btn_rect.setLeftTopAndSize( 0, getRect().getHeight() - getTopBorderHeight() + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "tab_top_blue.tga";
		tab_selected_img = "tab_top_selected_blue.tga";
	}
	else
	{
		btn_rect.setOriginAndSize( 0, 0 + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "tab_bottom_blue.tga";
		tab_selected_img = "tab_bottom_selected_blue.tga";
	}

	LLTextBox* textbox = NULL;
	LLButton* btn = NULL;
	
	if (placeholder)
	{
		btn_rect.translate(0, -LLBUTTON_V_PAD-2);
		textbox = new LLTextBox(trimmed_label, btn_rect, trimmed_label, font);
		
		btn = new LLButton("", LLRect(0,0,0,0));
	}
	else
	{
		if (mIsVertical)
		{
			btn = new LLButton("vert tab button",
							   btn_rect,
							   "",
							   "", 
							   "", 
							   &LLTabContainer::onTabBtn, NULL,
							   font,
							   trimmed_label, trimmed_label);
			btn->setImages("tab_left.tga", "tab_left_selected.tga");
			btn->setScaleImage(TRUE);
			btn->setHAlign(LLFontGL::LEFT);
			btn->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
			btn->setTabStop(FALSE);
			if (indent)
			{
				btn->setLeftHPad(indent);
			}
		}
		else
		{
			LLString tooltip = trimmed_label;
			tooltip += "\nAlt-Left arrow for previous tab";
			tooltip += "\nAlt-Right arrow for next tab";

			btn = new LLButton(LLString(child->getName()) + " tab",
							   btn_rect, 
							   "", "", "",
							   &LLTabContainer::onTabBtn, NULL, // set userdata below
							   font,
							   trimmed_label, trimmed_label );
			btn->setVisible( FALSE );
			btn->setToolTip( tooltip );
			btn->setScaleImage(TRUE);
			btn->setImages(tab_img, tab_selected_img);

			// Try to squeeze in a bit more text
			btn->setLeftHPad( 4 );
			btn->setRightHPad( 2 );
			btn->setHAlign(LLFontGL::LEFT);
			btn->setTabStop(FALSE);
			if (indent)
			{
				btn->setLeftHPad(indent);
			}

			if( getTabPosition() == TOP )
			{
				btn->setFollowsTop();
			}
			else
			{
				btn->setFollowsBottom();
			}
		}
	}
	
	LLTabTuple* tuple = new LLTabTuple( this, child, btn, on_tab_clicked, userdata, textbox );
	insertTuple( tuple, insertion_point );

	if (textbox)
	{
		textbox->setSaveToXML(false);
		addChild( textbox, 0 );
	}
	if (btn)
	{
		btn->setSaveToXML(false);
		btn->setCallbackUserData( tuple );
		addChild( btn, 0 );
	}
	if (child)
	{
		addChild(child, 1);
	}
	
	if( select )
	{
		selectLastTab();
	}

	updateMaxScrollPos();
}

void LLTabContainer::addPlaceholder(LLPanel* child, const LLString& label)
{
	addTabPanel(child, label, FALSE, NULL, NULL, 0, TRUE);
}

void LLTabContainer::removeTabPanel(LLPanel* child)
{
	if (mIsVertical)
	{
		// Fix-up button sizes
		S32 tab_count = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			LLRect rect;
			rect.setLeftTopAndSize(TABCNTRV_PAD + LLPANEL_BORDER_WIDTH + 2,	// JC - Fudge factor
								   (getRect().getHeight() - LLPANEL_BORDER_WIDTH - 1) - ((BTN_HEIGHT + TABCNTRV_PAD) * (tab_count)),
								   mMinTabWidth,
								   BTN_HEIGHT);
			if (tuple->mPlaceholderText)
			{
				tuple->mPlaceholderText->setRect(rect);
			}
			else
			{
				tuple->mButton->setRect(rect);
			}
			tab_count++;
		}
	}
	else
	{
		// Adjust the total tab width.
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			if( tuple->mTabPanel == child )
			{
				mTotalTabWidth -= tuple->mButton->getRect().getWidth();
				break;
			}
		}
	}
	
	BOOL has_focus = gFocusMgr.childHasKeyboardFocus(this);

	// If the tab being deleted is the selected one, select a different tab.
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
 			removeChild( tuple->mButton );
 			delete tuple->mButton;

 			removeChild( tuple->mTabPanel );
// 			delete tuple->mTabPanel;
			
			mTabList.erase( iter );
			delete tuple;

			break;
		}
	}

	// make sure we don't have more locked tabs than we have tabs
	mLockedTabCount = llmin(getTabCount(), mLockedTabCount);

	if (mCurrentTabIdx >= (S32)mTabList.size())
	{
		mCurrentTabIdx = mTabList.size()-1;
	}
	selectTab(mCurrentTabIdx);
	if (has_focus)
	{
		LLPanel* panelp = getPanelByIndex(mCurrentTabIdx);
		if (panelp)
		{
			panelp->setFocus(TRUE);
		}
	}

	updateMaxScrollPos();
}

void LLTabContainer::lockTabs(S32 num_tabs)
{
	// count current tabs or use supplied value and ensure no new tabs get
	// inserted between them
	mLockedTabCount = num_tabs > 0 ? llmin(getTabCount(), num_tabs) : getTabCount();
}

void LLTabContainer::unlockTabs()
{
	mLockedTabCount = 0;
}

void LLTabContainer::enableTabButton(S32 which, BOOL enable)
{
	if (which >= 0 && which < (S32)mTabList.size())
	{
		mTabList[which]->mButton->setEnabled(enable);
	}
}

void LLTabContainer::deleteAllTabs()
{
	// Remove all the tab buttons and delete them.  Also, unlink all the child panels.
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;

		removeChild( tuple->mButton );
		delete tuple->mButton;

 		removeChild( tuple->mTabPanel );
// 		delete tuple->mTabPanel;
	}

	// Actually delete the tuples themselves
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
	mTabList.clear();
	
	// And there isn't a current tab any more
	mCurrentTabIdx = -1;
}

LLPanel* LLTabContainer::getCurrentPanel()
{
	if (mCurrentTabIdx >= 0 && mCurrentTabIdx < (S32) mTabList.size())
	{
		return mTabList[mCurrentTabIdx]->mTabPanel;
	}
	return NULL;
}

S32 LLTabContainer::getCurrentPanelIndex()
{
	return mCurrentTabIdx;
}

S32 LLTabContainer::getTabCount()
{
	return mTabList.size();
}

LLPanel* LLTabContainer::getPanelByIndex(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		return mTabList[index]->mTabPanel;
	}
	return NULL;
}

S32 LLTabContainer::getIndexForPanel(LLPanel* panel)
{
	for (S32 index = 0; index < (S32)mTabList.size(); index++)
	{
		if (mTabList[index]->mTabPanel == panel)
		{
			return index;
		}
	}
	return -1;
}

S32 LLTabContainer::getPanelIndexByTitle(const LLString& title)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		if (title == mTabList[index]->mButton->getLabelSelected())
		{
			return index;
		}
	}
	return -1;
}

LLPanel *LLTabContainer::getPanelByName(const LLString& name)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		LLPanel *panel = mTabList[index]->mTabPanel;
		if (name == panel->getName())
		{
			return panel;
		}
	}
	return NULL;
}

// Change the name of the button for the current tab.
void LLTabContainer::setCurrentTabName(const LLString& name)
{
	// Might not have a tab selected
	if (mCurrentTabIdx < 0) return;

	mTabList[mCurrentTabIdx]->mButton->setLabelSelected(name);
	mTabList[mCurrentTabIdx]->mButton->setLabelUnselected(name);
}

void LLTabContainer::selectFirstTab()
{
	selectTab( 0 );
}


void LLTabContainer::selectLastTab()
{
	selectTab( mTabList.size()-1 );
}

void LLTabContainer::selectNextTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx+1;
	if (idx >= (S32)mTabList.size())
		idx = 0;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = (idx + 1 ) % (S32)mTabList.size();
	}

	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}

void LLTabContainer::selectPrevTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx-1;
	if (idx < 0)
		idx = mTabList.size()-1;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = idx - 1;
		if (idx < 0)
			idx = mTabList.size()-1;
	}
	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}	

BOOL LLTabContainer::selectTabPanel(LLPanel* child)
{
	S32 idx = 0;
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return selectTab( idx );
		}
		idx++;
	}
	return FALSE;
}

BOOL LLTabContainer::selectTab(S32 which)
{
	if (which >= getTabCount()) return FALSE;
	if (which < 0) return FALSE;

	//if( gFocusMgr.childHasKeyboardFocus( this ) )
	//{
	//	gFocusMgr.setKeyboardFocus( NULL );
	//}

	LLTabTuple* selected_tuple = getTab(which);
	if (!selected_tuple)
	{
		return FALSE;
	}
	
	BOOL is_visible = FALSE;
	if (getTab(which)->mButton->getEnabled())
	{
		setCurrentPanelIndex(which);

		S32 i = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			BOOL is_selected = ( tuple == selected_tuple );
			tuple->mTabPanel->setVisible( is_selected );
// 			tuple->mTabPanel->setFocus(is_selected); // not clear that we want to do this here.
			tuple->mButton->setToggleState( is_selected );
			// RN: this limits tab-stops to active button only, which would require arrow keys to switch tabs
			tuple->mButton->setTabStop( is_selected );
			
			if( is_selected && (mIsVertical || (getMaxScrollPos() > 0)))
			{
				// Make sure selected tab is within scroll region
				if (mIsVertical)
				{
					S32 num_visible = getTabCount() - getMaxScrollPos();
					if( i >= getScrollPos() && i <= getScrollPos() + num_visible)
					{
						setCurrentPanelIndex(which);
						is_visible = TRUE;
					}
					else
					{
						is_visible = FALSE;
					}
				}
				else
				{
					if( i < getScrollPos() )
					{
						setScrollPos(i);
					}
					else
					{
						S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
						S32 running_tab_width = tuple->mButton->getRect().getWidth();
						S32 j = i - 1;
						S32 min_scroll_pos = i;
						if (running_tab_width < available_width_with_arrows)
						{
							while (j >= 0)
							{
								LLTabTuple* other_tuple = getTab(j);
								running_tab_width += other_tuple->mButton->getRect().getWidth();
								if (running_tab_width > available_width_with_arrows)
								{
									break;
								}
								j--;
							}
							min_scroll_pos = j + 1;
						}
						setScrollPos(llclamp(getScrollPos(), min_scroll_pos, i));
						setScrollPos(llmin(getScrollPos(), getMaxScrollPos()));
					}
					is_visible = TRUE;
				}
			}
			i++;
		}
		if( selected_tuple->mOnChangeCallback )
		{
			selected_tuple->mOnChangeCallback( selected_tuple->mUserData, false );
		}
	}
	if (mIsVertical && getCurrentPanelIndex() >= 0)
	{
		LLTabTuple* tuple = getTab(getCurrentPanelIndex());
		tuple->mTabPanel->setVisible( TRUE );
		tuple->mButton->setToggleState( TRUE );
	}
	return is_visible;
}

BOOL LLTabContainer::selectTabByName(const LLString& name)
{
	LLPanel* panel = getPanelByName(name);
	if (!panel)
	{
		llwarns << "LLTabContainer::selectTabByName("
			<< name << ") failed" << llendl;
		return FALSE;
	}

	BOOL result = selectTabPanel(panel);
	return result;
}

BOOL LLTabContainer::getTabPanelFlashing(LLPanel *child)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		return tuple->mButton->getFlashing();
	}
	return FALSE;
}

void LLTabContainer::setTabPanelFlashing(LLPanel* child, BOOL state )
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setFlashing( state );
	}
}

void LLTabContainer::setTabImage(LLPanel* child, std::string image_name, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(image_name, LLFontGL::RIGHT, color);

		if (!mIsVertical)
		{
			const LLFontGL* fontp = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
			// remove current width from total tab strip width
			mTotalTabWidth -= tuple->mButton->getRect().getWidth();

			S32 image_overlay_width = tuple->mButton->getImageOverlay().notNull() ? 
				tuple->mButton->getImageOverlay()->getImage()->getWidth(0) :
				0;

			tuple->mPadding = image_overlay_width;

			tuple->mButton->setRightHPad(6);
			tuple->mButton->reshape(llclamp(fontp->getWidth(tuple->mButton->getLabelSelected()) + TAB_PADDING + tuple->mPadding, mMinTabWidth, mMaxTabWidth), 
									tuple->mButton->getRect().getHeight());
			// add back in button width to total tab strip width
			mTotalTabWidth += tuple->mButton->getRect().getWidth();

			// tabs have changed size, might need to scroll to see current tab
			updateMaxScrollPos();
		}
	}
}

void LLTabContainer::setTitle(const LLString& title)
{	
	if (mTitleBox)
	{
		mTitleBox->setText( title );
	}
}

const LLString LLTabContainer::getPanelTitle(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		LLButton* tab_button = mTabList[index]->mButton;
		return tab_button->getLabelSelected();
	}
	return LLString::null;
}

void LLTabContainer::setTopBorderHeight(S32 height)
{
	mTopBorderHeight = height;
}

S32 LLTabContainer::getTopBorderHeight() const
{
	return mTopBorderHeight;
}

void LLTabContainer::setTabChangeCallback(LLPanel* tab, void (*on_tab_clicked)(void*, bool))
{
	LLTabTuple* tuplep = getTabByPanel(tab);
	if (tuplep)
	{
		tuplep->mOnChangeCallback = on_tab_clicked;
	}
}

void LLTabContainer::setTabUserData(LLPanel* tab, void* userdata)
{
	LLTabTuple* tuplep = getTabByPanel(tab);
	if (tuplep)
	{
		tuplep->mUserData = userdata;
	}
}

void LLTabContainer::setRightTabBtnOffset(S32 offset)
{
	mNextArrowBtn->translate( -offset - mRightTabBtnOffset, 0 );
	mRightTabBtnOffset = offset;
	updateMaxScrollPos();
}

void LLTabContainer::setPanelTitle(S32 index, const LLString& title)
{
	if (index >= 0 && index < getTabCount())
	{
		LLTabTuple* tuple = getTab(index);
		LLButton* tab_button = tuple->mButton;
		const LLFontGL* fontp = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
		mTotalTabWidth -= tab_button->getRect().getWidth();
		tab_button->reshape(llclamp(fontp->getWidth(title) + TAB_PADDING + tuple->mPadding, mMinTabWidth, mMaxTabWidth), tab_button->getRect().getHeight());
		mTotalTabWidth += tab_button->getRect().getWidth();
		tab_button->setLabelSelected(title);
		tab_button->setLabelUnselected(title);
	}
	updateMaxScrollPos();
}


// static 
void LLTabContainer::onTabBtn( void* userdata )
{
	LLTabTuple* tuple = (LLTabTuple*) userdata;
	LLTabContainer* self = tuple->mTabContainer;
	self->selectTabPanel( tuple->mTabPanel );
	
	if( tuple->mOnChangeCallback )
	{
		tuple->mOnChangeCallback( tuple->mUserData, true );
	}

	tuple->mTabPanel->setFocus(TRUE);
}

// static 
void LLTabContainer::onCloseBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if( self->mCloseCallback )
	{
		self->mCloseCallback( self->mCallbackUserdata );
	}
}

// static 
void LLTabContainer::onNextBtn( void* userdata )
{
	// Scroll tabs to the left
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (!self->mScrolled)
	{
		self->scrollNext();
	}
	self->mScrolled = FALSE;
}

// static 
void LLTabContainer::onNextBtnHeld( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (self->mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		self->mScrollTimer.reset();
		self->scrollNext();
		self->mScrolled = TRUE;
	}
}

// static 
void LLTabContainer::onPrevBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (!self->mScrolled)
	{
		self->scrollPrev();
	}
	self->mScrolled = FALSE;
}

// static 
void LLTabContainer::onJumpFirstBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	self->mScrollPos = 0;
}

// static 
void LLTabContainer::onJumpLastBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	self->mScrollPos = self->mMaxScrollPos;
}

// static 
void LLTabContainer::onPrevBtnHeld( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (self->mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		self->mScrollTimer.reset();
		self->scrollPrev();
		self->mScrolled = TRUE;
	}
}

// static
LLView* LLTabContainer::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("tab_container");
	node->getAttributeString("name", name);

	// Figure out if we are creating a vertical or horizontal tab container.
	bool is_vertical = false;
	LLTabContainer::TabPosition tab_position = LLTabContainer::TOP;
	if (node->hasAttribute("tab_position"))
	{
		LLString tab_position_string;
		node->getAttributeString("tab_position", tab_position_string);
		LLString::toLower(tab_position_string);

		if ("top" == tab_position_string)
		{
			tab_position = LLTabContainer::TOP;
			is_vertical = false;
		}
		else if ("bottom" == tab_position_string)
		{
			tab_position = LLTabContainer::BOTTOM;
			is_vertical = false;
		}
		else if ("left" == tab_position_string)
		{
			is_vertical = true;
		}
	}
	BOOL border = FALSE;
	node->getAttributeBOOL("border", border);

	LLTabContainer*	tab_container = new LLTabContainer(name, LLRect::null, tab_position, border, is_vertical);
	
	S32 tab_min_width = tab_container->mMinTabWidth;
	if (node->hasAttribute("tab_width"))
	{
		node->getAttributeS32("tab_width", tab_min_width);
	}
	else if( node->hasAttribute("tab_min_width"))
	{
		node->getAttributeS32("tab_min_width", tab_min_width);
	}

	S32	tab_max_width = tab_container->mMaxTabWidth;
	if (node->hasAttribute("tab_max_width"))
	{
		node->getAttributeS32("tab_max_width", tab_max_width);
	}

	tab_container->setMinTabWidth(tab_min_width); 
	tab_container->setMaxTabWidth(tab_max_width); 
	
	BOOL hidden(tab_container->getTabsHidden());
	node->getAttributeBOOL("hide_tabs", hidden);
	tab_container->setTabsHidden(hidden);

	tab_container->setPanelParameters(node, parent);

	if (LLFloater::getFloaterHost())
	{
		LLFloater::getFloaterHost()->setTabContainer(tab_container);
	}

	//parent->addChild(tab_container);

	// Add all tab panels.
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		LLView *control = factory->createCtrlWidget(tab_container, child);
		if (control && control->isPanel())
		{
			LLPanel* panelp = (LLPanel*)control;
			LLString label;
			child->getAttributeString("label", label);
			if (label.empty())
			{
				label = panelp->getLabel();
			}
			BOOL placeholder = FALSE;
			child->getAttributeBOOL("placeholder", placeholder);
			tab_container->addTabPanel(panelp, label.c_str(), false,
									   NULL, NULL, 0, placeholder);
		}
	}

	tab_container->selectFirstTab();

	tab_container->postBuild();

	tab_container->initButtons(); // now that we have the correct rect
	
	return tab_container;
}

// private

void LLTabContainer::initButtons()
{
	// Hack:
	if (getRect().getHeight() == 0 || mPrevArrowBtn)
	{
		return; // Don't have a rect yet or already got called
	}
	
	LLString out_id;
	LLString in_id;

	if (mIsVertical)
	{
		// Left and right scroll arrows (for when there are too many tabs to show all at once).
		S32 btn_top = getRect().getHeight();
		S32 btn_top_lower = getRect().mBottom+TABCNTRV_ARROW_BTN_SIZE;

		LLRect up_arrow_btn_rect;
		up_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top, TABCNTRV_ARROW_BTN_SIZE, TABCNTRV_ARROW_BTN_SIZE );

		LLRect down_arrow_btn_rect;
		down_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top_lower, TABCNTRV_ARROW_BTN_SIZE, TABCNTRV_ARROW_BTN_SIZE );

		out_id = "UIImgBtnScrollUpOutUUID";
		in_id = "UIImgBtnScrollUpInUUID";
		mPrevArrowBtn = new LLButton("Up Arrow", up_arrow_btn_rect,
									 out_id, in_id, "",
									 &onPrevBtn, this, NULL );
		mPrevArrowBtn->setFollowsTop();
		mPrevArrowBtn->setFollowsLeft();

		out_id = "UIImgBtnScrollDownOutUUID";
		in_id = "UIImgBtnScrollDownInUUID";
		mNextArrowBtn = new LLButton("Down Arrow", down_arrow_btn_rect,
									 out_id, in_id, "",
									 &onNextBtn, this, NULL );
		mNextArrowBtn->setFollowsBottom();
		mNextArrowBtn->setFollowsLeft();
	}
	else // Horizontal
	{
		S32 arrow_fudge = 1;		//  match new art better 

		// tabs on bottom reserve room for resize handle (just in case)
		if (getTabPosition() == BOTTOM)
		{
			mRightTabBtnOffset = RESIZE_HANDLE_WIDTH;
		}

		// Left and right scroll arrows (for when there are too many tabs to show all at once).
		S32 btn_top = (getTabPosition() == TOP ) ? getRect().getHeight() - getTopBorderHeight() : TABCNTR_ARROW_BTN_SIZE + 1;

		LLRect left_arrow_btn_rect;
		left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1+TABCNTR_ARROW_BTN_SIZE, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

		LLRect jump_left_arrow_btn_rect;
		jump_left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

		S32 right_pad = TABCNTR_ARROW_BTN_SIZE + LLPANEL_BORDER_WIDTH + 1;

		LLRect right_arrow_btn_rect;
		right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad - TABCNTR_ARROW_BTN_SIZE,
												btn_top + arrow_fudge,
												TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );


		LLRect jump_right_arrow_btn_rect;
		jump_right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad,
													 btn_top + arrow_fudge,
													 TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

		out_id = "UIImgBtnJumpLeftOutUUID";
		in_id = "UIImgBtnJumpLeftInUUID";
		mJumpPrevArrowBtn = new LLButton("Jump Left Arrow", jump_left_arrow_btn_rect,
										 out_id, in_id, "",
										 &LLTabContainer::onJumpFirstBtn, this, LLFontGL::sSansSerif );
		mJumpPrevArrowBtn->setFollowsLeft();

		out_id = "UIImgBtnScrollLeftOutUUID";
		in_id = "UIImgBtnScrollLeftInUUID";
		mPrevArrowBtn = new LLButton("Left Arrow", left_arrow_btn_rect,
									 out_id, in_id, "",
									 &LLTabContainer::onPrevBtn, this, LLFontGL::sSansSerif );
		mPrevArrowBtn->setHeldDownCallback(onPrevBtnHeld);
		mPrevArrowBtn->setFollowsLeft();
	
		out_id = "UIImgBtnJumpRightOutUUID";
		in_id = "UIImgBtnJumpRightInUUID";
		mJumpNextArrowBtn = new LLButton("Jump Right Arrow", jump_right_arrow_btn_rect,
										 out_id, in_id, "",
										 &LLTabContainer::onJumpLastBtn, this,
										 LLFontGL::sSansSerif);
		mJumpNextArrowBtn->setFollowsRight();

		out_id = "UIImgBtnScrollRightOutUUID";
		in_id = "UIImgBtnScrollRightInUUID";
		mNextArrowBtn = new LLButton("Right Arrow", right_arrow_btn_rect,
									 out_id, in_id, "",
									 &LLTabContainer::onNextBtn, this,
									 LLFontGL::sSansSerif);
		mNextArrowBtn->setFollowsRight();

		if( getTabPosition() == TOP )
		{
			mNextArrowBtn->setFollowsTop();
			mPrevArrowBtn->setFollowsTop();
			mJumpPrevArrowBtn->setFollowsTop();
			mJumpNextArrowBtn->setFollowsTop();
		}
		else
		{
			mNextArrowBtn->setFollowsBottom();
			mPrevArrowBtn->setFollowsBottom();
			mJumpPrevArrowBtn->setFollowsBottom();
			mJumpNextArrowBtn->setFollowsBottom();
		}
	}

	mPrevArrowBtn->setHeldDownCallback(onPrevBtnHeld);
	mPrevArrowBtn->setSaveToXML(false);
	mPrevArrowBtn->setTabStop(FALSE);
	addChild(mPrevArrowBtn);

	mNextArrowBtn->setHeldDownCallback(onNextBtnHeld);
	mNextArrowBtn->setSaveToXML(false);
	mNextArrowBtn->setTabStop(FALSE);
	addChild(mNextArrowBtn);

	if (mJumpPrevArrowBtn)
	{
		mJumpPrevArrowBtn->setSaveToXML(false);
		mJumpPrevArrowBtn->setTabStop(FALSE);
		addChild(mJumpPrevArrowBtn);
	}

	if (mJumpNextArrowBtn)
	{
		mJumpNextArrowBtn->setSaveToXML(false);
		mJumpNextArrowBtn->setTabStop(FALSE);
		addChild(mJumpNextArrowBtn);
	}
	
	// set default tab group to be panel contents
	setDefaultTabGroup(1);
}

LLTabContainer::LLTabTuple* LLTabContainer::getTabByPanel(LLPanel* child)
{
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return tuple;
		}
	}
	return NULL;
}

void LLTabContainer::insertTuple(LLTabTuple * tuple, eInsertionPoint insertion_point)
{
	switch(insertion_point)
	{
	case START:
		// insert the new tab in the front of the list
		mTabList.insert(mTabList.begin() + mLockedTabCount, tuple);
		break;
	case LEFT_OF_CURRENT:
		// insert the new tab before the current tab (but not before mLockedTabCount)
		{
		tuple_list_t::iterator current_iter = mTabList.begin() + llmax(mLockedTabCount, mCurrentTabIdx);
		mTabList.insert(current_iter, tuple);
		}
		break;

	case RIGHT_OF_CURRENT:
		// insert the new tab after the current tab (but not before mLockedTabCount)
		{
		tuple_list_t::iterator current_iter = mTabList.begin() + llmax(mLockedTabCount, mCurrentTabIdx + 1);
		mTabList.insert(current_iter, tuple);
		}
		break;
	case END:
	default:
		mTabList.push_back( tuple );
	}
}



void LLTabContainer::updateMaxScrollPos()
{
	BOOL no_scroll = TRUE;
	if (mIsVertical)
	{
		S32 tab_total_height = (BTN_HEIGHT + TABCNTRV_PAD) * getTabCount();
		S32 available_height = getRect().getHeight() - getTopBorderHeight();
		if( tab_total_height > available_height )
		{
			S32 available_height_with_arrows = getRect().getHeight() - 2*(TABCNTRV_ARROW_BTN_SIZE + 3*TABCNTRV_PAD);
			S32 additional_needed = tab_total_height - available_height_with_arrows;
			setMaxScrollPos((S32) ceil(additional_needed / float(BTN_HEIGHT) ) );
			no_scroll = FALSE;
		}
	}
	else
	{
		S32 tab_space = 0;
		S32 available_space = 0;
		tab_space = mTotalTabWidth;
		available_space = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_TAB_H_PAD);

		if( tab_space > available_space )
		{
			S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
			// subtract off reserved portion on left
			available_width_with_arrows -= TABCNTR_TAB_PARTIAL_WIDTH;

			S32 running_tab_width = 0;
			setMaxScrollPos(getTabCount());
			for(tuple_list_t::reverse_iterator tab_it = mTabList.rbegin(); tab_it != mTabList.rend(); ++tab_it)
			{
				running_tab_width += (*tab_it)->mButton->getRect().getWidth();
				if (running_tab_width > available_width_with_arrows)
				{
					break;
				}
				setMaxScrollPos(getMaxScrollPos()-1);
			}
			// in case last tab doesn't actually fit on screen, make it the last scrolling position
			setMaxScrollPos(llmin(getMaxScrollPos(), getTabCount() - 1));
			no_scroll = FALSE;
		}
	}
	if (no_scroll)
	{
		setMaxScrollPos(0);
		setScrollPos(0);
	}
	if (getScrollPos() > getMaxScrollPos())
	{
		setScrollPos(getMaxScrollPos()); // maybe just enforce this via limits in setScrollPos instead?
	}
}

void LLTabContainer::commitHoveredButton(S32 x, S32 y)
{
	if (hasMouseCapture())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( TRUE );
			S32 local_x = x - tuple->mButton->getRect().mLeft;
			S32 local_y = y - tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) && tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
			}
		}
	}
}

