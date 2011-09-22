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

#include "boost/foreach.hpp"
#include "lltoolbar.h"

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
	min_width("min_width", 0),
	max_width("max_width", S32_MAX),
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
	mMinWidth(p.min_width),
	mMaxWidth(p.max_width),
	mBackgroundImage(p.background_image)
{
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

	addRow();

	BOOST_FOREACH (LLToolBarButton::Params button_p, p.buttons)
	{
		LLRect button_rect(button_p.rect);
		{ // remove any offset from button
			if (orientation == LLLayoutStack::HORIZONTAL)
			{
				button_rect.setOriginAndSize(0, 0, mMinWidth, getRect().getHeight());
			}
			else // VERTICAL
			{
				button_rect.setOriginAndSize(0, 0, mMinWidth, button_rect.getHeight());
			}
		}

		button_p.fillFrom((mButtonType == BTNTYPE_ICONS_ONLY)
							? p.button_icon						// icon only
							: p.button_icon_and_text);			// icon + text

		mButtons.push_back(LLUICtrlFactory::create<LLToolBarButton>(button_p));
		mNeedsLayout = true;
	}

	updateLayout();
}

void LLToolBar::addButton(LLToolBarButton* buttonp)
{
	LLLayoutPanel::Params panel_p;
	panel_p.name = buttonp->getName() + "_panel";
	panel_p.user_resize = false;
	panel_p.auto_resize= false;
	panel_p.fit_content = true;

	LLLayoutPanel* panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	
	panel->addChild(buttonp);
	mStacks.back()->addChild(panel);
}

void LLToolBar::updateLayout()
{
	mCenteringStack->updateLayout();

	if (!mNeedsLayout) return;
	mNeedsLayout = false;

	{ // clean up existing rows
		BOOST_FOREACH(LLToolBarButton* button, mButtons)
		{
			if (button->getParent())
			{
				button->getParent()->removeChild(button);
			}
		}	

		BOOST_FOREACH(LLLayoutStack* stack, mStacks)
		{
			delete stack;
		}
		mStacks.clear();
	}
	// start with one row of buttons
	addRow();

	S32 total_width = 0, total_height = 0;
	S32 max_total_width = 0, max_total_height = 0;
	S32 max_width = getRect().getWidth(), max_height = getRect().getHeight();
	BOOST_FOREACH(LLToolBarButton* button, mButtons)
	{
		S32 button_width = button->getRect().getWidth();
		S32 button_height = button->getRect().getHeight();

		if (getOrientation(mSideType) == LLLayoutStack::HORIZONTAL
			&& total_width + button_height > getRect().getWidth())
		{
			addRow();
			total_width = 0;
		}
		addButton(button);

		total_width += button_width;
		total_height += button_height;
		max_total_width = llmax(max_total_width, total_width);
		max_total_height = llmax(max_total_height, total_height);
		max_width = llmax(button->getRect().getWidth(), max_width);
		max_height = llmax(button->getRect().getHeight(), max_height);
	}

	if (getOrientation(mSideType) == LLLayoutStack::HORIZONTAL)
	{
		BOOST_FOREACH(LLLayoutStack* stack, mStacks)
		{
			stack->reshape(max_total_width, stack->getParent()->getRect().getHeight());
			stack->updateLayout();
		}
	}
	else
	{
		BOOST_FOREACH(LLLayoutStack* stack, mStacks)
		{
			stack->reshape(stack->getParent()->getRect().getWidth(), max_total_height);
			stack->updateLayout();
		}

		reshape(max_total_width, getRect().getHeight());
	}
}


void LLToolBar::draw()
{
	updateLayout();

	{	// draw background
		LLRect bg_rect;
		localRectToOtherView(mCenterPanel->getRect(),&bg_rect, this);
		mBackgroundImage->draw(bg_rect);		 
	}
	LLUICtrl::draw();
}

void LLToolBar::addRow()
{
	LLLayoutStack::ELayoutOrientation orientation = getOrientation(mSideType);

	LLLayoutStack::Params stack_p;
	stack_p.rect = getLocalRect();
	stack_p.name = llformat("button_stack_%d", mStacks.size());
	stack_p.orientation = orientation;
	stack_p.follows.flags = (orientation == LLLayoutStack::HORIZONTAL)
		? (FOLLOWS_TOP|FOLLOWS_BOTTOM)  // horizontal
		: (FOLLOWS_LEFT|FOLLOWS_RIGHT); // vertical

	mStacks.push_back(LLUICtrlFactory::create<LLLayoutStack>(stack_p));
	mCenterPanel->addChild(mStacks.back());
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
