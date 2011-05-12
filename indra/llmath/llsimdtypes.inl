/** 
 * @file llsimdtypes.inl
 * @brief Inlined definitions of basic SIMD math related types
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




//////////////////
// LLSimdScalar
//////////////////

inline LLSimdScalar operator+(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t += b;
	return t;
}

inline LLSimdScalar operator-(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t -= b;
	return t;
}

inline LLSimdScalar operator*(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t *= b;
	return t;
}

inline LLSimdScalar operator/(const LLSimdScalar& a, const LLSimdScalar& b)
{
	LLSimdScalar t(a);
	t /= b;
	return t;
}

inline LLSimdScalar operator-(const LLSimdScalar& a)
{
	static LL_ALIGN_16(const U32 signMask[4]) = {0x80000000, 0x80000000, 0x80000000, 0x80000000 };
	return _mm_xor_ps(*reinterpret_cast<const LLQuad*>(signMask), a);
}

inline LLBool32 operator==(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comieq_ss(a, b);
}

inline LLBool32 operator!=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comineq_ss(a, b);
}

inline LLBool32 operator<(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comilt_ss(a, b);
}

inline LLBool32 operator<=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comile_ss(a, b);
}

inline LLBool32 operator>(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comigt_ss(a, b);
}

inline LLBool32 operator>=(const LLSimdScalar& a, const LLSimdScalar& b)
{
	return _mm_comige_ss(a, b);
}

inline LLBool32 LLSimdScalar::isApproximatelyEqual(const LLSimdScalar& rhs, F32 tolerance /* = F_APPROXIMATELY_ZERO */) const
{
	const LLSimdScalar tol( tolerance );
	const LLSimdScalar diff = _mm_sub_ss( mQ, rhs.mQ );
	const LLSimdScalar absDiff = diff.getAbs();
	return absDiff <= tol;
}

inline void LLSimdScalar::setMax( const LLSimdScalar& a, const LLSimdScalar& b )
{
	mQ = _mm_max_ss( a, b );
}

inline void LLSimdScalar::setMin( const LLSimdScalar& a, const LLSimdScalar& b )
{
	mQ = _mm_min_ss( a, b );
}

inline LLSimdScalar& LLSimdScalar::operator=(F32 rhs) 
{ 
	mQ = _mm_set_ss(rhs); 
	return *this; 
}

inline LLSimdScalar& LLSimdScalar::operator+=(const LLSimdScalar& rhs) 
{
	mQ = _mm_add_ss( mQ, rhs );
	return *this;
}

inline LLSimdScalar& LLSimdScalar::operator-=(const LLSimdScalar& rhs)
{
	mQ = _mm_sub_ss( mQ, rhs );
	return *this;
}

inline LLSimdScalar& LLSimdScalar::operator*=(const LLSimdScalar& rhs)
{
	mQ = _mm_mul_ss( mQ, rhs );
	return *this;
}

inline LLSimdScalar& LLSimdScalar::operator/=(const LLSimdScalar& rhs)
{
	mQ = _mm_div_ss( mQ, rhs );
	return *this;
}

inline LLSimdScalar LLSimdScalar::getAbs() const
{
	static const LL_ALIGN_16(U32 F_ABS_MASK_4A[4]) = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
	return _mm_and_ps( mQ, *reinterpret_cast<const LLQuad*>(F_ABS_MASK_4A));
}

inline F32 LLSimdScalar::getF32() const
{ 
	F32 ret; 
	_mm_store_ss(&ret, mQ); 
	return ret; 
}
