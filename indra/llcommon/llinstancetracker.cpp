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
#include "llapr.h"

// STL headers
// std headers
// external library headers
// other Linden headers

void LLInstanceTrackerBase::StaticBase::incrementDepth()
{
	apr_atomic_inc32(&sIterationNestDepth);
}

void LLInstanceTrackerBase::StaticBase::decrementDepth()
{
	llassert(sIterationNestDepth);
	apr_atomic_dec32(&sIterationNestDepth);
}

U32 LLInstanceTrackerBase::StaticBase::getDepth()
{
	apr_uint32_t data = apr_atomic_read32(&sIterationNestDepth);
	return data;
}
