/** 
 * @file llsdutil.h
 * @author Phoenix
 * @date 2006-05-24
 * @brief Utility classes, functions, etc, for using structured data.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLSDUTIL_H
#define LL_LLSDUTIL_H

#include "llsd.h"

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

// U32
LLSD ll_sd_from_U32(const U32);
U32 ll_U32_from_sd(const LLSD& sd);

// U64
LLSD ll_sd_from_U64(const U64);
U64 ll_U64_from_sd(const LLSD& sd);

// IP Address
LLSD ll_sd_from_ipaddr(const U32);
U32 ll_ipaddr_from_sd(const LLSD& sd);

// Binary to string
LLSD ll_string_from_binary(const LLSD& sd);

//String to binary
LLSD ll_binary_from_string(const LLSD& sd);

// Serializes sd to static buffer and returns pointer, useful for gdb debugging.
char* ll_print_sd(const LLSD& sd);

// Serializes sd to static buffer and returns pointer, using "pretty printing" mode.
char* ll_pretty_print_sd_ptr(const LLSD* sd);
char* ll_pretty_print_sd(const LLSD& sd);

//compares the structure of an LLSD to a template LLSD and stores the
//"valid" values in a 3rd LLSD. Default values
//are pulled from the template.  Extra keys/values in the test
//are ignored in the resultant LLSD.  Ordering of arrays matters
//Returns false if the test is of same type but values differ in type
//Otherwise, returns true

BOOL compare_llsd_with_template(
	const LLSD& llsd_to_test,
	const LLSD& template_llsd,
	LLSD& resultant_llsd);

// Simple function to copy data out of input & output iterators if
// there is no need for casting.
template<typename Input> LLSD llsd_copy_array(Input iter, Input end)
{
	LLSD dest;
	for (; iter != end; ++iter)
	{
		dest.append(*iter);
	}
	return dest;
}

#endif // LL_LLSDUTIL_H
