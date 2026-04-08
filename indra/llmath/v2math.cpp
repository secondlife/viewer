/**
 * @file v2math.cpp
 * @brief Implementation of free function helpers for glm::vec2.
 *
 * This file used to define the LLVector2 class. After the LLVector2 →
 * glm::vec2 migration completed, the LLVector2 class was deleted entirely
 * and the file was repurposed to hold a small set of free function
 * helpers that operate on glm::vec2. See v2math.h for the rationale.
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

#include "linden_common.h"

#include "v2math.h"

#include "glm/glm.hpp"

#include <cmath>

F32 angle_between(const glm::vec2& a, const glm::vec2& b)
{
    const glm::vec2 an = glm::normalize(a);
    const glm::vec2 bn = glm::normalize(b);
    const F32 cosine = glm::dot(an, bn);
    return (cosine >= 1.0f) ? 0.0f
         : (cosine <= -1.0f) ? F_PI
         : std::acos(cosine);
}

F32 signed_angle_between(const glm::vec2& a, const glm::vec2& b)
{
    const F32 angle = angle_between(a, b);
    const F32 rhombus_square = a.x * b.y - b.x * a.y;
    return rhombus_square < 0 ? -angle : angle;
}

bool are_parallel(const glm::vec2& a, const glm::vec2& b, F32 epsilon)
{
    const glm::vec2 an = glm::normalize(a);
    const glm::vec2 bn = glm::normalize(b);
    const F32 d = glm::dot(an, bn);
    return (1.0f - std::fabs(d)) < epsilon;
}

