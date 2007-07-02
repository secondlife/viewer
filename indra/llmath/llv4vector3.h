/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4* class header file - vector processor enabled math
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLV4VECTOR3_H
#define LL_LLV4VECTOR3_H

#include "llv4math.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Vector3
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LL_LLV4MATH_ALIGN_PREFIX

class LLV4Vector3
{
public:
	union {
		F32		mV[LLV4_NUM_AXIS];
		V4F32	v;
	};

	enum {
		ALIGNMENT = 16
		};

	void				setVec(F32 x, F32 y, F32 z);
	void				setVec(F32 a);
}

LL_LLV4MATH_ALIGN_POSTFIX;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Vector3
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline void	LLV4Vector3::setVec(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
}

inline void	LLV4Vector3::setVec(F32 a)
{
#if LL_VECTORIZE
	v = _mm_set1_ps(a);
#else
	setVec(a, a, a);
#endif
}

#endif
