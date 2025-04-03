/**
 * @file v3color.h
 * @brief LLColor3 class header file.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_V3COLORUTIL_H
#define LL_V3COLORUTIL_H

#include "v3color.h"
#include "v4color.h"

inline LLColor3 componentDiv(LLColor3 const &left, LLColor3 const & right)
{
    return LLColor3(left.mV[0] / right.mV[0],
        left.mV[1] / right.mV[1],
        left.mV[2] / right.mV[2]);
}


inline LLColor3 componentMult(LLColor3 const &left, LLColor3 const & right)
{
    return LLColor3(left.mV[0] * right.mV[0],
        left.mV[1] * right.mV[1],
        left.mV[2] * right.mV[2]);
}


inline LLColor3 componentExp(LLColor3 const &v)
{
    return LLColor3(exp(v.mV[0]),
        exp(v.mV[1]),
        exp(v.mV[2]));
}

inline LLColor3 componentPow(LLColor3 const &v, F32 exponent)
{
    return LLColor3(pow(v.mV[0], exponent),
        pow(v.mV[1], exponent),
        pow(v.mV[2], exponent));
}

inline LLColor3 componentSaturate(LLColor3 const &v)
{
    return LLColor3(std::max(std::min(v.mV[0], 1.f), 0.f),
        std::max(std::min(v.mV[1], 1.f), 0.f),
        std::max(std::min(v.mV[2], 1.f), 0.f));
}


inline LLColor3 componentSqrt(LLColor3 const &v)
{
    return LLColor3(sqrt(v.mV[0]),
        sqrt(v.mV[1]),
        sqrt(v.mV[2]));
}

inline void componentMultBy(LLColor3 & left, LLColor3 const & right)
{
    left.mV[0] *= right.mV[0];
    left.mV[1] *= right.mV[1];
    left.mV[2] *= right.mV[2];
}

inline LLColor3 colorMix(LLColor3 const & left, LLColor3 const & right, F32 amount)
{
    return (left + ((right - left) * amount));
}

inline LLColor3 smear(F32 val)
{
    return LLColor3(val, val, val);
}

inline F32 color_intens(const LLColor3 &col)
{
    return col.mV[0] + col.mV[1] + col.mV[2];
}

inline F32 color_max(const LLColor3 &col)
{
    return llmax(col.mV[0], col.mV[1], col.mV[2]);
}

inline F32 color_max(const LLColor4 &col)
{
    return llmax(col.mV[0], col.mV[1], col.mV[2]);
}


inline F32 color_min(const LLColor3 &col)
{
    return llmin(col.mV[0], col.mV[1], col.mV[2]);
}

#endif
