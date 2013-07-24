/** 
 * @file llallocator.h
 * @brief Declaration of the LLAllocator class.
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

#ifndef LL_LLALLOCATOR_H
#define LL_LLALLOCATOR_H

#include <string>

#include "llallocator_heap_profile.h"

class LL_COMMON_API LLAllocator {
    friend class LLMemoryView;

public:
    void setProfilingEnabled(bool should_enable);

    static bool isProfiling();

    LLAllocatorHeapProfile const & getProfile();

private:
    std::string getRawProfile();

private:
    LLAllocatorHeapProfile mProf;
};

#endif // LL_LLALLOCATOR_H
