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
#include "../test/lldoctest.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

TEST_CASE("test_1")
{

        set_test_name("tuple");
        std::tuple<std::string, int> tup{ "abc", 17 
}

} // TEST_SUITE
