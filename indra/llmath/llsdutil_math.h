/** 
 * @file llsdutil_math.h
 * @author Brad
 * @date 2009-05-19
 * @brief Utility classes, functions, etc, for using structured data with math classes.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
