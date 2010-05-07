/** 
 * @file llworldview.cpp
 * @brief LLWorldView class implementation
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
}

void LLWorldView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (FALSE == gSavedSettings.getBOOL("SidebarCameraMovement") )
	{
		LLView* main_view = LLUI::getRootView()->findChild<LLView>("main_view");
		if(main_view)
		{
			width = main_view->getRect().getWidth();
		}
	}
	
	LLUICtrl::reshape(width, height, called_from_parent);
}
