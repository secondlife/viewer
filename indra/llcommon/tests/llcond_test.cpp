/**
 * @file   llcond_test.cpp
 * @author Nat Goodspeed
 * @date   2019-07-18
 * @brief  Test for llcond.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llcond.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llcoros.h"
#include "lldefs.h"                 // llless()

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llcond_data
    {
        LLScalarCond<int> cond{0};
    };
    typedef test_group<llcond_data> llcond_group;
    typedef llcond_group::object object;
    llcond_group llcondgrp("llcond");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("Immediate gratification");
        cond.set_one(1);
        ensure("wait_for_equal() failed",
               cond.wait_for_equal(F32Milliseconds(1), 1));
        ensure("wait_for_unequal() should have failed",
               ! cond.wait_for_unequal(F32Milliseconds(1), 1));
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("Simple two-coroutine test");
        LLCoros::instance().launch(
            "test<2>",
            [this]()
            {
                // Lambda immediately entered -- control comes here first.
                ensure_equals(cond.get(), 0);
                cond.set_all(1);
                cond.wait_equal(2);
                ensure_equals(cond.get(), 2);
                cond.set_all(3);
            });
        // Main coroutine is resumed only when the lambda waits.
        ensure_equals(cond.get(), 1);
        cond.set_all(2);
        cond.wait_equal(3);
    }

    template <typename T0, typename T1>
    struct compare
    {
        const char* desc;
        T0 lhs;
        T1 rhs;
        bool expect;

        void test() const
        {
            // fails
//          ensure_equals(desc, (lhs <  rhs), expect);
            ensure_equals(desc, llless(lhs, rhs), expect);
        }
    };

    template<> template<>
    void object::test<3>()
    {
        set_test_name("comparison");
        // Try to construct signed and unsigned variables such that the
        // compiler can't optimize away the code to compare at runtime.
        std::istringstream input("-1 10 20 10 20");
        int minus1, s10, s20;
        input >> minus1 >> s10 >> s20;
        unsigned u10, u20;
        input >> u10 >> u20;
        ensure_equals("minus1 wrong", minus1, -1);
        ensure_equals("s10 wrong", s10, 10);
        ensure_equals("s20 wrong", s20, 20);
        ensure_equals("u10 wrong", u10, 10);
        ensure_equals("u20 wrong", u20, 20);
        // signed < signed should always work!
        compare<int, int> ss[] =
            { {"minus1 < s10", minus1, s10, true},
              {"s10 < s10", s10, s10, false},
              {"s20 < s10", s20, s20, false}
            };
        for (const auto& cmp : ss)
        {
            cmp.test();
        }
        // unsigned < unsigned should always work!
        compare<unsigned, unsigned> uu[] =
            { {"u10 < u20", u10, u20, true},
              {"u20 < u20", u20, u20, false},
              {"u20 < u10", u20, u10, false}
            };
        for (const auto& cmp : uu)
        {
            cmp.test();
        }
        // signed < unsigned ??
        compare<int, unsigned> su[] =
            { {"minus1 < u10", minus1, u10, true},
              {"s10 < u10", s10, u10, false},
              {"s20 < u10", s20, u10, false}
            };
        for (const auto& cmp : su)
        {
            cmp.test();
        }
        // unsigned < signed ??
        compare<unsigned, int> us[] =
            { {"u10 < minus1", u10, minus1, false},
              {"u10 < s10", u10, s10, false},
              {"u10 < s20", u10, s20, true}
            };
        for (const auto& cmp : us)
        {
            cmp.test();
        }
    }
} // namespace tut
