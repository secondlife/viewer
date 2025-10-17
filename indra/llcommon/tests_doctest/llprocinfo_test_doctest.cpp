// ---------------------------------------------------------------------------
// Auto-generated from llprocinfo_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llprocinfo.h"
#include "../lltimer.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llprocinfo_test::procinfo_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert llprocinfo_test.cpp::procinfo_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        // void procinfo_object_t::test<1>()
        // {
        //     LLProcInfo::time_type user(bad_user), system(bad_system);

        //     set_test_name("getCPUUsage() basic function");

        //     LLProcInfo::getCPUUsage(user, system);

        //     ensure_not_equals("getCPUUsage() writes to its user argument", user, bad_user);
        //     ensure_not_equals("getCPUUsage() writes to its system argument", system, bad_system);
        // }
    }

    TUT_CASE("llprocinfo_test::procinfo_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert llprocinfo_test.cpp::procinfo_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        // void procinfo_object_t::test<2>()
        // {
        //     LLProcInfo::time_type user(bad_user), system(bad_system);
        //     LLProcInfo::time_type user2(bad_user), system2(bad_system);

        //     set_test_name("getCPUUsage() increases over time");

        //     LLProcInfo::getCPUUsage(user, system);

        //     for (int i(0); i < 100000; ++i)
        //     {
        //         ms_sleep(0);
        //     }

        //     LLProcInfo::getCPUUsage(user2, system2);

        //     ensure_equals("getCPUUsage() user value doesn't decrease over time", user2 >= user, true);
        //     ensure_equals("getCPUUsage() system value doesn't decrease over time", system2 >= system, true);
        // }
    }

}

