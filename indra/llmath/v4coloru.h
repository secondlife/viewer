/** 
 * @file v4coloru.h
 * @brief The LLColor4U class.
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

#ifndef LL_V4COLORU_H
#define LL_V4COLORU_H

#include "llerror.h"
//#include "vmath.h"
#include "llmath.h"
//#include "v4color.h"

#include "v3color.h"
#include "v4color.h"

//class LLColor3U;
class LLColor4;

//  LLColor4U = | red green blue alpha |

static const U32 LENGTHOFCOLOR4U = 4;


class LLColor4U
{
public:

	union
	{
		U8         mV[LENGTHOFCOLOR4U];
		U32        mAll;
		LLColor4*  mSources;
		LLColor4U* mSourcesU;
	};


	LLColor4U();						// Initializes LLColor4U to (0, 0, 0, 1)
	LLColor4U(U8 r, U8 g, U8 b);		// Initializes LLColor4U to (r, g, b, 1)
	LLColor4U(U8 r, U8 g, U8 b, U8 a);		// Initializes LLColor4U to (r. g, b, a)
	LLColor4U(const U8 *vec);			// Initializes LLColor4U to (vec[0]. vec[1], vec[2], 1)
	explicit LLColor4U(const LLSD& sd)
	{
		setValue(sd);
	}

	void setValue(const LLSD& sd)
	{
		mV[0] = sd[0].asInteger();
		mV[1] = sd[1].asInteger();
		mV[2] = sd[2].asInteger();
		mV[3] = sd[3].asInteger();
	}

	LLSD getValue() const
	{
		LLSD ret;
		ret[0] = mV[0];
		ret[1] = mV[1];
		ret[2] = mV[2];
		ret[3] = mV[3];
		return ret;
	}

	const LLColor4U&	setToBlack();						// zero LLColor4U to (0, 0, 0, 1)
	const LLColor4U&	setToWhite();						// zero LLColor4U to (0, 0, 0, 1)

	const LLColor4U&	set(U8 r, U8 g, U8 b, U8 a);// Sets LLColor4U to (r, g, b, a)
	const LLColor4U&	set(U8 r, U8 g, U8 b);		// Sets LLColor4U to (r, g, b) (no change in a)
	const LLColor4U&	set(const LLColor4U &vec);	// Sets LLColor4U to vec
	const LLColor4U&	set(const U8 *vec);			// Sets LLColor4U to vec

	const LLColor4U&	setVec(U8 r, U8 g, U8 b, U8 a);	// deprecated -- use set()
	const LLColor4U&	setVec(U8 r, U8 g, U8 b);		// deprecated -- use set()
	const LLColor4U&	setVec(const LLColor4U &vec);	// deprecated -- use set()
	const LLColor4U&	setVec(const U8 *vec);			// deprecated -- use set()

	const LLColor4U&    setAlpha(U8 a);

	F32			magVec() const;				// deprecated -- use length()
	F32			magVecSquared() const;		// deprecated -- use lengthSquared()

	F32			length() const;				// Returns magnitude squared of LLColor4U
	F32			lengthSquared() const;		// Returns magnitude squared of LLColor4U

	friend std::ostream&	 operator<<(std::ostream& s, const LLColor4U &a);		// Print a
	friend LLColor4U operator+(const LLColor4U &a, const LLColor4U &b);	// Return vector a + b
	friend LLColor4U operator-(const LLColor4U &a, const LLColor4U &b);	// Return vector a minus b
	friend LLColor4U operator*(const LLColor4U &a, const LLColor4U &b);	// Return a * b
	friend bool operator==(const LLColor4U &a, const LLColor4U &b);		// Return a == b
	friend bool operator!=(const LLColor4U &a, const LLColor4U &b);		// Return a != b

	friend const LLColor4U& operator+=(LLColor4U &a, const LLColor4U &b);	// Return vector a + b
	friend const LLColor4U& operator-=(LLColor4U &a, const LLColor4U &b);	// Return vector a minus b
	friend const LLColor4U& operator*=(LLColor4U &a, U8 k);				// Return rgb times scaler k (no alpha change)
	friend const LLColor4U& operator%=(LLColor4U &a, U8 k);				// Return alpha times scaler k (no rgb change)

	LLColor4U addClampMax(const LLColor4U &color);						// Add and clamp the max

	LLColor4U multAll(const F32 k);										// Multiply ALL channels by scalar k
	const LLColor4U& combine();

	inline void setVecScaleClamp(const LLColor3 &color);
	inline void setVecScaleClamp(const LLColor4 &color);

	static BOOL parseColor4U(const std::string& buf, LLColor4U* value);

	// conversion
	operator const LLColor4() const
	{
		return LLColor4(*this);
	}

	static LLColor4U white;
	static LLColor4U black;
	static LLColor4U red;
	static LLColor4U green;
	static LLColor4U blue;
};


// Non-member functions 
F32		distVec(const LLColor4U &a, const LLColor4U &b);			// Returns distance between a and b
F32		distVec_squared(const LLColor4U &a, const LLColor4U &b);	// Returns distance squared between a and b


inline LLColor4U::LLColor4U()
{
	mV[VX] = 0;
	mV[VY] = 0;
	mV[VZ] = 0;
	mV[VW] = 255;
}

inline LLColor4U::LLColor4U(U8 r, U8 g, U8 b)
{
	mV[VX] = r;
	mV[VY] = g;
	mV[VZ] = b;
	mV[VW] = 255;
}

inline LLColor4U::LLColor4U(U8 r, U8 g, U8 b, U8 a)
{
	mV[VX] = r;
	mV[VY] = g;
	mV[VZ] = b;
	mV[VW] = a;
}

inline LLColor4U::LLColor4U(const U8 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}

/*
inline LLColor4U::operator LLColor4()
{
	return(LLColor4((F32)mV[VRED]/255.f,(F32)mV[VGREEN]/255.f,(F32)mV[VBLUE]/255.f,(F32)mV[VALPHA]/255.f));
}
*/

inline const LLColor4U&	LLColor4U::setToBlack(void)
{
	mV[VX] = 0;
	mV[VY] = 0;
	mV[VZ] = 0;
	mV[VW] = 255;
	return (*this);
}

inline const LLColor4U&	LLColor4U::setToWhite(void)
{
	mV[VX] = 255;
	mV[VY] = 255;
	mV[VZ] = 255;
	mV[VW] = 255;
	return (*this);
}

inline const LLColor4U&	LLColor4U::set(const U8 x, const U8 y, const U8 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;

//  no change to alpha!
//	mV[VW] = 255;  

	return (*this);
}

inline const LLColor4U&	LLColor4U::set(const U8 r, const U8 g, const U8 b, U8 a)
{
	mV[0] = r;
	mV[1] = g;
	mV[2] = b;
	mV[3] = a;  
	return (*this);
}

inline const LLColor4U&	LLColor4U::set(const LLColor4U &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
	return (*this);
}

inline const LLColor4U&	LLColor4U::set(const U8 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
	return (*this);
}

// deprecated
inline const LLColor4U&	LLColor4U::setVec(const U8 x, const U8 y, const U8 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;

//  no change to alpha!
//	mV[VW] = 255;  

	return (*this);
}

// deprecated
inline const LLColor4U&	LLColor4U::setVec(const U8 r, const U8 g, const U8 b, U8 a)
{
	mV[0] = r;
	mV[1] = g;
	mV[2] = b;
	mV[3] = a;  
	return (*this);
}

// deprecated
inline const LLColor4U&	LLColor4U::setVec(const LLColor4U &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
	return (*this);
}

// deprecated
inline const LLColor4U&	LLColor4U::setVec(const U8 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
	return (*this);
}

inline const LLColor4U&	LLColor4U::setAlpha(U8 a)
{
	mV[VW] = a;
	return (*this);
}

// LLColor4U Magnitude and Normalization Functions

inline F32		LLColor4U::length(void) const
{
	return (F32) sqrt( ((F32)mV[VX]) * mV[VX] + ((F32)mV[VY]) * mV[VY] + ((F32)mV[VZ]) * mV[VZ] );
}

inline F32		LLColor4U::lengthSquared(void) const
{
	return ((F32)mV[VX]) * mV[VX] + ((F32)mV[VY]) * mV[VY] + ((F32)mV[VZ]) * mV[VZ];
}

// deprecated
inline F32		LLColor4U::magVec(void) const
{
	return (F32) sqrt( ((F32)mV[VX]) * mV[VX] + ((F32)mV[VY]) * mV[VY] + ((F32)mV[VZ]) * mV[VZ] );
}

// deprecated
inline F32		LLColor4U::magVecSquared(void) const
{
	return ((F32)mV[VX]) * mV[VX] + ((F32)mV[VY]) * mV[VY] + ((F32)mV[VZ]) * mV[VZ];
}

inline LLColor4U operator+(const LLColor4U &a, const LLColor4U &b)
{
	return LLColor4U(
		a.mV[VX] + b.mV[VX],
		a.mV[VY] + b.mV[VY],
		a.mV[VZ] + b.mV[VZ],
		a.mV[VW] + b.mV[VW]);
}

inline LLColor4U operator-(const LLColor4U &a, const LLColor4U &b)
{
	return LLColor4U(
		a.mV[VX] - b.mV[VX],
		a.mV[VY] - b.mV[VY],
		a.mV[VZ] - b.mV[VZ],
		a.mV[VW] - b.mV[VW]);
}

inline LLColor4U  operator*(const LLColor4U &a, const LLColor4U &b)
{
	return LLColor4U(
		a.mV[VX] * b.mV[VX],
		a.mV[VY] * b.mV[VY],
		a.mV[VZ] * b.mV[VZ],
		a.mV[VW] * b.mV[VW]);
}

inline LLColor4U LLColor4U::addClampMax(const LLColor4U &color)
{
	return LLColor4U(llmin((S32)mV[VX] + color.mV[VX], 255),
					llmin((S32)mV[VY] + color.mV[VY], 255),
					llmin((S32)mV[VZ] + color.mV[VZ], 255),
					llmin((S32)mV[VW] + color.mV[VW], 255));
}

inline LLColor4U LLColor4U::multAll(const F32 k)
{
	// Round to nearest
	return LLColor4U(
		(U8)llround(mV[VX] * k),
		(U8)llround(mV[VY] * k),
		(U8)llround(mV[VZ] * k),
		(U8)llround(mV[VW] * k));
}
/*
inline LLColor4U operator*(const LLColor4U &a, U8 k)
{	
	// only affects rgb (not a!)
	return LLColor4U(
		a.mV[VX] * k,
		a.mV[VY] * k,
		a.mV[VZ] * k,
		a.mV[VW]);
}

inline LLColor4U operator*(U8 k, const LLColor4U &a)
{
	// only affects rgb (not a!)
	return LLColor4U(
		a.mV[VX] * k,
		a.mV[VY] * k,
		a.mV[VZ] * k,
		a.mV[VW]);
}

inline LLColor4U operator%(U8 k, const LLColor4U &a)
{
	// only affects alpha (not rgb!)
	return LLColor4U(
		a.mV[VX],
		a.mV[VY],
		a.mV[VZ],
		a.mV[VW] * k );
}

inline LLColor4U operator%(const LLColor4U &a, U8 k)
{
	// only affects alpha (not rgb!)
	return LLColor4U(
		a.mV[VX],
		a.mV[VY],
		a.mV[VZ],
		a.mV[VW] * k );
}
*/

inline bool operator==(const LLColor4U &a, const LLColor4U &b)
{
	return (  (a.mV[VX] == b.mV[VX])
			&&(a.mV[VY] == b.mV[VY])
			&&(a.mV[VZ] == b.mV[VZ])
			&&(a.mV[VW] == b.mV[VW]));
}

inline bool operator!=(const LLColor4U &a, const LLColor4U &b)
{
	return (  (a.mV[VX] != b.mV[VX])
			||(a.mV[VY] != b.mV[VY])
			||(a.mV[VZ] != b.mV[VZ])
			||(a.mV[VW] != b.mV[VW]));
}

inline const LLColor4U& operator+=(LLColor4U &a, const LLColor4U &b)
{
	a.mV[VX] += b.mV[VX];
	a.mV[VY] += b.mV[VY];
	a.mV[VZ] += b.mV[VZ];
	a.mV[VW] += b.mV[VW];
	return a;
}

inline const LLColor4U& operator-=(LLColor4U &a, const LLColor4U &b)
{
	a.mV[VX] -= b.mV[VX];
	a.mV[VY] -= b.mV[VY];
	a.mV[VZ] -= b.mV[VZ];
	a.mV[VW] -= b.mV[VW];
	return a;
}

inline const LLColor4U& operator*=(LLColor4U &a, U8 k)
{
	// only affects rgb (not a!)
	a.mV[VX] *= k;
	a.mV[VY] *= k;
	a.mV[VZ] *= k;
	return a;
}

inline const LLColor4U& operator%=(LLColor4U &a, U8 k)
{
	// only affects alpha (not rgb!)
	a.mV[VW] *= k;
	return a;
}

inline F32		distVec(const LLColor4U &a, const LLColor4U &b)
{
	LLColor4U vec = a - b;
	return (vec.length());
}

inline F32		distVec_squared(const LLColor4U &a, const LLColor4U &b)
{
	LLColor4U vec = a - b;
	return (vec.lengthSquared());
}

void LLColor4U::setVecScaleClamp(const LLColor4& color)
{
	F32 color_scale_factor = 255.f;
	F32 max_color = llmax(color.mV[0], color.mV[1], color.mV[2]);
	if (max_color > 1.f)
	{
		color_scale_factor /= max_color;
	}
	const S32 MAX_COLOR = 255;
	S32 r = llround(color.mV[0] * color_scale_factor);
	if (r > MAX_COLOR)
	{
		r = MAX_COLOR;
	}
	else if (r < 0)
	{
		r = 0;
	}
	mV[0] = r;

	S32 g = llround(color.mV[1] * color_scale_factor);
	if (g > MAX_COLOR)
	{
		g = MAX_COLOR;
	}
	else if (g < 0)
	{
		g = 0;
	}
	mV[1] = g;

	S32 b = llround(color.mV[2] * color_scale_factor);
	if (b > MAX_COLOR)
	{
		b = MAX_COLOR;
	}
	else if (b < 0)
	{
		b = 0;
	}
	mV[2] = b;

	// Alpha shouldn't be scaled, just clamped...
	S32 a = llround(color.mV[3] * MAX_COLOR);
	if (a > MAX_COLOR)
	{
		a = MAX_COLOR;
	}
	else if (a < 0)
	{
		a = 0;
	}
	mV[3] = a;
}

void LLColor4U::setVecScaleClamp(const LLColor3& color)
{
	F32 color_scale_factor = 255.f;
	F32 max_color = llmax(color.mV[0], color.mV[1], color.mV[2]);
	if (max_color > 1.f)
	{
		color_scale_factor /= max_color;
	}

	const S32 MAX_COLOR = 255;
	S32 r = llround(color.mV[0] * color_scale_factor);
	if (r > MAX_COLOR)
	{
		r = MAX_COLOR;
	}
	else
	if (r < 0)
	{
		r = 0;
	}
	mV[0] = r;

	S32 g = llround(color.mV[1] * color_scale_factor);
	if (g > MAX_COLOR)
	{
		g = MAX_COLOR;
	}
	else
	if (g < 0)
	{
		g = 0;
	}
	mV[1] = g;

	S32 b = llround(color.mV[2] * color_scale_factor);
	if (b > MAX_COLOR)
	{
		b = MAX_COLOR;
	}
	if (b < 0)
	{
		b = 0;
	}
	mV[2] = b;

	mV[3] = 255;
}


#endif

