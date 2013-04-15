/**
 * @file   lllinstancetracker.cpp
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "llinstancetracker.h"
// STL headers
// std headers
// external library headers
// other Linden headers

static bool sInstanceTrackerData_initialized = false;
static void* sInstanceTrackerData[ kInstanceTrackTypeCount ];


void * & LLInstanceTrackerBase::getInstances(InstanceTrackType t)
{
	// std::map::insert() is just what we want here. You attempt to insert a
	// (key, value) pair. If the specified key doesn't yet exist, it inserts
	// the pair and returns a std::pair of (iterator, true). If the specified
	// key DOES exist, insert() simply returns (iterator, false). One lookup
	// handles both cases.
	if (!sInstanceTrackerData_initialized)
	{
		for (S32 i = 0; i < (S32) kInstanceTrackTypeCount; i++)
		{
			sInstanceTrackerData[i] = NULL;
		}
		sInstanceTrackerData_initialized = true;
	}

	return sInstanceTrackerData[t];
}

