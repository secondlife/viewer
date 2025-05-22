/**
 * @file v4coloru.cpp
 * @brief LLColor4U class implementation.
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

#include "linden_common.h"

#include "v4coloru.h"
#include "llmath.h"

// LLColor4U
LLColor4U LLColor4U::white(255, 255, 255, 255);
LLColor4U LLColor4U::black(  0,   0,   0, 255);
LLColor4U LLColor4U::red  (255,   0,   0, 255);
LLColor4U LLColor4U::green(  0, 255,   0, 255);
LLColor4U LLColor4U::blue (  0,   0, 255, 255);

std::ostream& operator<<(std::ostream& s, const LLColor4U& a)
{
    s << "{ " << (S32)a.mV[VRED] << ", " << (S32)a.mV[VGREEN] << ", " << (S32)a.mV[VBLUE] << ", " << (S32)a.mV[VALPHA] << " }";
    return s;
}

// static
bool LLColor4U::parseColor4U(const std::string& buf, LLColor4U* value)
{
    if (buf.empty() || value == nullptr)
    {
        return false;
    }

    U32 v[4]{};
    S32 count = sscanf(buf.c_str(), "%u, %u, %u, %u", v + 0, v + 1, v + 2, v + 3);
    if (1 == count)
    {
        // try this format
        count = sscanf(buf.c_str(), "%u %u %u %u", v + 0, v + 1, v + 2, v + 3);
    }
    if (4 != count)
    {
        return false;
    }

    for (S32 i = 0; i < 4; i++)
    {
        if (v[i] > U8_MAX)
        {
            return false;
        }
    }

    value->set(U8(v[VRED]), U8(v[VGREEN]), U8(v[VBLUE]), U8(v[VALPHA]));
    return true;
}
