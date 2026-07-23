/**
 * @file lltexturememorybudget.h
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

#ifndef LL_LLTEXTUREMEMORYBUDGET_H
#define LL_LLTEXTUREMEMORYBUDGET_H

#include "stdtypes.h"

namespace LLTextureMemoryBudget
{
    constexpr U32 MIN_AUTOMATIC_BUDGET_MB = 1024;
    constexpr U32 UNIFIED_MEMORY_BUDGET_DIVISOR = 8;
#if LL_DARWIN && LL_ARM64
    constexpr bool USES_UNIFIED_MEMORY = true;
#else
    constexpr bool USES_UNIFIED_MEMORY = false;
#endif

    U32 getAutomaticBudgetMB(U32 detected_vram_mb,
                             U32 configured_divisor,
                             U32 physical_memory_mb,
                             bool uses_unified_memory);
}

#endif // LL_LLTEXTUREMEMORYBUDGET_H
