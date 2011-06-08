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

LLBadgeOwner::LLBadgeOwner(LLHandle<LLUICtrl> ctrlHandle)
	: mBadge(NULL)
	, mBadgeOwnerCtrl(ctrlHandle)
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

		LLUICtrl * parent = mBadge->getParentUICtrl();

		if (parent)
		{
			parent->sendChildToFront(mBadge);
		}
	}
}

void LLBadgeOwner::addBadgeToParentPanel()
{
	if (mBadge && mBadgeOwnerCtrl.get())
	{
		// Find the appropriate parent panel for the badge

		LLUICtrl * owner_ctrl = mBadgeOwnerCtrl.get();
		LLUICtrl * parent = owner_ctrl->getParentUICtrl();

		LLPanel * parentPanel = NULL;

		while (parent)
		{
			parentPanel = dynamic_cast<LLPanel *>(parent);

			if (parentPanel && parentPanel->acceptsBadge())
			{
				break;
			}

			parent = parent->getParentUICtrl();
		}

		if (parentPanel)
		{
			parentPanel->addChild(mBadge);
		}
		else
		{
			llwarns << "Unable to find parent panel for badge " << mBadge->getName() << " on ui control " << owner_ctrl->getName() << llendl;
		}
	}
}

LLBadge* LLBadgeOwner::createBadge(const LLBadge::Params& p)
{
	LLBadge::Params badge_params(p);
	badge_params.owner = mBadgeOwnerCtrl;

	return LLUICtrlFactory::create<LLBadge>(badge_params);
}
