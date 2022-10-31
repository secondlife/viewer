/** 
 * @file llbadgeholder.cpp
 * @brief Source for badge holders
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

#include "llbadgeholder.h"

#include "llbadge.h"
#include "llview.h"


bool LLBadgeHolder::addBadge(LLBadge * badge)
{
    bool badge_added = false;

    LLView * this_view = dynamic_cast<LLView *>(this);

    if (this_view && mAcceptsBadge)
    {
        badge_added = badge->addToView(this_view);
    }

    return badge_added;
}
