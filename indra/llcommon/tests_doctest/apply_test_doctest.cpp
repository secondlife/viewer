// ---------------------------------------------------------------------------
// Auto-generated from apply_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "apply.h"
#include <iomanip>
#include "llsd.h"
#include "llsdutil.h"
#include <array>
#include <string>
#include <vector>

TUT_SUITE("llcommon")
{
    TUT_CASE("apply_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("apply(tuple)");
        //         LL::apply(statics::various,
        //                   std::make_tuple(statics::b, statics::i, statics::f, statics::s,
        //                                   statics::uu, statics::dt, statics::uri, statics::bin));
        //         ensure("apply(tuple) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("apply(array)");
        //         LL::apply(statics::ints, statics::fibs);
        //         ensure("apply(array) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("apply(vector)");
        //         LL::apply(statics::strings, statics::quick);
        //         ensure("apply(vector) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("apply(LLSD())");
        //         LL::apply(statics::voidfunc, LLSD());
        //         ensure("apply(LLSD()) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("apply(fn(int), LLSD scalar)");
        //         LL::apply(statics::intfunc, LLSD(statics::i));
        //         ensure("apply(fn(int), LLSD scalar) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("apply(fn(LLSD), LLSD scalar)");
        //         // This test verifies that LLSDParam<LLSD> doesn't send the compiler
        //         // into infinite recursion when the target is itself LLSD.
        //         LL::apply(statics::sdfunc, LLSD(statics::i));
        //         ensure("apply(fn(LLSD), LLSD scalar) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("apply(LLSD array)");
        //         LL::apply(statics::various,
        //                   llsd::array(statics::b, statics::i, statics::f, statics::s,
        //                               statics::uu, statics::dt, statics::uri, statics::bin));
        //         ensure("apply(LLSD array) failed", statics::called);
        //     }
    }

    TUT_CASE("apply_test::object_test_8")
    {
        DOCTEST_FAIL("TODO: convert apply_test.cpp::object::test<8> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<8>()
        //     {
        //         set_test_name("VAPPLY()");
        //         // Make a std::array<std::string> from statics::quick. We can't call a
        //         // variadic function with a data structure of dynamic length.
        //         std::array<std::string, 5> strray;
        //         for (size_t i = 0; i < strray.size(); ++i)
        //             strray[i] = statics::quick[i];
        //         // This doesn't work: the compiler doesn't know which overload of
        //         // collect() to pass to LL::apply().
        //         // LL::apply(statics::collect, strray);
        //         // That's what VAPPLY() is for.
        //         VAPPLY(statics::collect, strray);
        //         ensure("VAPPLY() failed", statics::called);
        //         ensure_equals("collected mismatch", statics::collected, statics::quick);
        //     }
    }

}

