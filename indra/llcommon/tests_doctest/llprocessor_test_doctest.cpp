/**
 * @file llprocessor_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL processor
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from llprocessor_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llprocessor.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llprocessor_test::processor_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert llprocessor_test.cpp::processor_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void processor_object_t::test<1>()
        //     {
        //         set_test_name("LLProcessorInfo regression test");

        //         LLProcessorInfo pi;
        //         F64 freq =  pi.getCPUFrequency();
        //         //bool sse =  pi.hasSSE();
        //         //bool sse2 = pi.hasSSE2();
        //         //bool alitvec = pi.hasAltivec();
        //         std::string family = pi.getCPUFamilyName();
        //         std::string brand =  pi.getCPUBrandName();
        //         //std::string steam =  pi.getCPUFeatureDescription();

        //         ensure_not_equals("Unknown Brand name", brand, "Unknown");
        //         ensure_not_equals("Unknown Family name", family, "Unknown");
        //         ensure("Reasonable CPU Frequency > 100 && < 10000", freq > 100 && freq < 10000);
        //     }
    }

}

