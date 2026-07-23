/**
 * @file lltexturememorybudget.cpp
 * @brief Helpers for calculating the viewer's automatic texture memory budget.
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

#include "llviewerprecompiledheaders.h"

#include "lltexturememorybudget.h"

#include <algorithm>

U32 LLTextureMemoryBudget::getAutomaticBudgetMB(U32 detected_vram_mb,
                                                U32 configured_divisor,
                                                U32 physical_memory_mb,
                                                bool uses_unified_memory)
{
    const U32 divisor = std::max(configured_divisor, 1U);
    U32 budget = std::max(detected_vram_mb / divisor, MIN_AUTOMATIC_BUDGET_MB);

    if (uses_unified_memory && physical_memory_mb > 0)
    {
        // The driver's reported VRAM is part of system RAM on unified-memory
        // Macs. Keep the automatic texture budget small enough to leave room
        // for CPU-side texture copies, the OpenGL translation layer, the rest
        // of the viewer, and the operating system.
        const U32 unified_memory_cap =
            std::max(physical_memory_mb / UNIFIED_MEMORY_BUDGET_DIVISOR,
                     MIN_AUTOMATIC_BUDGET_MB);
        budget = std::min(budget, unified_memory_cap);
    }

    return budget;
}
