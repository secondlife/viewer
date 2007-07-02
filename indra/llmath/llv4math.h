/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4* class header file - vector processor enabled math
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef	LL_LLV4MATH_H
#define	LL_LLV4MATH_H

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4MATH - GNUC
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if LL_GNUC && __GNUC__ >= 4 && __SSE__

#define			LL_VECTORIZE					1

#if LL_DARWIN

#include <Accelerate/Accelerate.h>
#include <xmmintrin.h>
typedef vFloat	V4F32;

#else

#include <xmmintrin.h>
typedef float	V4F32							__attribute__((vector_size(16)));

#endif

#endif
#if LL_GNUC

#define			LL_LLV4MATH_ALIGN_PREFIX
#define			LL_LLV4MATH_ALIGN_POSTFIX		__attribute__((aligned(16)))

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4MATH - MSVC
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if LL_MSVC && _M_IX86_FP

#define			LL_VECTORIZE					1

#include <xmmintrin.h>

typedef __m128	V4F32;

#endif
#if LL_MSVC

#define			LL_LLV4MATH_ALIGN_PREFIX		__declspec(align(16))
#define			LL_LLV4MATH_ALIGN_POSTFIX

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4MATH - default - no vectorization
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if !LL_VECTORIZE

#define			LL_VECTORIZE					0

struct			V4F32							{ F32 __pad__[4]; };

inline F32 llv4lerp(F32 a, F32 b, F32 w)		{ return ( b - a ) * w + a; }

#endif

#ifndef			LL_LLV4MATH_ALIGN_PREFIX
#	define			LL_LLV4MATH_ALIGN_PREFIX
#endif
#ifndef			LL_LLV4MATH_ALIGN_POSTFIX
#	define			LL_LLV4MATH_ALIGN_POSTFIX
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4MATH
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


#define			LLV4_NUM_AXIS					4

class LLV4Vector3;
class LLV4Matrix3;
class LLV4Matrix4;

#endif
