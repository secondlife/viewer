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
#include "../test/lltut.h"

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


namespace tut
{
    using namespace LLTrace;
    struct trace
    {
        ThreadRecorder mRecorder;
    };

    typedef test_group<trace> trace_t;
    typedef trace_t::object trace_object_t;
    tut::trace_t tut_singleton("LLTrace");

    static CountStatHandle<S32> sCupsOfCoffeeConsumed("coffeeconsumed", "Delicious cup of dark roast.");
    static SampleStatHandle<F32Milligrams> sCaffeineLevelStat("caffeinelevel", "Coffee buzz quotient");
    static EventStatHandle<S32Ounces> sOuncesPerCup("cupsize", "Large, huge, or ginormous");

    static F32 sCaffeineLevel(0.f);
    const F32Milligrams sCaffeinePerOz(18.f);

    void drink_coffee(S32 num_cups, S32Ounces cup_size)
    {
        add(sCupsOfCoffeeConsumed, num_cups);
        for (S32 i = 0; i < num_cups; i++)
        {
            record(sOuncesPerCup, cup_size);
        }

        sCaffeineLevel += F32Ounces(num_cups * cup_size).value() * sCaffeinePerOz.value();
        sample(sCaffeineLevelStat, sCaffeineLevel);
    }

    // basic data collection
    template<> template<>
    void trace_object_t::test<1>()
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
            at_work.stop();
            drink_coffee(1, S32VentiCup(1));
        }
        // don't need to stop recordings to get accurate values out of them
        //after_3pm.stop();
        //all_day.stop();

        ensure("count stats are counted when recording is active", 
            at_work.getSum(sCupsOfCoffeeConsumed) == 3 
                && all_day.getSum(sCupsOfCoffeeConsumed) == 5
                && after_3pm.getSum(sCupsOfCoffeeConsumed) == 2);
        ensure("measurement sums are counted when recording is active", 
            at_work.getSum(sOuncesPerCup) == S32Ounces(48) 
                && all_day.getSum(sOuncesPerCup) == S32Ounces(80)
                && after_3pm.getSum(sOuncesPerCup) == S32Ounces(36));
        ensure("measurement min is specific to when recording is active", 
            at_work.getMin(sOuncesPerCup) == S32GrandeCup(1) 
                && all_day.getMin(sOuncesPerCup) == S32TallCup(1)
                && after_3pm.getMin(sOuncesPerCup) == S32GrandeCup(1));
        ensure("measurement max is specific to when recording is active", 
            at_work.getMax(sOuncesPerCup) == S32GrandeCup(1) 
                && all_day.getMax(sOuncesPerCup) == S32VentiCup(1)
                && after_3pm.getMax(sOuncesPerCup) == S32VentiCup(1));
        ensure("sample min is specific to when recording is active", 
            at_work.getMin(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1)).value()
                && all_day.getMin(sCaffeineLevelStat) == F32Milligrams(0.f)
                && after_3pm.getMin(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(2)).value());
        ensure("sample max is specific to when recording is active", 
            at_work.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3)).value()
                && all_day.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3) + (S32Ounces)S32VentiCup(1)).value()
                && after_3pm.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3) + (S32Ounces)S32VentiCup(1)).value());
    }

}
