// ---------------------------------------------------------------------------
// Auto-generated from llcond_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llcond.h"
#include "llcoros.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llcond_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llcond_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("Immediate gratification");
        //         cond.set_one(1);
        //         ensure("wait_for_equal() failed",
        //                cond.wait_for_equal(F32Milliseconds(1), 1));
        //         ensure("wait_for_unequal() should have failed",
        //                ! cond.wait_for_unequal(F32Milliseconds(1), 1));
        //     }
    }

    TUT_CASE("llcond_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llcond_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("Simple two-coroutine test");
        //         LLCoros::instance().launch(
        //             "test<2>",
        //             [this]()
        //             {
        //                 // Lambda immediately entered -- control comes here first.
        //                 ensure_equals(cond.get(), 0);
        //                 cond.set_all(1);
        //                 cond.wait_equal(2);
        //                 ensure_equals(cond.get(), 2);
        //                 cond.set_all(3);
        //             });
        //         // Main coroutine is resumed only when the lambda waits.
        //         ensure_equals(cond.get(), 1);
        //         cond.set_all(2);
        //         cond.wait_equal(3);
        //     }
    }

}

