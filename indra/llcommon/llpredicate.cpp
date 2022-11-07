/** 
 * @file llpredicate.cpp
 * @brief abstraction for filtering objects by predicates, with arbitrary boolean expressions
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llpredicate.h"

namespace LLPredicate
{
    const U32 cPredicateFlagsFromEnum[5] = 
    {
        0xAAAAaaaa, // 10101010101010101010101010101010
        0xCCCCcccc, // 11001100110011001100110011001100
        0xF0F0F0F0, // 11110000111100001111000011110000
        0xFF00FF00, // 11111111000000001111111100000000
        0xFFFF0000  // 11111111111111110000000000000000 
    };
}

