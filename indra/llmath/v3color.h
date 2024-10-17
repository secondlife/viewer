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
#include "v3math.h" // needed for linearColor3v implemtation below
#include <string.h>

//  LLColor3 = |r g b|

static constexpr U32 LENGTHOFCOLOR3 = 3;

class LLColor3
{
public:
    F32 mV[LENGTHOFCOLOR3];

    static LLColor3 white;
    static LLColor3 black;
    static LLColor3 grey;

public:
    LLColor3();                                  // Initializes LLColor3 to (0, 0, 0)
    LLColor3(F32 r, F32 g, F32 b);               // Initializes LLColor3 to (r, g, b)
    LLColor3(const F32* vec);                    // Initializes LLColor3 to (vec[0]. vec[1], vec[2])
    LLColor3(const char* color_string);          // html format color ie "#FFDDEE"
    explicit LLColor3(const LLColor4& color4);   // "explicit" to avoid automatic conversion
    explicit LLColor3(const LLVector4& vector4); // "explicit" to avoid automatic conversion
    LLColor3(const LLSD& sd);

    LLSD getValue() const
    {
        LLSD ret;
        ret[0] = mV[VRED];
        ret[1] = mV[VGREEN];
        ret[2] = mV[VBLUE];
        return ret;
    }

    void setValue(const LLSD& sd)
    {
        mV[VRED]   = (F32)sd[0].asReal();
        mV[VGREEN] = (F32)sd[1].asReal();
        mV[VBLUE]  = (F32)sd[2].asReal();
    }

    void setHSL(F32 hue, F32 saturation, F32 luminance);
    void calcHSL(F32* hue, F32* saturation, F32* luminance) const;

    const LLColor3& setToBlack(); // Clears LLColor3 to (0, 0, 0)
    const LLColor3& setToWhite(); // Zero LLColor3 to (0, 0, 0)

    const LLColor3& setVec(F32 x, F32 y, F32 z); // deprecated
    const LLColor3& setVec(const LLColor3& vec); // deprecated
    const LLColor3& setVec(const F32* vec);      // deprecated

    const LLColor3& set(F32 x, F32 y, F32 z); // Sets LLColor3 to (x, y, z)
    const LLColor3& set(const LLColor3& vec); // Sets LLColor3 to vec
    const LLColor3& set(const F32* vec);      // Sets LLColor3 to vec

    // set from a vector of unknown type and size
    // may leave some data unmodified
    template<typename T>
    const LLColor3& set(const std::vector<T>& v);

    // write to a vector of unknown type and size
    // maye leave some data unmodified
    template<typename T>
    void write(std::vector<T>& v) const;

    F32 magVec() const;        // deprecated
    F32 magVecSquared() const; // deprecated
    F32 normVec();             // deprecated

    F32 length() const;        // Returns magnitude of LLColor3
    F32 lengthSquared() const; // Returns magnitude squared of LLColor3
    F32 normalize();           // Normalizes and returns the magnitude of LLColor3

    F32 brightness() const; // Returns brightness of LLColor3

    const LLColor3& operator=(const LLColor4& a);

    LL_FORCE_INLINE LLColor3 divide(const LLColor3& col2) const
    {
        return LLColor3(mV[VRED] / col2.mV[VRED], mV[VGREEN] / col2.mV[VGREEN], mV[VBLUE] / col2.mV[VBLUE]);
    }

    LL_FORCE_INLINE LLColor3 color_norm() const
    {
        F32 l = length();
        return LLColor3(mV[VRED] / l, mV[VGREEN] / l, mV[VBLUE] / l);
    }

    friend std::ostream& operator<<(std::ostream& s, const LLColor3& a);  // Print a
    friend LLColor3      operator+(const LLColor3& a, const LLColor3& b); // Return vector a + b
    friend LLColor3      operator-(const LLColor3& a, const LLColor3& b); // Return vector a minus b

    friend const LLColor3& operator+=(LLColor3& a, const LLColor3& b); // Return vector a + b
    friend const LLColor3& operator-=(LLColor3& a, const LLColor3& b); // Return vector a minus b
    friend const LLColor3& operator*=(LLColor3& a, const LLColor3& b);

    friend LLColor3 operator*(const LLColor3& a, const LLColor3& b); // Return component wise a * b
    friend LLColor3 operator*(const LLColor3& a, F32 k);             // Return a times scaler k
    friend LLColor3 operator*(F32 k, const LLColor3& a);             // Return a times scaler k

    friend bool operator==(const LLColor3& a, const LLColor3& b); // Return a == b
    friend bool operator!=(const LLColor3& a, const LLColor3& b); // Return a != b

    friend const LLColor3& operator*=(LLColor3& a, F32 k); // Return a times scaler k

    friend LLColor3 operator-(const LLColor3& a); // Return vector 1-rgb (inverse)

    inline void clamp();
    inline void exp(); // Do an exponential on the color
};

LLColor3 lerp(const LLColor3& a, const LLColor3& b, F32 u);

void LLColor3::clamp()
{
    // Clamp the color...
    if (mV[VRED] < 0.f)
    {
        mV[VRED] = 0.f;
    }
    else if (mV[VRED] > 1.f)
    {
        mV[VRED] = 1.f;
    }
    if (mV[VGREEN] < 0.f)
    {
        mV[VGREEN] = 0.f;
    }
    else if (mV[VGREEN] > 1.f)
    {
        mV[VGREEN] = 1.f;
    }
    if (mV[VBLUE] < 0.f)
    {
        mV[VBLUE] = 0.f;
    }
    else if (mV[VBLUE] > 1.f)
    {
        mV[VBLUE] = 1.f;
    }
}

// Non-member functions
F32 distVec(const LLColor3& a, const LLColor3& b);         // Returns distance between a and b
F32 distVec_squared(const LLColor3& a, const LLColor3& b); // Returns distance squared between a and b

inline LLColor3::LLColor3()
{
    mV[VRED]   = 0.f;
    mV[VGREEN] = 0.f;
    mV[VBLUE]  = 0.f;
}

inline LLColor3::LLColor3(F32 r, F32 g, F32 b)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
}

inline LLColor3::LLColor3(const F32* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
}

inline LLColor3::LLColor3(const char* color_string) // takes a string of format "RRGGBB" where RR is hex 00..FF
{
    if (strlen(color_string) < 6) /* Flawfinder: ignore */
    {
        mV[VRED]   = 0.f;
        mV[VGREEN] = 0.f;
        mV[VBLUE]  = 0.f;
        return;
    }

    char tempstr[7];
    strncpy(tempstr, color_string, 6); /* Flawfinder: ignore */
    tempstr[6] = '\0';
    mV[VBLUE]  = (F32)strtol(&tempstr[4], nullptr, 16) / 255.f;
    tempstr[4] = '\0';
    mV[VGREEN] = (F32)strtol(&tempstr[2], nullptr, 16) / 255.f;
    tempstr[2] = '\0';
    mV[VRED]   = (F32)strtol(&tempstr[0], nullptr, 16) / 255.f;
}

inline const LLColor3& LLColor3::setToBlack()
{
    mV[VRED]   = 0.f;
    mV[VGREEN] = 0.f;
    mV[VBLUE]  = 0.f;
    return (*this);
}

inline const LLColor3& LLColor3::setToWhite()
{
    mV[VRED]   = 1.f;
    mV[VGREEN] = 1.f;
    mV[VBLUE]  = 1.f;
    return (*this);
}

inline const LLColor3& LLColor3::set(F32 r, F32 g, F32 b)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    return (*this);
}

inline const LLColor3& LLColor3::set(const LLColor3& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    return (*this);
}

inline const LLColor3& LLColor3::set(const F32* vec)
{
    mV[VRED]   = vec[0];
    mV[VGREEN] = vec[1];
    mV[VBLUE]  = vec[2];
    return (*this);
}

// deprecated
inline const LLColor3& LLColor3::setVec(F32 r, F32 g, F32 b)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    return (*this);
}

// deprecated
inline const LLColor3& LLColor3::setVec(const LLColor3& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    return (*this);
}

// deprecated
inline const LLColor3& LLColor3::setVec(const F32* vec)
{
    mV[VRED]   = vec[0];
    mV[VGREEN] = vec[1];
    mV[VBLUE]  = vec[2];
    return (*this);
}

inline F32 LLColor3::brightness() const
{
    return (mV[VRED] + mV[VGREEN] + mV[VBLUE]) / 3.0f;
}

inline F32 LLColor3::length() const
{
    return sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
}

inline F32 LLColor3::lengthSquared() const
{
    return mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE];
}

inline F32 LLColor3::normalize()
{
    F32 mag = sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
    F32 oomag;

    if (mag)
    {
        oomag = 1.f / mag;
        mV[VRED] *= oomag;
        mV[VGREEN] *= oomag;
        mV[VBLUE] *= oomag;
    }
    return mag;
}

// deprecated
inline F32 LLColor3::magVec() const
{
    return sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
}

// deprecated
inline F32 LLColor3::magVecSquared() const
{
    return mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE];
}

// deprecated
inline F32 LLColor3::normVec()
{
    F32 mag = sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
    F32 oomag;

    if (mag)
    {
        oomag = 1.f / mag;
        mV[VRED] *= oomag;
        mV[VGREEN] *= oomag;
        mV[VBLUE] *= oomag;
    }
    return mag;
}

inline void LLColor3::exp()
{
#if 0
    mV[VRED] = ::exp(mV[VRED]);
    mV[VGREEN] = ::exp(mV[VGREEN]);
    mV[VBLUE] = ::exp(mV[VBLUE]);
#else
    mV[VRED]   = (F32)LL_FAST_EXP(mV[VRED]);
    mV[VGREEN] = (F32)LL_FAST_EXP(mV[VGREEN]);
    mV[VBLUE]  = (F32)LL_FAST_EXP(mV[VBLUE]);
#endif
}

inline LLColor3 operator+(const LLColor3& a, const LLColor3& b)
{
    return LLColor3(a.mV[VRED] + b.mV[VRED], a.mV[VGREEN] + b.mV[VGREEN], a.mV[VBLUE] + b.mV[VBLUE]);
}

inline LLColor3 operator-(const LLColor3& a, const LLColor3& b)
{
    return LLColor3(a.mV[VRED] - b.mV[VRED], a.mV[VGREEN] - b.mV[VGREEN], a.mV[VBLUE] - b.mV[VBLUE]);
}

inline LLColor3 operator*(const LLColor3& a, const LLColor3& b)
{
    return LLColor3(a.mV[VRED] * b.mV[VRED], a.mV[VGREEN] * b.mV[VGREEN], a.mV[VBLUE] * b.mV[VBLUE]);
}

inline LLColor3 operator*(const LLColor3& a, F32 k)
{
    return LLColor3(a.mV[VRED] * k, a.mV[VGREEN] * k, a.mV[VBLUE] * k);
}

inline LLColor3 operator*(F32 k, const LLColor3& a)
{
    return LLColor3(a.mV[VRED] * k, a.mV[VGREEN] * k, a.mV[VBLUE] * k);
}

inline bool operator==(const LLColor3& a, const LLColor3& b)
{
    return ((a.mV[VRED] == b.mV[VRED]) && (a.mV[VGREEN] == b.mV[VGREEN]) && (a.mV[VBLUE] == b.mV[VBLUE]));
}

inline bool operator!=(const LLColor3& a, const LLColor3& b)
{
    return ((a.mV[VRED] != b.mV[VRED]) || (a.mV[VGREEN] != b.mV[VGREEN]) || (a.mV[VBLUE] != b.mV[VBLUE]));
}

inline const LLColor3& operator*=(LLColor3& a, const LLColor3& b)
{
    a.mV[VRED] *= b.mV[VRED];
    a.mV[VGREEN] *= b.mV[VGREEN];
    a.mV[VBLUE] *= b.mV[VBLUE];
    return a;
}

inline const LLColor3& operator+=(LLColor3& a, const LLColor3& b)
{
    a.mV[VRED] += b.mV[VRED];
    a.mV[VGREEN] += b.mV[VGREEN];
    a.mV[VBLUE] += b.mV[VBLUE];
    return a;
}

inline const LLColor3& operator-=(LLColor3& a, const LLColor3& b)
{
    a.mV[VRED] -= b.mV[VRED];
    a.mV[VGREEN] -= b.mV[VGREEN];
    a.mV[VBLUE] -= b.mV[VBLUE];
    return a;
}

inline const LLColor3& operator*=(LLColor3& a, F32 k)
{
    a.mV[VRED] *= k;
    a.mV[VGREEN] *= k;
    a.mV[VBLUE] *= k;
    return a;
}

inline LLColor3 operator-(const LLColor3& a)
{
    return LLColor3(1.f - a.mV[VRED], 1.f - a.mV[VGREEN], 1.f - a.mV[VBLUE]);
}

// Non-member functions

inline F32 distVec(const LLColor3& a, const LLColor3& b)
{
    F32 x = a.mV[VRED] - b.mV[VRED];
    F32 y = a.mV[VGREEN] - b.mV[VGREEN];
    F32 z = a.mV[VBLUE] - b.mV[VBLUE];
    return sqrt(x * x + y * y + z * z);
}

inline F32 distVec_squared(const LLColor3& a, const LLColor3& b)
{
    F32 x = a.mV[VRED] - b.mV[VRED];
    F32 y = a.mV[VGREEN] - b.mV[VGREEN];
    F32 z = a.mV[VBLUE] - b.mV[VBLUE];
    return x * x + y * y + z * z;
}

inline LLColor3 lerp(const LLColor3& a, const LLColor3& b, F32 u)
{
    return LLColor3(a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u, a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u, a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u);
}

inline const LLColor3 srgbColor3(const LLColor3& a)
{
    LLColor3 srgbColor;
    srgbColor.mV[VRED]   = linearTosRGB(a.mV[VRED]);
    srgbColor.mV[VGREEN] = linearTosRGB(a.mV[VGREEN]);
    srgbColor.mV[VBLUE]  = linearTosRGB(a.mV[VBLUE]);

    return srgbColor;
}

inline const LLColor3 linearColor3p(const F32* v)
{
    LLColor3 linearColor;
    linearColor.mV[VRED]   = sRGBtoLinear(v[0]);
    linearColor.mV[VGREEN] = sRGBtoLinear(v[1]);
    linearColor.mV[VBLUE]  = sRGBtoLinear(v[2]);

    return linearColor;
}

template<class T>
inline const LLColor3 linearColor3(const T& a)
{
    return linearColor3p(a.mV);
}

template<class T>
inline const LLVector3 linearColor3v(const T& a)
{
    return LLVector3(linearColor3p(a.mV).mV);
}

template<typename T>
const LLColor3& LLColor3::set(const std::vector<T>& v)
{
    for (size_t i = 0; i < llmin(v.size(), 3); ++i)
    {
        mV[i] = (F32)v[i];
    }

    return *this;
}

// write to a vector of unknown type and size
// maye leave some data unmodified
template<typename T>
void LLColor3::write(std::vector<T>& v) const
{
    for (size_t i = 0; i < llmin(v.size(), 3); ++i)
    {
        v[i] = (T)mV[i];
    }
}

#endif
