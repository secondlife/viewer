/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4* class header file - vector processor enabled math
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLV4MATRIX3_H
#define LL_LLV4MATRIX3_H

#include "llv4math.h"
#include "llv4vector3.h"
#include "m3math.h"			// for operator LLMatrix3()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix3
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LL_LLV4MATH_ALIGN_PREFIX

class LLV4Matrix3
{
public:
	union {
		F32		mMatrix[LLV4_NUM_AXIS][LLV4_NUM_AXIS];
		V4F32	mV[LLV4_NUM_AXIS];
	};

	void				lerp(const LLV4Matrix3 &a, const LLV4Matrix3 &b, const F32 &w);
	void				multiply(const LLVector3 &a, LLVector3& out) const;
	void				multiply(const LLVector4 &a, LLV4Vector3& out) const;
	void				multiply(const LLVector3 &a, LLV4Vector3& out) const;

	const LLV4Matrix3&	transpose();
	const LLV4Matrix3&	operator=(const LLMatrix3& a);

	operator			LLMatrix3()	const { return (reinterpret_cast<const LLMatrix4*>(const_cast<const F32*>(&mMatrix[0][0])))->getMat3(); }

	friend LLVector3	operator*(const LLVector3& a, const LLV4Matrix3& b);
}

LL_LLV4MATH_ALIGN_POSTFIX;



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix3 - SSE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if LL_VECTORIZE

inline void LLV4Matrix3::lerp(const LLV4Matrix3 &a, const LLV4Matrix3 &b, const F32 &w)
{
	__m128 vw = _mm_set1_ps(w);
	mV[VX] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VX], a.mV[VX]), vw), a.mV[VX]); // ( b - a ) * w + a
	mV[VY] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VY], a.mV[VY]), vw), a.mV[VY]);
	mV[VZ] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VZ], a.mV[VZ]), vw), a.mV[VZ]);
}

inline void LLV4Matrix3::multiply(const LLVector3 &a, LLVector3& o) const
{
	LLV4Vector3 j;
	j.v = 				 	_mm_mul_ps(_mm_set1_ps(a.mV[VX]), mV[VX]); // ( ax * vx ) + ...
	j.v = _mm_add_ps(j.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VY]), mV[VY]));
	j.v = _mm_add_ps(j.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VZ]), mV[VZ]));
	o.setVec(j.mV);
}

inline void LLV4Matrix3::multiply(const LLVector4 &a, LLV4Vector3& o) const
{
	o.v =					_mm_mul_ps(_mm_set1_ps(a.mV[VX]), mV[VX]); // ( ax * vx ) + ...
	o.v = _mm_add_ps(o.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VY]), mV[VY]));
	o.v = _mm_add_ps(o.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VZ]), mV[VZ]));
}

inline void LLV4Matrix3::multiply(const LLVector3 &a, LLV4Vector3& o) const
{
	o.v =					_mm_mul_ps(_mm_set1_ps(a.mV[VX]), mV[VX]); // ( ax * vx ) + ...
	o.v = _mm_add_ps(o.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VY]), mV[VY]));
	o.v = _mm_add_ps(o.v  , _mm_mul_ps(_mm_set1_ps(a.mV[VZ]), mV[VZ]));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix3
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#else

inline void LLV4Matrix3::lerp(const LLV4Matrix3 &a, const LLV4Matrix3 &b, const F32 &w)
{
	mMatrix[VX][VX] = llv4lerp(a.mMatrix[VX][VX], b.mMatrix[VX][VX], w);
	mMatrix[VX][VY] = llv4lerp(a.mMatrix[VX][VY], b.mMatrix[VX][VY], w);
	mMatrix[VX][VZ] = llv4lerp(a.mMatrix[VX][VZ], b.mMatrix[VX][VZ], w);

	mMatrix[VY][VX] = llv4lerp(a.mMatrix[VY][VX], b.mMatrix[VY][VX], w);
	mMatrix[VY][VY] = llv4lerp(a.mMatrix[VY][VY], b.mMatrix[VY][VY], w);
	mMatrix[VY][VZ] = llv4lerp(a.mMatrix[VY][VZ], b.mMatrix[VY][VZ], w);

	mMatrix[VZ][VX] = llv4lerp(a.mMatrix[VZ][VX], b.mMatrix[VZ][VX], w);
	mMatrix[VZ][VY] = llv4lerp(a.mMatrix[VZ][VY], b.mMatrix[VZ][VY], w);
	mMatrix[VZ][VZ] = llv4lerp(a.mMatrix[VZ][VZ], b.mMatrix[VZ][VZ], w);
}

inline void LLV4Matrix3::multiply(const LLVector3 &a, LLVector3& o) const
{
	o.setVec(		a.mV[VX] * mMatrix[VX][VX] + 
					a.mV[VY] * mMatrix[VY][VX] + 
					a.mV[VZ] * mMatrix[VZ][VX],
					 
					a.mV[VX] * mMatrix[VX][VY] + 
					a.mV[VY] * mMatrix[VY][VY] + 
					a.mV[VZ] * mMatrix[VZ][VY],
					 
					a.mV[VX] * mMatrix[VX][VZ] + 
					a.mV[VY] * mMatrix[VY][VZ] + 
					a.mV[VZ] * mMatrix[VZ][VZ]);
}

inline void LLV4Matrix3::multiply(const LLVector4 &a, LLV4Vector3& o) const
{
	o.setVec(		a.mV[VX] * mMatrix[VX][VX] + 
					a.mV[VY] * mMatrix[VY][VX] + 
					a.mV[VZ] * mMatrix[VZ][VX],
					 
					a.mV[VX] * mMatrix[VX][VY] + 
					a.mV[VY] * mMatrix[VY][VY] + 
					a.mV[VZ] * mMatrix[VZ][VY],
					 
					a.mV[VX] * mMatrix[VX][VZ] + 
					a.mV[VY] * mMatrix[VY][VZ] + 
					a.mV[VZ] * mMatrix[VZ][VZ]);
}

inline void LLV4Matrix3::multiply(const LLVector3 &a, LLV4Vector3& o) const
{
	o.setVec(		a.mV[VX] * mMatrix[VX][VX] + 
					a.mV[VY] * mMatrix[VY][VX] + 
					a.mV[VZ] * mMatrix[VZ][VX],
					 
					a.mV[VX] * mMatrix[VX][VY] + 
					a.mV[VY] * mMatrix[VY][VY] + 
					a.mV[VZ] * mMatrix[VZ][VY],
					 
					a.mV[VX] * mMatrix[VX][VZ] + 
					a.mV[VY] * mMatrix[VY][VZ] + 
					a.mV[VZ] * mMatrix[VZ][VZ]);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix3
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#endif

inline const LLV4Matrix3&	LLV4Matrix3::transpose()
{
#if LL_VECTORIZE && defined(_MM_TRANSPOSE4_PS)
	_MM_TRANSPOSE4_PS(mV[VX], mV[VY], mV[VZ], mV[VW]);
	return *this;
#else
	F32 temp;
	temp = mMatrix[VX][VY]; mMatrix[VX][VY] = mMatrix[VY][VX]; mMatrix[VY][VX] = temp;
	temp = mMatrix[VX][VZ]; mMatrix[VX][VZ] = mMatrix[VZ][VX]; mMatrix[VZ][VX] = temp;
	temp = mMatrix[VY][VZ]; mMatrix[VY][VZ] = mMatrix[VZ][VY]; mMatrix[VZ][VY] = temp;
#endif
	return *this;
}

inline const LLV4Matrix3& LLV4Matrix3::operator=(const LLMatrix3& a)
{
	memcpy(mMatrix[VX], a.mMatrix[VX], sizeof(F32) * 3 );
	memcpy(mMatrix[VY], a.mMatrix[VY], sizeof(F32) * 3 );
	memcpy(mMatrix[VZ], a.mMatrix[VZ], sizeof(F32) * 3 );
	return *this;
}

inline LLVector3 operator*(const LLVector3& a, const LLV4Matrix3& b)
{
	return LLVector3(
				a.mV[VX] * b.mMatrix[VX][VX] + 
				a.mV[VY] * b.mMatrix[VY][VX] + 
				a.mV[VZ] * b.mMatrix[VZ][VX],
	
				a.mV[VX] * b.mMatrix[VX][VY] + 
				a.mV[VY] * b.mMatrix[VY][VY] + 
				a.mV[VZ] * b.mMatrix[VZ][VY],
	
				a.mV[VX] * b.mMatrix[VX][VZ] + 
				a.mV[VY] * b.mMatrix[VY][VZ] + 
				a.mV[VZ] * b.mMatrix[VZ][VZ] );
}

#endif
