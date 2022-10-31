/** 
 * @file llrefcount.cpp
 * @brief Base class for reference counted objects for use with LLPointer
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
#include "linden_common.h"

#include "llrefcount.h"

#include "llerror.h"

// maximum reference count before sounding memory leak alarm
const S32 gMaxRefCount = S32_MAX;

LLRefCount::LLRefCount(const LLRefCount& other)
:   mRef(0)
{
}

LLRefCount& LLRefCount::operator=(const LLRefCount&)
{
    // do nothing, since ref count is specific to *this* reference
    return *this;
}

LLRefCount::LLRefCount() :
    mRef(0)
{
}

LLRefCount::~LLRefCount()
{ 
    if (mRef != LL_REFCOUNT_FREE && mRef != 0)
    {
        LL_ERRS() << "deleting non-zero reference" << LL_ENDL;
    }
}

