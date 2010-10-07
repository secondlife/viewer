/** 
 * @file lltransientfloatermgr.cpp
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lltransientfloatermgr.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llviewerwindow.h"
#include "lldockablefloater.h"
#include "llmenugl.h"


LLTransientFloaterMgr::LLTransientFloaterMgr()
{
	gViewerWindow->getRootView()->getChild<LLUICtrl>("popup_holder")->setMouseDownCallback(boost::bind(
			&LLTransientFloaterMgr::leftMouseClickCallback, this, _2, _3, _4));

	mGroupControls.insert(std::pair<ETransientGroup, std::set<LLView*> >(GLOBAL, std::set<LLView*>()));
	mGroupControls.insert(std::pair<ETransientGroup, std::set<LLView*> >(DOCKED, std::set<LLView*>()));
	mGroupControls.insert(std::pair<ETransientGroup, std::set<LLView*> >(IM, std::set<LLView*>()));
}

void LLTransientFloaterMgr::registerTransientFloater(LLTransientFloater* floater)
{
	mTransSet.insert(floater);
}

void LLTransientFloaterMgr::unregisterTransientFloater(LLTransientFloater* floater)
{
	mTransSet.erase(floater);
}

void LLTransientFloaterMgr::addControlView(ETransientGroup group, LLView* view)
{
	mGroupControls.find(group)->second.insert(view);
}

void LLTransientFloaterMgr::removeControlView(ETransientGroup group, LLView* view)
{
	mGroupControls.find(group)->second.erase(view);
}

void LLTransientFloaterMgr::addControlView(LLView* view)
{
	addControlView(GLOBAL, view);
}

void LLTransientFloaterMgr::removeControlView(LLView* view)
{
	// we will still get focus lost callbacks on this view, but that's ok
	// since we run sanity checking logic every time
	removeControlView(GLOBAL, view);
}

void LLTransientFloaterMgr::hideTransientFloaters(S32 x, S32 y)
{
	for (std::set<LLTransientFloater*>::iterator it = mTransSet.begin(); it
			!= mTransSet.end(); it++)
	{
		LLTransientFloater* floater = *it;
		if (floater->isTransientDocked())
		{
			ETransientGroup group = floater->getGroup();

			bool hide = isControlClicked(mGroupControls.find(group)->second, x, y);
			if (hide)
			{
				floater->setTransientVisible(FALSE);
			}
		}
	}
}

bool LLTransientFloaterMgr::isControlClicked(std::set<LLView*>& set, S32 x, S32 y)
{
	bool res = true;
	for (controls_set_t::iterator it = set.begin(); it
			!= set.end(); it++)
	{
		LLView* control_view = *it;
		if (!control_view->getVisible())
		{
			continue;
		}

		LLRect rect = control_view->calcScreenRect();
		// if click inside view rect
		if (rect.pointInRect(x, y))
		{
			res = false;
			break;
		}
	}
	return res;
}

void LLTransientFloaterMgr::leftMouseClickCallback(S32 x, S32 y,
		MASK mask)
{
	// don't hide transient floater if any context menu opened
	if (LLMenuGL::sMenuContainer->getVisibleMenu() != NULL)
	{
		return;
	}

	bool hide = isControlClicked(mGroupControls.find(DOCKED)->second, x, y)
			&& isControlClicked(mGroupControls.find(GLOBAL)->second, x, y);
	if (hide)
	{
		hideTransientFloaters(x, y);
	}
}

void LLTransientFloater::init(LLFloater* thiz)
{
	// used since LLTransientFloater(this) can't be used in descendant constructor parameter initialization.
	mFloater = thiz;
}

