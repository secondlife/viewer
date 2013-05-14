/** 
 * @file v3color.h
 * @brief LLColor3 class header file.
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

#ifndef LL_V3COLOR_H
#define LL_V3COLOR_H

class LLColor4;
class LLVector4;

#include "llerror.h"
#include "llmath.h"
#include "llsd.h"
#include <string.h>

//  LLColor3 = |r g b|

static const U32 LENGTHOFCOLOR3 = 3;

class LLColor3
{
public:
	F32 mV[LENGTHOFCOLOR3];

	static LLColor3 white;
	static LLColor3 black;
	static LLColor3 grey;

public:
	LLColor3();							// Initializes LLColor3 to (0, 0, 0)
	LLColor3(F32 r, F32 g, F32 b);		// Initializes LLColor3 to (r, g, b)
	LLColor3(const F32 *vec);			// Initializes LLColor3 to (vec[0]. vec[1], vec[2])
	LLColor3(const char *color_string);       // html format color ie "#FFDDEE"
	explicit LLColor3(const LLColor4& color4);  // "explicit" to avoid automatic conversion
	explicit LLColor3(const LLVector4& vector4);  // "explicit" to avoid automatic conversion
	LLColor3(const LLSD& sd);
	

	LLSD getValue() const
	{
		LLSD ret;
		ret[0] = mV[0];
		ret[1] = mV[1];
		ret[2] = mV[2];
		return ret;
	}

	void setValue(const LLSD& sd)
	{
		mV[0] = (F32) sd[0].asReal();;
		mV[1] = (F32) sd[1].asReal();;
		mV[2] = (F32) sd[2].asReal();;
	}

	void setHSL(F32 hue, F32 saturation, F32 luminance);
	void calcHSL(F32* hue, F32* saturation, F32* luminance) const;
	
	const LLColor3&	setToBlack();					// Clears LLColor3 to (0, 0, 0)
	const LLColor3&	setToWhite();					// Zero LLColor3 to (0, 0, 0)
	
	const LLColor3&	setVec(F32 x, F32 y, F32 z);	// deprecated
	const LLColor3&	setVec(const LLColor3 &vec);	// deprecated
	const LLColor3&	setVec(const F32 *vec);			// deprecated

	const LLColor3&	set(F32 x, F32 y, F32 z);	// Sets LLColor3 to (x, y, z)
	const LLColor3&	set(const LLColor3 &vec);	// Sets LLColor3 to vec
	const LLColor3&	set(const F32 *vec);		// Sets LLColor3 to vec

	F32		magVec() const;				// deprecated
	F32		magVecSquared() const;		// deprecated
	F32		normVec();					// deprecated

	F32		length() const;				// Returns magnitude of LLColor3
	F32		lengthSquared() const;		// Returns magnitude squared of LLColor3
	F32		normalize();				// Normalizes and returns the magnitude of LLColor3

	F32		brightness() const;			// Returns brightness of LLColor3

	const LLColor3&	operator=(const LLColor4 &a);
	
	friend std::ostream&	 operator<<(std::ostream& s, const LLColor3 &a);		// Print a
	friend LLColor3 operator+(const LLColor3 &a, const LLColor3 &b);	// Return vector a + b
	friend LLColor3 operator-(const LLColor3 &a, const LLColor3 &b);	// Return vector a minus b

	friend const LLColor3& operator+=(LLColor3 &a, const LLColor3 &b);	// Return vector a + b
	friend const LLColor3& operator-=(LLColor3 &a, const LLColor3 &b);	// Return vector a minus b
	friend const LLColor3& operator*=(LLColor3 &a, const LLColor3 &b);

	friend LLColor3 operator*(const LLColor3 &a, const LLColor3 &b);	// Return component wise a * b
	friend LLColor3 operator*(const LLColor3 &a, F32 k);				// Return a times scaler k
	friend LLColor3 operator*(F32 k, const LLColor3 &a);				// Return a times scaler k

	friend bool operator==(const LLColor3 &a, const LLColor3 &b);		// Return a == b
	friend bool operator!=(const LLColor3 &a, const LLColor3 &b);		// Return a != b

	friend const LLColor3& operator*=(LLColor3 &a, F32 k);				// Return a times scaler k

	friend LLColor3 operator-(const LLColor3 &a);					// Return vector 1-rgb (inverse)

	inline void clamp();
	inline void exp();	// Do an exponential on the color
};

LLColor3 lerp(const LLColor3 &a, const LLColor3 &b, F32 u);


void LLColor3::clamp()
{
	// Clamp the color...
	if (mV[0] < 0.f)
	{
		mV[0] = 0.f;
	}
	else if (mV[0] > 1.f)
	{
		mV[0] = 1.f;
	}
	if (mV[1] < 0.f)
	{
		mV[1] = 0.f;
	}
	else if (mV[1] > 1.f)
	{
		mV[1] = 1.f;
	}
	if (mV[2] < 0.f)
	{
		mV[2] = 0.f;
	}
	else if (mV[2] > 1.f)
	{
		mV[2] = 1.f;
	}
}

// Non-member functions 
F32		distVec(const LLColor3 &a, const LLColor3 &b);		// Returns distance between a and b
F32		distVec_squared(const LLColor3 &a, const LLColor3 &b);// Returns distance squared between a and b

inline LLColor3::LLColor3(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}

inline LLColor3::LLColor3(F32 r, F32 g, F32 b)
{
	mV[VX] = r;
	mV[VY] = g;
	mV[VZ] = b;
}


inline LLColor3::LLColor3(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
}

#if LL_WINDOWS
# pragma warning( disable : 4996 ) // strncpy teh sux0r
#endif

inline LLColor3::LLColor3(const char* color_string) // takes a string of format "RRGGBB" where RR is hex 00..FF 
{
	if (strlen(color_string) <  6)		/* Flawfinder: ignore */
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mV[2] = 0.f;		
		return;
	}

	char tempstr[7];
	strncpy(tempstr,color_string,6);		/* Flawfinder: ignore */
	tempstr[6] = '\0';
	mV[VZ] = (F32)strtol(&tempstr[4],NULL,16)/255.f;
	tempstr[4] = '\0';
	mV[VY] = (F32)strtol(&tempstr[2],NULL,16)/255.f;
	tempstr[2] = '\0';
	mV[VX] = (F32)strtol(&tempstr[0],NULL,16)/255.f;
}

inline const LLColor3&	LLColor3::setToBlack(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
	return (*this);
}

inline const LLColor3&	LLColor3::setToWhite(void)
{
	mV[0] = 1.f;
	mV[1] = 1.f;
	mV[2] = 1.f;
	return (*this);
}

inline const LLColor3&	LLColor3::set(F32 r, F32 g, F32 b)
{
	mV[0] = r;
	mV[1] = g;
	mV[2] = b;
	return (*this);
}

inline const LLColor3&	LLColor3::set(const LLColor3 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
	return (*this);
}

inline const LLColor3&	LLColor3::set(const F32 *vec)
{
	mV[0] = vec[0];
	mV[1] = vec[1];
	mV[2] = vec[2];
	return (*this);
}

// deprecated
inline const LLColor3&	LLColor3::setVec(F32 r, F32 g, F32 b)
{
	mV[0] = r;
	mV[1] = g;
	mV[2] = b;
	return (*this);
}

// deprecated
inline const LLColor3&	LLColor3::setVec(const LLColor3 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
	return (*this);
}

// deprecated
inline const LLColor3&	LLColor3::setVec(const F32 *vec)
{
	mV[0] = vec[0];
	mV[1] = vec[1];
	mV[2] = vec[2];
	return (*this);
}

inline F32		LLColor3::brightness(void) const
{
	return (mV[0] + mV[1] + mV[2]) / 3.0f;
}

inline F32		LLColor3::length(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
}

inline F32		LLColor3::lengthSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2];
}

inline F32		LLColor3::normalize(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
	F32 oomag;

	if (mag)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
		mV[2] *= oomag;
	}
	return (mag);
}

// deprecated
inline F32		LLColor3::magVec(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
}

// deprecated
inline F32		LLColor3::magVecSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2];
}

// deprecated
inline F32		LLColor3::normVec(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
	F32 oomag;

	if (mag)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
		mV[2] *= oomag;
	}
	return (mag);
}

inline void LLColor3::exp()
{
#if 0
	mV[0] = ::exp(mV[0]);
	mV[1] = ::exp(mV[1]);
	mV[2] = ::exp(mV[2]);
#else
	mV[0] = (F32)LL_FAST_EXP(mV[0]);
	mV[1] = (F32)LL_FAST_EXP(mV[1]);
	mV[2] = (F32)LL_FAST_EXP(mV[2]);
#endif
}


inline LLColor3 operator+(const LLColor3 &a, const LLColor3 &b)
{
	return LLColor3(
		a.mV[0] + b.mV[0],
		a.mV[1] + b.mV[1],
		a.mV[2] + b.mV[2]);
}

inline LLColor3 operator-(const LLColor3 &a, const LLColor3 &b)
{
	return LLColor3(
		a.mV[0] - b.mV[0],
		a.mV[1] - b.mV[1],
		a.mV[2] - b.mV[2]);
}

inline LLColor3  operator*(const LLColor3 &a, const LLColor3 &b)
{
	return LLColor3(
		a.mV[0] * b.mV[0],
		a.mV[1] * b.mV[1],
		a.mV[2] * b.mV[2]);
}

inline LLColor3 operator*(const LLColor3 &a, F32 k)
{
	return LLColor3( a.mV[0] * k, a.mV[1] * k, a.mV[2] * k );
}

inline LLColor3 operator*(F32 k, const LLColor3 &a)
{
	return LLColor3( a.mV[0] * k, a.mV[1] * k, a.mV[2] * k );
}

inline bool operator==(const LLColor3 &a, const LLColor3 &b)
{
	return (  (a.mV[0] == b.mV[0])
			&&(a.mV[1] == b.mV[1])
			&&(a.mV[2] == b.mV[2]));
}

inline bool operator!=(const LLColor3 &a, const LLColor3 &b)
{
	return (  (a.mV[0] != b.mV[0])
			||(a.mV[1] != b.mV[1])
			||(a.mV[2] != b.mV[2]));
}

inline const LLColor3 &operator*=(LLColor3 &a, const LLColor3 &b)
{
	a.mV[0] *= b.mV[0];
	a.mV[1] *= b.mV[1];
	a.mV[2] *= b.mV[2];
	return a;
}

inline const LLColor3& operator+=(LLColor3 &a, const LLColor3 &b)
{
	a.mV[0] += b.mV[0];
	a.mV[1] += b.mV[1];
	a.mV[2] += b.mV[2];
	return a;
}

inline const LLColor3& operator-=(LLColor3 &a, const LLColor3 &b)
{
	a.mV[0] -= b.mV[0];
	a.mV[1] -= b.mV[1];
	a.mV[2] -= b.mV[2];
	return a;
}

inline const LLColor3& operator*=(LLColor3 &a, F32 k)
{
	a.mV[0] *= k;
	a.mV[1] *= k;
	a.mV[2] *= k;
	return a;
}

inline LLColor3 operator-(const LLColor3 &a)
{
	return LLColor3(
		1.f - a.mV[0],
		1.f - a.mV[1],
		1.f - a.mV[2] );
}

// Non-member functions

inline F32		distVec(const LLColor3 &a, const LLColor3 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	F32 z = a.mV[2] - b.mV[2];
	return (F32) sqrt( x*x + y*y + z*z );
}

inline F32		distVec_squared(const LLColor3 &a, const LLColor3 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	F32 z = a.mV[2] - b.mV[2];
	return x*x + y*y + z*z;
}

inline LLColor3 lerp(const LLColor3 &a, const LLColor3 &b, F32 u)
{
	return LLColor3(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u,
		a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u);
}


#endif
