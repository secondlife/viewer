/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4* class header file - vector processor enabled math
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
