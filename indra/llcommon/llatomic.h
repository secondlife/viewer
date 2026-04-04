/**
 * @file llatomic.h
 * @brief Base classes for atomic.
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#ifndef LL_LLATOMIC_H
#define LL_LLATOMIC_H

// Deprecated: prefer std::atomic<T> directly.
// This header is kept for backward compatibility only.

#include "stdtypes.h"
#include <atomic>

template <typename Type>
using LLAtomicBase = std::atomic<Type>;

using LLAtomicU32 = std::atomic<U32>;
using LLAtomicS32 = std::atomic<S32>;
using LLAtomicBool = std::atomic<bool>;
#endif // LL_LLATOMIC_H
