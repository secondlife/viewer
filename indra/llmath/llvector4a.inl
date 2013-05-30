/** 
 * @file llvector4a.inl
 * @brief LLVector4a inline function implementations
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

////////////////////////////////////
// LOAD/STORE
////////////////////////////////////

// Load from 16-byte aligned src array (preferred method of loading)
inline void LLVector4a::load4a(const F32* src)
{
	mQ = _mm_load_ps(src);
}

// Load from unaligned src array (NB: Significantly slower than load4a)
inline void LLVector4a::loadua(const F32* src)
{
	mQ = _mm_loadu_ps(src);
}

// Load only three floats beginning at address 'src'. Slowest method.
inline void LLVector4a::load3(const F32* src)
{
	// mQ = { 0.f, src[2], src[1], src[0] } = { W, Z, Y, X }
	// NB: This differs from the convention of { Z, Y, X, W }
	mQ = _mm_set_ps(0.f, src[2], src[1], src[0]);
}	

// Store to a 16-byte aligned memory address
inline void LLVector4a::store4a(F32* dst) const
{
	_mm_store_ps(dst, mQ);
}

////////////////////////////////////
// BASIC GET/SET 
////////////////////////////////////

// Return a "this" as an F32 pointer. Do not use unless you have a very good reason.  (Not sure? Ask Falcon)
F32* LLVector4a::getF32ptr()
{
	return (F32*) &mQ;
}

// Return a "this" as a const F32 pointer. Do not use unless you have a very good reason.  (Not sure? Ask Falcon)
const F32* const LLVector4a::getF32ptr() const
{
	return (const F32* const) &mQ;
}

// Read-only access a single float in this vector. Do not use in proximity to any function call that manipulates
// the data at the whole vector level or you will incur a substantial penalty. Consider using the splat functions instead
inline F32 LLVector4a::operator[](const S32 idx) const
{
	return ((F32*)&mQ)[idx];
}	

// Prefer this method for read-only access to a single element. Prefer the templated version if the elem is known at compile time.
inline LLSimdScalar LLVector4a::getScalarAt(const S32 idx) const
{
	// Return appropriate LLQuad. It will be cast to LLSimdScalar automatically (should be effectively a nop)
	switch (idx)
	{
		case 0:
			return mQ;
		case 1:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(1, 1, 1, 1));
		case 2:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(2, 2, 2, 2));
		case 3:
		default:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(3, 3, 3, 3));
	}
}

// Prefer this method for read-only access to a single element. Prefer the templated version if the elem is known at compile time.
template <int N> LL_FORCE_INLINE LLSimdScalar LLVector4a::getScalarAt() const
{
	return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(N, N, N, N));
}

template<> LL_FORCE_INLINE LLSimdScalar LLVector4a::getScalarAt<0>() const
{
	return mQ;
}

// Set to an x, y, z and optional w provided
inline void LLVector4a::set(F32 x, F32 y, F32 z, F32 w)
{
	mQ = _mm_set_ps(w, z, y, x);
}

// Set to all zeros
inline void LLVector4a::clear()
{
	mQ = LLVector4a::getZero().mQ;
}

inline void LLVector4a::splat(const F32 x)
{
	mQ = _mm_set1_ps(x);	
}

inline void LLVector4a::splat(const LLSimdScalar& x)
{
	mQ = _mm_shuffle_ps( x.getQuad(), x.getQuad(), _MM_SHUFFLE(0,0,0,0) );
}

// Set all 4 elements to element N of src, with N known at compile time
template <int N> void LLVector4a::splat(const LLVector4a& src)
{
	mQ = _mm_shuffle_ps(src.mQ, src.mQ, _MM_SHUFFLE(N, N, N, N) );
}

// Set all 4 elements to element i of v, with i NOT known at compile time
inline void LLVector4a::splat(const LLVector4a& v, U32 i)
{
	switch (i)
	{
		case 0:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(0, 0, 0, 0));
			break;
		case 1:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(1, 1, 1, 1));
			break;
		case 2:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(2, 2, 2, 2));
			break;
		case 3:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(3, 3, 3, 3));
			break;
	}
}

// Select bits from sourceIfTrue and sourceIfFalse according to bits in mask
inline void LLVector4a::setSelectWithMask( const LLVector4Logical& mask, const LLVector4a& sourceIfTrue, const LLVector4a& sourceIfFalse )
{
	// ((( sourceIfTrue ^ sourceIfFalse ) & mask) ^ sourceIfFalse )
	// E.g., sourceIfFalse = 1010b, sourceIfTrue = 0101b, mask = 1100b
	// (sourceIfTrue ^ sourceIfFalse) = 1111b --> & mask = 1100b --> ^ sourceIfFalse = 0110b, 
	// as expected (01 from sourceIfTrue, 10 from sourceIfFalse)
	// Courtesy of Mark++, http://markplusplus.wordpress.com/2007/03/14/fast-sse-select-operation/
	mQ = _mm_xor_ps( sourceIfFalse, _mm_and_ps( mask, _mm_xor_ps( sourceIfTrue, sourceIfFalse ) ) );
}

////////////////////////////////////
// ALGEBRAIC
////////////////////////////////////

// Set this to the element-wise (a + b)
inline void LLVector4a::setAdd(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_add_ps(a.mQ, b.mQ);
}

// Set this to element-wise (a - b)
inline void LLVector4a::setSub(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_sub_ps(a.mQ, b.mQ);
}

// Set this to element-wise multiply (a * b)
inline void LLVector4a::setMul(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_mul_ps(a.mQ, b.mQ);
}

// Set this to element-wise quotient (a / b)
inline void LLVector4a::setDiv(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_div_ps( a.mQ, b.mQ );
}

// Set this to the element-wise absolute value of src
inline void LLVector4a::setAbs(const LLVector4a& src)
{
	static const LL_ALIGN_16(U32 F_ABS_MASK_4A[4]) = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
	mQ = _mm_and_ps(src.mQ, *reinterpret_cast<const LLQuad*>(F_ABS_MASK_4A));
}

// Add to each component in this vector the corresponding component in rhs
inline void LLVector4a::add(const LLVector4a& rhs)
{
	mQ = _mm_add_ps(mQ, rhs.mQ);	
}

// Subtract from each component in this vector the corresponding component in rhs
inline void LLVector4a::sub(const LLVector4a& rhs)
{
	mQ = _mm_sub_ps(mQ, rhs.mQ);
}

// Multiply each component in this vector by the corresponding component in rhs
inline void LLVector4a::mul(const LLVector4a& rhs)
{
	mQ = _mm_mul_ps(mQ, rhs.mQ);	
}

// Divide each component in this vector by the corresponding component in rhs
inline void LLVector4a::div(const LLVector4a& rhs)
{
	// TODO: Check accuracy, maybe add divFast
	mQ = _mm_div_ps(mQ, rhs.mQ);
}

// Multiply this vector by x in a scalar fashion
inline void LLVector4a::mul(const F32 x) 
{
	LLVector4a t;
	t.splat(x);
	
	mQ = _mm_mul_ps(mQ, t.mQ);
}

// Set this to (a x b) (geometric cross-product)
inline void LLVector4a::setCross3(const LLVector4a& a, const LLVector4a& b)
{
	// Vectors are stored in memory in w, z, y, x order from high to low
	// Set vector1 = { a[W], a[X], a[Z], a[Y] }
	const LLQuad vector1 = _mm_shuffle_ps( a.mQ, a.mQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
	// Set vector2 = { b[W], b[Y], b[X], b[Z] }
	const LLQuad vector2 = _mm_shuffle_ps( b.mQ, b.mQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
	// mQ = { a[W]*b[W], a[X]*b[Y], a[Z]*b[X], a[Y]*b[Z] }
	mQ = _mm_mul_ps( vector1, vector2 );
	// vector3 = { a[W], a[Y], a[X], a[Z] }
	const LLQuad vector3 = _mm_shuffle_ps( a.mQ, a.mQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
	// vector4 = { b[W], b[X], b[Z], b[Y] }
	const LLQuad vector4 = _mm_shuffle_ps( b.mQ, b.mQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
	// mQ = { 0, a[X]*b[Y] - a[Y]*b[X], a[Z]*b[X] - a[X]*b[Z], a[Y]*b[Z] - a[Z]*b[Y] }
	mQ = _mm_sub_ps( mQ, _mm_mul_ps( vector3, vector4 ));
}

/* This function works, but may be slightly slower than the one below on older machines
 inline void LLVector4a::setAllDot3(const LLVector4a& a, const LLVector4a& b)
 {
 // ab = { a[W]*b[W], a[Z]*b[Z], a[Y]*b[Y], a[X]*b[X] }
 const LLQuad ab = _mm_mul_ps( a.mQ, b.mQ );
 // yzxw = { a[W]*b[W], a[Z]*b[Z], a[X]*b[X], a[Y]*b[Y] }
 const LLQuad wzxy = _mm_shuffle_ps( ab, ab, _MM_SHUFFLE(3, 2, 0, 1 ));
 // xPlusY = { 2*a[W]*b[W], 2 * a[Z] * b[Z], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] }
 const LLQuad xPlusY = _mm_add_ps(ab, wzxy);
 // xPlusYSplat = { a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] } 
 const LLQuad xPlusYSplat = _mm_movelh_ps(xPlusY, xPlusY);
 // zSplat = { a[Z]*b[Z], a[Z]*b[Z], a[Z]*b[Z], a[Z]*b[Z] }
 const LLQuad zSplat = _mm_shuffle_ps( ab, ab, _MM_SHUFFLE( 2, 2, 2, 2 ));
 // mQ = { a[Z] * b[Z] + a[Y] * b[Y] + a[X] * b[X], same, same, same }
 mQ = _mm_add_ps(zSplat, xPlusYSplat);
 }*/

// Set all elements to the dot product of the x, y, and z elements in a and b
inline void LLVector4a::setAllDot3(const LLVector4a& a, const LLVector4a& b)
{
	// ab = { a[W]*b[W], a[Z]*b[Z], a[Y]*b[Y], a[X]*b[X] }
	const LLQuad ab = _mm_mul_ps( a.mQ, b.mQ );
	// yzxw = { a[W]*b[W], a[Z]*b[Z], a[X]*b[X], a[Y]*b[Y] }
	const __m128i wzxy = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE(3, 2, 0, 1 ));
	// xPlusY = { 2*a[W]*b[W], 2 * a[Z] * b[Z], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] }
	const LLQuad xPlusY = _mm_add_ps(ab, _mm_castsi128_ps(wzxy));
	// xPlusYSplat = { a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] } 
	const LLQuad xPlusYSplat = _mm_movelh_ps(xPlusY, xPlusY);
	// zSplat = { a[Z]*b[Z], a[Z]*b[Z], a[Z]*b[Z], a[Z]*b[Z] }
	const __m128i zSplat = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE( 2, 2, 2, 2 ));
	// mQ = { a[Z] * b[Z] + a[Y] * b[Y] + a[X] * b[X], same, same, same }
	mQ = _mm_add_ps(_mm_castsi128_ps(zSplat), xPlusYSplat);
}

// Set all elements to the dot product of the x, y, z, and w elements in a and b
inline void LLVector4a::setAllDot4(const LLVector4a& a, const LLVector4a& b)
{
	// ab = { a[W]*b[W], a[Z]*b[Z], a[Y]*b[Y], a[X]*b[X] }
	const LLQuad ab = _mm_mul_ps( a.mQ, b.mQ );
	// yzxw = { a[W]*b[W], a[Z]*b[Z], a[X]*b[X], a[Y]*b[Y] }
	const __m128i zwxy = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE(2, 3, 0, 1 ));
	// zPlusWandXplusY = { a[W]*b[W] + a[Z]*b[Z], a[Z] * b[Z] + a[W]*b[W], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] }
	const LLQuad zPlusWandXplusY = _mm_add_ps(ab, _mm_castsi128_ps(zwxy));
	// xPlusYSplat = { a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y], a[Y]*b[Y] + a[X] * b[X], a[X] * b[X] + a[Y] * b[Y] } 
	const LLQuad xPlusYSplat = _mm_movelh_ps(zPlusWandXplusY, zPlusWandXplusY);
	const LLQuad zPlusWSplat = _mm_movehl_ps(zPlusWandXplusY, zPlusWandXplusY);

	// mQ = { a[W]*b[W] + a[Z] * b[Z] + a[Y] * b[Y] + a[X] * b[X], same, same, same }
	mQ = _mm_add_ps(xPlusYSplat, zPlusWSplat);
}

// Return the 3D dot product of this vector and b
inline LLSimdScalar LLVector4a::dot3(const LLVector4a& b) const
{
	const LLQuad ab = _mm_mul_ps( mQ, b.mQ );
	const LLQuad splatY = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(ab), _MM_SHUFFLE(1, 1, 1, 1) ) );
	const LLQuad splatZ = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(ab), _MM_SHUFFLE(2, 2, 2, 2) ) );
	const LLQuad xPlusY = _mm_add_ps( ab, splatY );
	return _mm_add_ps( xPlusY, splatZ );	
}

// Return the 4D dot product of this vector and b
inline LLSimdScalar LLVector4a::dot4(const LLVector4a& b) const
{
	// ab = { w, z, y, x }
 	const LLQuad ab = _mm_mul_ps( mQ, b.mQ );
 	// upperProdsInLowerElems = { y, x, y, x }
	const LLQuad upperProdsInLowerElems = _mm_movehl_ps( ab, ab );
	// sumOfPairs = { w+y, z+x, 2y, 2x }
 	const LLQuad sumOfPairs = _mm_add_ps( upperProdsInLowerElems, ab );
	// shuffled = { z+x, z+x, z+x, z+x }
	const LLQuad shuffled = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128( sumOfPairs ), _MM_SHUFFLE(1, 1, 1, 1) ) );
	return _mm_add_ss( sumOfPairs, shuffled );
}

// Normalize this vector with respect to the x, y, and z components only. Accurate to 22 bites of precision. W component is destroyed
// Note that this does not consider zero length vectors!
inline void LLVector4a::normalize3()
{
	// lenSqrd = a dot a
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	// rsqrt = approximate reciprocal square (i.e., { ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2 }
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	// Now we do one round of Newton-Raphson approximation to get full accuracy
	// According to the Newton-Raphson method, given a first 'w' for the root of f(x) = 1/x^2 - a (i.e., x = 1/sqrt(a))
	// the next better approximation w[i+1] = w - f(w)/f'(w) = w - (1/w^2 - a)/(-2*w^(-3))
	// w[i+1] = w + 0.5 * (1/w^2 - a) * w^3 = w + 0.5 * (w - a*w^3) = 1.5 * w - 0.5 * a * w^3
	// = 0.5 * w * (3 - a*w^2)
	// Our first approx is w = rsqrt. We need out = a * w[i+1] (this is the input vector 'a', not the 'a' from the above formula
	// which is actually lenSqrd). So out = a * [0.5*rsqrt * (3 - lenSqrd*rsqrt*rsqrt)]
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
}

// Normalize this vector with respect to all components. Accurate to 22 bites of precision.
// Note that this does not consider zero length vectors!
inline void LLVector4a::normalize4()
{
	// lenSqrd = a dot a
	LLVector4a lenSqrd; lenSqrd.setAllDot4( *this, *this );
	// rsqrt = approximate reciprocal square (i.e., { ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2 }
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	// Now we do one round of Newton-Raphson approximation to get full accuracy
	// According to the Newton-Raphson method, given a first 'w' for the root of f(x) = 1/x^2 - a (i.e., x = 1/sqrt(a))
	// the next better approximation w[i+1] = w - f(w)/f'(w) = w - (1/w^2 - a)/(-2*w^(-3))
	// w[i+1] = w + 0.5 * (1/w^2 - a) * w^3 = w + 0.5 * (w - a*w^3) = 1.5 * w - 0.5 * a * w^3
	// = 0.5 * w * (3 - a*w^2)
	// Our first approx is w = rsqrt. We need out = a * w[i+1] (this is the input vector 'a', not the 'a' from the above formula
	// which is actually lenSqrd). So out = a * [0.5*rsqrt * (3 - lenSqrd*rsqrt*rsqrt)]
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
}

// Normalize this vector with respect to the x, y, and z components only. Accurate to 22 bites of precision. W component is destroyed
// Note that this does not consider zero length vectors!
inline LLSimdScalar LLVector4a::normalize3withLength()
{
	// lenSqrd = a dot a
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	// rsqrt = approximate reciprocal square (i.e., { ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2, ~1/len(a)^2 }
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	// Now we do one round of Newton-Raphson approximation to get full accuracy
	// According to the Newton-Raphson method, given a first 'w' for the root of f(x) = 1/x^2 - a (i.e., x = 1/sqrt(a))
	// the next better approximation w[i+1] = w - f(w)/f'(w) = w - (1/w^2 - a)/(-2*w^(-3))
	// w[i+1] = w + 0.5 * (1/w^2 - a) * w^3 = w + 0.5 * (w - a*w^3) = 1.5 * w - 0.5 * a * w^3
	// = 0.5 * w * (3 - a*w^2)
	// Our first approx is w = rsqrt. We need out = a * w[i+1] (this is the input vector 'a', not the 'a' from the above formula
	// which is actually lenSqrd). So out = a * [0.5*rsqrt * (3 - lenSqrd*rsqrt*rsqrt)]
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
	return _mm_sqrt_ss(lenSqrd);
}

// Normalize this vector with respect to the x, y, and z components only. Accurate only to 10-12 bits of precision. W component is destroyed
// Note that this does not consider zero length vectors!
inline void LLVector4a::normalize3fast()
{
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	const LLQuad approxRsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	mQ = _mm_mul_ps( mQ, approxRsqrt );
}

// Return true if this vector is normalized with respect to x,y,z up to tolerance
inline LLBool32 LLVector4a::isNormalized3( F32 tolerance ) const
{
	static LL_ALIGN_16(const U32 ones[4]) = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };
	LLSimdScalar tol = _mm_load_ss( &tolerance );
	tol = _mm_mul_ss( tol, tol );
	LLVector4a lenSquared; lenSquared.setAllDot3( *this, *this );
	lenSquared.sub( *reinterpret_cast<const LLVector4a*>(ones) );
	lenSquared.setAbs(lenSquared);
	return _mm_comile_ss( lenSquared, tol );		
}

// Return true if this vector is normalized with respect to all components up to tolerance
inline LLBool32 LLVector4a::isNormalized4( F32 tolerance ) const
{
	static LL_ALIGN_16(const U32 ones[4]) = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };
	LLSimdScalar tol = _mm_load_ss( &tolerance );
	tol = _mm_mul_ss( tol, tol );
	LLVector4a lenSquared; lenSquared.setAllDot4( *this, *this );
	lenSquared.sub( *reinterpret_cast<const LLVector4a*>(ones) );
	lenSquared.setAbs(lenSquared);
	return _mm_comile_ss( lenSquared, tol );		
}

// Set all elements to the length of vector 'v' 
inline void LLVector4a::setAllLength3( const LLVector4a& v )
{
	LLVector4a lenSqrd;
	lenSqrd.setAllDot3(v, v);
	
	mQ = _mm_sqrt_ps(lenSqrd.mQ);
}

// Get this vector's length
inline LLSimdScalar LLVector4a::getLength3() const
{
	return _mm_sqrt_ss( dot3( (const LLVector4a)mQ ) );
}

// Set the components of this vector to the minimum of the corresponding components of lhs and rhs
inline void LLVector4a::setMin(const LLVector4a& lhs, const LLVector4a& rhs)
{
	mQ = _mm_min_ps(lhs.mQ, rhs.mQ);
}

// Set the components of this vector to the maximum of the corresponding components of lhs and rhs
inline void LLVector4a::setMax(const LLVector4a& lhs, const LLVector4a& rhs)
{
	mQ = _mm_max_ps(lhs.mQ, rhs.mQ);
}

// Set this to  (c * lhs) + rhs * ( 1 - c)
inline void LLVector4a::setLerp(const LLVector4a& lhs, const LLVector4a& rhs, F32 c)
{
	LLVector4a a = lhs;
	a.mul(c);
	
	LLVector4a b = rhs;
	b.mul(1.f-c);
	
	setAdd(a, b);
}

inline LLBool32 LLVector4a::isFinite3() const
{
	static LL_ALIGN_16(const U32 nanOrInfMask[4]) = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };
	ll_assert_aligned(nanOrInfMask,16);
	const __m128i nanOrInfMaskV = *reinterpret_cast<const __m128i*> (nanOrInfMask);
	const __m128i maskResult = _mm_and_si128( _mm_castps_si128(mQ), nanOrInfMaskV );
	const LLVector4Logical equalityCheck = _mm_castsi128_ps(_mm_cmpeq_epi32( maskResult, nanOrInfMaskV ));
	return !equalityCheck.areAnySet( LLVector4Logical::MASK_XYZ );
}
	
inline LLBool32 LLVector4a::isFinite4() const
{
	static LL_ALIGN_16(const U32 nanOrInfMask[4]) = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };
	const __m128i nanOrInfMaskV = *reinterpret_cast<const __m128i*> (nanOrInfMask);
	const __m128i maskResult = _mm_and_si128( _mm_castps_si128(mQ), nanOrInfMaskV );
	const LLVector4Logical equalityCheck = _mm_castsi128_ps(_mm_cmpeq_epi32( maskResult, nanOrInfMaskV ));
	return !equalityCheck.areAnySet( LLVector4Logical::MASK_XYZW );
}

inline void LLVector4a::setRotatedInv( const LLRotation& rot, const LLVector4a& vec )
{
	LLRotation inv; inv.setTranspose( rot );
	setRotated( inv, vec );
}

inline void LLVector4a::setRotatedInv( const LLQuaternion2& quat, const LLVector4a& vec )
{
	LLQuaternion2 invRot; invRot.setConjugate( quat );
	setRotated(invRot, vec);
}

inline void LLVector4a::clamp( const LLVector4a& low, const LLVector4a& high )
{
	const LLVector4Logical highMask = greaterThan( high );
	const LLVector4Logical lowMask = lessThan( low );

	setSelectWithMask( highMask, high, *this );
	setSelectWithMask( lowMask, low, *this );
}


////////////////////////////////////
// LOGICAL
////////////////////////////////////	
// The functions in this section will compare the elements in this vector
// to those in rhs and return an LLVector4Logical with all bits set in elements
// where the comparison was true and all bits unset in elements where the comparison
// was false. See llvector4logica.h
////////////////////////////////////
// WARNING: Other than equals3 and equals4, these functions do NOT account
// for floating point tolerance. You should include the appropriate tolerance
// in the inputs.
////////////////////////////////////

inline LLVector4Logical LLVector4a::greaterThan(const LLVector4a& rhs) const
{	
	return _mm_cmpgt_ps(mQ, rhs.mQ);
}

inline LLVector4Logical LLVector4a::lessThan(const LLVector4a& rhs) const
{
	return _mm_cmplt_ps(mQ, rhs.mQ);
}

inline LLVector4Logical LLVector4a::greaterEqual(const LLVector4a& rhs) const
{
	return _mm_cmpge_ps(mQ, rhs.mQ);
}

inline LLVector4Logical LLVector4a::lessEqual(const LLVector4a& rhs) const
{
	return _mm_cmple_ps(mQ, rhs.mQ);
}

inline LLVector4Logical LLVector4a::equal(const LLVector4a& rhs) const
{
	return _mm_cmpeq_ps(mQ, rhs.mQ);
}

// Returns true if this and rhs are componentwise equal up to the specified absolute tolerance
inline bool LLVector4a::equals4(const LLVector4a& rhs, F32 tolerance ) const
{
	LLVector4a diff; diff.setSub( *this, rhs );
	diff.setAbs( diff );
	const LLQuad tol = _mm_set1_ps( tolerance );
	const LLQuad cmp = _mm_cmplt_ps( diff, tol );
	return (_mm_movemask_ps( cmp ) & LLVector4Logical::MASK_XYZW) == LLVector4Logical::MASK_XYZW;
}

inline bool LLVector4a::equals3(const LLVector4a& rhs, F32 tolerance ) const
{
	LLVector4a diff; diff.setSub( *this, rhs );
	diff.setAbs( diff );
	const LLQuad tol = _mm_set1_ps( tolerance );
	const LLQuad t = _mm_cmplt_ps( diff, tol ); 
	return (_mm_movemask_ps( t ) & LLVector4Logical::MASK_XYZ) == LLVector4Logical::MASK_XYZ;
	
}

////////////////////////////////////
// OPERATORS
////////////////////////////////////	

// Do NOT add aditional operators without consulting someone with SSE experience
inline const LLVector4a& LLVector4a::operator= ( const LLVector4a& rhs )
{
	mQ = rhs.mQ;
	return *this;
}

inline const LLVector4a& LLVector4a::operator= ( const LLQuad& rhs )
{
	mQ = rhs;
	return *this;
}

inline LLVector4a::operator LLQuad() const
{
	return mQ;
}
