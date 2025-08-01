/**
 * @file   llallocator_heap_profile_test.cpp
 * @author Brad Kittenbrink
 * @date   2008-02-
 * @brief  Test for llallocator_heap_profile.cpp.
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

#include "../llallocator_heap_profile.h"
#include "../test/lldoctest.h"

TEST_SUITE("LLAllocatorHeapProfile") {

struct llallocator_heap_profile_data
{

        LLAllocatorHeapProfile prof;

        static char const * const sample_win_profile;
        // *TODO - get test output from mac/linux tcmalloc
        static char const * const sample_mac_profile;
        static char const * const sample_lin_profile;

        static char const * const crash_testcase;
    
};

TEST_CASE_FIXTURE(llallocator_heap_profile_data, "test_1")
{

        prof.parse(sample_win_profile);

        ensure_equals("count lines", prof.mLines.size() , 5);
        CHECK_MESSAGE(prof.mLines[0].mLiveCount == 2131854U, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mLiveSize == 2245710106ULL, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mTotalCount == 14069198U, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mTotalSize == 4295177308ULL, "alloc counts");
        ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[2].mTrace.size(), 4);
        ensure_equals("count markers", prof.mLines[3].mTrace.size(), 6);
        ensure_equals("count markers", prof.mLines[4].mTrace.size(), 7);

        //prof.dump(std::cout);
    
}

TEST_CASE_FIXTURE(llallocator_heap_profile_data, "test_2")
{

        prof.parse(crash_testcase);

        ensure_equals("count lines", prof.mLines.size(), 2);
        CHECK_MESSAGE(prof.mLines[0].mLiveCount == 3U, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mLiveSize == 1049652ULL, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mTotalCount == 8U, "alloc counts");
        CHECK_MESSAGE(prof.mLines[0].mTotalSize == 1049748ULL, "alloc counts");
        ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);

        //prof.dump(std::cout);
    
}

TEST_CASE_FIXTURE(llallocator_heap_profile_data, "test_3")
{

        // test that we don't crash on edge case data
        prof.parse("");
        CHECK_MESSAGE(prof.mLines.empty(, "emtpy on error"));

        prof.parse("heap profile:");
        CHECK_MESSAGE(prof.mLines.empty(, "emtpy on error"));
    
}

} // TEST_SUITE

