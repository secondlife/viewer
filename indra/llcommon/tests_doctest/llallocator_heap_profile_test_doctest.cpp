// ---------------------------------------------------------------------------
// Auto-generated from llallocator_heap_profile_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
// #include "../llallocator_heap_profile.h"  // not available on Windows

TUT_SUITE("llcommon")
{
    TUT_CASE("llallocator_heap_profile_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llallocator_heap_profile_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         prof.parse(sample_win_profile);

        //         ensure_equals("count lines", prof.mLines.size() , 5);
        //         ensure_equals("alloc counts", prof.mLines[0].mLiveCount, 2131854U);
        //         ensure_equals("alloc counts", prof.mLines[0].mLiveSize, 2245710106ULL);
        //         ensure_equals("alloc counts", prof.mLines[0].mTotalCount, 14069198U);
        //         ensure_equals("alloc counts", prof.mLines[0].mTotalSize, 4295177308ULL);
        //         ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        //         ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);
        //         ensure_equals("count markers", prof.mLines[2].mTrace.size(), 4);
        //         ensure_equals("count markers", prof.mLines[3].mTrace.size(), 6);
        //         ensure_equals("count markers", prof.mLines[4].mTrace.size(), 7);

        //         //prof.dump(std::cout);
        //     }
    }

    TUT_CASE("llallocator_heap_profile_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llallocator_heap_profile_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         prof.parse(crash_testcase);

        //         ensure_equals("count lines", prof.mLines.size(), 2);
        //         ensure_equals("alloc counts", prof.mLines[0].mLiveCount, 3U);
        //         ensure_equals("alloc counts", prof.mLines[0].mLiveSize, 1049652ULL);
        //         ensure_equals("alloc counts", prof.mLines[0].mTotalCount, 8U);
        //         ensure_equals("alloc counts", prof.mLines[0].mTotalSize, 1049748ULL);
        //         ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        //         ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);

        //         //prof.dump(std::cout);
        //     }
    }

    TUT_CASE("llallocator_heap_profile_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llallocator_heap_profile_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         // test that we don't crash on edge case data
        //         prof.parse("");
        //         ensure("emtpy on error", prof.mLines.empty());

        //         prof.parse("heap profile:");
        //         ensure("emtpy on error", prof.mLines.empty());
        //     }
    }

}

