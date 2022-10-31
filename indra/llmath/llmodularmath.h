/** 
 * @file llmodularmath.h
 * @brief Useful modular math functions.
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

#ifndef LLMODULARMATH_H
#define LLMODULARMATH_H

namespace LLModularMath
{
    // Return difference between lhs and rhs
    // treating the U32 operands and result
    // as unsigned values of given width.
    template<int width>
    inline U32 subtract(U32 lhs, U32 rhs)
    {
        // Generate a bit mask which will truncate
        // unsigned values to given width at compile time.
        const U32 mask = (1 << width) - 1;
        
        // Operands are unsigned, so modular
        // arithmetic applies. If lhs < rhs,
        // difference will wrap in to lower
        // bits of result, which is then masked
        // to give a value that can be represented
        // by an unsigned value of width bits.
        return mask & (lhs - rhs);
    }   
}

#endif
