/**
 * @file llpounceable_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL pounceable
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
// Auto-generated from llpounceable_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llpounceable.h"
#include <boost/bind.hpp>

TUT_SUITE("llcommon")
{
    TUT_CASE("llpounceable_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("LLPounceableStatic out-of-order test");
        //         // LLPounceable<T, LLPounceableStatic>::callWhenReady() must work even
        //         // before LLPounceable's constructor runs. That's the whole point of
        //         // implementing it with an LLSingleton queue. This models (say)
        //         // LLPounceableStatic<LLMessageSystem*, LLPounceableStatic>.
        //         ensure("static_check should still be null", ! static_check);
        //         Data myData("test<1>");
        //         gForward = &myData;         // should run setter
        //         ensure_equals("static_check should be &myData", static_check, &myData);
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("LLPounceableQueue different queues");
        //         // We expect that LLPounceable<T, LLPounceableQueue> should have
        //         // different queues because that specialization stores the queue
        //         // directly in the LLPounceable instance.
        //         Data *aptr = 0, *bptr = 0;
        //         LLPounceable<Data*> a, b;
        //         a.callWhenReady(boost::bind(setter, &aptr, _1));
        //         b.callWhenReady(boost::bind(setter, &bptr, _1));
        //         ensure("aptr should be null", ! aptr);
        //         ensure("bptr should be null", ! bptr);
        //         Data adata("a"), bdata("b");
        //         a = &adata;
        //         ensure_equals("aptr should be &adata", aptr, &adata);
        //         // but we haven't yet set b
        //         ensure("bptr should still be null", !bptr);
        //         b = &bdata;
        //         ensure_equals("bptr should be &bdata", bptr, &bdata);
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("LLPounceableStatic different queues");
        //         // LLPounceable<T, LLPounceableStatic> should also have a distinct
        //         // queue for each instance, but that engages an additional map lookup
        //         // because there's only one LLSingleton for each T.
        //         Data *aptr = 0, *bptr = 0;
        //         LLPounceable<Data*, LLPounceableStatic> a, b;
        //         a.callWhenReady(boost::bind(setter, &aptr, _1));
        //         b.callWhenReady(boost::bind(setter, &bptr, _1));
        //         ensure("aptr should be null", ! aptr);
        //         ensure("bptr should be null", ! bptr);
        //         Data adata("a"), bdata("b");
        //         a = &adata;
        //         ensure_equals("aptr should be &adata", aptr, &adata);
        //         // but we haven't yet set b
        //         ensure("bptr should still be null", !bptr);
        //         b = &bdata;
        //         ensure_equals("bptr should be &bdata", bptr, &bdata);
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("LLPounceable<T> looks like T");
        //         // We want LLPounceable<T, TAG> to be drop-in replaceable for a plain
        //         // T for read constructs. In particular, it should behave like a dumb
        //         // pointer -- and with zero abstraction cost for such usage.
        //         Data* aptr = 0;
        //         Data a("a");
        //         // should be able to initialize a pounceable (when its constructor
        //         // runs)
        //         LLPounceable<Data*> pounceable(&a);
        //         // should be able to pass LLPounceable<T> to function accepting T
        //         setter(&aptr, pounceable);
        //         ensure_equals("aptr should be &a", aptr, &a);
        //         // should be able to dereference with *
        //         ensure_equals("deref with *", (*pounceable).mData, "a");
        //         // should be able to dereference with ->
        //         ensure_equals("deref with ->", pounceable->mData, "a");
        //         // bool operations
        //         ensure("test with operator bool()", pounceable);
        //         ensure("test with operator !()", ! (! pounceable));
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("Multiple callWhenReady() queue items");
        //         Data *p1 = 0, *p2 = 0, *p3 = 0;
        //         Data a("a");
        //         LLPounceable<Data*> pounceable;
        //         // queue up a couple setter() calls for later
        //         pounceable.callWhenReady(boost::bind(setter, &p1, _1));
        //         pounceable.callWhenReady(boost::bind(setter, &p2, _1));
        //         // should still be pending
        //         ensure("p1 should be null", !p1);
        //         ensure("p2 should be null", !p2);
        //         ensure("p3 should be null", !p3);
        //         pounceable = 0;
        //         // assigning a new empty value shouldn't flush the queue
        //         ensure("p1 should still be null", !p1);
        //         ensure("p2 should still be null", !p2);
        //         ensure("p3 should still be null", !p3);
        //         // using whichever syntax
        //         pounceable.reset(0);
        //         // try to make ensure messages distinct... tough to pin down which
        //         // ensure() failed if multiple ensure() calls in the same test<n> have
        //         // the same message!
        //         ensure("p1 should again be null", !p1);
        //         ensure("p2 should again be null", !p2);
        //         ensure("p3 should again be null", !p3);
        //         pounceable.reset(&a);       // should flush queue
        //         ensure_equals("p1 should be &a", p1, &a);
        //         ensure_equals("p2 should be &a", p2, &a);
        //         ensure("p3 still not set", !p3);
        //         // immediate call
        //         pounceable.callWhenReady(boost::bind(setter, &p3, _1));
        //         ensure_equals("p3 should be &a", p3, &a);
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("queue order");
        //         std::string data;
        //         LLPounceable<std::string*> pounceable;
        //         pounceable.callWhenReady(boost::bind(append, _1, "a"));
        //         pounceable.callWhenReady(boost::bind(append, _1, "b"));
        //         pounceable.callWhenReady(boost::bind(append, _1, "c"));
        //         pounceable = &data;
        //         ensure_equals("callWhenReady() must preserve chronological order",
        //                       data, "abc");

        //         std::string data2;
        //         pounceable = NULL;
        //         pounceable.callWhenReady(boost::bind(append, _1, "d"));
        //         pounceable.callWhenReady(boost::bind(append, _1, "e"));
        //         pounceable.callWhenReady(boost::bind(append, _1, "f"));
        //         pounceable = &data2;
        //         ensure_equals("LLPounceable must reset queue when fired",
        //                       data2, "def");
        //     }
    }

    TUT_CASE("llpounceable_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert llpounceable_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("compile-fail test, uncomment to check");
        //         // The following declaration should fail: only LLPounceableQueue and
        //         // LLPounceableStatic should work as tags.
        // //      LLPounceable<Data*, int> pounceable;
        //     }
    }

}

