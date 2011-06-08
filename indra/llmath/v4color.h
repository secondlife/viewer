/** 
 * @file v4color.h
 * @brief LLColor4 class header file.
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

#ifndef LL_V4COLOR_H
#define LL_V4COLOR_H

#include "llerror.h"
//#include "vmath.h"
#include "llmath.h"
#include "llsd.h"

class LLColor3;
class LLColor4U;
class LLVector4;

//  LLColor4 = |x y z w|

static const U32 LENGTHOFCOLOR4 = 4;

static const U32 MAX_LENGTH_OF_COLOR_NAME = 15; //Give plenty of room for additional colors...

class LLColor4
{
	public:
		F32 mV[LENGTHOFCOLOR4];
		LLColor4();						// Initializes LLColor4 to (0, 0, 0, 1)
		LLColor4(F32 r, F32 g, F32 b);		// Initializes LLColor4 to (r, g, b, 1)
		LLColor4(F32 r, F32 g, F32 b, F32 a);		// Initializes LLColor4 to (r. g, b, a)
		LLColor4(U32 clr);							// Initializes LLColor4 to (r=clr>>24, etc))
		LLColor4(const F32 *vec);			// Initializes LLColor4 to (vec[0]. vec[1], vec[2], 1)
		LLColor4(const LLColor3 &vec, F32 a = 1.f);	// Initializes LLColor4 to (vec, a)
		explicit LLColor4(const LLSD& sd);
		explicit LLColor4(const LLColor4U& color4u);  // "explicit" to avoid automatic conversion
		explicit LLColor4(const LLVector4& vector4);  // "explicit" to avoid automatic conversion

		LLSD getValue() const
		{
			LLSD ret;
			ret[0] = mV[0];
			ret[1] = mV[1];
			ret[2] = mV[2];
			ret[3] = mV[3];
			return ret;
		}
	
		void setValue(const LLSD& sd);

		void setHSL(F32 hue, F32 saturation, F32 luminance);
		void calcHSL(F32* hue, F32* saturation, F32* luminance) const;

		const LLColor4&	setToBlack();						// zero LLColor4 to (0, 0, 0, 1)
		const LLColor4&	setToWhite();						// zero LLColor4 to (0, 0, 0, 1)

		const LLColor4&	setVec(F32 r, F32 g, F32 b, F32 a);	// deprecated -- use set()
		const LLColor4&	setVec(F32 r, F32 g, F32 b);		// deprecated -- use set()
		const LLColor4&	setVec(const LLColor4 &vec);		// deprecated -- use set()
		const LLColor4&	setVec(const LLColor3 &vec);		// deprecated -- use set()
		const LLColor4&	setVec(const LLColor3 &vec, F32 a);	// deprecated -- use set()
		const LLColor4&	setVec(const F32 *vec);				// deprecated -- use set()
		const LLColor4&	setVec(const LLColor4U& color4u); 	// deprecated -- use set()

		const LLColor4&	set(F32 r, F32 g, F32 b, F32 a);	// Sets LLColor4 to (r, g, b, a)
		const LLColor4&	set(F32 r, F32 g, F32 b);	// Sets LLColor4 to (r, g, b) (no change in a)
		const LLColor4&	set(const LLColor4 &vec);	// Sets LLColor4 to vec
		const LLColor4&	set(const LLColor3 &vec);	// Sets LLColor4 to LLColor3 vec (no change in alpha)
		const LLColor4&	set(const LLColor3 &vec, F32 a);	// Sets LLColor4 to LLColor3 vec, with alpha specified
		const LLColor4&	set(const F32 *vec);			// Sets LLColor4 to vec
		const LLColor4&	set(const LLColor4U& color4u); // Sets LLColor4 to color4u, rescaled.


		const LLColor4&    setAlpha(F32 a);

		F32			magVec() const;				// deprecated -- use length()
		F32			magVecSquared() const;		// deprecated -- use lengthSquared()
		F32			normVec();					// deprecated -- use normalize()

		F32			length() const;				// Returns magnitude of LLColor4
		F32			lengthSquared() const;		// Returns magnitude squared of LLColor4
		F32			normalize();				// deprecated -- use normalize()

		BOOL		isOpaque() { return mV[VALPHA] == 1.f; }

		F32 operator[](int idx) const { return mV[idx]; }
		F32 &operator[](int idx) { return mV[idx]; }
	
	    const LLColor4& operator=(const LLColor3 &a);	// Assigns vec3 to vec4 and returns vec4
		
		bool operator<(const LLColor4& rhs) const;
		friend std::ostream&	 operator<<(std::ostream& s, const LLColor4 &a);		// Print a
		friend LLColor4 operator+(const LLColor4 &a, const LLColor4 &b);	// Return vector a + b
		friend LLColor4 operator-(const LLColor4 &a, const LLColor4 &b);	// Return vector a minus b
		friend LLColor4 operator*(const LLColor4 &a, const LLColor4 &b);	// Return component wise a * b
		friend LLColor4 operator*(const LLColor4 &a, F32 k);				// Return rgb times scaler k (no alpha change)
		friend LLColor4 operator*(F32 k, const LLColor4 &a);				// Return rgb times scaler k (no alpha change)
		friend LLColor4 operator%(const LLColor4 &a, F32 k);				// Return alpha times scaler k (no rgb change)
		friend LLColor4 operator%(F32 k, const LLColor4 &a);				// Return alpha times scaler k (no rgb change)
		friend bool operator==(const LLColor4 &a, const LLColor4 &b);		// Return a == b
		friend bool operator!=(const LLColor4 &a, const LLColor4 &b);		// Return a != b
		
		friend bool operator==(const LLColor4 &a, const LLColor3 &b);		// Return a == b
		friend bool operator!=(const LLColor4 &a, const LLColor3 &b);		// Return a != b

		friend const LLColor4& operator+=(LLColor4 &a, const LLColor4 &b);	// Return vector a + b
		friend const LLColor4& operator-=(LLColor4 &a, const LLColor4 &b);	// Return vector a minus b
		friend const LLColor4& operator*=(LLColor4 &a, F32 k);				// Return rgb times scaler k (no alpha change)
		friend const LLColor4& operator%=(LLColor4 &a, F32 k);				// Return alpha times scaler k (no rgb change)

		friend const LLColor4& operator*=(LLColor4 &a, const LLColor4 &b); // Doesn't multiply alpha! (for lighting)

		// conversion
		operator const LLColor4U() const;

		// Basic color values.
		static LLColor4 red;
		static LLColor4 green;
		static LLColor4 blue;
		static LLColor4 black;
		static LLColor4 white;
		static LLColor4 yellow;
		static LLColor4 magenta;
		static LLColor4 cyan;
		static LLColor4 smoke;
		static LLColor4 grey;
		static LLColor4 orange;
		static LLColor4 purple;
		static LLColor4 pink;
		static LLColor4 transparent;

		// Extra color values.
		static LLColor4 grey1;
		static LLColor4 grey2;
		static LLColor4 grey3;
		static LLColor4 grey4;

		static LLColor4 red1;
		static LLColor4 red2;
		static LLColor4 red3;
		static LLColor4 red4;
		static LLColor4 red5;

		static LLColor4 green1;
		static LLColor4 green2;
		static LLColor4 green3;
		static LLColor4 green4;
		static LLColor4 green5;
		static LLColor4 green6;

		static LLColor4 blue1;
		static LLColor4 blue2;
		static LLColor4 blue3;
		static LLColor4 blue4;
		static LLColor4 blue5;
		static LLColor4 blue6;

		static LLColor4 yellow1;
		static LLColor4 yellow2;
		static LLColor4 yellow3;
		static LLColor4 yellow4;
		static LLColor4 yellow5;
		static LLColor4 yellow6;
		static LLColor4 yellow7;
		static LLColor4 yellow8;
		static LLColor4 yellow9;

		static LLColor4 orange1;
		static LLColor4 orange2;
		static LLColor4 orange3;
		static LLColor4 orange4;
		static LLColor4 orange5;
		static LLColor4 orange6;

		static LLColor4 magenta1;
		static LLColor4 magenta2;
		static LLColor4 magenta3;
		static LLColor4 magenta4;

		static LLColor4 purple1;
		static LLColor4 purple2;
		static LLColor4 purple3;
		static LLColor4 purple4;
		static LLColor4 purple5;
		static LLColor4 purple6;

		static LLColor4 pink1;
		static LLColor4 pink2;

		static LLColor4 cyan1;
		static LLColor4 cyan2;
		static LLColor4 cyan3;
		static LLColor4 cyan4;
		static LLColor4 cyan5;
		static LLColor4 cyan6;
	
		static BOOL parseColor(const std::string& buf, LLColor4* color);
		static BOOL parseColor4(const std::string& buf, LLColor4* color);

		inline void clamp();
};


// Non-member functions 
F32		distVec(const LLColor4 &a, const LLColor4 &b);			// Returns distance between a and b
F32		distVec_squared(const LLColor4 &a, const LLColor4 &b);	// Returns distance squared between a and b
LLColor3	vec4to3(const LLColor4 &vec);
LLColor4	vec3to4(const LLColor3 &vec);
LLColor4 lerp(const LLColor4 &a, const LLColor4 &b, F32 u);

inline LLColor4::LLColor4(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}

inline LLColor4::LLColor4(const LLSD& sd)
{
	this->setValue(sd);
}

inline LLColor4::LLColor4(F32 r, F32 g, F32 b)
{
	mV[VX] = r;
	mV[VY] = g;
	mV[VZ] = b;
	mV[VW] = 1.f;
}

inline LLColor4::LLColor4(F32 r, F32 g, F32 b, F32 a)
{
	mV[VX] = r;
	mV[VY] = g;
	mV[VZ] = b;
	mV[VW] = a;
}

inline LLColor4::LLColor4(U32 clr)
{
	mV[VX] = (clr&0xff) * (1.0f/255.0f);
	mV[VY] = ((clr>>8)&0xff) * (1.0f/255.0f);
	mV[VZ] = ((clr>>16)&0xff) * (1.0f/255.0f);
	mV[VW] = (clr>>24) * (1.0f/255.0f);
}


inline LLColor4::LLColor4(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}

inline const LLColor4&	LLColor4::setToBlack(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
	return (*this);
}

inline const LLColor4&	LLColor4::setToWhite(void)
{
	mV[VX] = 1.f;
	mV[VY] = 1.f;
	mV[VZ] = 1.f;
	mV[VW] = 1.f;
	return (*this);
}

inline const LLColor4&	LLColor4::set(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;

//  no change to alpha!
//	mV[VW] = 1.f;  

	return (*this);
}

inline const LLColor4&	LLColor4::set(F32 x, F32 y, F32 z, F32 a)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = a;  
	return (*this);
}

inline const LLColor4&	LLColor4::set(const LLColor4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
	return (*this);
}


inline const LLColor4&	LLColor4::set(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
	return (*this);
}

// deprecated
inline const LLColor4&	LLColor4::setVec(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;

//  no change to alpha!
//	mV[VW] = 1.f;  

	return (*this);
}

// deprecated
inline const LLColor4&	LLColor4::setVec(F32 x, F32 y, F32 z, F32 a)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = a;  
	return (*this);
}

// deprecated
inline const LLColor4&	LLColor4::setVec(const LLColor4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
	return (*this);
}


// deprecated
inline const LLColor4&	LLColor4::setVec(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
	return (*this);
}

inline const LLColor4&	LLColor4::setAlpha(F32 a)
{
	mV[VW] = a;
	return (*this);
}

// LLColor4 Magnitude and Normalization Functions

inline F32		LLColor4::length(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}

inline F32		LLColor4::lengthSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}

inline F32		LLColor4::normalize(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;

	if (mag)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	return (mag);
}

// deprecated
inline F32		LLColor4::magVec(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}

// deprecated
inline F32		LLColor4::magVecSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}

// deprecated
inline F32		LLColor4::normVec(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;

	if (mag)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	return (mag);
}

// LLColor4 Operators


inline LLColor4 operator+(const LLColor4 &a, const LLColor4 &b)
{
	return LLColor4(
		a.mV[VX] + b.mV[VX],
		a.mV[VY] + b.mV[VY],
		a.mV[VZ] + b.mV[VZ],
		a.mV[VW] + b.mV[VW]);
}

inline LLColor4 operator-(const LLColor4 &a, const LLColor4 &b)
{
	return LLColor4(
		a.mV[VX] - b.mV[VX],
		a.mV[VY] - b.mV[VY],
		a.mV[VZ] - b.mV[VZ],
		a.mV[VW] - b.mV[VW]);
}

inline LLColor4  operator*(const LLColor4 &a, const LLColor4 &b)
{
	return LLColor4(
		a.mV[VX] * b.mV[VX],
		a.mV[VY] * b.mV[VY],
		a.mV[VZ] * b.mV[VZ],
		a.mV[VW] * b.mV[VW]);
}

inline LLColor4 operator*(const LLColor4 &a, F32 k)
{	
	// only affects rgb (not a!)
	return LLColor4(
		a.mV[VX] * k,
		a.mV[VY] * k,
		a.mV[VZ] * k,
		a.mV[VW]);
}

inline LLColor4 operator*(F32 k, const LLColor4 &a)
{
	// only affects rgb (not a!)
	return LLColor4(
		a.mV[VX] * k,
		a.mV[VY] * k,
		a.mV[VZ] * k,
		a.mV[VW]);
}

inline LLColor4 operator%(F32 k, const LLColor4 &a)
{
	// only affects alpha (not rgb!)
	return LLColor4(
		a.mV[VX],
		a.mV[VY],
		a.mV[VZ],
		a.mV[VW] * k);
}

inline LLColor4 operator%(const LLColor4 &a, F32 k)
{
	// only affects alpha (not rgb!)
	return LLColor4(
		a.mV[VX],
		a.mV[VY],
		a.mV[VZ],
		a.mV[VW] * k);
}

inline bool operator==(const LLColor4 &a, const LLColor4 &b)
{
	return (  (a.mV[VX] == b.mV[VX])
			&&(a.mV[VY] == b.mV[VY])
			&&(a.mV[VZ] == b.mV[VZ])
			&&(a.mV[VW] == b.mV[VW]));
}

inline bool operator!=(const LLColor4 &a, const LLColor4 &b)
{
	return (  (a.mV[VX] != b.mV[VX])
			||(a.mV[VY] != b.mV[VY])
			||(a.mV[VZ] != b.mV[VZ])
			||(a.mV[VW] != b.mV[VW]));
}

inline const LLColor4& operator+=(LLColor4 &a, const LLColor4 &b)
{
	a.mV[VX] += b.mV[VX];
	a.mV[VY] += b.mV[VY];
	a.mV[VZ] += b.mV[VZ];
	a.mV[VW] += b.mV[VW];
	return a;
}

inline const LLColor4& operator-=(LLColor4 &a, const LLColor4 &b)
{
	a.mV[VX] -= b.mV[VX];
	a.mV[VY] -= b.mV[VY];
	a.mV[VZ] -= b.mV[VZ];
	a.mV[VW] -= b.mV[VW];
	return a;
}

inline const LLColor4& operator*=(LLColor4 &a, F32 k)
{
	// only affects rgb (not a!)
	a.mV[VX] *= k;
	a.mV[VY] *= k;
	a.mV[VZ] *= k;
	return a;
}

inline const LLColor4& operator *=(LLColor4 &a, const LLColor4 &b)
{
	a.mV[VX] *= b.mV[VX];
	a.mV[VY] *= b.mV[VY];
	a.mV[VZ] *= b.mV[VZ];
//	a.mV[VW] *= b.mV[VW];
	return a;
}

inline const LLColor4& operator%=(LLColor4 &a, F32 k)
{
	// only affects alpha (not rgb!)
	a.mV[VW] *= k;
	return a;
}


// Non-member functions

inline F32		distVec(const LLColor4 &a, const LLColor4 &b)
{
	LLColor4 vec = a - b;
	return (vec.length());
}

inline F32		distVec_squared(const LLColor4 &a, const LLColor4 &b)
{
	LLColor4 vec = a - b;
	return (vec.lengthSquared());
}

inline LLColor4 lerp(const LLColor4 &a, const LLColor4 &b, F32 u)
{
	return LLColor4(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u,
		a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u,
		a.mV[VW] + (b.mV[VW] - a.mV[VW]) * u);
}

inline bool LLColor4::operator<(const LLColor4& rhs) const
{
	if (mV[0] != rhs.mV[0])
	{
		return mV[0] < rhs.mV[0];
	}
	if (mV[1] != rhs.mV[1])
	{
		return mV[1] < rhs.mV[1];
	}
	if (mV[2] != rhs.mV[2])
	{
		return mV[2] < rhs.mV[2];
	}

	return mV[3] < rhs.mV[3];
}

void LLColor4::clamp()
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
	if (mV[3] < 0.f)
	{
		mV[3] = 0.f;
	}
	else if (mV[3] > 1.f)
	{
		mV[3] = 1.f;
	}
}

#endif

