/** 
 * @file llsimdmath.h
 * @brief Common header for SIMD-based math library (llvector4a, llmatrix3a, etc.)
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_SIMD_MATH_H
#define LL_SIMD_MATH_H

#ifndef LLMATH_H
#error "Please include llmath.h before this file."
#endif

#if ( ( LL_DARWIN || LL_LINUX ) && !(__SSE2__) ) || ( LL_WINDOWS && ( _M_IX86_FP < 2 && ADDRESS_SIZE == 32 ) )
#error SSE2 not enabled. LLVector4a and related class will not compile.
#endif

#if !LL_WINDOWS
#include <stdint.h>
#endif

#include <xmmintrin.h>
#include <emmintrin.h>

#include "llmemory.h"
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
