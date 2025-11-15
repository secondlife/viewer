/**
 * @file lltrace_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL trace
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from lltrace_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"
#include "lltracerecording.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lltrace_test::trace_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert lltrace_test.cpp::trace_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void trace_object_t::test<1>()
        //     {
        //         sample(sCaffeineLevelStat, sCaffeineLevel);

        //         Recording all_day;
        //         Recording at_work;
        //         Recording after_3pm;

        //         all_day.start();
        //         {
        //             // warm up with one grande cup
        //             drink_coffee(1, S32TallCup(1));

        //             // go to work
        //             at_work.start();
        //             {
        //                 // drink 3 tall cups, 1 after 3 pm
        //                 drink_coffee(2, S32GrandeCup(1));
        //                 after_3pm.start();
        //                 drink_coffee(1, S32GrandeCup(1));
        //             }
        //             at_work.stop();
        //             drink_coffee(1, S32VentiCup(1));
        //         }
        //         // don't need to stop recordings to get accurate values out of them
        //         //after_3pm.stop();
        //         //all_day.stop();

        //         ensure("count stats are counted when recording is active",
        //             at_work.getSum(sCupsOfCoffeeConsumed) == 3
        //                 && all_day.getSum(sCupsOfCoffeeConsumed) == 5
        //                 && after_3pm.getSum(sCupsOfCoffeeConsumed) == 2);
        //         ensure("measurement sums are counted when recording is active",
        //             at_work.getSum(sOuncesPerCup) == S32Ounces(48)
        //                 && all_day.getSum(sOuncesPerCup) == S32Ounces(80)
        //                 && after_3pm.getSum(sOuncesPerCup) == S32Ounces(36));
        //         ensure("measurement min is specific to when recording is active",
        //             at_work.getMin(sOuncesPerCup) == S32GrandeCup(1)
        //                 && all_day.getMin(sOuncesPerCup) == S32TallCup(1)
        //                 && after_3pm.getMin(sOuncesPerCup) == S32GrandeCup(1));
        //         ensure("measurement max is specific to when recording is active",
        //             at_work.getMax(sOuncesPerCup) == S32GrandeCup(1)
        //                 && all_day.getMax(sOuncesPerCup) == S32VentiCup(1)
        //                 && after_3pm.getMax(sOuncesPerCup) == S32VentiCup(1));
        //         ensure("sample min is specific to when recording is active",
        //             at_work.getMin(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1)).value()
        //                 && all_day.getMin(sCaffeineLevelStat) == F32Milligrams(0.f)
        //                 && after_3pm.getMin(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(2)).value());
        //         ensure("sample max is specific to when recording is active",
        //             at_work.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3)).value()
        //                 && all_day.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3) + (S32Ounces)S32VentiCup(1)).value()
        //                 && after_3pm.getMax(sCaffeineLevelStat) == sCaffeinePerOz * ((S32Ounces)S32TallCup(1) + (S32Ounces)S32GrandeCup(3) + (S32Ounces)S32VentiCup(1)).value());
        //     }
    }

}

