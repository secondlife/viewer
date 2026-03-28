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

#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

namespace tut
{
    using tut_compat::ensure;
}

namespace
{
class Foo
{
public:
    virtual bool is_happy() const = 0;
};

class Bar: public Foo
{
public:
    bool is_happy() const override { return true; }
};

class Vehicle
{
public:
    virtual bool has_wheels() const = 0;
};

class WheeledVehicle: public Vehicle
{
public:
    virtual bool has_wheels() const final override { return true; }
};

class Bicycle: public WheeledVehicle
{
};

class DoNotCopy
{
public:
    DoNotCopy() {}
    DoNotCopy(const DoNotCopy& ref) = delete;
};
} // anonymous namespace

TUT_SUITE("LLCPPFeatures")
{
    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_1")
    {
        using namespace tut;
        S32 explicit_val{3};
        ensure(explicit_val == 3);

        S32 default_val{};
        ensure(default_val == 0);

        std::vector<S32> fibs{1, 1, 2, 3, 5};
        ensure(fibs[4] == 5);
    }

    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_2")
    {
        using namespace tut;
        std::vector<S32> numbers{3, 6, 9};

        auto& aval = numbers[1];
        ensure("auto element", aval == 6);

        auto it = numbers.rbegin();
        *it += 1;
        S32 val = *it;
        ensure("auto iterator", val == 10);
    }

    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_3")
    {
        using namespace tut;
        std::vector<S32> numbers{3, 6, 9};
        for (auto it = numbers.begin(); it != numbers.end(); ++it)
        {
            auto& n = *it;
            n *= 2;
        }
        ensure("iterator for vector", numbers[2] == 18);

        std::vector<S32> numbersb{3, 6, 9};
        for (auto& n: numbersb)
        {
            n *= 2;
        }
        ensure("range for vector", numbersb[2] == 18);

        S32 pows[] = {1, 2, 4, 8, 16};
        S32 sum{};
        for (const auto& v: pows)
        {
            sum += v;
        }
        ensure("for C-array", sum == 31);
    }

    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_4")
    {
        using namespace tut;
        Bar b;
        ensure("override", b.is_happy());
    }

    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_5")
    {
        using namespace tut;
        Bicycle bi;
        ensure("final", bi.has_wheels());
    }

    TUT_CASE("LLCPPFeatures::cpp_features_test_object_t_test_6")
    {
        DoNotCopy nc;
        (void)nc;
    }
}
