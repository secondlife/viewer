/** 
 * @file lltoolbar.cpp
 * @author Richard Nelson
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include <boost/foreach.hpp>
#include "lltoolbar.h"

#include "llcommandmanager.h"
#include "lltrans.h"

// uncomment this and remove the one in llui.cpp when there is an external reference to this translation unit
// thanks, MSVC!
//static LLDefaultChildRegistry::Register<LLToolBar> r1("toolbar");

namespace LLToolBarEnums
{
	LLLayoutStack::ELayoutOrientation getOrientation(SideType sideType)
	{
		LLLayoutStack::ELayoutOrientation orientation = LLLayoutStack::HORIZONTAL;

		if ((sideType == SIDE_LEFT) || (sideType == SIDE_RIGHT))
		{
			orientation = LLLayoutStack::VERTICAL;
		}

		return orientation;
	}
}
using namespace LLToolBarEnums;


namespace LLInitParam
{
	void TypeValues<ButtonType>::declareValues()
	{
		declare("icons_only",		BTNTYPE_ICONS_ONLY);
		declare("icons_with_text",	BTNTYPE_ICONS_WITH_TEXT);
	}

	void TypeValues<SideType>::declareValues()
	{
		declare("bottom",	SIDE_BOTTOM);
		declare("left",		SIDE_LEFT);
		declare("right",	SIDE_RIGHT);
		declare("top",		SIDE_TOP);
	}
}

LLToolBar::Params::Params()
:	button_display_mode("button_display_mode"),
	buttons("button"),
	side("side", SIDE_TOP),
	button_icon("button_icon"),
	button_icon_and_text("button_icon_and_text"),
	wrap("wrap", true),
	min_button_width("min_button_width", 0),
	max_button_width("max_button_width", S32_MAX),
	button_height("button_height"),
	pad_left("pad_left"),
	pad_top("pad_top"),
	pad_right("pad_right"),
	pad_bottom("pad_bottom"),
	pad_between("pad_between"),
	button_panel("button_panel")
{}

LLToolBar::LLToolBar(const Params& p)
:	LLUICtrl(p),
	mButtonType(p.button_display_mode),
	mSideType(p.side),
	mWrap(p.wrap),
	mNeedsLayout(false),
	mButtonPanel(NULL),
	mCenteringStack(NULL),
	mMinButtonWidth(llmin(p.min_button_width(), p.max_button_width())),
	mMaxButtonWidth(llmax(p.max_button_width(), p.min_button_width())),
	mButtonHeight(p.button_height),
	mPadLeft(p.pad_left),
	mPadRight(p.pad_right),
	mPadTop(p.pad_top),
	mPadBottom(p.pad_bottom),
	mPadBetween(p.pad_between)
{
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_ONLY] = p.button_icon;
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_WITH_TEXT] = p.button_icon_and_text;
}

void LLToolBar::initFromParams(const LLToolBar::Params& p)
{
	LLLayoutStack::ELayoutOrientation orientation = getOrientation(p.side);

	LLLayoutStack::Params centering_stack_p;
	centering_stack_p.name = "centering_stack";
	centering_stack_p.rect = getLocalRect();
	centering_stack_p.follows.flags = FOLLOWS_ALL;
	centering_stack_p.orientation = orientation;

	mCenteringStack = LLUICtrlFactory::create<LLLayoutStack>(centering_stack_p);
	addChild(mCenteringStack);
	
	LLLayoutPanel::Params border_panel_p;
	border_panel_p.name = "border_panel";
	border_panel_p.rect = getLocalRect();
	border_panel_p.auto_resize = true;
	border_panel_p.user_resize = false;
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	LLLayoutPanel::Params center_panel_p;
	center_panel_p.name = "center_panel";
	center_panel_p.rect = getLocalRect();
	center_panel_p.auto_resize = false;
	center_panel_p.user_resize = false;
	center_panel_p.fit_content = true;
	LLLayoutPanel* center_panel = LLUICtrlFactory::create<LLLayoutPanel>(center_panel_p);
	mCenteringStack->addChild(center_panel);

	LLPanel::Params button_panel_p(p.button_panel);
	button_panel_p.rect = center_panel->getLocalRect();
	switch(p.side())
	{
	case SIDE_LEFT:
		button_panel_p.follows.flags = FOLLOWS_BOTTOM|FOLLOWS_LEFT;
		break;
	case SIDE_RIGHT:
		button_panel_p.follows.flags = FOLLOWS_BOTTOM|FOLLOWS_RIGHT;
		break;
	case SIDE_TOP:
		button_panel_p.follows.flags = FOLLOWS_TOP|FOLLOWS_LEFT;
		break;
	case SIDE_BOTTOM:
		button_panel_p.follows.flags = FOLLOWS_BOTTOM|FOLLOWS_LEFT;
		break;
	}
	mButtonPanel = LLUICtrlFactory::create<LLPanel>(button_panel_p);
	center_panel->addChild(mButtonPanel);
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	BOOST_FOREACH (LLToolBarButton::Params button_p, p.buttons)
	{
		button_p.fillFrom(mButtonParams[mButtonType]);
		LLToolBarButton* buttonp = LLUICtrlFactory::create<LLToolBarButton>(button_p);

		mButtons.push_back(buttonp);
		mButtonPanel->addChild(buttonp);

		mNeedsLayout = true;
	}
}

bool LLToolBar::addCommand(LLCommand * command)
{
	//
	// Init basic toolbar button params
	//
	LLToolBarButton::Params button_p(mButtonParams[mButtonType]);
	button_p.name = command->name();
	button_p.label = LLTrans::getString(command->labelRef());
	button_p.tool_tip = LLTrans::getString(command->tooltipRef());

	//
	// Add it to the list of buttons
	//
	LLToolBarButton * toolbar_button = LLUICtrlFactory::create<LLToolBarButton>(button_p);
	mButtons.push_back(toolbar_button);
	mButtonPanel->addChild(toolbar_button);

	mNeedsLayout = true;

	return true;
}

void LLToolBar::resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth)
{
	// make buttons in current row all same girth
	BOOST_FOREACH(LLToolBarButton* button, buttons_in_row)
	{
		if (getOrientation(mSideType) == LLLayoutStack::HORIZONTAL)
		{
			button->reshape(llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth), max_row_girth);
		}
		else // VERTICAL
		{
			button->reshape(max_row_girth, button->getRect().getHeight());
		}
	}
}

void LLToolBar::updateLayoutAsNeeded()
{
	if (!mNeedsLayout) return;

	LLLayoutStack::ELayoutOrientation orientation = getOrientation(mSideType);

	// our terminology for orientation-agnostic layout is such that
	// length refers to a distance in the direction we stack the buttons 
	// and girth refers to a distance in the direction buttons wrap
	S32 max_row_girth = 0;
	S32 max_row_length = 0;

	S32 max_length;
	S32 max_total_girth;
	S32 cur_start;
	S32 cur_row ;
	S32 row_pad_start;
	S32 row_pad_end;
	S32 girth_pad_end;
	S32 row_running_length;

	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		max_length = getRect().getWidth() - mPadLeft - mPadRight;
		max_total_girth = getRect().getHeight() - mPadTop - mPadBottom;
		row_pad_start = mPadLeft;
		row_running_length = row_pad_start;
		row_pad_end = mPadRight;
		cur_row = mPadTop;
		girth_pad_end = mPadBottom;
	}
	else // VERTICAL
	{
		max_length = getRect().getHeight() - mPadTop - mPadBottom;
		max_total_girth = getRect().getWidth() - mPadLeft - mPadRight;
		row_pad_start = mPadTop;
		row_running_length = row_pad_start;
		row_pad_end = mPadBottom;
		cur_row = mPadLeft;
		girth_pad_end = mPadRight;
	}
	
	cur_start = row_pad_start;


	LLRect panel_rect = mButtonPanel->getLocalRect();

	std::vector<LLToolBarButton*> buttons_in_row;

	BOOST_FOREACH(LLToolBarButton* button, mButtons)
	{
		button->reshape(mMinButtonWidth, mButtonHeight);
		button->autoResize();

		S32 button_clamped_width = llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth);
		S32 button_length = (orientation == LLLayoutStack::HORIZONTAL)
							? button_clamped_width
							: button->getRect().getHeight();
		S32 button_girth = (orientation == LLLayoutStack::HORIZONTAL)
							? button->getRect().getHeight()
							: button_clamped_width;
		
		// wrap if needed
		if (mWrap
			&& row_running_length + button_length > max_length	// out of room...
			&& cur_start != row_pad_start)						// ...and not first button in row
		{
			if (orientation == LLLayoutStack::VERTICAL)
			{	// row girth (width in this case) is clamped to allowable button widths
				max_row_girth = llclamp(max_row_girth, mMinButtonWidth, mMaxButtonWidth);
			}

			// make buttons in current row all same girth
			resizeButtonsInRow(buttons_in_row, max_row_girth);
			buttons_in_row.clear();

			max_row_length = llmax(max_row_length, row_running_length);
			row_running_length = row_pad_start;
			cur_start = row_pad_start;
			cur_row += max_row_girth + mPadBetween;
			max_row_girth = 0;
		}

		LLRect button_rect;
		if (orientation == LLLayoutStack::HORIZONTAL)
		{
			button_rect.setLeftTopAndSize(cur_start, panel_rect.mTop - cur_row, button_clamped_width, button->getRect().getHeight());
		}
		else // VERTICAL
		{
			button_rect.setLeftTopAndSize(cur_row, panel_rect.mTop - cur_start, button_clamped_width, button->getRect().getHeight());
		}
		button->setShape(button_rect);
		
		buttons_in_row.push_back(button);

		row_running_length += button_length + mPadBetween;
		cur_start = row_running_length;
		max_row_girth = llmax(button_girth, max_row_girth);
	}

	// final resizing in "girth" direction
	S32 total_girth =	cur_row				// current row position...
						+ max_row_girth		// ...incremented by size of final row...
						+ girth_pad_end;	// ...plus padding reserved on end
	max_row_length = llmax(max_row_length, row_running_length - mPadBetween + row_pad_end);

	resizeButtonsInRow(buttons_in_row, max_row_girth);

	// grow and optionally shift toolbar to accommodate buttons
	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		if (mSideType == SIDE_TOP)
		{ // shift down to maintain top edge
			mButtonPanel->translate(0, mButtonPanel->getRect().getHeight() - total_girth);
		}

		mButtonPanel->reshape(max_row_length, total_girth);
	}
	else // VERTICAL
	{
		if (mSideType == SIDE_RIGHT)
		{ // shift left to maintain right edge
			mButtonPanel->translate(mButtonPanel->getRect().getWidth() - total_girth, 0);
		}
		mButtonPanel->reshape(total_girth, max_row_length);
	}

	// re-center toolbar buttons
	mCenteringStack->updateLayout();

	// don't clear flag until after we've resized ourselves, to avoid laying out every frame
	mNeedsLayout = false;
}


void LLToolBar::draw()
{
	updateLayoutAsNeeded();
	LLUICtrl::draw();
}

void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	mNeedsLayout = true;
}

