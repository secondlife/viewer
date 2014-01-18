/** 
 * @file llsdutil_math.h
 * @author Brad
 * @date 2009-05-19
 * @brief Utility classes, functions, etc, for using structured data with math classes.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLSDUTIL_MATH_H
#define LL_LLSDUTIL_MATH_H

class LL_COMMON_API LLSD;

// vector3
class LLVector3;
LLSD ll_sd_from_vector3(const LLVector3& vec);
LLVector3 ll_vector3_from_sd(const LLSD& sd, S32 start_index = 0);

// vector4
class LLVector4;
LLSD ll_sd_from_vector4(const LLVector4& vec);
LLVector4 ll_vector4_from_sd(const LLSD& sd, S32 start_index = 0);

// vector3d (double)
class LLVector3d;
LLSD ll_sd_from_vector3d(const LLVector3d& vec);
LLVector3d ll_vector3d_from_sd(const LLSD& sd, S32 start_index = 0);

// vector2
class LLVector2;
LLSD ll_sd_from_vector2(const LLVector2& vec);
LLVector2 ll_vector2_from_sd(const LLSD& sd);

// Quaternion
class LLQuaternion;
LLSD ll_sd_from_quaternion(const LLQuaternion& quat);
LLQuaternion ll_quaternion_from_sd(const LLSD& sd);

// color4
class LLColor4;
LLSD ll_sd_from_color4(const LLColor4& c);
LLColor4 ll_color4_from_sd(const LLSD& sd);

#endif // LL_LLSDUTIL_MATH_H
