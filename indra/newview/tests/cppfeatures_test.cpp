/**
 * @file cppfeatures_test
 * @author Vir
 * @date 2021-03
 * @brief cpp features
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

// Tests related to newer C++ features, for verifying support across compilers and platforms

#include "linden_common.h"
#include "../test/lldoctest.h"

TEST_SUITE("LLCPPFeatures") {

TEST_CASE("test_1")
{

    S32 explicit_val{3
}

TEST_CASE("test_2")
{

    std::vector<S32> numbers{3,6,9
}

TEST_CASE("test_3")
{


    // Traditional iterator for with container
    //
    // Problems:
    // * Have to create a new variable for the iterator, which is unrelated to the problem you're trying to solve.
    // * Redundant and somewhat fragile. Have to make sure begin() and end() are both from the right container.
    std::vector<S32> numbers{3,6,9
}

TEST_CASE("test_4")
{

    Bar b;
    CHECK_MESSAGE(b.is_happy(, "override"));

}

TEST_CASE("test_5")
{

    Bicycle bi;
    CHECK_MESSAGE(bi.has_wheels(, "final"));

}

TEST_CASE("test_6")
{

    DoNotCopy nc; // OK, default constructor
    //DoNotCopy nc2(nc); // No, can't copy
    //DoNotCopy nc3 = nc; // No, this also calls copy constructor (even though it looks like an assignment)

}

TEST_CASE("test_7")
{

    DefaultCopyOK d; // OK
    DefaultCopyOK d2(d); // OK
    DefaultCopyOK d3 = d; // OK
    CHECK_MESSAGE(d.val(, "default copy d")==123);
    CHECK_MESSAGE(d.val(, "default copy d2")==d2.val());
    CHECK_MESSAGE(d.val(, "default copy d3")==d3.val());

}

TEST_CASE("test_8")
{

    InitInline ii;
    CHECK_MESSAGE(ii.mFoo==10, "init member inline 1");

    InitInlineWithConstructor iici;
    CHECK_MESSAGE(iici.mFoo=10, "init member inline 2");
    CHECK_MESSAGE(iici.mBar==25, "init member inline 3");

}

TEST_CASE("test_9")
{

    S32 val = compute2();
    CHECK_MESSAGE(val==2, "constexpr 1");

    // Compile-time factorial. You used to need complex templates to do something this useless.
    S32 fac5 = ce_factorial(5);
    CHECK_MESSAGE(fac5==120, "constexpr 2");

}

TEST_CASE("test_10")
{

    // static_assert(ce_factorial(6)==720); No, needs a flag we don't currently set.
    static_assert(ce_factorial(6)==720, "bad factorial"); // OK

}

TEST_CASE("test_11")
{

    stringmap<S32> name_counts{ {"alice", 3
}

} // TEST_SUITE
