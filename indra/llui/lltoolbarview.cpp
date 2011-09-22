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
#include "llbutton.h"

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");

LLToolBarView::LLToolBarView(const Params& p)
:	LLUICtrl(p)
{
}

BOOL LLToolBarView::postBuild()
{
	LLButton* btn = getChild<LLButton>("color_pipette");
	btn->setVisible(TRUE);
	LLRect ctrl_rect = getRect();
	LLRect btn_rect = btn->getRect();
	llinfos << "Merov debug : control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
	llinfos << "Merov debug : button rect = " << btn_rect.mLeft << ", " << btn_rect.mTop << ", " << btn_rect.mRight << ", " << btn_rect.mBottom << llendl; 
	btn_rect.mLeft = 0;
	btn_rect.mTop = ctrl_rect.getHeight();
	btn_rect.mRight = 28;
	btn_rect.mBottom = btn_rect.mTop - 28;
	btn->setRect(btn_rect);
	btn_rect = btn->getRect();
	llinfos << "Merov debug : button rect = " << btn_rect.mLeft << ", " << btn_rect.mTop << ", " << btn_rect.mRight << ", " << btn_rect.mBottom << llendl; 
	return TRUE;
}

void LLToolBarView::draw()
{
	LLButton* btn = getChild<LLButton>("color_pipette");
	btn->setVisible(TRUE);
	static bool debug_print = true;
	if (debug_print)
	{
		LLRect ctrl_rect = getRect();
		LLRect btn_rect = btn->getRect();
		llinfos << "Merov debug : draw control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw button rect = " << btn_rect.mLeft << ", " << btn_rect.mTop << ", " << btn_rect.mRight << ", " << btn_rect.mBottom << llendl; 
		debug_print = false;
	}
	LLUICtrl::draw();
}
