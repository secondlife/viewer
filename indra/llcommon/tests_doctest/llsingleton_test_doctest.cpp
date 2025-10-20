/**
 * @file llsingleton_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL singleton
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
// Auto-generated from llsingleton_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llsingleton.h"
#include "wrapllerrs.h"
#include "llsd.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llsingleton_test::singleton_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<1>()
        //     {

        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<2>()
        //     {
        //         LLSingletonTest* singleton_test = LLSingletonTest::getInstance();
        //         ensure(singleton_test);
        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_3")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<3>()
        //     {
        //         //Construct the instance
        //         LLSingletonTest::getInstance();
        //         ensure(LLSingletonTest::instanceExists());

        //         //Delete the instance
        //         LLSingletonTest::deleteSingleton();
        //         ensure(!LLSingletonTest::instanceExists());

        //         //Construct it again.
        //         LLSingletonTest* singleton_test = LLSingletonTest::getInstance();
        //         ensure(singleton_test);
        //         ensure(LLSingletonTest::instanceExists());
        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_12")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<12> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<12>()
        //     {
        //         set_test_name("LLParamSingleton");

        //         WrapLLErrs catcherr;
        //         // query methods
        //         ensure("false positive on instanceExists()", ! PSing1::instanceExists());
        //         ensure("false positive on wasDeleted()", ! PSing1::wasDeleted());
        //         // try to reference before initializing
        //         std::string threw = catcherr.catch_llerrs([](){
        //                 (void)PSing1::instance();
        //             });
        //         ensure_contains("too-early instance() didn't throw", threw, "Uninitialized");
        //         // getInstance() behaves the same as instance()
        //         threw = catcherr.catch_llerrs([](){
        //                 (void)PSing1::getInstance();
        //             });
        //         ensure_contains("too-early getInstance() didn't throw", threw, "Uninitialized");
        //         // initialize using LLSD::String constructor
        //         PSing1::initParamSingleton("string");
        //         ensure_equals(PSing1::instance().desc(), "string");
        //         ensure("false negative on instanceExists()", PSing1::instanceExists());
        //         // try to initialize again
        //         threw = catcherr.catch_llerrs([](){
        //                 PSing1::initParamSingleton("again");
        //             });
        //         ensure_contains("second ctor(string) didn't throw", threw, "twice");
        //         // try to initialize using the other constructor -- should be
        //         // well-formed, but illegal at runtime
        //         threw = catcherr.catch_llerrs([](){
        //                 PSing1::initParamSingleton(17);
        //             });
        //         ensure_contains("other ctor(int) didn't throw", threw, "twice");
        //         PSing1::deleteSingleton();
        //         ensure("false negative on wasDeleted()", PSing1::wasDeleted());
        //         threw = catcherr.catch_llerrs([](){
        //                 (void)PSing1::instance();
        //             });
        //         ensure_contains("accessed deleted LLParamSingleton", threw, "deleted");
        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_13")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<13> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<13>()
        //     {
        //         set_test_name("LLParamSingleton alternate ctor");

        //         WrapLLErrs catcherr;
        //         // We don't have to restate all the tests for PSing1. Only test validly
        //         // using the other constructor.
        //         PSing2::initParamSingleton(17);
        //         ensure_equals(PSing2::instance().desc(), "17");
        //         // can't do it twice
        //         std::string threw = catcherr.catch_llerrs([](){
        //                 PSing2::initParamSingleton(34);
        //             });
        //         ensure_contains("second ctor(int) didn't throw", threw, "twice");
        //         // can't use the other constructor either
        //         threw = catcherr.catch_llerrs([](){
        //                 PSing2::initParamSingleton("string");
        //             });
        //         ensure_contains("other ctor(string) didn't throw", threw, "twice");
        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_14")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<14> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<14>()
        //     {
        //         set_test_name("Circular LLParamSingleton constructor");
        //         WrapLLErrs catcherr;
        //         std::string threw = catcherr.catch_llerrs([](){
        //                 CircularPCtor::initParamSingleton();
        //             });
        //         ensure_contains("constructor circularity didn't throw", threw, "constructor");
        //     }
    }

    TUT_CASE("llsingleton_test::singleton_object_t_test_15")
    {
        DOCTEST_FAIL("TODO: convert llsingleton_test.cpp::singleton_object_t::test<15> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void singleton_object_t::test<15>()
        //     {
        //         set_test_name("Circular LLParamSingleton initSingleton()");
        //         WrapLLErrs catcherr;
        //         std::string threw = catcherr.catch_llerrs([](){
        //                 CircularPInit::initParamSingleton();
        //             });
        //         ensure("initSingleton() circularity threw", threw.empty());
        //     }
    }

}

