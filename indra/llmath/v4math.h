/** 
 * @file v4math.h
 * @brief LLVector4 class header file.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_V4MATH_H
#define LL_V4MATH_H

#include "llerror.h"
#include "llmath.h"
#include "v3math.h"

class LLMatrix3;
class LLMatrix4;
class LLQuaternion;

//  LLVector4 = |x y z w|

static const U32 LENGTHOFVECTOR4 = 4;

class LLVector4
{
	public:
		F32 mV[LENGTHOFVECTOR4];
		LLVector4();						// Initializes LLVector4 to (0, 0, 0, 1)
		explicit LLVector4(const F32 *vec);			// Initializes LLVector4 to (vec[0]. vec[1], vec[2], vec[3])
		explicit LLVector4(const F64 *vec);			// Initialized LLVector4 to ((F32) vec[0], (F32) vec[1], (F32) vec[3], (F32) vec[4]);
		explicit LLVector4(const LLVector3 &vec);			// Initializes LLVector4 to (vec, 1)
		explicit LLVector4(const LLVector3 &vec, F32 w);	// Initializes LLVector4 to (vec, w)
		LLVector4(F32 x, F32 y, F32 z);		// Initializes LLVector4 to (x. y, z, 1)
		LLVector4(F32 x, F32 y, F32 z, F32 w);

		LLSD getValue() const
		{
			LLSD ret;
			ret[0] = mV[0];
			ret[1] = mV[1];
			ret[2] = mV[2];
			ret[3] = mV[3];
			return ret;
		}

		inline BOOL isFinite() const;									// checks to see if all values of LLVector3 are finite

		inline void	clear();		// Clears LLVector4 to (0, 0, 0, 1)
		inline void	clearVec();		// deprecated
		inline void	zeroVec();		// deprecated

		inline void	set(F32 x, F32 y, F32 z);			// Sets LLVector4 to (x, y, z, 1)
		inline void	set(F32 x, F32 y, F32 z, F32 w);	// Sets LLVector4 to (x, y, z, w)
		inline void	set(const LLVector4 &vec);			// Sets LLVector4 to vec
		inline void	set(const LLVector3 &vec, F32 w = 1.f); // Sets LLVector4 to LLVector3 vec
		inline void	set(const F32 *vec);				// Sets LLVector4 to vec

		inline void	setVec(F32 x, F32 y, F32 z);		// deprecated
		inline void	setVec(F32 x, F32 y, F32 z, F32 w);	// deprecated
		inline void	setVec(const LLVector4 &vec);		// deprecated
		inline void	setVec(const LLVector3 &vec, F32 w = 1.f); // deprecated
		inline void	setVec(const F32 *vec);				// deprecated

		F32	length() const;				// Returns magnitude of LLVector4
		F32	lengthSquared() const;		// Returns magnitude squared of LLVector4
		F32	normalize();				// Normalizes and returns the magnitude of LLVector4

		F32			magVec() const;				// deprecated
		F32			magVecSquared() const;		// deprecated
		F32			normVec();					// deprecated

		// Sets all values to absolute value of their original values
		// Returns TRUE if data changed
		BOOL abs();
		
		BOOL isExactlyClear() const		{ return (mV[VW] == 1.0f) && !mV[VX] && !mV[VY] && !mV[VZ]; }
		BOOL isExactlyZero() const		{ return !mV[VW] && !mV[VX] && !mV[VY] && !mV[VZ]; }

		const LLVector4&	rotVec(F32 angle, const LLVector4 &vec);	// Rotates about vec by angle radians
		const LLVector4&	rotVec(F32 angle, F32 x, F32 y, F32 z);		// Rotates about x,y,z by angle radians
		const LLVector4&	rotVec(const LLMatrix4 &mat);				// Rotates by MAT4 mat
		const LLVector4&	rotVec(const LLQuaternion &q);				// Rotates by QUAT q

		const LLVector4&	scaleVec(const LLVector4& vec);	// Scales component-wise by vec

		F32 operator[](int idx) const { return mV[idx]; }
		F32 &operator[](int idx) { return mV[idx]; }
	
		friend std::ostream&	 operator<<(std::ostream& s, const LLVector4 &a);		// Print a
		friend LLVector4 operator+(const LLVector4 &a, const LLVector4 &b);	// Return vector a + b
		friend LLVector4 operator-(const LLVector4 &a, const LLVector4 &b);	// Return vector a minus b
		friend F32  operator*(const LLVector4 &a, const LLVector4 &b);		// Return a dot b
		friend LLVector4 operator%(const LLVector4 &a, const LLVector4 &b);	// Return a cross b
		friend LLVector4 operator/(const LLVector4 &a, F32 k);				// Return a divided by scaler k
		friend LLVector4 operator*(const LLVector4 &a, F32 k);				// Return a times scaler k
		friend LLVector4 operator*(F32 k, const LLVector4 &a);				// Return a times scaler k
		friend bool operator==(const LLVector4 &a, const LLVector4 &b);		// Return a == b
		friend bool operator!=(const LLVector4 &a, const LLVector4 &b);		// Return a != b

		friend const LLVector4& operator+=(LLVector4 &a, const LLVector4 &b);	// Return vector a + b
		friend const LLVector4& operator-=(LLVector4 &a, const LLVector4 &b);	// Return vector a minus b
		friend const LLVector4& operator%=(LLVector4 &a, const LLVector4 &b);	// Return a cross b
		friend const LLVector4& operator*=(LLVector4 &a, F32 k);				// Return a times scaler k
		friend const LLVector4& operator/=(LLVector4 &a, F32 k);				// Return a divided by scaler k

		friend LLVector4 operator-(const LLVector4 &a);					// Return vector -a
};

// Non-member functions 
F32 angle_between(const LLVector4 &a, const LLVector4 &b);		// Returns angle (radians) between a and b
BOOL are_parallel(const LLVector4 &a, const LLVector4 &b, F32 epsilon=F_APPROXIMATELY_ZERO);		// Returns TRUE if a and b are very close to parallel
F32	dist_vec(const LLVector4 &a, const LLVector4 &b);			// Returns distance between a and b
F32	dist_vec_squared(const LLVector4 &a, const LLVector4 &b);	// Returns distance squared between a and b
LLVector3	vec4to3(const LLVector4 &vec);
LLVector4	vec3to4(const LLVector3 &vec);
LLVector4 lerp(const LLVector4 &a, const LLVector4 &b, F32 u); // Returns a vector that is a linear interpolation between a and b

// Constructors

inline LLVector4::LLVector4(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}

inline LLVector4::LLVector4(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}

inline LLVector4::LLVector4(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}

inline LLVector4::LLVector4(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}

inline LLVector4::LLVector4(const F64 *vec)
{
	mV[VX] = (F32) vec[VX];
	mV[VY] = (F32) vec[VY];
	mV[VZ] = (F32) vec[VZ];
	mV[VW] = (F32) vec[VW];
}

inline LLVector4::LLVector4(const LLVector3 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = 1.f;
}

inline LLVector4::LLVector4(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}


inline BOOL LLVector4::isFinite() const
{
	return (llfinite(mV[VX]) && llfinite(mV[VY]) && llfinite(mV[VZ]) && llfinite(mV[VW]));
}

// Clear and Assignment Functions

inline void	LLVector4::clear(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}

// deprecated
inline void	LLVector4::clearVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}

// deprecated
inline void	LLVector4::zeroVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 0.f;
}

inline void	LLVector4::set(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}

inline void	LLVector4::set(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}

inline void	LLVector4::set(const LLVector4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
}

inline void	LLVector4::set(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}

inline void	LLVector4::set(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}


// deprecated
inline void	LLVector4::setVec(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}

// deprecated
inline void	LLVector4::setVec(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}

// deprecated
inline void	LLVector4::setVec(const LLVector4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
}

// deprecated
inline void	LLVector4::setVec(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}

// deprecated
inline void	LLVector4::setVec(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}

// LLVector4 Magnitude and Normalization Functions

inline F32		LLVector4::length(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}

inline F32		LLVector4::lengthSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}

inline F32		LLVector4::magVec(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}

inline F32		LLVector4::magVecSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}

// LLVector4 Operators

inline LLVector4 operator+(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 c(a);
	return c += b;
}

inline LLVector4 operator-(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 c(a);
	return c -= b;
}

inline F32  operator*(const LLVector4 &a, const LLVector4 &b)
{
	return (a.mV[VX]*b.mV[VX] + a.mV[VY]*b.mV[VY] + a.mV[VZ]*b.mV[VZ]);
}

inline LLVector4 operator%(const LLVector4 &a, const LLVector4 &b)
{
	return LLVector4(a.mV[VY]*b.mV[VZ] - b.mV[VY]*a.mV[VZ], a.mV[VZ]*b.mV[VX] - b.mV[VZ]*a.mV[VX], a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY]);
}

inline LLVector4 operator/(const LLVector4 &a, F32 k)
{
	F32 t = 1.f / k;
	return LLVector4( a.mV[VX] * t, a.mV[VY] * t, a.mV[VZ] * t );
}


inline LLVector4 operator*(const LLVector4 &a, F32 k)
{
	return LLVector4( a.mV[VX] * k, a.mV[VY] * k, a.mV[VZ] * k );
}

inline LLVector4 operator*(F32 k, const LLVector4 &a)
{
	return LLVector4( a.mV[VX] * k, a.mV[VY] * k, a.mV[VZ] * k );
}

inline bool operator==(const LLVector4 &a, const LLVector4 &b)
{
	return (  (a.mV[VX] == b.mV[VX])
			&&(a.mV[VY] == b.mV[VY])
			&&(a.mV[VZ] == b.mV[VZ]));
}

inline bool operator!=(const LLVector4 &a, const LLVector4 &b)
{
	return (  (a.mV[VX] != b.mV[VX])
			||(a.mV[VY] != b.mV[VY])
			||(a.mV[VZ] != b.mV[VZ])
			||(a.mV[VW] != b.mV[VW]) );
}

inline const LLVector4& operator+=(LLVector4 &a, const LLVector4 &b)
{
	a.mV[VX] += b.mV[VX];
	a.mV[VY] += b.mV[VY];
	a.mV[VZ] += b.mV[VZ];
	return a;
}

inline const LLVector4& operator-=(LLVector4 &a, const LLVector4 &b)
{
	a.mV[VX] -= b.mV[VX];
	a.mV[VY] -= b.mV[VY];
	a.mV[VZ] -= b.mV[VZ];
	return a;
}

inline const LLVector4& operator%=(LLVector4 &a, const LLVector4 &b)
{
	LLVector4 ret(a.mV[VY]*b.mV[VZ] - b.mV[VY]*a.mV[VZ], a.mV[VZ]*b.mV[VX] - b.mV[VZ]*a.mV[VX], a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY]);
	a = ret;
	return a;
}

inline const LLVector4& operator*=(LLVector4 &a, F32 k)
{
	a.mV[VX] *= k;
	a.mV[VY] *= k;
	a.mV[VZ] *= k;
	return a;
}

inline const LLVector4& operator/=(LLVector4 &a, F32 k)
{
	F32 t = 1.f / k;
	a.mV[VX] *= t;
	a.mV[VY] *= t;
	a.mV[VZ] *= t;
	return a;
}

inline LLVector4 operator-(const LLVector4 &a)
{
	return LLVector4( -a.mV[VX], -a.mV[VY], -a.mV[VZ] );
}

inline F32	dist_vec(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 vec = a - b;
	return (vec.length());
}

inline F32	dist_vec_squared(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 vec = a - b;
	return (vec.lengthSquared());
}

inline LLVector4 lerp(const LLVector4 &a, const LLVector4 &b, F32 u)
{
	return LLVector4(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u,
		a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u,
		a.mV[VW] + (b.mV[VW] - a.mV[VW]) * u);
}

inline F32		LLVector4::normalize(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;

	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}

// deprecated
inline F32		LLVector4::normVec(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;

	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}


#endif

