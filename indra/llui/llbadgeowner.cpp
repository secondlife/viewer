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

#include "llbadgeholder.h"
#include "llbadgeowner.h"
#include "llpanel.h"

//
// Classes
//

LLBadgeOwner::LLBadgeOwner(LLHandle< LLView > viewHandle)
	: mHasBadgeHolderParent(false),
	mBadge(NULL),
	mBadgeOwnerView(viewHandle)
{
}

void LLBadgeOwner::initBadgeParams(const LLBadge::Params& p)
{
	if (!p.equals(LLUICtrlFactory::getDefaultParams<LLBadge>()))
	{
		mBadge = createBadge(p);
		mHasBadgeHolderParent = false;

		LLView * owner_view = mBadgeOwnerView.get();
		if (owner_view)
		{
			mBadge->addToView(owner_view);
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

void LLBadgeOwner::setDrawBadgeAtTop(bool draw_at_top)
{
	if (mBadge)
	{
		mBadge->setDrawAtParentTop(draw_at_top);
	}
}

void LLBadgeOwner::addBadgeToParentHolder()
{
	LLView * owner_view = mBadgeOwnerView.get();
	
	if (mBadge && owner_view)
	{
		LLBadgeHolder * badge_holder = NULL;

		// Find the appropriate holder for the badge
		LLView * parent = owner_view->getParent();

		while (parent)
		{
			LLBadgeHolder * badge_holder_panel = dynamic_cast<LLBadgeHolder *>(parent);

			if (badge_holder_panel && badge_holder_panel->acceptsBadge())
			{
				badge_holder = badge_holder_panel;
				break;
			}

			parent = parent->getParent();
		}

		if (badge_holder)
		{
			mHasBadgeHolderParent = badge_holder->addBadge(mBadge);
		}
	}
}

LLBadge* LLBadgeOwner::createBadge(const LLBadge::Params& p)
{
	LLBadge::Params badge_params(p);
	badge_params.owner = mBadgeOwnerView;

	return LLUICtrlFactory::create<LLBadge>(badge_params);
}
