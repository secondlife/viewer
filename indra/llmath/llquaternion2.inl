/** 
 * @file llquaternion2.inl
 * @brief LLQuaternion2 inline definitions
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

#include "llquaternion2.h"

static const LLQuad LL_V4A_PLUS_ONE = {1.f, 1.f, 1.f, 1.f};
static const LLQuad LL_V4A_MINUS_ONE = {-1.f, -1.f, -1.f, -1.f};

// Ctor from LLQuaternion
inline LLQuaternion2::LLQuaternion2( const LLQuaternion& quat )
{
	mQ.set(quat.mQ[VX], quat.mQ[VY], quat.mQ[VZ], quat.mQ[VW]);
}

//////////////////////////
// Get/Set
//////////////////////////

// Return the internal LLVector4a representation of the quaternion
inline const LLVector4a& LLQuaternion2::getVector4a() const
{
	return mQ;
}

inline LLVector4a& LLQuaternion2::getVector4aRw()
{
	return mQ;
}

/////////////////////////
// Quaternion modification
/////////////////////////

// Set this quaternion to the conjugate of src
inline void LLQuaternion2::setConjugate(const LLQuaternion2& src)
{
	static LL_ALIGN_16( const U32 F_QUAT_INV_MASK_4A[4] ) = { 0x80000000, 0x80000000, 0x80000000, 0x00000000 };
	mQ = _mm_xor_ps(src.mQ, *reinterpret_cast<const LLQuad*>(&F_QUAT_INV_MASK_4A));	
}

// Renormalizes the quaternion. Assumes it has nonzero length.
inline void LLQuaternion2::normalize()
{
	mQ.normalize4();
}

// Quantize this quaternion to 8 bit precision
inline void LLQuaternion2::quantize8()
{
	mQ.quantize8( LL_V4A_MINUS_ONE, LL_V4A_PLUS_ONE );
	normalize();
}

// Quantize this quaternion to 16 bit precision
inline void LLQuaternion2::quantize16()
{
	mQ.quantize16( LL_V4A_MINUS_ONE, LL_V4A_PLUS_ONE );
	normalize();
}


/////////////////////////
// Quaternion inspection
/////////////////////////

// Return true if this quaternion is equal to 'rhs'. 
// Note! Quaternions exhibit "double-cover", so any rotation has two equally valid
// quaternion representations and they will NOT compare equal.
inline bool LLQuaternion2::equals(const LLQuaternion2 &rhs, F32 tolerance/* = F_APPROXIMATELY_ZERO*/) const
{
	return mQ.equals4(rhs.mQ, tolerance);
}

// Return true if all components are finite and the quaternion is normalized
inline bool LLQuaternion2::isOkRotation() const
{
	return mQ.isFinite4() && mQ.isNormalized4();
}

