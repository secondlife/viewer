/**
 * @file v2math.h
 * @brief Free function helpers for glm::vec2.
 *
 * This file used to define the LLVector2 class. After the LLVector2 →
 * glm::vec2 migration completed, the LLVector2 class was deleted entirely
 * and the file was repurposed to hold the small set of free function
 * helpers (`angle_between`, `signed_angle_between`, `are_parallel`,
 * `dist_vec`, `dist_vec_squared`, `lerp`) that operate on glm::vec2.
 *
 * These mirror the equivalent helpers that exist in `v3math.h` for
 * LLVector3 and `v4math.h` for LLVector4. They are not strictly required —
 * `glm::distance`, `glm::dot`, `glm::mix` etc. cover most of the same
 * functionality directly — but they preserve the existing call-site idioms
 * and allow code that was written against the original `v2math.h` to keep
 * working without inline rewrites at every call site.
 *
 * The file name and the include path remained `v2math.h` to minimise churn
 * in dependent code. A future cleanup pass could rename this to something
 * like `glm_vec2_helpers.h` if desired.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#pragma once

#include "llmath.h"

#include "glm/vec2.hpp"

// Free function helpers for glm::vec2. Implementations are in v2math.cpp
// and use glm primitives directly (glm::normalize, glm::dot, glm::distance,
// glm::mix).

F32 angle_between(const glm::vec2& a, const glm::vec2& b);
F32 signed_angle_between(const glm::vec2& a, const glm::vec2& b);
bool are_parallel(const glm::vec2& a, const glm::vec2& b, F32 epsilon = F_APPROXIMATELY_ZERO);
F32 dist_vec(const glm::vec2& a, const glm::vec2& b);
F32 dist_vec_squared(const glm::vec2& a, const glm::vec2& b);
glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, F32 u);
