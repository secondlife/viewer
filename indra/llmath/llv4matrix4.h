/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4* class header file - vector processor enabled math
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLV4MATRIX4_H
#define LL_LLV4MATRIX4_H

#include "llv4math.h"
#include "llv4matrix3.h"	// just for operator LLV4Matrix3()
#include "llv4vector3.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix4
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LL_LLV4MATH_ALIGN_PREFIX

class LLV4Matrix4
{
public:
	union {
		F32		mMatrix[LLV4_NUM_AXIS][LLV4_NUM_AXIS];
		V4F32	mV[LLV4_NUM_AXIS];
	};

	void				lerp(const LLV4Matrix4 &a, const LLV4Matrix4 &b, const F32 &w);
	void				multiply(const LLVector3 &a, LLVector3& o) const;
	void				multiply(const LLVector3 &a, LLV4Vector3& o) const;

	const LLV4Matrix4&	transpose();
	const LLV4Matrix4&  translate(const LLVector3 &vec);
	const LLV4Matrix4&  translate(const LLV4Vector3 &vec);
	const LLV4Matrix4&	operator=(const LLMatrix4& a);

	operator			LLMatrix4()	const { return *(reinterpret_cast<const LLMatrix4*>(const_cast<const F32*>(&mMatrix[0][0]))); }
	operator			LLV4Matrix3()	const { return *(reinterpret_cast<const LLV4Matrix3*>(const_cast<const F32*>(&mMatrix[0][0]))); }
	
	friend LLVector3	operator*(const LLVector3 &a, const LLV4Matrix4 &b);
}

LL_LLV4MATH_ALIGN_POSTFIX;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix4 - SSE
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if LL_VECTORIZE

inline void LLV4Matrix4::lerp(const LLV4Matrix4 &a, const LLV4Matrix4 &b, const F32 &w)
{
	__m128 vw = _mm_set1_ps(w);
	mV[VX] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VX], a.mV[VX]), vw), a.mV[VX]); // ( b - a ) * w + a
	mV[VY] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VY], a.mV[VY]), vw), a.mV[VY]);
	mV[VZ] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VZ], a.mV[VZ]), vw), a.mV[VZ]);
	mV[VW] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b.mV[VW], a.mV[VW]), vw), a.mV[VW]);
}

inline void LLV4Matrix4::multiply(const LLVector3 &a, LLVector3& o) const
{
	LLV4Vector3 j;
	j.v = _mm_add_ps(mV[VW], _mm_mul_ps(_mm_set1_ps(a.mV[VX]), mV[VX])); // ( ax * vx ) + vw
	j.v = _mm_add_ps(j.v   , _mm_mul_ps(_mm_set1_ps(a.mV[VY]), mV[VY]));
	j.v = _mm_add_ps(j.v   , _mm_mul_ps(_mm_set1_ps(a.mV[VZ]), mV[VZ]));
	o.setVec(j.mV);
}

inline void LLV4Matrix4::multiply(const LLVector3 &a, LLV4Vector3& o) const
{
	o.v = _mm_add_ps(mV[VW], _mm_mul_ps(_mm_set1_ps(a.mV[VX]), mV[VX])); // ( ax * vx ) + vw
	o.v = _mm_add_ps(o.v   , _mm_mul_ps(_mm_set1_ps(a.mV[VY]), mV[VY]));
	o.v = _mm_add_ps(o.v   , _mm_mul_ps(_mm_set1_ps(a.mV[VZ]), mV[VZ]));
}

inline const LLV4Matrix4& LLV4Matrix4::translate(const LLV4Vector3 &vec)
{
	mV[VW] = _mm_add_ps(mV[VW], vec.v);
	return (*this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix4
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#else

inline void LLV4Matrix4::lerp(const LLV4Matrix4 &a, const LLV4Matrix4 &b, const F32 &w)
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

	mMatrix[VW][VX] = llv4lerp(a.mMatrix[VW][VX], b.mMatrix[VW][VX], w);
	mMatrix[VW][VY] = llv4lerp(a.mMatrix[VW][VY], b.mMatrix[VW][VY], w);
	mMatrix[VW][VZ] = llv4lerp(a.mMatrix[VW][VZ], b.mMatrix[VW][VZ], w);
}

inline void LLV4Matrix4::multiply(const LLVector3 &a, LLVector3& o) const
{
	o.setVec(		a.mV[VX] * mMatrix[VX][VX] + 
					a.mV[VY] * mMatrix[VY][VX] + 
					a.mV[VZ] * mMatrix[VZ][VX] +
					mMatrix[VW][VX],
					 
					a.mV[VX] * mMatrix[VX][VY] + 
					a.mV[VY] * mMatrix[VY][VY] + 
					a.mV[VZ] * mMatrix[VZ][VY] +
					mMatrix[VW][VY],
					 
					a.mV[VX] * mMatrix[VX][VZ] + 
					a.mV[VY] * mMatrix[VY][VZ] + 
					a.mV[VZ] * mMatrix[VZ][VZ] +
					mMatrix[VW][VZ]);
}

inline void LLV4Matrix4::multiply(const LLVector3 &a, LLV4Vector3& o) const
{
	o.setVec(		a.mV[VX] * mMatrix[VX][VX] + 
					a.mV[VY] * mMatrix[VY][VX] + 
					a.mV[VZ] * mMatrix[VZ][VX] +
					mMatrix[VW][VX],
					 
					a.mV[VX] * mMatrix[VX][VY] + 
					a.mV[VY] * mMatrix[VY][VY] + 
					a.mV[VZ] * mMatrix[VZ][VY] +
					mMatrix[VW][VY],
					 
					a.mV[VX] * mMatrix[VX][VZ] + 
					a.mV[VY] * mMatrix[VY][VZ] + 
					a.mV[VZ] * mMatrix[VZ][VZ] +
					mMatrix[VW][VZ]);
}

inline const LLV4Matrix4& LLV4Matrix4::translate(const LLV4Vector3 &vec)
{
	mMatrix[3][0] += vec.mV[0];
	mMatrix[3][1] += vec.mV[1];
	mMatrix[3][2] += vec.mV[2];
	return (*this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLV4Matrix4
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#endif

inline const LLV4Matrix4& LLV4Matrix4::operator=(const LLMatrix4& a)
{
	memcpy(mMatrix, a.mMatrix, sizeof(F32) * 16 );
	return *this;
}

inline const LLV4Matrix4& LLV4Matrix4::transpose()
{
#if LL_VECTORIZE && defined(_MM_TRANSPOSE4_PS)
	_MM_TRANSPOSE4_PS(mV[VX], mV[VY], mV[VZ], mV[VW]);
#else
	LLV4Matrix4 mat;
	mat.mMatrix[0][0] = mMatrix[0][0];
	mat.mMatrix[1][0] = mMatrix[0][1];
	mat.mMatrix[2][0] = mMatrix[0][2];
	mat.mMatrix[3][0] = mMatrix[0][3];

	mat.mMatrix[0][1] = mMatrix[1][0];
	mat.mMatrix[1][1] = mMatrix[1][1];
	mat.mMatrix[2][1] = mMatrix[1][2];
	mat.mMatrix[3][1] = mMatrix[1][3];

	mat.mMatrix[0][2] = mMatrix[2][0];
	mat.mMatrix[1][2] = mMatrix[2][1];
	mat.mMatrix[2][2] = mMatrix[2][2];
	mat.mMatrix[3][2] = mMatrix[2][3];

	mat.mMatrix[0][3] = mMatrix[3][0];
	mat.mMatrix[1][3] = mMatrix[3][1];
	mat.mMatrix[2][3] = mMatrix[3][2];
	mat.mMatrix[3][3] = mMatrix[3][3];

	*this = mat;
#endif
	return *this;
}

inline const LLV4Matrix4& LLV4Matrix4::translate(const LLVector3 &vec)
{
	mMatrix[3][0] += vec.mV[0];
	mMatrix[3][1] += vec.mV[1];
	mMatrix[3][2] += vec.mV[2];
	return (*this);
}

inline LLVector3 operator*(const LLVector3 &a, const LLV4Matrix4 &b)
{
	return LLVector3(a.mV[VX] * b.mMatrix[VX][VX] + 
					 a.mV[VY] * b.mMatrix[VY][VX] + 
					 a.mV[VZ] * b.mMatrix[VZ][VX] +
					 b.mMatrix[VW][VX],
					 
					 a.mV[VX] * b.mMatrix[VX][VY] + 
					 a.mV[VY] * b.mMatrix[VY][VY] + 
					 a.mV[VZ] * b.mMatrix[VZ][VY] +
					 b.mMatrix[VW][VY],
					 
					 a.mV[VX] * b.mMatrix[VX][VZ] + 
					 a.mV[VY] * b.mMatrix[VY][VZ] + 
					 a.mV[VZ] * b.mMatrix[VZ][VZ] +
					 b.mMatrix[VW][VZ]);
}


#endif
