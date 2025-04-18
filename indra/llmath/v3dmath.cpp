/**
 * @file v3dmath.cpp
 * @brief LLVector3d class implementation.
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

//#include <sstream>    // gcc 2.95.2 doesn't support sstream

#include "v3dmath.h"

#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"
#include "llquantize.h"

// LLVector3d
// WARNING: Don't use these for global const definitions!
// For example:
//      const LLQuaternion(0.5f * F_PI, LLVector3d::zero);
// at the top of a *.cpp file might not give you what you think.
const LLVector3d LLVector3d::zero(0,0,0);
const LLVector3d LLVector3d::x_axis(1, 0, 0);
const LLVector3d LLVector3d::y_axis(0, 1, 0);
const LLVector3d LLVector3d::z_axis(0, 0, 1);
const LLVector3d LLVector3d::x_axis_neg(-1, 0, 0);
const LLVector3d LLVector3d::y_axis_neg(0, -1, 0);
const LLVector3d LLVector3d::z_axis_neg(0, 0, -1);


// Clamps each values to range (min,max).
// Returns true if data changed.
bool LLVector3d::clamp(F64 min, F64 max)
{
    bool ret{ false };

    if (mdV[VX] < min) { mdV[VX] = min; ret = true; }
    if (mdV[VY] < min) { mdV[VY] = min; ret = true; }
    if (mdV[VZ] < min) { mdV[VZ] = min; ret = true; }

    if (mdV[VX] > max) { mdV[VX] = max; ret = true; }
    if (mdV[VY] > max) { mdV[VY] = max; ret = true; }
    if (mdV[VZ] > max) { mdV[VZ] = max; ret = true; }

    return ret;
}

// Sets all values to absolute value of their original values
// Returns true if data changed
bool LLVector3d::abs()
{
    bool ret{ false };

    if (mdV[VX] < 0.0) { mdV[VX] = -mdV[VX]; ret = true; }
    if (mdV[VY] < 0.0) { mdV[VY] = -mdV[VY]; ret = true; }
    if (mdV[VZ] < 0.0) { mdV[VZ] = -mdV[VZ]; ret = true; }

    return ret;
}

std::ostream& operator<<(std::ostream& s, const LLVector3d &a)
{
    s << "{ " << a.mdV[VX] << ", " << a.mdV[VY] << ", " << a.mdV[VZ] << " }";
    return s;
}

const LLVector3d& LLVector3d::operator=(const LLVector4 &a)
{
    mdV[VX] = a.mV[VX];
    mdV[VY] = a.mV[VY];
    mdV[VZ] = a.mV[VZ];
    return *this;
}

const LLVector3d& LLVector3d::rotVec(const LLMatrix3& mat)
{
    *this = *this * mat;
    return *this;
}

const LLVector3d& LLVector3d::rotVec(const LLQuaternion& q)
{
    *this = *this * q;
    return *this;
}

const LLVector3d& LLVector3d::rotVec(F64 angle, const LLVector3d& vec)
{
    if (!vec.isExactlyZero() && angle)
    {
        *this = *this * LLMatrix3((F32)angle, vec);
    }
    return *this;
}

const LLVector3d& LLVector3d::rotVec(F64 angle, F64 x, F64 y, F64 z)
{
    LLVector3d vec(x, y, z);
    if (!vec.isExactlyZero() && angle)
    {
        *this = *this * LLMatrix3((F32)angle, vec);
    }
    return *this;
}


bool LLVector3d::parseVector3d(const std::string& buf, LLVector3d* value)
{
    if (buf.empty() || value == nullptr)
    {
        return false;
    }

    LLVector3d v;
    S32 count = sscanf(buf.c_str(), "%lf %lf %lf", v.mdV + VX, v.mdV + VY, v.mdV + VZ);
    if (3 == count)
    {
        value->setVec(v);
        return true;
    }

    return false;
}

