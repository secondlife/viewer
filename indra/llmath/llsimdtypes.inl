/** 
 * @file llsimdtypes.inl
 * @brief Inlined definitions of basic SIMD math related types
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
