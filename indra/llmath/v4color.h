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
#include "llmath.h"
#include "llsd.h"

class LLColor3;
class LLColor4U;
class LLVector4;

//  LLColor4 = |x y z w|

static constexpr U32 LENGTHOFCOLOR4 = 4;

static constexpr U32 MAX_LENGTH_OF_COLOR_NAME = 15; // Give plenty of room for additional colors...

class LLColor4
{
public:
    F32 mV[LENGTHOFCOLOR4];
    LLColor4();                                 // Initializes LLColor4 to (0, 0, 0, 1)
    LLColor4(F32 r, F32 g, F32 b);              // Initializes LLColor4 to (r, g, b, 1)
    LLColor4(F32 r, F32 g, F32 b, F32 a);       // Initializes LLColor4 to (r. g, b, a)
    LLColor4(const LLColor3& vec, F32 a = 1.f); // Initializes LLColor4 to (vec, a)
    explicit LLColor4(const LLSD& sd);
    explicit LLColor4(const F32* vec);           // Initializes LLColor4 to (vec[0]. vec[1], vec[2], 1)
    explicit LLColor4(U32 clr);                  // Initializes LLColor4 to (r=clr>>24, etc))
    explicit LLColor4(const LLColor4U& color4u); // "explicit" to avoid automatic conversion
    explicit LLColor4(const LLVector4& vector4); // "explicit" to avoid automatic conversion

    LLSD getValue() const
    {
        LLSD ret;
        ret[0] = mV[VRED];
        ret[1] = mV[VGREEN];
        ret[2] = mV[VBLUE];
        ret[3] = mV[VALPHA];
        return ret;
    }

    void setValue(const LLSD& sd);

    void setHSL(F32 hue, F32 saturation, F32 luminance);
    void calcHSL(F32* hue, F32* saturation, F32* luminance) const;

    const LLColor4& setToBlack(); // zero LLColor4 to (0, 0, 0, 1)
    const LLColor4& setToWhite(); // zero LLColor4 to (0, 0, 0, 1)

    const LLColor4& setVec(F32 r, F32 g, F32 b, F32 a); // deprecated -- use set()
    const LLColor4& setVec(F32 r, F32 g, F32 b);        // deprecated -- use set()
    const LLColor4& setVec(const LLColor4& vec);        // deprecated -- use set()
    const LLColor4& setVec(const LLColor3& vec);        // deprecated -- use set()
    const LLColor4& setVec(const LLColor3& vec, F32 a); // deprecated -- use set()
    const LLColor4& setVec(const F32* vec);             // deprecated -- use set()
    const LLColor4& setVec(const LLColor4U& color4u);   // deprecated -- use set()

    const LLColor4& set(F32 r, F32 g, F32 b, F32 a); // Sets LLColor4 to (r, g, b, a)
    const LLColor4& set(F32 r, F32 g, F32 b);        // Sets LLColor4 to (r, g, b) (no change in a)
    const LLColor4& set(const LLColor4& vec);        // Sets LLColor4 to vec
    const LLColor4& set(const LLColor3& vec);        // Sets LLColor4 to LLColor3 vec (no change in alpha)
    const LLColor4& set(const LLColor3& vec, F32 a); // Sets LLColor4 to LLColor3 vec, with alpha specified
    const LLColor4& set(const F32* vec);             // Sets LLColor4 to vec
    const LLColor4& set(const F64* vec);             // Sets LLColor4 to (double)vec
    const LLColor4& set(const LLColor4U& color4u);   // Sets LLColor4 to color4u, rescaled.

    // set from a vector of unknown type and size
    // may leave some data unmodified
    template<typename T>
    const LLColor4& set(const std::vector<T>& v);

    // write to a vector of unknown type and size
    // maye leave some data unmodified
    template<typename T>
    void write(std::vector<T>& v) const;

    const LLColor4& setAlpha(F32 a);

    F32 magVec() const;        // deprecated -- use length()
    F32 magVecSquared() const; // deprecated -- use lengthSquared()
    F32 normVec();             // deprecated -- use normalize()

    F32 length() const;        // Returns magnitude of LLColor4
    F32 lengthSquared() const; // Returns magnitude squared of LLColor4
    F32 normalize();           // deprecated -- use normalize()

    bool isOpaque() const { return mV[VALPHA] == 1.f; }

    F32  operator[](int idx) const { return mV[idx]; }
    F32& operator[](int idx) { return mV[idx]; }

    const LLColor4& operator=(const LLColor3& a); // Assigns vec3 to vec4 and returns vec4

    bool                 operator<(const LLColor4& rhs) const;
    friend std::ostream& operator<<(std::ostream& s, const LLColor4& a);  // Print a
    friend LLColor4      operator+(const LLColor4& a, const LLColor4& b); // Return vector a + b
    friend LLColor4      operator-(const LLColor4& a, const LLColor4& b); // Return vector a minus b
    friend LLColor4      operator*(const LLColor4& a, const LLColor4& b); // Return component wise a * b
    friend LLColor4      operator*(const LLColor4& a, F32 k);             // Return rgb times scaler k (no alpha change)
    friend LLColor4      operator/(const LLColor4& a, F32 k);             // Return rgb divided by scalar k (no alpha change)
    friend LLColor4      operator*(F32 k, const LLColor4& a);             // Return rgb times scaler k (no alpha change)
    friend LLColor4      operator%(const LLColor4& a, F32 k);             // Return alpha times scaler k (no rgb change)
    friend LLColor4      operator%(F32 k, const LLColor4& a);             // Return alpha times scaler k (no rgb change)

    friend bool operator==(const LLColor4& a, const LLColor4& b); // Return a == b
    friend bool operator!=(const LLColor4& a, const LLColor4& b); // Return a != b

    friend bool operator==(const LLColor4& a, const LLColor3& b); // Return a == b
    friend bool operator!=(const LLColor4& a, const LLColor3& b); // Return a != b

    friend const LLColor4& operator+=(LLColor4& a, const LLColor4& b); // Return vector a + b
    friend const LLColor4& operator-=(LLColor4& a, const LLColor4& b); // Return vector a minus b
    friend const LLColor4& operator*=(LLColor4& a, F32 k);             // Return rgb times scaler k (no alpha change)
    friend const LLColor4& operator%=(LLColor4& a, F32 k);             // Return alpha times scaler k (no rgb change)

    friend const LLColor4& operator*=(LLColor4& a, const LLColor4& b); // Doesn't multiply alpha! (for lighting)

    // conversion
    operator LLColor4U() const;

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

    static bool parseColor(const std::string& buf, LLColor4* color);
    static bool parseColor4(const std::string& buf, LLColor4* color);

    inline void clamp();
};

// Non-member functions
F32      distVec(const LLColor4& a, const LLColor4& b);         // Returns distance between a and b
F32      distVec_squared(const LLColor4& a, const LLColor4& b); // Returns distance squared between a and b
LLColor3 vec4to3(const LLColor4& vec);
LLColor4 vec3to4(const LLColor3& vec);
LLColor4 lerp(const LLColor4& a, const LLColor4& b, F32 u);

inline LLColor4::LLColor4()
{
    mV[VRED]   = 0.f;
    mV[VGREEN] = 0.f;
    mV[VBLUE]  = 0.f;
    mV[VALPHA] = 1.f;
}

inline LLColor4::LLColor4(const LLSD& sd)
{
    this->setValue(sd);
}

inline LLColor4::LLColor4(F32 r, F32 g, F32 b)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = 1.f;
}

inline LLColor4::LLColor4(F32 r, F32 g, F32 b, F32 a)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = a;
}

inline LLColor4::LLColor4(U32 clr)
{
    mV[VRED]   = (clr & 0xff) * (1.0f / 255.0f);
    mV[VGREEN] = ((clr >> 8) & 0xff) * (1.0f / 255.0f);
    mV[VBLUE]  = ((clr >> 16) & 0xff) * (1.0f / 255.0f);
    mV[VALPHA] = (clr >> 24) * (1.0f / 255.0f);
}

inline LLColor4::LLColor4(const F32* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
}

inline const LLColor4& LLColor4::setToBlack(void)
{
    mV[VRED]   = 0.f;
    mV[VGREEN] = 0.f;
    mV[VBLUE]  = 0.f;
    mV[VALPHA] = 1.f;
    return (*this);
}

inline const LLColor4& LLColor4::setToWhite(void)
{
    mV[VRED]   = 1.f;
    mV[VGREEN] = 1.f;
    mV[VBLUE]  = 1.f;
    mV[VALPHA] = 1.f;
    return (*this);
}

inline const LLColor4& LLColor4::set(F32 x, F32 y, F32 z)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;

    //  no change to alpha!
    //  mV[VALPHA] = 1.f;

    return (*this);
}

inline const LLColor4& LLColor4::set(F32 x, F32 y, F32 z, F32 a)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;
    mV[VALPHA] = a;
    return (*this);
}

inline const LLColor4& LLColor4::set(const LLColor4& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    mV[VALPHA] = vec.mV[VALPHA];
    return (*this);
}

inline const LLColor4& LLColor4::set(const F32* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
    return (*this);
}

inline const LLColor4& LLColor4::set(const F64* vec)
{
    mV[VRED]   = static_cast<F32>(vec[VRED]);
    mV[VGREEN] = static_cast<F32>(vec[VGREEN]);
    mV[VBLUE]  = static_cast<F32>(vec[VBLUE]);
    mV[VALPHA] = static_cast<F32>(vec[VALPHA]);
    return (*this);
}

// deprecated
inline const LLColor4& LLColor4::setVec(F32 x, F32 y, F32 z)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;

    //  no change to alpha!
    //  mV[VALPHA] = 1.f;

    return (*this);
}

// deprecated
inline const LLColor4& LLColor4::setVec(F32 x, F32 y, F32 z, F32 a)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;
    mV[VALPHA] = a;
    return (*this);
}

// deprecated
inline const LLColor4& LLColor4::setVec(const LLColor4& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    mV[VALPHA] = vec.mV[VALPHA];
    return (*this);
}

// deprecated
inline const LLColor4& LLColor4::setVec(const F32* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
    return (*this);
}

inline const LLColor4& LLColor4::setAlpha(F32 a)
{
    mV[VALPHA] = a;
    return (*this);
}

// LLColor4 Magnitude and Normalization Functions

inline F32 LLColor4::length() const
{
    return sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
}

inline F32 LLColor4::lengthSquared() const
{
    return mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE];
}

inline F32 LLColor4::normalize()
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
inline F32 LLColor4::magVec() const
{
    return sqrt(mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE]);
}

// deprecated
inline F32 LLColor4::magVecSquared() const
{
    return mV[VRED] * mV[VRED] + mV[VGREEN] * mV[VGREEN] + mV[VBLUE] * mV[VBLUE];
}

// deprecated
inline F32 LLColor4::normVec()
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

// LLColor4 Operators

inline LLColor4 operator+(const LLColor4& a, const LLColor4& b)
{
    return LLColor4(a.mV[VRED] + b.mV[VRED], a.mV[VGREEN] + b.mV[VGREEN], a.mV[VBLUE] + b.mV[VBLUE], a.mV[VALPHA] + b.mV[VALPHA]);
}

inline LLColor4 operator-(const LLColor4& a, const LLColor4& b)
{
    return LLColor4(a.mV[VRED] - b.mV[VRED], a.mV[VGREEN] - b.mV[VGREEN], a.mV[VBLUE] - b.mV[VBLUE], a.mV[VALPHA] - b.mV[VALPHA]);
}

inline LLColor4 operator*(const LLColor4& a, const LLColor4& b)
{
    return LLColor4(a.mV[VRED] * b.mV[VRED], a.mV[VGREEN] * b.mV[VGREEN], a.mV[VBLUE] * b.mV[VBLUE], a.mV[VALPHA] * b.mV[VALPHA]);
}

inline LLColor4 operator*(const LLColor4& a, F32 k)
{
    // only affects rgb (not a!)
    return LLColor4(a.mV[VRED] * k, a.mV[VGREEN] * k, a.mV[VBLUE] * k, a.mV[VALPHA]);
}

inline LLColor4 operator/(const LLColor4& a, F32 k)
{
    return LLColor4(a.mV[VRED] / k, a.mV[VGREEN] / k, a.mV[VBLUE] / k, a.mV[VALPHA]);
}

inline LLColor4 operator*(F32 k, const LLColor4& a)
{
    // only affects rgb (not a!)
    return LLColor4(a.mV[VRED] * k, a.mV[VGREEN] * k, a.mV[VBLUE] * k, a.mV[VALPHA]);
}

inline LLColor4 operator%(F32 k, const LLColor4& a)
{
    // only affects alpha (not rgb!)
    return LLColor4(a.mV[VRED], a.mV[VGREEN], a.mV[VBLUE], a.mV[VALPHA] * k);
}

inline LLColor4 operator%(const LLColor4& a, F32 k)
{
    // only affects alpha (not rgb!)
    return LLColor4(a.mV[VRED], a.mV[VGREEN], a.mV[VBLUE], a.mV[VALPHA] * k);
}

inline bool operator==(const LLColor4& a, const LLColor4& b)
{
    return ((a.mV[VRED] == b.mV[VRED]) && (a.mV[VGREEN] == b.mV[VGREEN]) && (a.mV[VBLUE] == b.mV[VBLUE]) && (a.mV[VALPHA] == b.mV[VALPHA]));
}

inline bool operator!=(const LLColor4& a, const LLColor4& b)
{
    return ((a.mV[VRED] != b.mV[VRED]) || (a.mV[VGREEN] != b.mV[VGREEN]) || (a.mV[VBLUE] != b.mV[VBLUE]) || (a.mV[VALPHA] != b.mV[VALPHA]));
}

inline const LLColor4& operator+=(LLColor4& a, const LLColor4& b)
{
    a.mV[VRED] += b.mV[VRED];
    a.mV[VGREEN] += b.mV[VGREEN];
    a.mV[VBLUE] += b.mV[VBLUE];
    a.mV[VALPHA] += b.mV[VALPHA];
    return a;
}

inline const LLColor4& operator-=(LLColor4& a, const LLColor4& b)
{
    a.mV[VRED] -= b.mV[VRED];
    a.mV[VGREEN] -= b.mV[VGREEN];
    a.mV[VBLUE] -= b.mV[VBLUE];
    a.mV[VALPHA] -= b.mV[VALPHA];
    return a;
}

inline const LLColor4& operator*=(LLColor4& a, F32 k)
{
    // only affects rgb (not a!)
    a.mV[VRED] *= k;
    a.mV[VGREEN] *= k;
    a.mV[VBLUE] *= k;
    return a;
}

inline const LLColor4& operator*=(LLColor4& a, const LLColor4& b)
{
    a.mV[VRED] *= b.mV[VRED];
    a.mV[VGREEN] *= b.mV[VGREEN];
    a.mV[VBLUE] *= b.mV[VBLUE];
    //  a.mV[VALPHA] *= b.mV[VALPHA];
    return a;
}

inline const LLColor4& operator%=(LLColor4& a, F32 k)
{
    // only affects alpha (not rgb!)
    a.mV[VALPHA] *= k;
    return a;
}

// Non-member functions

inline F32 distVec(const LLColor4& a, const LLColor4& b)
{
    LLColor4 vec = a - b;
    return vec.length();
}

inline F32 distVec_squared(const LLColor4& a, const LLColor4& b)
{
    LLColor4 vec = a - b;
    return vec.lengthSquared();
}

inline LLColor4 lerp(const LLColor4& a, const LLColor4& b, F32 u)
{
    return LLColor4(a.mV[VRED] + (b.mV[VRED] - a.mV[VRED]) * u,
                    a.mV[VGREEN] + (b.mV[VGREEN] - a.mV[VGREEN]) * u,
                    a.mV[VBLUE] + (b.mV[VBLUE] - a.mV[VBLUE]) * u,
                    a.mV[VALPHA] + (b.mV[VALPHA] - a.mV[VALPHA]) * u);
}

inline bool LLColor4::operator<(const LLColor4& rhs) const
{
    if (mV[VRED] != rhs.mV[VRED])
    {
        return mV[VRED] < rhs.mV[VRED];
    }
    if (mV[VGREEN] != rhs.mV[VGREEN])
    {
        return mV[VGREEN] < rhs.mV[VGREEN];
    }
    if (mV[VBLUE] != rhs.mV[VBLUE])
    {
        return mV[VBLUE] < rhs.mV[VBLUE];
    }

    return mV[VALPHA] < rhs.mV[VALPHA];
}

void LLColor4::clamp()
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
    if (mV[VALPHA] < 0.f)
    {
        mV[VALPHA] = 0.f;
    }
    else if (mV[VALPHA] > 1.f)
    {
        mV[VALPHA] = 1.f;
    }
}

// Return the given linear space color value in gamma corrected (sRGB) space
inline const LLColor4 srgbColor4(const LLColor4& a)
{
    LLColor4 srgbColor;

    srgbColor.mV[VRED]   = linearTosRGB(a.mV[VRED]);
    srgbColor.mV[VGREEN] = linearTosRGB(a.mV[VGREEN]);
    srgbColor.mV[VBLUE]  = linearTosRGB(a.mV[VBLUE]);
    srgbColor.mV[VALPHA] = a.mV[VALPHA];

    return srgbColor;
}

// Return the given gamma corrected (sRGB) color in linear space
inline const LLColor4 linearColor4(const LLColor4& a)
{
    LLColor4 linearColor;
    linearColor.mV[VRED]   = sRGBtoLinear(a.mV[VRED]);
    linearColor.mV[VGREEN] = sRGBtoLinear(a.mV[VGREEN]);
    linearColor.mV[VBLUE]  = sRGBtoLinear(a.mV[VBLUE]);
    linearColor.mV[VALPHA] = a.mV[VALPHA];

    return linearColor;
}

template<typename T>
const LLColor4& LLColor4::set(const std::vector<T>& v)
{
    for (size_t i = 0; i < llmin(v.size(), 4); ++i)
    {
        mV[i] = (F32)v[i];
    }

    return *this;
}

template<typename T>
void LLColor4::write(std::vector<T>& v) const
{
    for (size_t i = 0; i < llmin(v.size(), 4); ++i)
    {
        v[i] = (T)mV[i];
    }
}

#endif
