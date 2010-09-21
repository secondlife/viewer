/** 
 * @file llsimdtypes.h
 * @brief Declaration of basic SIMD math related types
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

#ifndef LL_SIMD_TYPES_H
#define LL_SIMD_TYPES_H

#ifndef LL_SIMD_MATH_H
#error "Please include llmath.h before this file."
#endif

typedef __m128	LLQuad;


#if LL_WINDOWS
#pragma warning(push)
#pragma warning( disable : 4800 3 ) // Disable warning about casting int to bool for this class.
#if defined(_MSC_VER) && (_MSC_VER < 1500)
// VC++ 2005 is missing these intrinsics
// __forceinline is MSVC specific and attempts to override compiler inlining judgment. This is so
// even in debug builds this call is a NOP.
__forceinline const __m128 _mm_castsi128_ps( const __m128i a ) { return reinterpret_cast<const __m128&>(a); }
__forceinline const __m128i _mm_castps_si128( const __m128 a ) { return reinterpret_cast<const __m128i&>(a); }
#endif // _MSC_VER

#endif // LL_WINDOWS

class LLBool32
{
public:
	inline LLBool32() {}
	inline LLBool32(int rhs) : m_bool(rhs) {}
	inline LLBool32(unsigned int rhs) : m_bool(rhs) {}
	inline LLBool32(bool rhs) { m_bool = static_cast<const int>(rhs); }
	inline LLBool32& operator= (bool rhs) { m_bool = (int)rhs; return *this; }
	inline bool operator== (bool rhs) const { return static_cast<const bool&>(m_bool) == rhs; }
	inline bool operator!= (bool rhs) const { return !operator==(rhs); }
	inline operator bool() const { return static_cast<const bool&>(m_bool); }

private:
	int m_bool;
};

#if LL_WINDOWS
#pragma warning(pop)
#endif

class LLSimdScalar
{
public:
	inline LLSimdScalar() {}
	inline LLSimdScalar(LLQuad q) 
	{ 
		mQ = q; 
	}

	inline LLSimdScalar(F32 f) 
	{ 
		mQ = _mm_set_ss(f); 
	}

	static inline const LLSimdScalar& getZero()
	{
		extern const LLQuad F_ZERO_4A;
		return reinterpret_cast<const LLSimdScalar&>(F_ZERO_4A);
	}

	inline F32 getF32() const;

	inline LLBool32 isApproximatelyEqual(const LLSimdScalar& rhs, F32 tolerance = F_APPROXIMATELY_ZERO) const;

	inline LLSimdScalar getAbs() const;

	inline void setMax( const LLSimdScalar& a, const LLSimdScalar& b );
	
	inline void setMin( const LLSimdScalar& a, const LLSimdScalar& b );

	inline LLSimdScalar& operator=(F32 rhs);

	inline LLSimdScalar& operator+=(const LLSimdScalar& rhs);

	inline LLSimdScalar& operator-=(const LLSimdScalar& rhs);

	inline LLSimdScalar& operator*=(const LLSimdScalar& rhs);

	inline LLSimdScalar& operator/=(const LLSimdScalar& rhs);

	inline operator LLQuad() const
	{ 
		return mQ; 
	}
	
	inline const LLQuad& getQuad() const 
	{ 
		return mQ; 
	}

private:
	LLQuad mQ;
};

#endif //LL_SIMD_TYPES_H
