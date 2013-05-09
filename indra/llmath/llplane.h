/** 
 * @file llplane.h
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLPLANE_H
#define LL_LLPLANE_H

#include "v3math.h"
#include "v4math.h"

// A simple way to specify a plane is to give its normal,
// and it's nearest approach to the origin.
// 
// Given the equation for a plane : A*x + B*y + C*z + D = 0
// The plane normal = [A, B, C]
// The closest approach = D / sqrt(A*A + B*B + C*C)


LL_ALIGN_PREFIX(16)
class LLPlane
{
public:
	
	// Constructors
	LLPlane() {}; // no default constructor
	LLPlane(const LLVector3 &p0, F32 d) { setVec(p0, d); }
	LLPlane(const LLVector3 &p0, const LLVector3 &n) { setVec(p0, n); }
	inline void setVec(const LLVector3 &p0, F32 d) { mV.set(p0[0], p0[1], p0[2], d); }
	
	// Set
	inline void setVec(const LLVector3 &p0, const LLVector3 &n)
	{
		F32 d = -(p0 * n);
		setVec(n, d);
	}
	inline void setVec(const LLVector3 &p0, const LLVector3 &p1, const LLVector3 &p2)
	{
		LLVector3 u, v, w;
		u = p1 - p0;
		v = p2 - p0;
		w = u % v;
		w.normVec();
		F32 d = -(w * p0);
		setVec(w, d);
	}
	
	inline LLPlane& operator=(const LLVector4& v2) {  mV.set(v2[0],v2[1],v2[2],v2[3]); return *this;}
	
	inline LLPlane& operator=(const LLVector4a& v2) {  mV.set(v2[0],v2[1],v2[2],v2[3]); return *this;}	
	
	inline void set(const LLPlane& p2) { mV = p2.mV; }
	
	// 
	F32 dist(const LLVector3 &v2) const { return mV[0]*v2[0] + mV[1]*v2[1] + mV[2]*v2[2] + mV[3]; }
	
	inline LLSimdScalar dot3(const LLVector4a& b) const { return mV.dot3(b); }
	
	// Read-only access a single float in this vector. Do not use in proximity to any function call that manipulates
	// the data at the whole vector level or you will incur a substantial penalty. Consider using the splat functions instead	
	inline F32 operator[](const S32 idx) const { return mV[idx]; }
	
	// preferable when index is known at compile time
	template <int N> LL_FORCE_INLINE void getAt(LLSimdScalar& v) const { v = mV.getScalarAt<N>(); } 
	
	// reset the vector to 0, 0, 0, 1
	inline void clear() { mV.set(0, 0, 0, 1); }
	
	inline void getVector3(LLVector3& vec) const { vec.set(mV[0], mV[1], mV[2]); }
	
	// Retrieve the mask indicating which of the x, y, or z axis are greater or equal to zero.
	inline U8 calcPlaneMask() 
	{ 
		return mV.greaterEqual(LLVector4a::getZero()).getGatheredBits() & LLVector4Logical::MASK_XYZ;
	}
		
private:
	LLVector4a mV;
} LL_ALIGN_POSTFIX(16);



#endif // LL_LLPLANE_H
