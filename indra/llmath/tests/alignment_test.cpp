/**
 * @file v3dmath_test.cpp
 * @author Vir
 * @date 2011-12
 * @brief v3dmath test cases.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Tests related to allocating objects with alignment constraints, particularly for SSE support.

#include "linden_common.h"
#include "../test/lldoctest.h"
#include "../llmath.h"
#include "../llsimdmath.h"
#include "../llvector4a.h"

TEST_SUITE("LLAlignment") {

TEST_CASE("test_1")
{

#   ifdef LL_DEBUG
//  skip("This test fails on Windows when compiled in debug mode.");
#   endif

    const int num_tests = 7;
    void *align_ptr;
    for (int i=0; i<num_tests; i++)
    {
        align_ptr = ll_aligned_malloc_16(sizeof(MyVector4a));
        CHECK_MESSAGE(is_aligned(align_ptr,16, "ll_aligned_malloc_16 failed"));

        align_ptr = ll_aligned_realloc_16(align_ptr,2*sizeof(MyVector4a), sizeof(MyVector4a));
        CHECK_MESSAGE(is_aligned(align_ptr,16, "ll_aligned_realloc_16 failed"));

        ll_aligned_free_16(align_ptr);

        align_ptr = ll_aligned_malloc_32(sizeof(MyVector4a));
        CHECK_MESSAGE(is_aligned(align_ptr,32, "ll_aligned_malloc_32 failed"));
        ll_aligned_free_32(align_ptr);
    
}

TEST_CASE("test_2")
{

    MyVector4a vec1;
    CHECK_MESSAGE(is_aligned(&vec1,16, "LLAlignment vec1 unaligned"));

    MyVector4a veca[12];
    CHECK_MESSAGE(is_aligned(veca,16, "LLAlignment veca unaligned"));

}

TEST_CASE("test_3")
{

#   ifdef LL_DEBUG
//  skip("This test fails on Windows when compiled in debug mode.");
#   endif

    const int ARR_SIZE = 7;
    for(int i=0; i<ARR_SIZE; i++)
    {
        MyVector4a *vecp = new MyVector4a;
        CHECK_MESSAGE(is_aligned(vecp,16, "LLAlignment vecp unaligned"));
        delete vecp;
    
}

} // TEST_SUITE

