/**
 * @file llbboxlocal.cpp
 * @brief General purpose bounding box class (Not axis aligned).
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

#include "llbboxlocal.h"
#include "m4math.h"

void LLBBoxLocal::addPoint(const glm::vec3& p)
{
    mMin.x = llmin( p.x, mMin.x );
    mMin.y = llmin( p.y, mMin.y );
    mMin.z = llmin( p.z, mMin.z );
    mMax.x = llmax( p.x, mMax.x );
    mMax.y = llmax( p.y, mMax.y );
    mMax.z = llmax( p.z, mMax.z );
}

void LLBBoxLocal::expand( F32 delta )
{
    mMin.x -= delta;
    mMin.y -= delta;
    mMin.z -= delta;
    mMax.x += delta;
    mMax.y += delta;
    mMax.z += delta;
}

LLBBoxLocal operator*(const LLBBoxLocal &a, const LLMatrix4 &b)
{
    // LLMatrix4 operator* is defined for LLVector3, so bridge at the call site
    return LLBBoxLocal( LLVector3(a.mMin) * b, LLVector3(a.mMax) * b );
}
