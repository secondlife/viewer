/** 
 * @file lltabcontainer.cpp
 * @brief LLTabContainer class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "lltabcontainer.h"

#include "llfocusmgr.h"
#include "lllocalcliprect.h"
#include "llrect.h"
#include "llresizehandle.h"
#include "lltextbox.h"
#include "llcriticaldamp.h"
#include "lluictrlfactory.h"
#include "llrender.h"
#include "llfloater.h"
#include "lltrans.h"

//----------------------------------------------------------------------------

// Implementation Notes:
//  - Each tab points to a LLPanel (see LLTabTuple below)
//  - When a tab is selected, the validation callback
//    (LLUICtrl::mValidateSignal) is called
//  -  If the validation callback returns true (or none is provided),
//     the tab is changed and the commit callback
//     (LLUICtrl::mCommitSignal) is called
//  - Callbacks pass the LLTabContainer as the control,
//    and the NAME of the selected PANEL as the LLSD data

//----------------------------------------------------------------------------

const F32 SCROLL_STEP_TIME = 0.4f;
const F32 SCROLL_DELAY_TIME = 0.5f;

void LLTabContainer::TabPositions::declareValues()
{
	declare("top", LLTabContainer::TOP);
	declare("bottom", LLTabContainer::BOTTOM);
	declare("left", LLTabContainer::LEFT);
}

//----------------------------------------------------------------------------

// Structure used to map tab buttons to and from tab panels
class LLTabTuple
{
public:
	LLTabTuple( LLTabContainer* c, LLPanel* p, LLButton* b, LLTextBox* placeholder = NULL)
		:
		mTabContainer(c),
		mTabPanel(p),
		mButton(b),
		mOldState(FALSE),
		mPlaceholderText(placeholder),
		mPadding(0)
	{}

	LLTabContainer*  mTabContainer;
	LLPanel*		 mTabPanel;
	LLButton*		 mButton;
	BOOL			 mOldState;
	LLTextBox*		 mPlaceholderText;
	S32				 mPadding;
};

//----------------------------------------------------------------------------

//============================================================================
/*
 * @file lltabcontainer.cpp
 * @brief class implements LLButton with LLIconCtrl on it
 */
class LLCustomButtonIconCtrl : public LLButton
{
public:
	struct Params
	: public LLInitParam::Block<Params, LLButton::Params>
	{
		// LEFT, RIGHT, TOP, BOTTOM paddings of LLIconCtrl in this class has same value
		Optional<S32>					icon_ctrl_pad;

		Params():
		icon_ctrl_pad("icon_ctrl_pad", 1)
		{}
	};

protected:
	friend class LLUICtrlFactory;
	LLCustomButtonIconCtrl(const Params& p):
		LLButton(p),
		mIcon(NULL),
		mIconAlignment(LLFontGL::HCENTER),
		mIconCtrlPad(p.icon_ctrl_pad)
		{}

public:

	void updateLayout()
	{
		LLRect button_rect = getRect();
		LLRect icon_rect = mIcon->getRect();

		S32 icon_size = button_rect.getHeight() - 2*mIconCtrlPad;

		switch(mIconAlignment)
		{
		case LLFontGL::LEFT:
			icon_rect.setLeftTopAndSize(button_rect.mLeft + mIconCtrlPad, button_rect.mTop - mIconCtrlPad, 
				icon_size, icon_size);
			setLeftHPad(icon_size + mIconCtrlPad * 2);
			break;
		case LLFontGL::HCENTER:
			icon_rect.setLeftTopAndSize(button_rect.mRight - (button_rect.getWidth() + mIconCtrlPad - icon_size)/2, button_rect.mTop - mIconCtrlPad, 
				icon_size, icon_size);
			setRightHPad(icon_size + mIconCtrlPad * 2);
			break;
		case LLFontGL::RIGHT:
			icon_rect.setLeftTopAndSize(button_rect.mRight - mIconCtrlPad - icon_size, button_rect.mTop - mIconCtrlPad, 
				icon_size, icon_size);
			setRightHPad(icon_size + mIconCtrlPad * 2);
			break;
		default:
			break;
		}
		mIcon->setRect(icon_rect);
	}

	void setIcon(LLIconCtrl* icon, LLFontGL::HAlign alignment = LLFontGL::LEFT)
	{
		if(icon)
		{
			if(mIcon)
			{
				removeChild(mIcon);
				mIcon->die();
			}
			mIcon = icon;
			mIconAlignment = alignment;

			addChild(mIcon);
			updateLayout();
		}
	}


private:
	LLIconCtrl* mIcon;
	LLFontGL::HAlign mIconAlignment;
	S32 mIconCtrlPad;
};
//============================================================================

struct LLPlaceHolderPanel : public LLPanel
{
	// create dummy param block to register with "placeholder" nane
	struct Params : public LLPanel::Params{};
	LLPlaceHolderPanel(const Params& p) : LLPanel(p)
	{}
};
static LLDefaultChildRegistry::Register<LLPlaceHolderPanel> r1("placeholder");
static LLDefaultChildRegistry::Register<LLTabContainer> r2("tab_container");

LLTabContainer::TabParams::TabParams()
:	tab_top_image_unselected("tab_top_image_unselected"),
	tab_top_image_selected("tab_top_image_selected"),
	tab_bottom_image_unselected("tab_bottom_image_unselected"),
	tab_bottom_image_selected("tab_bottom_image_selected"),
	tab_left_image_unselected("tab_left_image_unselected"),
	tab_left_image_selected("tab_left_image_selected")
{}

LLTabContainer::Params::Params()
:	tab_width("tab_width"),
	tab_min_width("tab_min_width"),
	tab_max_width("tab_max_width"),
	tab_height("tab_height"),
	label_pad_bottom("label_pad_bottom"),
	label_pad_left("label_pad_left"),
	tab_position("tab_position"),
	hide_tabs("hide_tabs", false),
	tab_padding_right("tab_padding_right"),
	first_tab("first_tab"),
	middle_tab("middle_tab"),
	last_tab("last_tab"),
	use_custom_icon_ctrl("use_custom_icon_ctrl", false),
	tab_icon_ctrl_pad("tab_icon_ctrl_pad", 0),
	use_ellipses("use_ellipses"),
	font_halign("font_halign")
{
	name(std::string("tab_container"));
	mouse_opaque = false;
}

LLTabContainer::LLTabContainer(const LLTabContainer::Params& p)
:	LLPanel(p),
	mCurrentTabIdx(-1),
	mTabsHidden(p.hide_tabs),
	mScrolled(FALSE),
	mScrollPos(0),
	mScrollPosPixels(0),
	mMaxScrollPos(0),
	mTitleBox(NULL),
	mTopBorderHeight(LLPANEL_BORDER_WIDTH),
	mLockedTabCount(0),
	mMinTabWidth(0),
	mMaxTabWidth(p.tab_max_width),
	mTabHeight(p.tab_height),
	mLabelPadBottom(p.label_pad_bottom),
	mLabelPadLeft(p.label_pad_left),
	mPrevArrowBtn(NULL),
	mNextArrowBtn(NULL),
	mIsVertical( p.tab_position == LEFT ),
	// Horizontal Specific
	mJumpPrevArrowBtn(NULL),
	mJumpNextArrowBtn(NULL),
	mRightTabBtnOffset(p.tab_padding_right),
	mTotalTabWidth(0),
	mTabPosition(p.tab_position),
	mFontHalign(p.font_halign),
	mFont(p.font),
	mFirstTabParams(p.first_tab),
	mMiddleTabParams(p.middle_tab),
	mLastTabParams(p.last_tab),
	mCustomIconCtrlUsed(p.use_custom_icon_ctrl),
	mTabIconCtrlPad(p.tab_icon_ctrl_pad),
	mUseTabEllipses(p.use_ellipses)
{
	static LLUICachedControl<S32> tabcntr_vert_tab_min_width ("UITabCntrVertTabMinWidth", 0);

	mDragAndDropDelayTimer.stop();

	if (p.tab_width.isProvided())
	{
		mMinTabWidth = p.tab_width;
	}
	else if (!mIsVertical)
	{
		mMinTabWidth = p.tab_min_width;
	}
	else
	{
		// *HACK: support default min width for legacy vertical
		// tab containers
		mMinTabWidth = tabcntr_vert_tab_min_width;
	}

	initButtons( );
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
void LLTabContainer::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape( width, height, called_from_parent );
	updateMaxScrollPos();
}

//virtual
LLView* LLTabContainer::getChildView(const std::string& name, BOOL recurse) const
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
			LLView *child = panel->getChildView(name, recurse);
			if (child)
			{
				return child;
			}
		}
	}
	return LLView::getChildView(name, recurse);
}

//virtual
LLView* LLTabContainer::findChildView(const std::string& name, BOOL recurse) const
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
			LLView *child = panel->findChildView(name, recurse);
			if (child)
			{
				return child;
			}
		}
	}
	return LLView::findChildView(name, recurse);
}

bool LLTabContainer::addChild(LLView* view, S32 tab_group)
{
	LLPanel* panelp = dynamic_cast<LLPanel*>(view);

	if (panelp)
	{
		addTabPanel(TabPanelParams().panel(panelp).label(panelp->getLabel()).is_placeholder(dynamic_cast<LLPlaceHolderPanel*>(view) != NULL));
		return true;
	}
	else
	{
		return LLUICtrl::addChild(view, tab_group);
	}
}

BOOL LLTabContainer::postBuild()
{
	selectFirstTab();

	return TRUE;
}

// virtual
void LLTabContainer::draw()
{
	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	static LLUICachedControl<S32> tabcntrv_arrow_btn_size ("UITabCntrvArrowBtnSize", 0);
	static LLUICachedControl<S32> tabcntr_tab_h_pad ("UITabCntrTabHPad", 0);
	static LLUICachedControl<S32> tabcntr_arrow_btn_size ("UITabCntrArrowBtnSize", 0);
	static LLUICachedControl<S32> tabcntr_tab_partial_width ("UITabCntrTabPartialWidth", 0);
	S32 target_pixel_scroll = 0;
	S32 cur_scroll_pos = getScrollPos();
	if (cur_scroll_pos > 0)
	{
		S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + tabcntr_arrow_btn_size  + tabcntr_arrow_btn_size + 1);
		if (!mIsVertical)
		{
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
			target_pixel_scroll -= tabcntr_tab_partial_width;
			// clamp so that rightmost tab never leaves right side of screen
			target_pixel_scroll = llmin(mTotalTabWidth - available_width_with_arrows, target_pixel_scroll);
		}
	}

	setScrollPosPixels((S32)lerp((F32)getScrollPosPixels(), (F32)target_pixel_scroll, LLCriticalDamp::getInterpolant(0.08f)));

	BOOL has_scroll_arrows = !getTabsHidden() && ((mMaxScrollPos > 0) || (mScrollPosPixels > 0));
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
		top = getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1 - (has_scroll_arrows ? tabcntrv_arrow_btn_size : 0);
		top += getScrollPosPixels();
	}
	else
	{
		// Set the leftmost position of the tab buttons.
		left = LLPANEL_BORDER_WIDTH + (has_scroll_arrows ? (tabcntr_arrow_btn_size * 2) : tabcntr_tab_h_pad);
		left -= getScrollPosPixels();
	}
	
	// Hide all the buttons
	if (getTabsHidden())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}
	}

	{
		LLRect clip_rect = getLocalRect();
		clip_rect.mLeft+=(LLPANEL_BORDER_WIDTH + 2);
		clip_rect.mRight-=(LLPANEL_BORDER_WIDTH + 2);
		LLLocalClipRect clip(clip_rect);
		LLPanel::draw();
	}

	// if tabs are hidden, don't draw them and leave them in the invisible state
	if (!getTabsHidden())
	{
		// Show all the buttons
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( TRUE );
		}

		S32 max_scroll_visible = getTabCount() - getMaxScrollPos() + getScrollPos();
		S32 idx = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;

			tuple->mButton->translate( left ? left - tuple->mButton->getRect().mLeft : 0,
									   top ? top - tuple->mButton->getRect().mTop : 0 );
			if (top) top -= BTN_HEIGHT + tabcntrv_pad;
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

			idx++;
		}


		if( mIsVertical && has_scroll_arrows )
		{
			// Redraw the arrows so that they appears on top.
			gGL.pushMatrix();
			gGL.translatef((F32)mPrevArrowBtn->getRect().mLeft, (F32)mPrevArrowBtn->getRect().mBottom, 0.f);
			mPrevArrowBtn->draw();
			gGL.popMatrix();

			gGL.pushMatrix();
			gGL.translatef((F32)mNextArrowBtn->getRect().mLeft, (F32)mNextArrowBtn->getRect().mBottom, 0.f);
			mNextArrowBtn->draw();
			gGL.popMatrix();
		}
	}

	mPrevArrowBtn->setFlashing(FALSE);
	mNextArrowBtn->setFlashing(FALSE);
}


// virtual
BOOL LLTabContainer::handleMouseDown( S32 x, S32 y, MASK mask )
{
	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0) && !getTabsHidden();

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

	S32 tab_count = getTabCount();
	if (tab_count > 0)
	{
		LLTabTuple* firsttuple = getTab(0);
		LLRect tab_rect;
		if (mIsVertical)
		{
			tab_rect = LLRect(firsttuple->mButton->getRect().mLeft,
							  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - tabcntrv_pad : mPrevArrowBtn->getRect().mTop,
							  firsttuple->mButton->getRect().mRight,
							  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + tabcntrv_pad : mNextArrowBtn->getRect().mBottom );
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
			S32 index = getCurrentPanelIndex();
			index = llclamp(index, 0, tab_count-1);
			LLButton* tab_button = getTab(index)->mButton;
			gFocusMgr.setMouseCapture(this);
			tab_button->setFocus(TRUE);
		}
	}
	return handled;
}

// virtual
BOOL LLTabContainer::handleHover( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0) && !getTabsHidden();

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
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0)  && !getTabsHidden();

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
BOOL LLTabContainer::handleToolTip( S32 x, S32 y, MASK mask)
{
	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	BOOL handled = LLPanel::handleToolTip( x, y, mask);
	if (!handled && getTabCount() > 0) 
	{
		LLTabTuple* firsttuple = getTab(0);

		BOOL has_scroll_arrows = (getMaxScrollPos() > 0);
		LLRect clip;
		if (mIsVertical)
		{
			clip = LLRect(firsttuple->mButton->getRect().mLeft,
						  has_scroll_arrows ? mPrevArrowBtn->getRect().mBottom - tabcntrv_pad : mPrevArrowBtn->getRect().mTop,
						  firsttuple->mButton->getRect().mRight,
						  has_scroll_arrows ? mNextArrowBtn->getRect().mTop + tabcntrv_pad : mNextArrowBtn->getRect().mBottom );
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
				handled = tuple->mButton->handleToolTip( local_x, local_y, mask);
				if( handled )
				{
					break;
				}
			}
		}
	}
	return handled;
}

// virtual
BOOL LLTabContainer::handleKeyHere(KEY key, MASK mask)
{
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
BOOL LLTabContainer::handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,	EDragAndDropType type, void* cargo_data, EAcceptance *accept, std::string	&tooltip)
{
	BOOL has_scroll_arrows = (getMaxScrollPos() > 0);

	if( mDragAndDropDelayTimer.getStarted() && mDragAndDropDelayTimer.getElapsedTimeF32() > SCROLL_DELAY_TIME )
	{
		if (has_scroll_arrows)
		{
			if (mJumpPrevArrowBtn && mJumpPrevArrowBtn->getRect().pointInRect(x, y))
			{
				S32	local_x	= x	- mJumpPrevArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mJumpPrevArrowBtn->getRect().mBottom;
				mJumpPrevArrowBtn->handleHover(local_x,	local_y, mask);
			}
			if (mJumpNextArrowBtn && mJumpNextArrowBtn->getRect().pointInRect(x, y))
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

void LLTabContainer::addTabPanel(LLPanel* panelp)
{
	addTabPanel(TabPanelParams().panel(panelp));
}

// function to update images
void LLTabContainer::update_images(LLTabTuple* tuple, TabParams params, LLTabContainer::TabPosition pos)
{
	if (tuple && tuple->mButton)
	{
		if (pos == LLTabContainer::TOP)
		{
			tuple->mButton->setImageUnselected(static_cast<LLUIImage*>(params.tab_top_image_unselected));
			tuple->mButton->setImageSelected(static_cast<LLUIImage*>(params.tab_top_image_selected));
		}
		else if (pos == LLTabContainer::BOTTOM)
		{
			tuple->mButton->setImageUnselected(static_cast<LLUIImage*>(params.tab_bottom_image_unselected));
			tuple->mButton->setImageSelected(static_cast<LLUIImage*>(params.tab_bottom_image_selected));
		}
		else if (pos == LLTabContainer::LEFT)
		{
			tuple->mButton->setImageUnselected(static_cast<LLUIImage*>(params.tab_left_image_unselected));
			tuple->mButton->setImageSelected(static_cast<LLUIImage*>(params.tab_left_image_selected));
		}
	}
}

void LLTabContainer::addTabPanel(const TabPanelParams& panel)
{
	LLPanel* child = panel.panel();

	llassert(child);
	if (!child) return;

	const std::string& label = panel.label.isProvided() 
			? panel.label() 
			: panel.panel()->getLabel();
	BOOL select = panel.select_tab(); 
	S32 indent = panel.indent();
	BOOL placeholder = panel.is_placeholder;
	eInsertionPoint insertion_point = panel.insert_at();

	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	static LLUICachedControl<S32> tabcntr_button_panel_overlap ("UITabCntrButtonPanelOverlap", 0);
	static LLUICachedControl<S32> tab_padding ("UITabPadding", 0);
	if (child->getParent() == this)
	{
		// already a child of mine
		return;
	}

	// Store the original label for possible xml export.
	child->setLabel(label);
	std::string trimmed_label = label;
	LLStringUtil::trim(trimmed_label);

	S32 button_width = mMinTabWidth;
	if (!mIsVertical)
	{
		button_width = llclamp(mFont->getWidth(trimmed_label) + tab_padding, mMinTabWidth, mMaxTabWidth);
	}
	
	// Tab panel
	S32 tab_panel_top;
	S32 tab_panel_bottom;
	if (!getTabsHidden()) 
	{
		if( getTabPosition() == LLTabContainer::TOP )
		{
			S32 tab_height = mIsVertical ? BTN_HEIGHT : mTabHeight;
			tab_panel_top = getRect().getHeight() - getTopBorderHeight() - (tab_height - tabcntr_button_panel_overlap);	
			tab_panel_bottom = LLPANEL_BORDER_WIDTH;
		}
		else
		{
			tab_panel_top = getRect().getHeight() - getTopBorderHeight();
			tab_panel_bottom = (mTabHeight - tabcntr_button_panel_overlap);  // Run to the edge, covering up the border
		}
	}
	else
	{
		//Scip tab button space if they are invisible(EXT - 576)
		tab_panel_top = getRect().getHeight();
		tab_panel_bottom = LLPANEL_BORDER_WIDTH;
	}

	LLRect tab_panel_rect;
	if (!getTabsHidden() && mIsVertical)
	{
		tab_panel_rect = LLRect(mMinTabWidth + (LLPANEL_BORDER_WIDTH * 2) + tabcntrv_pad, 
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
	// add this child later

	child->setVisible( FALSE );  // Will be made visible when selected

	mTotalTabWidth += button_width;

	// Tab button
	LLRect btn_rect;  // Note: btn_rect.mLeft is just a dummy.  Will be updated in draw().
	LLUIImage* tab_img = NULL;
	LLUIImage* tab_selected_img = NULL;
	S32 tab_fudge = 1;		//  To make new tab art look better, nudge buttons up 1 pel

	if (mIsVertical)
	{
		btn_rect.setLeftTopAndSize(tabcntrv_pad + LLPANEL_BORDER_WIDTH + 2,	// JC - Fudge factor
								   (getRect().getHeight() - getTopBorderHeight() - LLPANEL_BORDER_WIDTH - 1) - ((BTN_HEIGHT + tabcntrv_pad) * getTabCount()),
								   mMinTabWidth,
								   BTN_HEIGHT);
	}
	else if( getTabPosition() == LLTabContainer::TOP )
	{
		btn_rect.setLeftTopAndSize( 0, getRect().getHeight() - getTopBorderHeight() + tab_fudge, button_width, mTabHeight);
		tab_img = mMiddleTabParams.tab_top_image_unselected;
		tab_selected_img = mMiddleTabParams.tab_top_image_selected; 
	}
	else
	{
		btn_rect.setOriginAndSize( 0, 0 + tab_fudge, button_width, mTabHeight);
		tab_img = mMiddleTabParams.tab_bottom_image_unselected;
		tab_selected_img = mMiddleTabParams.tab_bottom_image_selected;
	}

	LLTextBox* textbox = NULL;
	LLButton* btn = NULL;
	LLCustomButtonIconCtrl::Params custom_btn_params;
	{
		custom_btn_params.icon_ctrl_pad(mTabIconCtrlPad);
	}
	LLButton::Params normal_btn_params;
	
	if (placeholder)
	{
		btn_rect.translate(0, -6); // *TODO: make configurable
		LLTextBox::Params params;
		params.name(trimmed_label);
		params.rect(btn_rect);
		params.initial_value(trimmed_label);
		params.font(mFont);
		textbox = LLUICtrlFactory::create<LLTextBox> (params);
		
		LLButton::Params p;
		p.name("placeholder");
		btn = LLUICtrlFactory::create<LLButton>(p);
	}
	else
	{
		if (mIsVertical)
		{
			LLButton::Params& p = (mCustomIconCtrlUsed)?
					custom_btn_params:normal_btn_params;

			p.name(std::string("vert tab button"));
			p.rect(btn_rect);
			p.follows.flags(FOLLOWS_TOP | FOLLOWS_LEFT);
			p.click_callback.function(boost::bind(&LLTabContainer::onTabBtn, this, _2, child));
			p.font(mFont);
			p.label(trimmed_label);
			p.image_unselected(mMiddleTabParams.tab_left_image_unselected);
			p.image_selected(mMiddleTabParams.tab_left_image_selected);
			p.scale_image(true);
			p.font_halign = mFontHalign;
			p.pad_bottom( mLabelPadBottom );
			p.tab_stop(false);
			p.label_shadow(false);
			if (indent)
			{
				p.pad_left(indent);
			}
			
			
			if(mCustomIconCtrlUsed)
			{
				btn = LLUICtrlFactory::create<LLCustomButtonIconCtrl>(custom_btn_params);
				
			}
			else
			{
				btn = LLUICtrlFactory::create<LLButton>(p);
			}
		}
		else
		{
			LLButton::Params& p = (mCustomIconCtrlUsed)?
					custom_btn_params:normal_btn_params;
			p.name(std::string(child->getName()) + " tab");
			p.rect(btn_rect);
			p.click_callback.function(boost::bind(&LLTabContainer::onTabBtn, this, _2, child));
			p.font(mFont);
			p.label(trimmed_label);
			p.visible(false);
			p.scale_image(true);
			p.image_unselected(tab_img);
			p.image_selected(tab_selected_img);
			p.tab_stop(false);
			p.label_shadow(false);
			// Try to squeeze in a bit more text
			p.pad_left( mLabelPadLeft );
			p.pad_right(2);
			p.pad_bottom( mLabelPadBottom );
			p.font_halign = mFontHalign;
			p.follows.flags = FOLLOWS_LEFT;
			p.follows.flags = FOLLOWS_LEFT;
	
			if (indent)
			{
				p.pad_left(indent);
			}

			if( getTabPosition() == TOP )
			{
				p.follows.flags = p.follows.flags() | FOLLOWS_TOP;
			}
			else
			{
				p.follows.flags = p.follows.flags() | FOLLOWS_BOTTOM;
			}

			if(mCustomIconCtrlUsed)
			{
				btn = LLUICtrlFactory::create<LLCustomButtonIconCtrl>(custom_btn_params);
			}
			else
			{
				btn = LLUICtrlFactory::create<LLButton>(p);
			}
		}
	}
	
	LLTabTuple* tuple = new LLTabTuple( this, child, btn, textbox );
	insertTuple( tuple, insertion_point );

	// if new tab was added as a first or last tab, update button image 
	// and update button image of any tab it may have affected
	if (tuple == mTabList.front())
	{  
		update_images(tuple, mFirstTabParams, getTabPosition());

		if (mTabList.size() == 2) 
		{		
			update_images(mTabList[1], mLastTabParams, getTabPosition());
		}
		else if (mTabList.size() > 2) 
		{
			update_images(mTabList[1], mMiddleTabParams, getTabPosition());
		}
	}
	else if (tuple == mTabList.back())
	{
		update_images(tuple, mLastTabParams, getTabPosition());

		if (mTabList.size() > 2)
		{
			update_images(mTabList[mTabList.size()-2], mMiddleTabParams, getTabPosition());
		}
	}

	//Don't add button and textbox if tab buttons are invisible(EXT - 576)
	if (!getTabsHidden())
	{
		if (textbox)
		{
			addChild( textbox, 0 );
		}
		if (btn)
		{
			addChild( btn, 0 );
		}
	}

	if (child)
	{
		LLUICtrl::addChild(child, 1);
	}

	sendChildToFront(mPrevArrowBtn);
	sendChildToFront(mNextArrowBtn);
	sendChildToFront(mJumpPrevArrowBtn);
	sendChildToFront(mJumpNextArrowBtn);
	
	if( select )
	{
		selectLastTab();
	}

	updateMaxScrollPos();
}

void LLTabContainer::addPlaceholder(LLPanel* child, const std::string& label)
{
	addTabPanel(TabPanelParams().panel(child).label(label).is_placeholder(true));
}

void LLTabContainer::removeTabPanel(LLPanel* child)
{
	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	if (mIsVertical)
	{
		// Fix-up button sizes
		S32 tab_count = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			LLRect rect;
			rect.setLeftTopAndSize(tabcntrv_pad + LLPANEL_BORDER_WIDTH + 2,	// JC - Fudge factor
								   (getRect().getHeight() - LLPANEL_BORDER_WIDTH - 1) - ((BTN_HEIGHT + tabcntrv_pad) * (tab_count)),
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
			// update tab button images if removing the first or last tab
			if ((tuple == mTabList.front()) && (mTabList.size() > 1))
			{
				update_images(mTabList[1], mFirstTabParams, getTabPosition());
			}
			else if ((tuple == mTabList.back()) && (mTabList.size() > 2))
			{
				update_images(mTabList[mTabList.size()-2], mLastTabParams, getTabPosition());
			}

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

S32 LLTabContainer::getPanelIndexByTitle(const std::string& title)
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

LLPanel* LLTabContainer::getPanelByName(const std::string& name)
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
void LLTabContainer::setCurrentTabName(const std::string& name)
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
	if (which >= getTabCount() || which < 0)
		return FALSE;

	LLTabTuple* selected_tuple = getTab(which);
	if (!selected_tuple)
	{
		return FALSE;
	}
	
	LLSD cbdata;
	if (selected_tuple->mTabPanel)
		cbdata = selected_tuple->mTabPanel->getName();

	BOOL res = FALSE;
	if( !mValidateSignal || (*mValidateSignal)( this, cbdata ) )
	{
		res = setTab(which);
		if (res && mCommitSignal)
		{
			(*mCommitSignal)(this, cbdata);
		}
	}
	
	return res;
}

// private
BOOL LLTabContainer::setTab(S32 which)
{
	static LLUICachedControl<S32> tabcntr_arrow_btn_size ("UITabCntrArrowBtnSize", 0);
	LLTabTuple* selected_tuple = getTab(which);
	if (!selected_tuple)
	{
		return FALSE;
	}

	BOOL is_visible = FALSE;
	if (selected_tuple->mButton->getEnabled())
	{
		setCurrentPanelIndex(which);

		S32 i = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			BOOL is_selected = ( tuple == selected_tuple );
			tuple->mButton->setUseEllipses(mUseTabEllipses);
			tuple->mButton->setHAlign(mFontHalign);
			tuple->mTabPanel->setVisible( is_selected );
// 			tuple->mTabPanel->setFocus(is_selected); // not clear that we want to do this here.
			tuple->mButton->setToggleState( is_selected );
			// RN: this limits tab-stops to active button only, which would require arrow keys to switch tabs
			tuple->mButton->setTabStop( is_selected );
			
			if (is_selected)
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
				else if (getMaxScrollPos() > 0)
				{
					if( i < getScrollPos() )
					{
						setScrollPos(i);
					}
					else
					{
						S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + tabcntr_arrow_btn_size  + tabcntr_arrow_btn_size + 1);
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
				else
				{
					is_visible = TRUE;
				}
			}
			i++;
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

BOOL LLTabContainer::selectTabByName(const std::string& name)
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
		tuple->mButton->setImageOverlay(image_name, LLFontGL::LEFT, color);
		reshapeTuple(tuple);
	}
}

void LLTabContainer::setTabImage(LLPanel* child, const LLUUID& image_id, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(image_id, LLFontGL::LEFT, color);
		reshapeTuple(tuple);
	}
}

void LLTabContainer::setTabImage(LLPanel* child, LLIconCtrl* icon)
{
	LLTabTuple* tuple = getTabByPanel(child);
	LLCustomButtonIconCtrl* button;

	if(tuple)
	{
		button = dynamic_cast<LLCustomButtonIconCtrl*>(tuple->mButton);
		if(button)
		{
			button->setIcon(icon);
		}
	}
}

void LLTabContainer::reshapeTuple(LLTabTuple* tuple)
{
	static LLUICachedControl<S32> tab_padding ("UITabPadding", 0);

	if (!mIsVertical)
	{
		// remove current width from total tab strip width
		mTotalTabWidth -= tuple->mButton->getRect().getWidth();

		S32 image_overlay_width = tuple->mButton->getImageOverlay().notNull() ?
		tuple->mButton->getImageOverlay()->getImage()->getWidth(0) : 0;

		tuple->mPadding = image_overlay_width;

		tuple->mButton->reshape(llclamp(mFont->getWidth(tuple->mButton->getLabelSelected()) + tab_padding + tuple->mPadding, mMinTabWidth, mMaxTabWidth),
								tuple->mButton->getRect().getHeight());
		// add back in button width to total tab strip width
		mTotalTabWidth += tuple->mButton->getRect().getWidth();

		// tabs have changed size, might need to scroll to see current tab
		updateMaxScrollPos();
	}
}

void LLTabContainer::setTitle(const std::string& title)
{	
	if (mTitleBox)
	{
		mTitleBox->setText( title );
	}
}

const std::string LLTabContainer::getPanelTitle(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		LLButton* tab_button = mTabList[index]->mButton;
		return tab_button->getLabelSelected();
	}
	return LLStringUtil::null;
}

void LLTabContainer::setTopBorderHeight(S32 height)
{
	mTopBorderHeight = height;
}

S32 LLTabContainer::getTopBorderHeight() const
{
	return mTopBorderHeight;
}

void LLTabContainer::setRightTabBtnOffset(S32 offset)
{
	mNextArrowBtn->translate( -offset - mRightTabBtnOffset, 0 );
	mRightTabBtnOffset = offset;
	updateMaxScrollPos();
}

void LLTabContainer::setPanelTitle(S32 index, const std::string& title)
{
	static LLUICachedControl<S32> tab_padding ("UITabPadding", 0);

	if (index >= 0 && index < getTabCount())
	{
		LLTabTuple* tuple = getTab(index);
		LLButton* tab_button = tuple->mButton;
		const LLFontGL* fontp = LLFontGL::getFontSansSerifSmall();
		mTotalTabWidth -= tab_button->getRect().getWidth();
		tab_button->reshape(llclamp(fontp->getWidth(title) + tab_padding + tuple->mPadding, mMinTabWidth, mMaxTabWidth), tab_button->getRect().getHeight());
		mTotalTabWidth += tab_button->getRect().getWidth();
		tab_button->setLabelSelected(title);
		tab_button->setLabelUnselected(title);
	}
	updateMaxScrollPos();
}


void LLTabContainer::onTabBtn( const LLSD& data, LLPanel* panel )
{
	LLTabTuple* tuple = getTabByPanel(panel);
	selectTabPanel( panel );

	if (tuple)
	{
		tuple->mTabPanel->setFocus(TRUE);
	}
}

void LLTabContainer::onNextBtn( const LLSD& data )
{
	if (!mScrolled)
	{
		scrollNext();
	}
	mScrolled = FALSE;
}

void LLTabContainer::onNextBtnHeld( const LLSD& data )
{
	if (mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		mScrollTimer.reset();
		scrollNext();
		mScrolled = TRUE;
	}
}

void LLTabContainer::onPrevBtn( const LLSD& data )
{
	if (!mScrolled)
	{
		scrollPrev();
	}
	mScrolled = FALSE;
}

void LLTabContainer::onJumpFirstBtn( const LLSD& data )
{
	mScrollPos = 0;
}

void LLTabContainer::onJumpLastBtn( const LLSD& data )
{
	mScrollPos = mMaxScrollPos;
}

void LLTabContainer::onPrevBtnHeld( const LLSD& data )
{
	if (mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		mScrollTimer.reset();
		scrollPrev();
		mScrolled = TRUE;
	}
}

// private

void LLTabContainer::initButtons()
{
	// Hack:
	if (getRect().getHeight() == 0 || mPrevArrowBtn)
	{
		return; // Don't have a rect yet or already got called
	}
	
	if (mIsVertical)
	{
		static LLUICachedControl<S32> tabcntrv_arrow_btn_size ("UITabCntrvArrowBtnSize", 0);
		// Left and right scroll arrows (for when there are too many tabs to show all at once).
		S32 btn_top = getRect().getHeight();
		S32 btn_top_lower = getRect().mBottom+tabcntrv_arrow_btn_size;

		LLRect up_arrow_btn_rect;
		up_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top, tabcntrv_arrow_btn_size, tabcntrv_arrow_btn_size );

		LLRect down_arrow_btn_rect;
		down_arrow_btn_rect.setLeftTopAndSize( mMinTabWidth/2 , btn_top_lower, tabcntrv_arrow_btn_size, tabcntrv_arrow_btn_size );

		LLButton::Params prev_btn_params;
		prev_btn_params.name(std::string("Up Arrow"));
		prev_btn_params.rect(up_arrow_btn_rect);
		prev_btn_params.follows.flags(FOLLOWS_TOP | FOLLOWS_LEFT);
		prev_btn_params.image_unselected.name("scrollbutton_up_out_blue.tga");
		prev_btn_params.image_selected.name("scrollbutton_up_in_blue.tga");
		prev_btn_params.click_callback.function(boost::bind(&LLTabContainer::onPrevBtn, this, _2));
		mPrevArrowBtn = LLUICtrlFactory::create<LLButton>(prev_btn_params);

		LLButton::Params next_btn_params;
		next_btn_params.name(std::string("Down Arrow"));
		next_btn_params.rect(down_arrow_btn_rect);
		next_btn_params.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
		next_btn_params.image_unselected.name("scrollbutton_down_out_blue.tga");
		next_btn_params.image_selected.name("scrollbutton_down_in_blue.tga");
		next_btn_params.click_callback.function(boost::bind(&LLTabContainer::onNextBtn, this, _2));
		mNextArrowBtn = LLUICtrlFactory::create<LLButton>(next_btn_params);
	}
	else // Horizontal
	{
		static LLUICachedControl<S32> tabcntr_arrow_btn_size ("UITabCntrArrowBtnSize", 0);
		S32 arrow_fudge = 1;		//  match new art better 

		// Left and right scroll arrows (for when there are too many tabs to show all at once).
		S32 btn_top = (getTabPosition() == TOP ) ? getRect().getHeight() - getTopBorderHeight() : tabcntr_arrow_btn_size + 1;

		LLRect left_arrow_btn_rect;
		left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1+tabcntr_arrow_btn_size, btn_top + arrow_fudge, tabcntr_arrow_btn_size, mTabHeight );

		LLRect jump_left_arrow_btn_rect;
		jump_left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1, btn_top + arrow_fudge, tabcntr_arrow_btn_size, mTabHeight );

		S32 right_pad = tabcntr_arrow_btn_size + LLPANEL_BORDER_WIDTH + 1;

		LLRect right_arrow_btn_rect;
		right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad - tabcntr_arrow_btn_size,
												btn_top + arrow_fudge,
												tabcntr_arrow_btn_size, mTabHeight );


		LLRect jump_right_arrow_btn_rect;
		jump_right_arrow_btn_rect.setLeftTopAndSize( getRect().getWidth() - mRightTabBtnOffset - right_pad,
													 btn_top + arrow_fudge,
													 tabcntr_arrow_btn_size, mTabHeight );

		LLButton::Params p;
		p.name(std::string("Jump Left Arrow"));
		p.image_unselected.name("jump_left_out.tga");
		p.image_selected.name("jump_left_in.tga");
		p.click_callback.function(boost::bind(&LLTabContainer::onJumpFirstBtn, this, _2));
		p.rect(jump_left_arrow_btn_rect);
		p.follows.flags(FOLLOWS_LEFT);
		
		mJumpPrevArrowBtn = LLUICtrlFactory::create<LLButton>(p);

		p = LLButton::Params();
		p.name(std::string("Left Arrow"));
		p.rect(left_arrow_btn_rect);
		p.follows.flags(FOLLOWS_LEFT);
		p.image_unselected.name("scrollbutton_left_out_blue.tga");
		p.image_selected.name("scrollbutton_left_in_blue.tga");
		p.click_callback.function(boost::bind(&LLTabContainer::onPrevBtn, this, _2));
		p.mouse_held_callback.function(boost::bind(&LLTabContainer::onPrevBtnHeld, this, _2));
		
		mPrevArrowBtn = LLUICtrlFactory::create<LLButton>(p);

		p = LLButton::Params();
		p.name(std::string("Jump Right Arrow"));
		p.rect(jump_right_arrow_btn_rect);
		p.follows.flags(FOLLOWS_RIGHT);
		p.image_unselected.name("jump_right_out.tga");
		p.image_selected.name("jump_right_in.tga");
		p.click_callback.function(boost::bind(&LLTabContainer::onJumpLastBtn, this, _2));

		mJumpNextArrowBtn = LLUICtrlFactory::create<LLButton>(p);

		p = LLButton::Params();
		p.name(std::string("Right Arrow"));
		p.rect(right_arrow_btn_rect);
		p.follows.flags(FOLLOWS_RIGHT);
		p.image_unselected.name("scrollbutton_right_out_blue.tga");
		p.image_selected.name("scrollbutton_right_in_blue.tga");
		p.click_callback.function(boost::bind(&LLTabContainer::onNextBtn, this, _2));
		p.mouse_held_callback.function(boost::bind(&LLTabContainer::onNextBtnHeld, this, _2));

		mNextArrowBtn = LLUICtrlFactory::create<LLButton>(p);

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

	mPrevArrowBtn->setTabStop(FALSE);
	addChild(mPrevArrowBtn);

	mNextArrowBtn->setTabStop(FALSE);
	addChild(mNextArrowBtn);

	if (mJumpPrevArrowBtn)
	{
		mJumpPrevArrowBtn->setTabStop(FALSE);
		addChild(mJumpPrevArrowBtn);
	}

	if (mJumpNextArrowBtn)
	{
		mJumpNextArrowBtn->setTabStop(FALSE);
		addChild(mJumpNextArrowBtn);
	}
	
	// set default tab group to be panel contents
	setDefaultTabGroup(1);
}

//this is a work around for the current LLPanel::initFromParams hack
//so that it doesn't overwrite the default tab group.
//will be removed when LLPanel is fixed soon.
void LLTabContainer::initFromParams(const LLPanel::Params& p)
{
	LLPanel::initFromParams(p);

	setDefaultTabGroup(1);
}

LLTabTuple* LLTabContainer::getTabByPanel(LLPanel* child)
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
	static LLUICachedControl<S32> tabcntrv_pad ("UITabCntrvPad", 0);
	BOOL no_scroll = TRUE;
	if (mIsVertical)
	{
		S32 tab_total_height = (BTN_HEIGHT + tabcntrv_pad) * getTabCount();
		S32 available_height = getRect().getHeight() - getTopBorderHeight();
		if( tab_total_height > available_height )
		{
			static LLUICachedControl<S32> tabcntrv_arrow_btn_size ("UITabCntrvArrowBtnSize", 0);
			S32 available_height_with_arrows = getRect().getHeight() - 2*(tabcntrv_arrow_btn_size + 3*tabcntrv_pad);
			S32 additional_needed = tab_total_height - available_height_with_arrows;
			setMaxScrollPos((S32) ceil(additional_needed / float(BTN_HEIGHT) ) );
			no_scroll = FALSE;
		}
	}
	else
	{
		static LLUICachedControl<S32> tabcntr_tab_h_pad ("UITabCntrTabHPad", 0);
		static LLUICachedControl<S32> tabcntr_arrow_btn_size ("UITabCntrArrowBtnSize", 0);
		static LLUICachedControl<S32> tabcntr_tab_partial_width ("UITabCntrTabPartialWidth", 0);
		S32 tab_space = 0;
		S32 available_space = 0;
		tab_space = mTotalTabWidth;
		available_space = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + tabcntr_tab_h_pad);

		if( tab_space > available_space )
		{
			S32 available_width_with_arrows = getRect().getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + tabcntr_arrow_btn_size  + tabcntr_arrow_btn_size + 1);
			// subtract off reserved portion on left
			available_width_with_arrows -= tabcntr_tab_partial_width;

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
	if (!getTabsHidden() && hasMouseCapture())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			S32 local_x = x - tuple->mButton->getRect().mLeft;
			S32 local_y = y - tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) && tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
			}
		}
	}
}
