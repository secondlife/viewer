/** 
 * @file llsimdmath.h
 * @brief Common header for SIMD-based math library (llvector4a, llmatrix3a, etc.)
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2007-2010, Linden Research, Inc.
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

#ifndef	LL_SIMD_MATH_H
#define	LL_SIMD_MATH_H

#ifndef LLMATH_H
#error "Please include llmath.h before this file."
#endif

#if ( ( LL_DARWIN || LL_LINUX ) && !(__SSE2__) ) || ( LL_WINDOWS && ( _M_IX86_FP < 2 ) )
#error SSE2 not enabled. LLVector4a and related class will not compile.
#endif

#include <stdint.h>

template <typename T> T* LL_NEXT_ALIGNED_ADDRESS(T* address) 
{ 
	return reinterpret_cast<T*>(
		(reinterpret_cast<uintptr_t>(address) + 0xF) & ~0xF);
}

template <typename T> T* LL_NEXT_ALIGNED_ADDRESS_64(T* address) 
{ 
	return reinterpret_cast<T*>(
		(reinterpret_cast<uintptr_t>(address) + 0x3F) & ~0x3F);
}

#if LL_LINUX || LL_DARWIN

#define			LL_ALIGN_PREFIX(x)
#define			LL_ALIGN_POSTFIX(x)		__attribute__((aligned(x)))

#elif LL_WINDOWS

#define			LL_ALIGN_PREFIX(x)		__declspec(align(x))
#define			LL_ALIGN_POSTFIX(x)

#else
#error "LL_ALIGN_PREFIX and LL_ALIGN_POSTFIX undefined"
#endif

#define LL_ALIGN_16(var) LL_ALIGN_PREFIX(16) var LL_ALIGN_POSTFIX(16)



#include <xmmintrin.h>
#include <emmintrin.h>

#include "llsimdtypes.h"
#include "llsimdtypes.inl"

class LLMatrix3a;
class LLRotation;
class LLMatrix3;

#include "llquaternion.h"

#include "llvector4logical.h"
#include "llvector4a.h"
#include "llmatrix3a.h"
#include "llquaternion2.h"
#include "llvector4a.inl"
#include "llmatrix3a.inl"
#include "llquaternion2.inl"


#endif //LL_SIMD_MATH_H
