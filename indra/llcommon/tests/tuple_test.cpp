/**
 * @file   tuple_test.cpp
 * @author Nat Goodspeed
 * @date   2021-10-04
 * @brief  Test for tuple.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "tuple.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct tuple_data
    {
    };
    typedef test_group<tuple_data> tuple_group;
    typedef tuple_group::object object;
    tuple_group tuplegrp("tuple");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("tuple");
        std::tuple<std::string, int> tup{ "abc", 17 };
        std::tuple<int, std::string, int> ptup{ tuple_cons(34, tup) };
        std::tuple<std::string, int> tup2;
        int i;
        std::tie(i, tup2) = tuple_split(ptup);
        ensure_equals("tuple_car() fail", i, 34);
        ensure_equals("tuple_cdr() (0) fail", std::get<0>(tup2), "abc");
        ensure_equals("tuple_cdr() (1) fail", std::get<1>(tup2), 17);
    }
} // namespace tut
