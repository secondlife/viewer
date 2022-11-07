/** 
 * @file llallocator_heap_profile.h
 * @brief Declaration of the parser for tcmalloc heap profile data.
 * @author Brad Kittenbrink
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

#ifndef LL_LLALLOCATOR_HEAP_PROFILE_H
#define LL_LLALLOCATOR_HEAP_PROFILE_H

#include "stdtypes.h"

#include <map>
#include <vector>

class LLAllocatorHeapProfile
{
public:
    typedef int stack_marker;

    typedef std::vector<stack_marker> stack_trace;

    struct line {
        line(U32 live_count, U64 live_size, U32 tot_count, U64 tot_size) :
            mLiveSize(live_size),
            mTotalSize(tot_size),
            mLiveCount(live_count),
            mTotalCount(tot_count)
        {
        }
        U64 mLiveSize, mTotalSize;
        U32 mLiveCount, mTotalCount;
        stack_trace mTrace;
    };

    typedef std::vector<line> lines_t;

    LLAllocatorHeapProfile()
    {
    }

    void parse(std::string const & prof_text);

    void dump(std::ostream & out) const;

public:
    lines_t mLines;
};


#endif // LL_LLALLOCATOR_HEAP_PROFILE_H
