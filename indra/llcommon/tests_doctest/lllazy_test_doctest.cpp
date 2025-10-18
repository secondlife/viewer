// ---------------------------------------------------------------------------
// Auto-generated from lllazy_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
// #include "lllazy.h"  // not available on Windows
#include <iostream>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/bind.hpp>
#include "../test/catch_and_store_what_in.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lllazy_test::lllazy_object_test_1")
    {
        DOCTEST_FAIL("TODO: convert lllazy_test.cpp::lllazy_object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void lllazy_object::test<1>()
        //     {
        //         // Instantiate an official one, just because we can
        //         NeedsTesting nt;
        //         // and a test one
        //         TestNeedsTesting tnt;
        // //      std::cout << nt.describe() << '\n';
        //         ensure_equals(nt.describe(), "NeedsTesting(YuckyFoo, YuckyBar(RealYuckyBar))");
        // //      std::cout << tnt.describe() << '\n';
        //         ensure_equals(tnt.describe(),
        //                       "TestNeedsTesting(NeedsTesting(TestFoo, TestBar(YuckyBar(TestYuckyBar))))");
        //     }
    }

    TUT_CASE("lllazy_test::lllazy_object_test_2")
    {
        DOCTEST_FAIL("TODO: convert lllazy_test.cpp::lllazy_object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void lllazy_object::test<2>()
        //     {
        //         TestNeedsTesting tnt;
        //         std::string threw = catch_what<LLLazyCommon::InstanceChange>([&tnt](){
        //                 tnt.toolate();
        //             });
        //         ensure_contains("InstanceChange exception", threw, "replace LLLazy instance");
        //     }
    }

    TUT_CASE("lllazy_test::lllazy_object_test_3")
    {
        DOCTEST_FAIL("TODO: convert lllazy_test.cpp::lllazy_object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void lllazy_object::test<3>()
        //     {
        //         {
        //             LazyMember lm;
        //             // operator*() on-demand instantiation
        //             ensure_equals(lm.getYuckyFoo().whoami(), "YuckyFoo");
        //         }
        //         {
        //             LazyMember lm;
        //             // operator->() on-demand instantiation
        //             ensure_equals(lm.whoisit(), "YuckyFoo");
        //         }
        //     }
    }

    TUT_CASE("lllazy_test::lllazy_object_test_4")
    {
        DOCTEST_FAIL("TODO: convert lllazy_test.cpp::lllazy_object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void lllazy_object::test<4>()
        //     {
        //         {
        //             // factory setter
        //             TestLazyMember tlm;
        //             ensure_equals(tlm.whoisit(), "TestFoo");
        //         }
        //         {
        //             // instance setter
        //             TestLazyMember tlm(new TestFoo());
        //             ensure_equals(tlm.whoisit(), "TestFoo");
        //         }
        //     }
    }

}

