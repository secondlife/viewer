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
{

}

void LLToolBar::draw()
{
	gl_rect_2d(getLocalRect(), LLColor4::blue, TRUE);
}

void LLToolBar::initFromParams(const LLToolBar::Params& p)
{
	LLLayoutStack::Params stack_p;
	stack_p.rect = getLocalRect();
	stack_p.follows.flags = FOLLOWS_ALL;
	stack_p.name = "button_stack";
	stack_p.orientation = p.orientation;

	mStack = LLUICtrlFactory::create<LLLayoutStack>(stack_p);
	addChild(mStack);

	BOOST_FOREACH (LLButton::Params button_p, p.buttons)
	{
		LLLayoutPanel::Params panel_p;
		panel_p.name = button_p.name() + "_panel";
		panel_p.rect = button_p.rect;
		panel_p.user_resize = false;
		panel_p.auto_resize= false;

		LLLayoutPanel* panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
		LLButton* button = LLUICtrlFactory::create<LLButton>(button_p);
		panel->addChild(button);
		mStack->addChild(panel);
	}
}
