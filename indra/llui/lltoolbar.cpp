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

//static LLDefaultChildRegistry::Register<LLToolBar> r1("toolbar");

LLToolBar::Params::Params()
:	orientation("orientation"),
	buttons("button")
{}

LLToolBar::LLToolBar(const Params& p)
:	LLUICtrl(p),
	mOrientation(p.orientation),
	mStack(NULL)
{}

void LLToolBar::initFromParams(const LLToolBar::Params& p)
{
	LLLayoutStack::Params centering_stack_p;
	centering_stack_p.rect = getLocalRect();
	centering_stack_p.follows.flags = FOLLOWS_ALL;
	centering_stack_p.orientation = p.orientation;
	centering_stack_p.name = "centering_stack";

	LLLayoutPanel::Params border_panel_p;
	border_panel_p.name = "border_panel";
	border_panel_p.rect = getLocalRect();
	border_panel_p.auto_resize = true;
	border_panel_p.user_resize = false;

	LLLayoutStack* centering_stack = LLUICtrlFactory::create<LLLayoutStack>(centering_stack_p);
	addChild(centering_stack);
	
	LLLayoutPanel::Params center_panel_p;
	center_panel_p.name = "center_panel";
	center_panel_p.rect = getLocalRect();
	center_panel_p.auto_resize = false;
	center_panel_p.user_resize = false;
	center_panel_p.fit_content = true;

	centering_stack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));
	LLLayoutPanel* center_panel = LLUICtrlFactory::create<LLLayoutPanel>(center_panel_p);
	centering_stack->addChild(center_panel);
	centering_stack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	LLLayoutStack::Params stack_p;
	stack_p.rect = getLocalRect();
	stack_p.name = "button_stack";
	stack_p.orientation = p.orientation;
	stack_p.follows.flags = (mOrientation == LLLayoutStack::HORIZONTAL)
							? (FOLLOWS_TOP|FOLLOWS_BOTTOM)  // horizontal
							: (FOLLOWS_LEFT|FOLLOWS_RIGHT); // vertical

	mStack = LLUICtrlFactory::create<LLLayoutStack>(stack_p);
	center_panel->addChild(mStack);

	BOOST_FOREACH (LLToolBarButton::Params button_p, p.buttons)
	{
		// remove any offset from button
		LLRect button_rect(button_p.rect);

		if (mOrientation == LLLayoutStack::HORIZONTAL)
		{
			button_rect.setOriginAndSize(0, 0, 0, getRect().getHeight());
		}
		else // VERTICAL
		{
			button_rect.setOriginAndSize(0, 0, 0, button_rect.getHeight());
		}
		button_p.follows.flags = FOLLOWS_NONE;
		button_p.rect = button_rect;
		button_p.chrome = true;
		button_p.auto_resize = true;

		LLToolBarButton* button = LLUICtrlFactory::create<LLToolBarButton>(button_p);

		addButton(button);
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
	mStack->addChild(panel);
	mButtons.push_back(buttonp);
}

void LLToolBar::updateLayout()
{
	S32 total_width = 0;
	S32 total_height = 0;
	S32 max_width = getRect().getWidth();
	S32 max_height = getRect().getHeight();

	BOOST_FOREACH(LLToolBarButton* button, mButtons)
	{
		total_width += button->getRect().getWidth();
		total_height += button->getRect().getHeight();
		max_width = llmax(button->getRect().getWidth(), max_width);
		max_height = llmax(button->getRect().getHeight(), max_height);
	}

	if (mOrientation == LLLayoutStack::HORIZONTAL)
	{
		mStack->reshape(total_width, mStack->getParent()->getRect().getHeight());
	}
	else
	{
		mStack->reshape(mStack->getParent()->getRect().getWidth(), total_height);
		reshape(max_width, getRect().getHeight());
	}
}


void LLToolBar::draw()
{
	LLUICtrl::draw();
}

