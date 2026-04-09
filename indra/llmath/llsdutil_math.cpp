/**
 * @file llsdutil_math.cpp
 * @author Phoenix
 * @date 2006-05-24
 * @brief Implementation of classes, functions, etc, for using structured data.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llsdutil_math.h"

#include "v3math.h"
#include "v4math.h"
#include "v3dmath.h"
#include "v2math.h"
#include "llquaternion.h"
#include "v4color.h"

#if LL_WINDOWS
#   include "llwin32headers.h"    // for htonl
#elif LL_LINUX
#   include <netinet/in.h>
#elif LL_DARWIN
#   include <arpa/inet.h>
#endif

#include "llsdserialize.h"

// vector3
LLSD ll_sd_from_vector3(const LLVector3& vec)
{
    LLSD rv;
    rv.append(static_cast<F64>(vec.mV[VX]));
    rv.append(static_cast<F64>(vec.mV[VY]));
    rv.append(static_cast<F64>(vec.mV[VZ]));
    return rv;
}

LLVector3 ll_vector3_from_sd(const LLSD& sd, S32 start_index)
{
    LLVector3 rv;
    rv.mV[VX] = static_cast<F32>(sd[start_index].asReal());
    rv.mV[VY] = static_cast<F32>(sd[++start_index].asReal());
    rv.mV[VZ] = static_cast<F32>(sd[++start_index].asReal());
    return rv;
}

// vector4
LLSD ll_sd_from_vector4(const LLVector4& vec)
{
    LLSD rv;
    rv.append(static_cast<F64>(vec.mV[VX]));
    rv.append(static_cast<F64>(vec.mV[VY]));
    rv.append(static_cast<F64>(vec.mV[VZ]));
    rv.append(static_cast<F64>(vec.mV[VW]));
    return rv;
}

LLVector4 ll_vector4_from_sd(const LLSD& sd, S32 start_index)
{
    LLVector4 rv;
    rv.mV[VX] = static_cast<F32>(sd[start_index].asReal());
    rv.mV[VY] = static_cast<F32>(sd[++start_index].asReal());
    rv.mV[VZ] = static_cast<F32>(sd[++start_index].asReal());
    rv.mV[VW] = static_cast<F32>(sd[++start_index].asReal());
    return rv;
}

// vector3d
LLSD ll_sd_from_vector3d(const LLVector3d& vec)
{
    LLSD rv;
    rv.append(vec.mdV[VX]);
    rv.append(vec.mdV[VY]);
    rv.append(vec.mdV[VZ]);
    return rv;
}

LLVector3d ll_vector3d_from_sd(const LLSD& sd, S32 start_index)
{
    LLVector3d rv;
    rv.mdV[VX] = sd[start_index].asReal();
    rv.mdV[VY] = sd[++start_index].asReal();
    rv.mdV[VZ] = sd[++start_index].asReal();
    return rv;
}

// vec3 (glm)
LLSD ll_sd_from_vec3(const glm::vec3& vec)
{
    LLSD rv;
    rv.append(static_cast<F64>(vec.x));
    rv.append(static_cast<F64>(vec.y));
    rv.append(static_cast<F64>(vec.z));
    return rv;
}

glm::vec3 ll_vec3_from_sd(const LLSD& sd, S32 start_index)
{
    return glm::vec3(static_cast<F32>(sd[start_index].asReal()),
                     static_cast<F32>(sd[start_index + 1].asReal()),
                     static_cast<F32>(sd[start_index + 2].asReal()));
}

// vec2
LLSD ll_sd_from_vec2(const glm::vec2& vec)
{
    LLSD rv;
    rv.append(static_cast<F64>(vec.x));
    rv.append(static_cast<F64>(vec.y));
    return rv;
}

glm::vec2 ll_vec2_from_sd(const LLSD& sd)
{
    return glm::vec2(static_cast<F32>(sd[0].asReal()),
                     static_cast<F32>(sd[1].asReal()));
}

// Quaternion
LLSD ll_sd_from_quaternion(const LLQuaternion& quat)
{
    LLSD rv;
    rv.append(static_cast<F64>(quat.mQ[VX]));
    rv.append(static_cast<F64>(quat.mQ[VY]));
    rv.append(static_cast<F64>(quat.mQ[VZ]));
    rv.append(static_cast<F64>(quat.mQ[VW]));
    return rv;
}

LLQuaternion ll_quaternion_from_sd(const LLSD& sd)
{
    LLQuaternion quat(static_cast<F32>(sd[0].asReal()),
                      static_cast<F32>(sd[1].asReal()),
                      static_cast<F32>(sd[2].asReal()),
                      static_cast<F32>(sd[3].asReal()));
    return quat;
}

// quat (glm). xyzw wire format — see header comment. Component access
// uses the named glm accessors which match LL's mQ[VX..VW] one-to-one
// in this glm build (xyzw memory layout, no GLM_FORCE_QUAT_DATA_WXYZ;
// locked by equivalence tests #34-#35).
LLSD ll_sd_from_quat(const glm::quat& quat)
{
    LLSD rv;
    rv.append(static_cast<F64>(quat.x));
    rv.append(static_cast<F64>(quat.y));
    rv.append(static_cast<F64>(quat.z));
    rv.append(static_cast<F64>(quat.w));
    return rv;
}

glm::quat ll_quat_from_sd(const LLSD& sd)
{
    // glm::quat ctor is (w, x, y, z) — w first. The wire format is
    // xyzw, so the constructor argument order is shuffled accordingly.
    return glm::quat(static_cast<F32>(sd[3].asReal()),   // w
                     static_cast<F32>(sd[0].asReal()),   // x
                     static_cast<F32>(sd[1].asReal()),   // y
                     static_cast<F32>(sd[2].asReal()));  // z
}

// color4
LLSD ll_sd_from_color4(const LLColor4& c)
{
    LLSD rv;
    rv.append(c.mV[0]);
    rv.append(c.mV[1]);
    rv.append(c.mV[2]);
    rv.append(c.mV[3]);
    return rv;
}

LLColor4 ll_color4_from_sd(const LLSD& sd)
{
    LLColor4 c;
    c.setValue(sd);
    return c;
}
