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

inline LLColor3 componentDiv(const LLColor3& left, const LLColor3& right)
{
    return LLColor3(left.mV[VRED] / right.mV[VRED], left.mV[VGREEN] / right.mV[VGREEN], left.mV[VBLUE] / right.mV[VBLUE]);
}

inline LLColor3 componentMult(const LLColor3& left, const LLColor3& right)
{
    return LLColor3(left.mV[VRED] * right.mV[VRED], left.mV[VGREEN] * right.mV[VGREEN], left.mV[VBLUE] * right.mV[VBLUE]);
}

inline LLColor3 componentExp(const LLColor3& v)
{
    return LLColor3(exp(v.mV[VRED]), exp(v.mV[VGREEN]), exp(v.mV[VBLUE]));
}

inline LLColor3 componentPow(const LLColor3& v, F32 exponent)
{
    return LLColor3(pow(v.mV[VRED], exponent), pow(v.mV[VGREEN], exponent), pow(v.mV[VBLUE], exponent));
}

inline LLColor3 componentSaturate(const LLColor3& v)
{
    return LLColor3(std::max(std::min(v.mV[VRED], 1.f), 0.f),
                    std::max(std::min(v.mV[VGREEN], 1.f), 0.f),
                    std::max(std::min(v.mV[VBLUE], 1.f), 0.f));
}

inline LLColor3 componentSqrt(const LLColor3& v)
{
    return LLColor3(sqrt(v.mV[VRED]), sqrt(v.mV[VGREEN]), sqrt(v.mV[VBLUE]));
}

inline void componentMultBy(LLColor3& left, const LLColor3& right)
{
    left.mV[VRED] *= right.mV[VRED];
    left.mV[VGREEN] *= right.mV[VGREEN];
    left.mV[VBLUE] *= right.mV[VBLUE];
}

inline LLColor3 colorMix(const LLColor3& left, const LLColor3& right, F32 amount)
{
    return (left + ((right - left) * amount));
}

inline LLColor3 smear(F32 val)
{
    return LLColor3(val, val, val);
}

inline F32 color_intens(const LLColor3& col)
{
    return col.mV[VRED] + col.mV[VGREEN] + col.mV[VBLUE];
}

inline F32 color_max(const LLColor3& col)
{
    return llmax(col.mV[VRED], col.mV[VGREEN], col.mV[VBLUE]);
}

inline F32 color_max(const LLColor4& col)
{
    return llmax(col.mV[VRED], col.mV[VGREEN], col.mV[VBLUE]);
}

inline F32 color_min(const LLColor3& col)
{
    return llmin(col.mV[VRED], col.mV[VGREEN], col.mV[VBLUE]);
}

#endif
