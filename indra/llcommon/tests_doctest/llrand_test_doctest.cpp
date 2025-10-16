// ---------------------------------------------------------------------------
// Auto-generated from llrand_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llrand.h"
#include "stringize.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llrand_test::random_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<1>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("frand", ll_frand(), 0.0f, 1.0f);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<2>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("drand", ll_drand(), 0.0, 1.0);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_3")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<3>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("frand(2.0f)", ll_frand(2.0f) - 1.0f, -1.0f, 1.0f);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_4")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<4>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             // Negate the result so we don't have to allow a templated low-end
        //             // comparison as well.
        //             ensure_in_range("-frand(-7.0)", -ll_frand(-7.0), 0.0f, 7.0f);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_5")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<5>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("-drand(-2.0)", -ll_drand(-2.0), 0.0, 2.0);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_6")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<6>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("rand(100)", ll_rand(100), 0, 100);
        //         }
        //     }
    }

    TUT_CASE("llrand_test::random_object_t_test_7")
    {
        DOCTEST_FAIL("TODO: convert llrand_test.cpp::random_object_t::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void random_object_t::test<7>()
        //     {
        //         for(S32 ii = 0; ii < 100000; ++ii)
        //         {
        //             ensure_in_range("-rand(-127)", -ll_rand(-127), 0, 127);
        //         }
        //     }
    }

}

