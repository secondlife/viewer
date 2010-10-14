/** 
 * @file llworldview.cpp
 * @brief LLWorldView class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llworldview.h"

#include "llviewercontrol.h"
#include "llsidetray.h"
/////////////////////////////////////////////////////
// LLFloaterView

static LLDefaultChildRegistry::Register<LLWorldView> r("world_view");

LLWorldView::LLWorldView(const Params& p)
:	LLUICtrl (p)
{
	gSavedSettings.getControl("SidebarCameraMovement")->getSignal()->connect(boost::bind(&LLWorldView::toggleSidebarCameraMovement, this, _2));
}

void LLWorldView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	//if (FALSE == gSavedSettings.getBOOL("SidebarCameraMovement") )
	//{
	//	LLView* main_view = LLUI::getRootView()->findChild<LLView>("main_view");
	//	if(main_view)
	//	{
	//		width = main_view->getRect().getWidth();
	//	}
	//}
	
	LLUICtrl::reshape(width, height, called_from_parent);
}
void LLWorldView::toggleSidebarCameraMovement(const LLSD::Boolean& new_visibility)
{
	reshape(getParent()->getRect().getWidth(),getRect().getHeight());
}

