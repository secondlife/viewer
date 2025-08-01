/**
 * @file llsingleton_test.cpp
 * @date 2011-08-11
 * @brief Unit test for the LLSingleton class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "linden_common.h"

#include "lltrace.h"
#include "lltracethreadrecorder.h"
#include "lltracerecording.h"
#include "../test/lldoctest.h"

#ifdef LL_WINDOWS
#pragma warning(disable : 4244) // possible loss of data on conversions
#endif

namespace LLUnits
{
    // using powers of 2 to allow strict floating point equality
    LL_DECLARE_BASE_UNIT(Ounces, "oz");
    LL_DECLARE_DERIVED_UNIT(TallCup, "", Ounces, / 12);
    LL_DECLARE_DERIVED_UNIT(GrandeCup, "", Ounces, / 16);
    LL_DECLARE_DERIVED_UNIT(VentiCup, "", Ounces, / 20);

    LL_DECLARE_BASE_UNIT(Grams, "g");
    LL_DECLARE_DERIVED_UNIT(Milligrams, "mg", Grams, * 1000);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Ounces);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, TallCup);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, GrandeCup);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, VentiCup);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Grams);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Milligrams);


TEST_SUITE("LLTrace") {

struct trace
{

        ThreadRecorder mRecorder;
    
};

TEST_CASE_FIXTURE(trace, "test_1")
{

        sample(sCaffeineLevelStat, sCaffeineLevel);

        Recording all_day;
        Recording at_work;
        Recording after_3pm;

        all_day.start();
        {
            // warm up with one grande cup
            drink_coffee(1, S32TallCup(1));

            // go to work
            at_work.start();
            {
                // drink 3 tall cups, 1 after 3 pm
                drink_coffee(2, S32GrandeCup(1));
                after_3pm.start();
                drink_coffee(1, S32GrandeCup(1));
            
}

} // TEST_SUITE

