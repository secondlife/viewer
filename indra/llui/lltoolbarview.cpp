/** 
 * @file lltoolbarview.cpp
 * @author Merov Linden
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

#include "lltoolbarview.h"
#include "lltoolbar.h"
#include "llbutton.h"

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");

LLToolBarView::LLToolBarView(const Params& p)
:	LLUICtrl(p)
{
}

BOOL LLToolBarView::postBuild()
{
	LLRect ctrl_rect = getRect();
	LLButton* btn = getChild<LLButton>("test");
	LLRect btn_rect = btn->getRect();
	llinfos << "Merov debug : control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
	llinfos << "Merov debug : test    rect = " << btn_rect.mLeft << ", " << btn_rect.mTop << ", " << btn_rect.mRight << ", " << btn_rect.mBottom << llendl; 
	return TRUE;
}

void LLToolBarView::draw()
{
	static bool debug_print = true;

	LLToolBar* toolbar_bottom = getChild<LLToolBar>("toolbar_bottom");
	LLToolBar* toolbar_left = getChild<LLToolBar>("toolbar_left");
	LLToolBar* toolbar_right = getChild<LLToolBar>("toolbar_right");
	
	LLRect bottom_rect = toolbar_bottom->getRect();
	LLRect left_rect = toolbar_left->getRect();
	LLRect right_rect = toolbar_right->getRect();
	
	if (debug_print)
	{
		LLRect ctrl_rect = getRect();
		llinfos << "Merov debug : draw control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw bottom  rect = " << bottom_rect.mLeft << ", " << bottom_rect.mTop << ", " << bottom_rect.mRight << ", " << bottom_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw left    rect = " << left_rect.mLeft << ", " << left_rect.mTop << ", " << left_rect.mRight << ", " << left_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw right   rect = " << right_rect.mLeft << ", " << right_rect.mTop << ", " << right_rect.mRight << ", " << right_rect.mBottom << llendl; 
		debug_print = false;
	}
	// Debug draw
	gl_rect_2d(getLocalRect(), LLColor4::blue, TRUE);
	gl_rect_2d(bottom_rect, LLColor4::red, TRUE);
	gl_rect_2d(left_rect, LLColor4::green, TRUE);
	gl_rect_2d(right_rect, LLColor4::yellow, TRUE);
	
	LLUICtrl::draw();
}
