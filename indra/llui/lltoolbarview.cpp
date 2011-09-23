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

LLToolBarView::LLToolBarView(const LLToolBarView::Params& p)
:	LLUICtrl(p)
{
}

void LLToolBarView::initFromParams(const LLToolBarView::Params& p)
{
	// Initialize the base object
	LLUICtrl::initFromParams(p);
}

LLToolBarView::~LLToolBarView()
{
}

void LLToolBarView::draw()
{
	static bool debug_print = true;
	static S32 old_width = 0;
	static S32 old_height = 0;

	LLToolBar* toolbar_bottom = getChild<LLToolBar>("toolbar_bottom");
	LLToolBar* toolbar_left = getChild<LLToolBar>("toolbar_left");
	LLToolBar* toolbar_right = getChild<LLToolBar>("toolbar_right");
	LLPanel* sizer_left = getChild<LLPanel>("sizer_left");
	
	LLRect bottom_rect = toolbar_bottom->getRect();
	LLRect left_rect = toolbar_left->getRect();
	LLRect right_rect = toolbar_right->getRect();
	LLRect sizer_left_rect = sizer_left->getRect();
	
	if ((old_width != getRect().getWidth()) || (old_height != getRect().getHeight()))
		debug_print = true;
	if (debug_print)
	{
		LLRect ctrl_rect = getRect();
		llinfos << "Merov debug : draw control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw bottom  rect = " << bottom_rect.mLeft << ", " << bottom_rect.mTop << ", " << bottom_rect.mRight << ", " << bottom_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw left    rect = " << left_rect.mLeft << ", " << left_rect.mTop << ", " << left_rect.mRight << ", " << left_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw right   rect = " << right_rect.mLeft << ", " << right_rect.mTop << ", " << right_rect.mRight << ", " << right_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw s left  rect = " << sizer_left_rect.mLeft << ", " << sizer_left_rect.mTop << ", " << sizer_left_rect.mRight << ", " << sizer_left_rect.mBottom << llendl; 
		old_width = ctrl_rect.getWidth();
		old_height = ctrl_rect.getHeight();
		debug_print = false;
	}
	// Debug draw
	LLColor4 back_color = LLColor4::blue;
	back_color[VALPHA] = 0.5f;
//	gl_rect_2d(getLocalRect(), back_color, TRUE);
//	gl_rect_2d(bottom_rect, LLColor4::red, TRUE);
//	gl_rect_2d(left_rect, LLColor4::green, TRUE);
//	gl_rect_2d(right_rect, LLColor4::yellow, TRUE);
	
	LLUICtrl::draw();
}
