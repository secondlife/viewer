/**
 * @file v2math.h
 * @brief LLVector2 class header file.
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

#ifndef LL_V2MATH_H
#define LL_V2MATH_H

#include "llmath.h"
#include "v3math.h"

class LLVector4;
class LLMatrix3;
class LLQuaternion;

//  Llvector2 = |x y z w|

static constexpr U32 LENGTHOFVECTOR2 = 2;

class LLVector2
{
    public:
        F32 mV[LENGTHOFVECTOR2];

        static LLVector2 zero;

        LLVector2();                              // Initializes LLVector2 to (0, 0)
        LLVector2(F32 x, F32 y);                  // Initializes LLVector2 to (x. y)
        LLVector2(const F32 *vec);                // Initializes LLVector2 to (vec[0]. vec[1])
        explicit LLVector2(const LLVector3 &vec); // Initializes LLVector2 to (vec[0]. vec[1])
        explicit LLVector2(const LLSD &sd);

        // Clears LLVector2 to (0, 0).  DEPRECATED - prefer zeroVec.
        void    clear();
        void    setZero();
        void    clearVec(); // deprecated
        void    zeroVec();  // deprecated

        void    set(F32 x, F32 y);          // Sets LLVector2 to (x, y)
        void    set(const LLVector2 &vec);  // Sets LLVector2 to vec
        void    set(const F32 *vec);            // Sets LLVector2 to vec

        LLSD    getValue() const;
        void    setValue(const LLSD& sd);

        void    setVec(F32 x, F32 y);           // deprecated
        void    setVec(const LLVector2 &vec);   // deprecated
        void    setVec(const F32 *vec);         // deprecated

        inline bool isFinite() const; // checks to see if all values of LLVector2 are finite

        F32     length() const;             // Returns magnitude of LLVector2
        F32     lengthSquared() const;      // Returns magnitude squared of LLVector2
        F32     normalize();                    // Normalizes and returns the magnitude of LLVector2

        F32     magVec() const;             // deprecated
        F32     magVecSquared() const;      // deprecated
        F32     normVec();                  // deprecated

        bool    abs();                      // sets all values to absolute value of original value (first octant), returns true if changed

        const LLVector2&    scaleVec(const LLVector2& vec);             // scales per component by vec

        bool isNull() const;          // Returns true if vector has a _very_small_ length
        bool isExactlyZero() const      { return !mV[VX] && !mV[VY]; }

        F32 operator[](int idx) const { return mV[idx]; }
        F32 &operator[](int idx) { return mV[idx]; }

        friend bool operator<(const LLVector2 &a, const LLVector2 &b);  // For sorting. x is "more significant" than y
        friend LLVector2 operator+(const LLVector2 &a, const LLVector2 &b); // Return vector a + b
        friend LLVector2 operator-(const LLVector2 &a, const LLVector2 &b); // Return vector a minus b
        friend F32 operator*(const LLVector2 &a, const LLVector2 &b);       // Return a dot b
        friend LLVector2 operator%(const LLVector2 &a, const LLVector2 &b); // Return a cross b
        friend LLVector2 operator/(const LLVector2 &a, F32 k);              // Return a divided by scaler k
        friend LLVector2 operator*(const LLVector2 &a, F32 k);              // Return a times scaler k
        friend LLVector2 operator*(F32 k, const LLVector2 &a);              // Return a times scaler k
        friend bool operator==(const LLVector2 &a, const LLVector2 &b);     // Return a == b
        friend bool operator!=(const LLVector2 &a, const LLVector2 &b);     // Return a != b

        friend const LLVector2& operator+=(LLVector2 &a, const LLVector2 &b);   // Return vector a + b
        friend const LLVector2& operator-=(LLVector2 &a, const LLVector2 &b);   // Return vector a minus b
        friend const LLVector2& operator%=(LLVector2 &a, const LLVector2 &b);   // Return a cross b
        friend const LLVector2& operator*=(LLVector2 &a, F32 k);                // Return a times scaler k
        friend const LLVector2& operator/=(LLVector2 &a, F32 k);                // Return a divided by scaler k

        friend LLVector2 operator-(const LLVector2 &a);                 // Return vector -a

        friend std::ostream& operator<<(std::ostream& s, const LLVector2 &a);       // Stream a
};


// Non-member functions

F32 angle_between(const LLVector2& a, const LLVector2& b);  // Returns angle (radians) between a and b
F32 signed_angle_between(const LLVector2& a, const LLVector2& b); // Returns signed angle (radians) between a and b
bool are_parallel(const LLVector2& a, const LLVector2& b, F32 epsilon = F_APPROXIMATELY_ZERO);    // Returns true if a and b are very close to parallel
F32 dist_vec(const LLVector2& a, const LLVector2& b);       // Returns distance between a and b
F32 dist_vec_squared(const LLVector2& a, const LLVector2& b);// Returns distance squared between a and b
F32 dist_vec_squared2D(const LLVector2& a, const LLVector2& b);// Returns distance squared between a and b ignoring Z component
LLVector2 lerp(const LLVector2& a, const LLVector2& b, F32 u); // Returns a vector that is a linear interpolation between a and b

// Constructors

inline LLVector2::LLVector2()
{
    clear();
}

inline LLVector2::LLVector2(F32 x, F32 y)
{
    set(x, y);
}

inline LLVector2::LLVector2(const F32 *vec)
{
    set(vec);
}

inline LLVector2::LLVector2(const LLVector3 &vec)
{
    set(vec.mV);
}

inline LLVector2::LLVector2(const LLSD &sd)
{
    setValue(sd);
}

// Clear and Assignment Functions

inline void LLVector2::clear()
{
    mV[VX] = mV[VY] = 0.f;
}

inline void LLVector2::setZero()
{
    clear();
}

// deprecated
inline void LLVector2::clearVec()
{
    clear();
}

// deprecated
inline void LLVector2::zeroVec()
{
    clear();
}

inline void LLVector2::set(F32 x, F32 y)
{
    mV[VX] = x;
    mV[VY] = y;
}

inline void LLVector2::set(const LLVector2 &vec)
{
    set(vec.mV);
}

inline void LLVector2::set(const F32 *vec)
{
    set(vec[VX], vec[VY]);
}


// deprecated
inline void LLVector2::setVec(F32 x, F32 y)
{
    set(x, y);
}

// deprecated
inline void LLVector2::setVec(const LLVector2 &vec)
{
    set(vec);
}

// deprecated
inline void LLVector2::setVec(const F32 *vec)
{
    set(vec);
}


// LLVector2 Magnitude and Normalization Functions

inline F32 LLVector2::length() const
{
    return sqrt(lengthSquared());
}

inline F32 LLVector2::lengthSquared() const
{
    return mV[VX]*mV[VX] + mV[VY]*mV[VY];
}

inline F32 LLVector2::normalize()
{
    F32 mag = length();

    if (mag > FP_MAG_THRESHOLD)
    {
        *this /= mag;
    }
    else
    {
        clear();
        mag = 0;
    }
    return mag;
}

// checker
inline bool LLVector2::isFinite() const
{
    return llfinite(mV[VX]) && llfinite(mV[VY]);
}

// deprecated
inline F32 LLVector2::magVec() const
{
    return length();
}

// deprecated
inline F32 LLVector2::magVecSquared() const
{
    return lengthSquared();
}

// deprecated
inline F32 LLVector2::normVec()
{
    return normalize();
}

inline const LLVector2& LLVector2::scaleVec(const LLVector2& vec)
{
    mV[VX] *= vec.mV[VX];
    mV[VY] *= vec.mV[VY];

    return *this;
}

inline bool LLVector2::isNull() const
{
    return F_APPROXIMATELY_ZERO > mV[VX]*mV[VX] + mV[VY]*mV[VY];
}


// LLVector2 Operators

// For sorting. By convention, x is "more significant" than y.
inline bool operator<(const LLVector2& a, const LLVector2& b)
{
    if (a.mV[VX] == b.mV[VX])
    {
        return a.mV[VY] < b.mV[VY];
    }
    else
    {
        return a.mV[VX] < b.mV[VX];
    }
}


inline LLVector2 operator+(const LLVector2& a, const LLVector2& b)
{
    LLVector2 c(a);
    return c += b;
}

inline LLVector2 operator-(const LLVector2& a, const LLVector2& b)
{
    LLVector2 c(a);
    return c -= b;
}

inline F32 operator*(const LLVector2& a, const LLVector2& b)
{
    return (a.mV[VX]*b.mV[VX] + a.mV[VY]*b.mV[VY]);
}

inline LLVector2 operator%(const LLVector2& a, const LLVector2& b)
{
    return LLVector2(a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY], a.mV[VY]*b.mV[VX] - b.mV[VY]*a.mV[VX]);
}

inline LLVector2 operator/(const LLVector2& a, F32 k)
{
    F32 t = 1.f / k;
    return LLVector2( a.mV[VX] * t, a.mV[VY] * t );
}

inline LLVector2 operator*(const LLVector2& a, F32 k)
{
    return LLVector2( a.mV[VX] * k, a.mV[VY] * k );
}

inline LLVector2 operator*(F32 k, const LLVector2& a)
{
    return LLVector2( a.mV[VX] * k, a.mV[VY] * k );
}

inline bool operator==(const LLVector2& a, const LLVector2& b)
{
    return (  (a.mV[VX] == b.mV[VX])
            &&(a.mV[VY] == b.mV[VY]));
}

inline bool operator!=(const LLVector2& a, const LLVector2& b)
{
    return (  (a.mV[VX] != b.mV[VX])
            ||(a.mV[VY] != b.mV[VY]));
}

inline const LLVector2& operator+=(LLVector2& a, const LLVector2& b)
{
    a.mV[VX] += b.mV[VX];
    a.mV[VY] += b.mV[VY];
    return a;
}

inline const LLVector2& operator-=(LLVector2& a, const LLVector2& b)
{
    a.mV[VX] -= b.mV[VX];
    a.mV[VY] -= b.mV[VY];
    return a;
}

inline const LLVector2& operator%=(LLVector2& a, const LLVector2& b)
{
    LLVector2 ret(a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY], a.mV[VY]*b.mV[VX] - b.mV[VY]*a.mV[VX]);
    a = ret;
    return a;
}

inline const LLVector2& operator*=(LLVector2& a, F32 k)
{
    a.mV[VX] *= k;
    a.mV[VY] *= k;
    return a;
}

inline const LLVector2& operator/=(LLVector2& a, F32 k)
{
    return a *= 1.f / k;
}

inline LLVector2 operator-(const LLVector2& a)
{
    return LLVector2( -a.mV[VX], -a.mV[VY] );
}

inline void update_min_max(LLVector2& min, LLVector2& max, const LLVector2& pos)
{
    for (U32 i = 0; i < 2; i++)
    {
        if (min.mV[i] > pos.mV[i])
        {
            min.mV[i] = pos.mV[i];
        }
        if (max.mV[i] < pos.mV[i])
        {
            max.mV[i] = pos.mV[i];
        }
    }
}

inline std::ostream& operator<<(std::ostream& s, const LLVector2& a)
{
    s << "{ " << a.mV[VX] << ", " << a.mV[VY] << " }";
    return s;
}

#endif
