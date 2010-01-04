/** 
 * @file lltransientfloatermgr.cpp
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "lltransientfloatermgr.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llviewerwindow.h"
#include "lldockablefloater.h"
#include "llmenugl.h"


LLTransientFloaterMgr::LLTransientFloaterMgr()
{
	gViewerWindow->getRootView()->addMouseDownCallback(boost::bind(
			&LLTransientFloaterMgr::leftMouseClickCallback, this, _1, _2, _3));
}

void LLTransientFloaterMgr::registerTransientFloater(LLFloater* floater)
{
	mTransSet.insert(floater);
}

void LLTransientFloaterMgr::unregisterTransientFloater(LLFloater* floater)
{
	mTransSet.erase(floater);
}

void LLTransientFloaterMgr::addControlView(LLView* view)
{
	mControlsSet.insert(view);
}

void LLTransientFloaterMgr::removeControlView(LLView* view)
{
	// we will still get focus lost callbacks on this view, but that's ok
	// since we run sanity checking logic every time
	mControlsSet.erase(view);
}

void LLTransientFloaterMgr::hideTransientFloaters()
{
	for (std::set<LLFloater*>::iterator it = mTransSet.begin(); it
			!= mTransSet.end(); it++)
	{
		LLFloater* floater = *it;
		if (floater->isDocked())
		{
			floater->setVisible(FALSE);
		}
	}
}

void LLTransientFloaterMgr::leftMouseClickCallback(S32 x, S32 y,
		MASK mask)
{
	bool hide = true;
	for (controls_set_t::iterator it = mControlsSet.begin(); it
			!= mControlsSet.end(); it++)
	{
		// don't hide transient floater if any context menu opened
		if (LLMenuGL::sMenuContainer->getVisibleMenu() != NULL)
		{
			hide = false;
			break;
		}

		LLView* control_view = *it;
		if (!control_view->getVisible())
		{
			continue;
		}

		LLRect rect = control_view->calcScreenRect();
		// if click inside view rect
		if (rect.pointInRect(x, y))
		{
			hide = false;
			break;
		}
	}

	if (hide)
	{
		hideTransientFloaters();
	}
}

