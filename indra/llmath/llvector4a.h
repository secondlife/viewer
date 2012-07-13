/** 
 * @file llvector4a.h
 * @brief LLVector4a class header file - memory aligned and vectorized 4 component vector
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

#ifndef	LL_LLVECTOR4A_H
#define	LL_LLVECTOR4A_H


class LLRotation;

#include <assert.h>
#include "llpreprocessor.h"
#include "llmemory.h"

///////////////////////////////////
// FIRST TIME USERS PLEASE READ
//////////////////////////////////
// This is just the beginning of LLVector4a. There are many more useful functions
// yet to be implemented. For example, setNeg to negate a vector, rotate() to apply
// a matrix rotation, various functions to manipulate only the X, Y, and Z elements
// and many others (including a whole variety of accessors). So if you don't see a 
// function here that you need, please contact Falcon or someone else with SSE 
// experience (Richard, I think, has some and davep has a little as of the time 
// of this writing, July 08, 2010) about getting it implemented before you resort to
// LLVector3/LLVector4. 
/////////////////////////////////

LL_ALIGN_PREFIX(16)
class LLVector4a
{
public:

	///////////////////////////////////
	// STATIC METHODS
	///////////////////////////////////
	
	// Call initClass() at startup to avoid 15,000+ cycle penalties from denormalized numbers
	static void initClass()
	{
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
	}

	// Return a vector of all zeros
	static inline const LLVector4a& getZero()
	{
		extern const LLVector4a LL_V4A_ZERO;
		return LL_V4A_ZERO;
	}
	
	// Return a vector of all epsilon, where epsilon is a small float suitable for approximate equality checks
	static inline const LLVector4a& getEpsilon()
	{
		extern const LLVector4a LL_V4A_EPSILON;
		return LL_V4A_EPSILON;
	}

	// Copy 16 bytes from src to dst. Source and destination must be 16-byte aligned
	static inline void copy4a(F32* dst, const F32* src)
	{
		_mm_store_ps(dst, _mm_load_ps(src));
	}

	// Copy words 16-byte blocks from src to dst. Source and destination must not overlap. 
	// Source and dest must be 16-byte aligned and size must be multiple of 16.
	static void memcpyNonAliased16(F32* __restrict dst, const F32* __restrict src, size_t bytes);

	////////////////////////////////////
	// CONSTRUCTORS 
	////////////////////////////////////
	
	LLVector4a()
	{ //DO NOT INITIALIZE -- The overhead is completely unnecessary
		ll_assert_aligned(this,16);
	}
	
	LLVector4a(F32 x, F32 y, F32 z, F32 w = 0.f)
	{
		set(x,y,z,w);
	}
	
	LLVector4a(F32 x)
	{
		splat(x);
	}
	
	LLVector4a(const LLSimdScalar& x)
	{
		splat(x);
	}

	LLVector4a(LLQuad q)
	{
		mQ = q;
	}

	////////////////////////////////////
	// LOAD/STORE
	////////////////////////////////////
	
	// Load from 16-byte aligned src array (preferred method of loading)
	inline void load4a(const F32* src);
	
	// Load from unaligned src array (NB: Significantly slower than load4a)
	inline void loadua(const F32* src);
	
	// Load only three floats beginning at address 'src'. Slowest method.
	inline void load3(const F32* src);
	
	// Store to a 16-byte aligned memory address
	inline void store4a(F32* dst) const;
	
	////////////////////////////////////
	// BASIC GET/SET 
	////////////////////////////////////
	
	// Return a "this" as an F32 pointer. Do not use unless you have a very good reason.  (Not sure? Ask Falcon)
	inline F32* getF32ptr();
	
	// Return a "this" as a const F32 pointer. Do not use unless you have a very good reason.  (Not sure? Ask Falcon)
	inline const F32* const getF32ptr() const;
	
	// Read-only access a single float in this vector. Do not use in proximity to any function call that manipulates
	// the data at the whole vector level or you will incur a substantial penalty. Consider using the splat functions instead
	inline F32 operator[](const S32 idx) const;

	// Prefer this method for read-only access to a single element. Prefer the templated version if the elem is known at compile time.
	inline LLSimdScalar getScalarAt(const S32 idx) const;

	// Prefer this method for read-only access to a single element. Prefer the templated version if the elem is known at compile time.
	template <int N> LL_FORCE_INLINE LLSimdScalar getScalarAt() const;

	// Set to an x, y, z and optional w provided
	inline void set(F32 x, F32 y, F32 z, F32 w = 0.f);
	
	// Set to all zeros. This is preferred to using ::getZero()
	inline void clear();
	
	// Set all elements to 'x'
	inline void splat(const F32 x);

	// Set all elements to 'x'
	inline void splat(const LLSimdScalar& x);
	
	// Set all 4 elements to element N of src, with N known at compile time
	template <int N> void splat(const LLVector4a& src);
	
	// Set all 4 elements to element i of v, with i NOT known at compile time
	inline void splat(const LLVector4a& v, U32 i);
	
	// Select bits from sourceIfTrue and sourceIfFalse according to bits in mask
	inline void setSelectWithMask( const LLVector4Logical& mask, const LLVector4a& sourceIfTrue, const LLVector4a& sourceIfFalse );
	
	////////////////////////////////////
	// ALGEBRAIC
	////////////////////////////////////
	
	// Set this to the element-wise (a + b)
	inline void setAdd(const LLVector4a& a, const LLVector4a& b);
	
	// Set this to element-wise (a - b)
	inline void setSub(const LLVector4a& a, const LLVector4a& b);
	
	// Set this to element-wise multiply (a * b)
	inline void setMul(const LLVector4a& a, const LLVector4a& b);
	
	// Set this to element-wise quotient (a / b)
	inline void setDiv(const LLVector4a& a, const LLVector4a& b);
	
	// Set this to the element-wise absolute value of src
	inline void setAbs(const LLVector4a& src);
	
	// Add to each component in this vector the corresponding component in rhs
	inline void add(const LLVector4a& rhs);
	
	// Subtract from each component in this vector the corresponding component in rhs
	inline void sub(const LLVector4a& rhs);
	
	// Multiply each component in this vector by the corresponding component in rhs
	inline void mul(const LLVector4a& rhs);
	
	// Divide each component in this vector by the corresponding component in rhs
	inline void div(const LLVector4a& rhs);
	
	// Multiply this vector by x in a scalar fashion
	inline void mul(const F32 x);

	// Set this to (a x b) (geometric cross-product)
	inline void setCross3(const LLVector4a& a, const LLVector4a& b);
	
	// Set all elements to the dot product of the x, y, and z elements in a and b
	inline void setAllDot3(const LLVector4a& a, const LLVector4a& b);

	// Set all elements to the dot product of the x, y, z, and w elements in a and b
	inline void setAllDot4(const LLVector4a& a, const LLVector4a& b);

	// Return the 3D dot product of this vector and b
	inline LLSimdScalar dot3(const LLVector4a& b) const;

	// Return the 4D dot product of this vector and b
	inline LLSimdScalar dot4(const LLVector4a& b) const;

	// Normalize this vector with respect to the x, y, and z components only. Accurate to 22 bites of precision. W component is destroyed
	// Note that this does not consider zero length vectors!
	inline void normalize3();

	// Same as normalize3() but with respect to all 4 components
	inline void normalize4();

	// Same as normalize3(), but returns length as a SIMD scalar
	inline LLSimdScalar normalize3withLength();

	// Normalize this vector with respect to the x, y, and z components only. Accurate only to 10-12 bits of precision. W component is destroyed
	// Note that this does not consider zero length vectors!
	inline void normalize3fast();

	// Return true if this vector is normalized with respect to x,y,z up to tolerance
	inline LLBool32 isNormalized3( F32 tolerance = 1e-3 ) const;

	// Return true if this vector is normalized with respect to all components up to tolerance
	inline LLBool32 isNormalized4( F32 tolerance = 1e-3 ) const;

	// Set all elements to the length of vector 'v' 
	inline void setAllLength3( const LLVector4a& v );

	// Get this vector's length
	inline LLSimdScalar getLength3() const;
	
	// Set the components of this vector to the minimum of the corresponding components of lhs and rhs
	inline void setMin(const LLVector4a& lhs, const LLVector4a& rhs);
	
	// Set the components of this vector to the maximum of the corresponding components of lhs and rhs
	inline void setMax(const LLVector4a& lhs, const LLVector4a& rhs);
	
	// Clamps this vector to be within the component-wise range low to high (inclusive)
	inline void clamp( const LLVector4a& low, const LLVector4a& high );

	// Set this to  (c * lhs) + rhs * ( 1 - c)
	inline void setLerp(const LLVector4a& lhs, const LLVector4a& rhs, F32 c);
	
	// Return true (nonzero) if x, y, z (and w for Finite4) are all finite floats
	inline LLBool32 isFinite3() const;	
	inline LLBool32 isFinite4() const;

	// Set this vector to 'vec' rotated by the LLRotation or LLQuaternion2 provided
	void setRotated( const LLRotation& rot, const LLVector4a& vec );
	void setRotated( const class LLQuaternion2& quat, const LLVector4a& vec );

	// Set this vector to 'vec' rotated by the INVERSE of the LLRotation or LLQuaternion2 provided
	inline void setRotatedInv( const LLRotation& rot, const LLVector4a& vec );
	inline void setRotatedInv( const class LLQuaternion2& quat, const LLVector4a& vec );

	// Quantize this vector to 8 or 16 bit precision
	void quantize8( const LLVector4a& low, const LLVector4a& high );
	void quantize16( const LLVector4a& low, const LLVector4a& high );

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
	
	inline LLVector4Logical greaterThan(const LLVector4a& rhs) const;

	inline LLVector4Logical lessThan(const LLVector4a& rhs) const;
	
	inline LLVector4Logical greaterEqual(const LLVector4a& rhs) const;

	inline LLVector4Logical lessEqual(const LLVector4a& rhs) const;
	
	inline LLVector4Logical equal(const LLVector4a& rhs) const;

	// Returns true if this and rhs are componentwise equal up to the specified absolute tolerance
	inline bool equals4(const LLVector4a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;

	inline bool equals3(const LLVector4a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;

	////////////////////////////////////
	// OPERATORS
	////////////////////////////////////	
	
	// Do NOT add aditional operators without consulting someone with SSE experience
	inline const LLVector4a& operator= ( const LLVector4a& rhs );
	
	inline const LLVector4a& operator= ( const LLQuad& rhs );

	inline operator LLQuad() const;	

private:
	LLQuad mQ;
} LL_ALIGN_POSTFIX(16);

inline void update_min_max(LLVector4a& min, LLVector4a& max, const LLVector4a& p)
{
	min.setMin(min, p);
	max.setMax(max, p);
}

#endif
