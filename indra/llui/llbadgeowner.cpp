/** 
 * @file llbadgeowner.cpp
 * @brief Class to manage badges attached to a UI control
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

#include "linden_common.h"

#include "llbadgeowner.h"
#include "llpanel.h"

//
// Classes
//

LLBadgeOwner::LLBadgeOwner(LLHandle< LLView > viewHandle)
	: mBadge(NULL)
	, mBadgeOwnerView(viewHandle)
{
}

void LLBadgeOwner::initBadgeParams(const LLBadge::Params& p)
{
	if (!p.equals(LLUICtrlFactory::getDefaultParams<LLBadge>()))
	{
		mBadge = createBadge(p);
	}
}

void LLBadgeOwner::setBadgeLabel(const LLStringExplicit& label)
{
	if (mBadge == NULL)
	{
		mBadge = createBadge(LLUICtrlFactory::getDefaultParams<LLBadge>());

		addBadgeToParentPanel();
	}

	if (mBadge)
	{
		mBadge->setLabel(label);

		//
		// Push the badge to the front so it renders on top
		//

		LLView * parent = mBadge->getParent();

		if (parent)
		{
			parent->sendChildToFront(mBadge);
		}
	}
}

void LLBadgeOwner::setBadgeVisibility(bool visible)
{
	if (mBadge)
	{
		mBadge->setVisible(visible);
	}
}

void LLBadgeOwner::addBadgeToParentPanel()
{
	LLView * owner_view = mBadgeOwnerView.get();
	
	if (mBadge && owner_view)
	{
		// Badge parent is badge owner by default
		LLView * badge_parent = owner_view;

		// Find the appropriate parent for the badge
		LLView * parent = owner_view->getParent();

		while (parent)
		{
			LLPanel * parent_panel = dynamic_cast<LLPanel *>(parent);

			if (parent_panel && parent_panel->acceptsBadge())
			{
				badge_parent = parent;
				break;
			}

			parent = parent->getParent();
		}

		if (badge_parent)
		{
			badge_parent->addChild(mBadge);
		}
		else
		{
			llwarns << "Unable to find parent panel for badge " << mBadge->getName() << " on " << owner_view->getName() << llendl;
		}
	}
}

LLBadge* LLBadgeOwner::createBadge(const LLBadge::Params& p)
{
	LLBadge::Params badge_params(p);
	badge_params.owner = mBadgeOwnerView;

	return LLUICtrlFactory::create<LLBadge>(badge_params);
}
