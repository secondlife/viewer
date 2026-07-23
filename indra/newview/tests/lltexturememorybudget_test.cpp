/**
 * @file lltexturememorybudget_test.cpp
 * @brief Tests for automatic texture memory budget calculations.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "../lltexturememorybudget.h"
#include "../test/lltut.h"

namespace tut
{
    struct texture_memory_budget_test
    {
    };

    typedef test_group<texture_memory_budget_test> texture_memory_budget_test_t;
    typedef texture_memory_budget_test_t::object texture_memory_budget_object_t;
    texture_memory_budget_test_t tut_texture_memory_budget("LLTextureMemoryBudget");

    template<> template<>
    void texture_memory_budget_object_t::test<1>()
    {
        ensure_equals("dedicated VRAM uses the configured divisor",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(8192, 2, 32768, false),
                      4096U);
        ensure_equals("a zero divisor is treated as one",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(8192, 0, 32768, false),
                      8192U);
        ensure_equals("the automatic budget has a minimum",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(512, 2, 32768, false),
                      LLTextureMemoryBudget::MIN_AUTOMATIC_BUDGET_MB);
    }

    template<> template<>
    void texture_memory_budget_object_t::test<2>()
    {
        ensure_equals("unified-memory budget is capped by physical memory",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(38338, 1, 49152, true),
                      6144U);
        ensure_equals("a smaller detected VRAM budget remains unchanged",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(4096, 2, 49152, true),
                      2048U);
        ensure_equals("small unified-memory systems retain the minimum budget",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(6144, 2, 8192, true),
                      LLTextureMemoryBudget::MIN_AUTOMATIC_BUDGET_MB);
        ensure_equals("missing physical memory data does not collapse the budget",
                      LLTextureMemoryBudget::getAutomaticBudgetMB(8192, 2, 0, true),
                      4096U);
    }
}
