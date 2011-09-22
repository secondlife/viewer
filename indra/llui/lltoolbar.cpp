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

LLToolBar::Params::Params()
:	button_display_mode("button_display_mode"),
	buttons("button"),
	side("side"),
	button_icon("button_icon"),
	button_icon_and_text("button_icon_and_text"),
	wrap("wrap", true),
	min_button_width("min_button_width", 0),
	max_button_width("max_button_width", S32_MAX),
	background_image("background_image")
{}

LLToolBar::LLToolBar(const Params& p)
:	LLUICtrl(p),
	mButtonType(p.button_display_mode),
	mSideType(p.side),
	mWrap(p.wrap),
	mNeedsLayout(false),
	mCenterPanel(NULL),
	mCenteringStack(NULL),
	mMinButtonWidth(p.min_button_width),
	mMaxButtonWidth(p.max_button_width),
	mBackgroundImage(p.background_image)
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
	mCenterPanel = LLUICtrlFactory::create<LLLayoutPanel>(center_panel_p);
	mCenteringStack->addChild(mCenterPanel);
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	BOOST_FOREACH (LLToolBarButton::Params button_p, p.buttons)
	{
		// buttons always follow left and top, for all orientations
		button_p.follows.flags = FOLLOWS_LEFT|FOLLOWS_TOP;
		button_p.fillFrom(mButtonParams[mButtonType]);

		LLRect button_rect(button_p.rect);
		{ // remove any offset from button
			if (orientation == LLLayoutStack::HORIZONTAL)
			{
				button_rect.setOriginAndSize(0, 0, mMinButtonWidth, button_rect.getHeight());
			}
			else // VERTICAL
			{
				button_rect.setOriginAndSize(0, 0, mMinButtonWidth, button_rect.getHeight());
			}
		}

		// use our calculated rect
		button_p.rect = button_rect;
		LLToolBarButton* buttonp = LLUICtrlFactory::create<LLToolBarButton>(button_p);

		mButtons.push_back(buttonp);
		mCenterPanel->addChild(buttonp);

		mNeedsLayout = true;
	}
}

bool LLToolBar::addCommand(LLCommand * command)
{
	//
	// Init basic toolbar button params
	//

	LLToolBarButton::Params button_p;
	button_p.fillFrom(mButtonParams[mButtonType]);

	button_p.name = command->name();
	button_p.label = LLTrans::getString(command->labelRef());
	button_p.tool_tip = LLTrans::getString(command->tooltipRef());

	//
	// Set up the button rectangle
	//

	S32 btn_width = mMinButtonWidth;
	S32 btn_height = mButtonParams[mButtonType].rect.height;

	if ((mSideType == LLToolBarEnums::SIDE_BOTTOM) || (mSideType == LLToolBarEnums::SIDE_TOP))
	{
		btn_height = getRect().getHeight();
	}

	LLRect button_rect;
	button_rect.setOriginAndSize(0, 0, btn_width, btn_height);

	button_p.rect = button_rect;

	//
	// Add it to the list of buttons
	//

	LLToolBarButton * toolbar_button = LLUICtrlFactory::create<LLToolBarButton>(button_p);

	toolbar_button->reshape(llclamp(toolbar_button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth), toolbar_button->getRect().getHeight());

	mButtons.push_back(toolbar_button);
	mCenterPanel->addChild(toolbar_button);

	mNeedsLayout = true;

	return true;
}

void LLToolBar::updateLayoutAsNeeded()
{
	if (!mNeedsLayout) return;

	LLLayoutStack::ELayoutOrientation orientation = getOrientation(mSideType);
	
	// our terminology for orientation-agnostic layout is that
	// length refers to a distance in the direction we stack the buttons 
	// and girth refers to a distance in the direction buttons wrap
	S32 row_running_length = 0;
	S32 max_length = (orientation == LLLayoutStack::HORIZONTAL)
					? getRect().getWidth()
					: getRect().getHeight();
	S32 max_row_girth = 0;
	S32 cur_start = 0;
	S32 cur_row = 0;

	LLRect panel_rect = mCenterPanel->getLocalRect();

	std::vector<LLToolBarButton*> buttons_in_row;

		BOOST_FOREACH(LLToolBarButton* button, mButtons)
		{
		S32 button_clamped_width = llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth);
		S32 button_length = (orientation == LLLayoutStack::HORIZONTAL)
							? button_clamped_width
							: button->getRect().getHeight();
		S32 button_girth = (orientation == LLLayoutStack::HORIZONTAL)
							? button->getRect().getHeight()
							: button_clamped_width;
		
		// handle wrapping
		if (row_running_length + button_length > max_length 
			&& cur_start != 0) // not first button in row
		{ // go ahead and wrap
			if (orientation == LLLayoutStack::VERTICAL)
			{
				// row girth is clamped to allowable button widths
				max_row_girth = llclamp(max_row_girth, mMinButtonWidth, mMaxButtonWidth);
			}
			// make buttons in current row all same girth
			BOOST_FOREACH(LLToolBarButton* button, buttons_in_row)
			{
				if (orientation == LLLayoutStack::HORIZONTAL)
				{
					button->reshape(llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth), max_row_girth);
		}	
				else // VERTICAL
		{
					button->reshape(max_row_girth, button->getRect().getHeight());
				}
		}
			buttons_in_row.clear();

			row_running_length = 0;
			cur_start = 0;
			cur_row += max_row_girth;
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

		row_running_length += button_length;
		cur_start = row_running_length;
		max_row_girth = llmax(button_girth, max_row_girth);
	}

	// final resizing in "girth" direction
	S32 total_girth = cur_row + max_row_girth; // increment by size of final row

	// grow and optionally shift toolbar to accomodate buttons
	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		if (mSideType == SIDE_TOP)
		{ // shift down to maintain top edge
			translate(0, getRect().getHeight() - total_girth);
		}

		reshape(getRect().getWidth(), total_girth);
	}
	else // VERTICAL
	{
		if (mSideType == SIDE_RIGHT)
		{ // shift left to maintain right edge
			translate(getRect().getWidth() - total_girth, 0);
		}
		reshape(total_girth, getRect().getHeight());
		}

	// recenter toolbar buttons
	mCenteringStack->updateLayout();

	// don't clear flag until after we've resized ourselves, to avoid laying out every frame
	mNeedsLayout = false;
}


void LLToolBar::draw()
{
	updateLayoutAsNeeded();

	{	// draw background
		LLRect bg_rect;
		localRectToOtherView(mCenterPanel->getRect(),&bg_rect, this);
		mBackgroundImage->draw(bg_rect);		 
	}
	LLUICtrl::draw();
}

void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	mNeedsLayout = true;
}

namespace LLInitParam
{
	void TypeValues<ButtonType>::declareValues()
	{
		declare("icons_only",		BTNTYPE_ICONS_ONLY);
		declare("icons_with_text",	BTNTYPE_ICONS_WITH_TEXT);
	}

	void TypeValues<SideType>::declareValues()
	{
		declare("none",		SIDE_NONE);
		declare("bottom",	SIDE_BOTTOM);
		declare("left",		SIDE_LEFT);
		declare("right",	SIDE_RIGHT);
		declare("top",		SIDE_TOP);
	}
}
