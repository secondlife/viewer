/** 
 * @file llbadgeowner.h
 * @brief Header for badge owners
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

#ifndef LL_LLBADGEOWNER_H
#define LL_LLBADGEOWNER_H

#include "llbadge.h"
#include "llview.h"

//
// Classes
//

class LLBadgeOwner
{
public:

	LLBadgeOwner(LLHandle< LLView > viewHandle);

	void initBadgeParams(const LLBadge::Params& p);
	void addBadgeToParentHolder();
	
	bool hasBadgeHolderParent() const { return mHasBadgeHolderParent; };
	void setBadgeVisibility(bool visible);
	void setDrawBadgeAtTop(bool draw_at_top);

private:

	LLBadge* createBadge(const LLBadge::Params& p);

private:
	bool				mHasBadgeHolderParent;
	LLBadge*			mBadge;
	LLHandle< LLView >	mBadgeOwnerView;
};

#endif  // LL_LLBADGEOWNER_H
