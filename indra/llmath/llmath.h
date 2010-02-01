/** 
 * @file llmath.h
 * @brief Useful math constants and macros.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LLMATH_H
#define LLMATH_H

#include <cmath>
#include <cstdlib>
#include <complex>
#include "lldefs.h"
//#include "llstl.h" // *TODO: Remove when LLString is gone
//#include "llstring.h" // *TODO: Remove when LLString is gone
// lltut.h uses is_approx_equal_fraction(). This was moved to its own header
// file in llcommon so we can use lltut.h for llcommon tests without making
// llcommon depend on llmath.
#include "is_approx_equal_fraction.h"

// work around for Windows & older gcc non-standard function names.
#if LL_WINDOWS
#include <float.h>
#define llisnan(val)	_isnan(val)
#define llfinite(val)	_finite(val)
#elif (LL_LINUX && __GNUC__ <= 2)
#define llisnan(val)	isnan(val)
#define llfinite(val)	isfinite(val)
#elif LL_SOLARIS
#define llisnan(val)    isnan(val)
#define llfinite(val)   (val <= std::numeric_limits<double>::max())
#else
#define llisnan(val)	std::isnan(val)
#define llfinite(val)	std::isfinite(val)
#endif

// Single Precision Floating Point Routines
#ifndef fsqrtf
#define fsqrtf(x)		((F32)sqrt((F64)(x)))
#endif
#ifndef sqrtf
#define sqrtf(x)		((F32)sqrt((F64)(x)))
#endif

#ifndef cosf
#define cosf(x)		((F32)cos((F64)(x)))
#endif
#ifndef sinf
#define sinf(x)		((F32)sin((F64)(x)))
#endif
#ifndef tanf
#define tanf(x)		((F32)tan((F64)(x)))
#endif
#ifndef acosf
#define acosf(x)		((F32)acos((F64)(x)))
#endif

#ifndef powf
#define powf(x,y) ((F32)pow((F64)(x),(F64)(y)))
#endif

const F32	GRAVITY			= -9.8f;

// mathematical constants
const F32	F_PI		= 3.1415926535897932384626433832795f;
const F32	F_TWO_PI	= 6.283185307179586476925286766559f;
const F32	F_PI_BY_TWO	= 1.5707963267948966192313216916398f;
const F32	F_SQRT_TWO_PI = 2.506628274631000502415765284811f;
const F32	F_E			= 2.71828182845904523536f;
const F32	F_SQRT2		= 1.4142135623730950488016887242097f;
const F32	F_SQRT3		= 1.73205080756888288657986402541f;
const F32	OO_SQRT2	= 0.7071067811865475244008443621049f;
const F32	DEG_TO_RAD	= 0.017453292519943295769236907684886f;
const F32	RAD_TO_DEG	= 57.295779513082320876798154814105f;
const F32	F_APPROXIMATELY_ZERO = 0.00001f;
const F32	F_LN2		= 0.69314718056f;
const F32	OO_LN2		= 1.4426950408889634073599246810019f;

const F32	F_ALMOST_ZERO	= 0.0001f;
const F32	F_ALMOST_ONE	= 1.0f - F_ALMOST_ZERO;

// BUG: Eliminate in favor of F_APPROXIMATELY_ZERO above?
const F32 FP_MAG_THRESHOLD = 0.0000001f;

// TODO: Replace with logic like is_approx_equal
inline BOOL is_approx_zero( F32 f ) { return (-F_APPROXIMATELY_ZERO < f) && (f < F_APPROXIMATELY_ZERO); }

// These functions work by interpreting sign+exp+mantissa as an unsigned
// integer.
// For example:
// x = <sign>1 <exponent>00000010 <mantissa>00000000000000000000000
// y = <sign>1 <exponent>00000001 <mantissa>11111111111111111111111
//
// interpreted as ints = 
// x = 10000001000000000000000000000000
// y = 10000000111111111111111111111111
// which is clearly a different of 1 in the least significant bit
// Values with the same exponent can be trivially shown to work.
//
// WARNING: Denormals of opposite sign do not work
// x = <sign>1 <exponent>00000000 <mantissa>00000000000000000000001
// y = <sign>0 <exponent>00000000 <mantissa>00000000000000000000001
// Although these values differ by 2 in the LSB, the sign bit makes
// the int comparison fail.
//
// WARNING: NaNs can compare equal
// There is no special treatment of exceptional values like NaNs
//
// WARNING: Infinity is comparable with F32_MAX and negative 
// infinity is comparable with F32_MIN

inline BOOL is_approx_equal(F32 x, F32 y)
{
	const S32 COMPARE_MANTISSA_UP_TO_BIT = 0x02;
	return (std::abs((S32) ((U32&)x - (U32&)y) ) < COMPARE_MANTISSA_UP_TO_BIT);
}

inline BOOL is_approx_equal(F64 x, F64 y)
{
	const S64 COMPARE_MANTISSA_UP_TO_BIT = 0x02;
	return (std::abs((S32) ((U64&)x - (U64&)y) ) < COMPARE_MANTISSA_UP_TO_BIT);
}

inline S32 llabs(const S32 a)
{
	return S32(std::labs(a));
}

inline F32 llabs(const F32 a)
{
	return F32(std::fabs(a));
}

inline F64 llabs(const F64 a)
{
	return F64(std::fabs(a));
}

inline S32 lltrunc( F32 f )
{
#if LL_WINDOWS && !defined( __INTEL_COMPILER )
		// Avoids changing the floating point control word.
		// Add or subtract 0.5 - epsilon and then round
		const static U32 zpfp[] = { 0xBEFFFFFF, 0x3EFFFFFF };
		S32 result;
		__asm {
			fld		f
			mov		eax,	f
			shr		eax,	29
			and		eax,	4
			fadd	dword ptr [zpfp + eax]
			fistp	result
		}
		return result;
#else
		return (S32)f;
#endif
}

inline S32 lltrunc( F64 f )
{
	return (S32)f;
}

inline S32 llfloor( F32 f )
{
#if LL_WINDOWS && !defined( __INTEL_COMPILER )
		// Avoids changing the floating point control word.
		// Accurate (unlike Stereopsis version) for all values between S32_MIN and S32_MAX and slightly faster than Stereopsis version.
		// Add -(0.5 - epsilon) and then round
		const U32 zpfp = 0xBEFFFFFF;
		S32 result;
		__asm { 
			fld		f
			fadd	dword ptr [zpfp]
			fistp	result
		}
		return result;
#else
		return (S32)floorf(f);
#endif
}


inline S32 llceil( F32 f )
{
	// This could probably be optimized, but this works.
	return (S32)ceil(f);
}


#ifndef BOGUS_ROUND
// Use this round.  Does an arithmetic round (0.5 always rounds up)
inline S32 llround(const F32 val)
{
	return llfloor(val + 0.5f);
}

#else // BOGUS_ROUND
// Old llround implementation - does banker's round (toward nearest even in the case of a 0.5.
// Not using this because we don't have a consistent implementation on both platforms, use
// llfloor(val + 0.5f), which is consistent on all platforms.
inline S32 llround(const F32 val)
{
	#if LL_WINDOWS
		// Note: assumes that the floating point control word is set to rounding mode (the default)
		S32 ret_val;
		_asm fld	val
		_asm fistp	ret_val;
		return ret_val;
	#elif LL_LINUX
		// Note: assumes that the floating point control word is set
		// to rounding mode (the default)
		S32 ret_val;
		__asm__ __volatile__( "flds %1    \n\t"
							  "fistpl %0  \n\t"
							  : "=m" (ret_val)
							  : "m" (val) );
		return ret_val;
	#else
		return llfloor(val + 0.5f);
	#endif
}

// A fast arithmentic round on intel, from Laurent de Soras http://ldesoras.free.fr
inline int round_int(double x)
{
	const float round_to_nearest = 0.5f;
	int i;
	__asm
	{
		fld x
		fadd st, st (0)
		fadd round_to_nearest
		fistp i
		sar i, 1
	}
	return (i);
}
#endif // BOGUS_ROUND

inline F32 llround( F32 val, F32 nearest )
{
	return F32(floor(val * (1.0f / nearest) + 0.5f)) * nearest;
}

inline F64 llround( F64 val, F64 nearest )
{
	return F64(floor(val * (1.0 / nearest) + 0.5)) * nearest;
}

// these provide minimum peak error
//
// avg  error = -0.013049 
// peak error = -31.4 dB
// RMS  error = -28.1 dB

const F32 FAST_MAG_ALPHA = 0.960433870103f;
const F32 FAST_MAG_BETA = 0.397824734759f;

// these provide minimum RMS error
//
// avg  error = 0.000003 
// peak error = -32.6 dB
// RMS  error = -25.7 dB
//
//const F32 FAST_MAG_ALPHA = 0.948059448969f;
//const F32 FAST_MAG_BETA = 0.392699081699f;

inline F32 fastMagnitude(F32 a, F32 b)
{ 
	a = (a > 0) ? a : -a;
	b = (b > 0) ? b : -b;
	return(FAST_MAG_ALPHA * llmax(a,b) + FAST_MAG_BETA * llmin(a,b));
}



////////////////////
//
// Fast F32/S32 conversions
//
// Culled from www.stereopsis.com/FPU.html

const F64 LL_DOUBLE_TO_FIX_MAGIC	= 68719476736.0*1.5;     //2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor
const S32 LL_SHIFT_AMOUNT			= 16;                    //16.16 fixed point representation,

// Endian dependent code
#ifdef LL_LITTLE_ENDIAN
	#define LL_EXP_INDEX				1
	#define LL_MAN_INDEX				0
#else
	#define LL_EXP_INDEX				0
	#define LL_MAN_INDEX				1
#endif

/* Deprecated: use llround(), lltrunc(), or llfloor() instead
// ================================================================================================
// Real2Int
// ================================================================================================
inline S32 F64toS32(F64 val)
{
	val		= val + LL_DOUBLE_TO_FIX_MAGIC;
	return ((S32*)&val)[LL_MAN_INDEX] >> LL_SHIFT_AMOUNT; 
}

// ================================================================================================
// Real2Int
// ================================================================================================
inline S32 F32toS32(F32 val)
{
	return F64toS32 ((F64)val);
}
*/

////////////////////////////////////////////////
//
// Fast exp and log
//

// Implementation of fast exp() approximation (from a paper by Nicol N. Schraudolph
// http://www.inf.ethz.ch/~schraudo/pubs/exp.pdf
static union
{
	double d;
	struct
	{
#ifdef LL_LITTLE_ENDIAN
		S32 j, i;
#else
		S32 i, j;
#endif
	} n;
} LLECO; // not sure what the name means

#define LL_EXP_A (1048576 * OO_LN2) // use 1512775 for integer
#define LL_EXP_C (60801)			// this value of C good for -4 < y < 4

#define LL_FAST_EXP(y) (LLECO.n.i = llround(F32(LL_EXP_A*(y))) + (1072693248 - LL_EXP_C), LLECO.d)



inline F32 llfastpow(const F32 x, const F32 y)
{
	return (F32)(LL_FAST_EXP(y * log(x)));
}


inline F32 snap_to_sig_figs(F32 foo, S32 sig_figs)
{
	// compute the power of ten
	F32 bar = 1.f;
	for (S32 i = 0; i < sig_figs; i++)
	{
		bar *= 10.f;
	}

	foo = (F32)llround(foo * bar);

	// shift back
	foo /= bar;
	return foo;
}

inline F32 lerp(F32 a, F32 b, F32 u) 
{
	return a + ((b - a) * u);
}

inline F32 lerp2d(F32 x00, F32 x01, F32 x10, F32 x11, F32 u, F32 v)
{
	F32 a = x00 + (x01-x00)*u;
	F32 b = x10 + (x11-x10)*u;
	F32 r = a + (b-a)*v;
	return r;
}

inline F32 ramp(F32 x, F32 a, F32 b)
{
	return (a == b) ? 0.0f : ((a - x) / (a - b));
}

inline F32 rescale(F32 x, F32 x1, F32 x2, F32 y1, F32 y2)
{
	return lerp(y1, y2, ramp(x, x1, x2));
}

inline F32 clamp_rescale(F32 x, F32 x1, F32 x2, F32 y1, F32 y2)
{
	if (y1 < y2)
	{
		return llclamp(rescale(x,x1,x2,y1,y2),y1,y2);
	}
	else
	{
		return llclamp(rescale(x,x1,x2,y1,y2),y2,y1);
	}
}


inline F32 cubic_step( F32 x, F32 x0, F32 x1, F32 s0, F32 s1 )
{
	if (x <= x0)
		return s0;

	if (x >= x1)
		return s1;

	F32 f = (x - x0) / (x1 - x0);

	return	s0 + (s1 - s0) * (f * f) * (3.0f - 2.0f * f);
}

inline F32 cubic_step( F32 x )
{
	x = llclampf(x);

	return	(x * x) * (3.0f - 2.0f * x);
}

inline F32 quadratic_step( F32 x, F32 x0, F32 x1, F32 s0, F32 s1 )
{
	if (x <= x0)
		return s0;

	if (x >= x1)
		return s1;

	F32 f = (x - x0) / (x1 - x0);
	F32 f_squared = f * f;

	return	(s0 * (1.f - f_squared)) + ((s1 - s0) * f_squared);
}

inline F32 llsimple_angle(F32 angle)
{
	while(angle <= -F_PI)
		angle += F_TWO_PI;
	while(angle >  F_PI)
		angle -= F_TWO_PI;
	return angle;
}

//SDK - Renamed this to get_lower_power_two, since this is what this actually does.
inline U32 get_lower_power_two(U32 val, U32 max_power_two)
{
	if(!max_power_two)
	{
		max_power_two = 1 << 31 ;
	}
	if(max_power_two & (max_power_two - 1))
	{
		return 0 ;
	}

	for(; val < max_power_two ; max_power_two >>= 1) ;
	
	return max_power_two ;
}

// calculate next highest power of two, limited by max_power_two
// This is taken from a brilliant little code snipped on http://acius2.blogspot.com/2007/11/calculating-next-power-of-2.html
// Basically we convert the binary to a solid string of 1's with the same
// number of digits, then add one.  We subtract 1 initially to handle
// the case where the number passed in is actually a power of two.
// WARNING: this only works with 32 bit ints.
inline U32 get_next_power_two(U32 val, U32 max_power_two)
{
	if(!max_power_two)
	{
		max_power_two = 1 << 31 ;
	}

	if(val >= max_power_two)
	{
		return max_power_two;
	}

	val--;
	val = (val >> 1) | val;
	val = (val >> 2) | val;
	val = (val >> 4) | val;
	val = (val >> 8) | val;
	val = (val >> 16) | val;
	val++;

	return val;
}

//get the gaussian value given the linear distance from axis x and guassian value o
inline F32 llgaussian(F32 x, F32 o)
{
	return 1.f/(F_SQRT_TWO_PI*o)*powf(F_E, -(x*x)/(2*o*o));
}

#endif
