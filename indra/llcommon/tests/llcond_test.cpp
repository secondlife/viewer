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
} // namespace tut
