/**
 * @file threadsafeschedule_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for threadsafeschedule
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
// Auto-generated from threadsafeschedule_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "threadsafeschedule.h"
#include <chrono>

TUT_SUITE("llcommon")
{
    TUT_CASE("threadsafeschedule_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert threadsafeschedule_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("push");
        //         // Simply calling push() a few times might result in indeterminate
        //         // delivery order if the resolution of steady_clock is coarser than
        //         // the real time required for each push() call. Explicitly increment
        //         // the timestamp for each one -- but since we're passing explicit
        //         // timestamps, make the queue reorder them.
        //         auto now{ Queue::Clock::now() };
        //         queue.push(Queue::TimeTuple(now + 200ms, "ghi"s));
        //         // Given the various push() overloads, you have to match the type
        //         // exactly: conversions are ambiguous.
        //         queue.push(now, "abc"s);
        //         queue.push(now + 100ms, "def"s);
        //         queue.close();
        //         auto entry = queue.pop();
        //         ensure_equals("failed to pop first", std::get<0>(entry), "abc"s);
        //         entry = queue.pop();
        //         ensure_equals("failed to pop second", std::get<0>(entry), "def"s);
        //         ensure("queue not closed", queue.isClosed());
        //         ensure("queue prematurely done", ! queue.done());
        //         std::string s;
        //         bool popped = queue.tryPopFor(1s, s);
        //         ensure("failed to pop third", popped);
        //         ensure_equals("third is wrong", s, "ghi"s);
        //         popped = queue.tryPop(s);
        //         ensure("queue not empty", ! popped);
        //         ensure("queue not done", queue.done());
        //     }
    }

}

