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
#include "llmath.h"

#include "v3color.h"
#include "v4color.h"

class LLColor4;

//  LLColor4U = | red green blue alpha |

static constexpr U32 LENGTHOFCOLOR4U = 4;

class LLColor4U
{
public:
    U8 mV[LENGTHOFCOLOR4U];

    LLColor4U();                       // Initializes LLColor4U to (0, 0, 0, 1)
    LLColor4U(U8 r, U8 g, U8 b);       // Initializes LLColor4U to (r, g, b, 1)
    LLColor4U(U8 r, U8 g, U8 b, U8 a); // Initializes LLColor4U to (r. g, b, a)
    LLColor4U(const U8* vec);          // Initializes LLColor4U to (vec[0]. vec[1], vec[2], 1)
    explicit LLColor4U(const LLSD& sd) { setValue(sd); }

    void setValue(const LLSD& sd)
    {
        mV[VRED]   = sd[VRED].asInteger();
        mV[VGREEN] = sd[VGREEN].asInteger();
        mV[VBLUE]  = sd[VBLUE].asInteger();
        mV[VALPHA] = sd[VALPHA].asInteger();
    }

    LLSD getValue() const
    {
        LLSD ret;
        ret[VRED]   = mV[VRED];
        ret[VGREEN] = mV[VGREEN];
        ret[VBLUE]  = mV[VBLUE];
        ret[VALPHA] = mV[VALPHA];
        return ret;
    }

    const LLColor4U& setToBlack(); // zero LLColor4U to (0, 0, 0, 1)
    const LLColor4U& setToWhite(); // zero LLColor4U to (0, 0, 0, 1)

    const LLColor4U& set(U8 r, U8 g, U8 b, U8 a); // Sets LLColor4U to (r, g, b, a)
    const LLColor4U& set(U8 r, U8 g, U8 b);       // Sets LLColor4U to (r, g, b) (no change in a)
    const LLColor4U& set(const LLColor4U& vec);   // Sets LLColor4U to vec
    const LLColor4U& set(const U8* vec);          // Sets LLColor4U to vec

    const LLColor4U& setVec(U8 r, U8 g, U8 b, U8 a); // deprecated -- use set()
    const LLColor4U& setVec(U8 r, U8 g, U8 b);       // deprecated -- use set()
    const LLColor4U& setVec(const LLColor4U& vec);   // deprecated -- use set()
    const LLColor4U& setVec(const U8* vec);          // deprecated -- use set()

    const LLColor4U& setAlpha(U8 a);

    F32 magVec() const;        // deprecated -- use length()
    F32 magVecSquared() const; // deprecated -- use lengthSquared()

    F32 length() const;        // Returns magnitude squared of LLColor4U
    F32 lengthSquared() const; // Returns magnitude squared of LLColor4U

    friend std::ostream& operator<<(std::ostream& s, const LLColor4U& a);    // Print a
    friend LLColor4U     operator+(const LLColor4U& a, const LLColor4U& b);  // Return vector a + b
    friend LLColor4U     operator-(const LLColor4U& a, const LLColor4U& b);  // Return vector a minus b
    friend LLColor4U     operator*(const LLColor4U& a, const LLColor4U& b);  // Return a * b
    friend bool          operator==(const LLColor4U& a, const LLColor4U& b); // Return a == b
    friend bool          operator!=(const LLColor4U& a, const LLColor4U& b); // Return a != b

    friend const LLColor4U& operator+=(LLColor4U& a, const LLColor4U& b); // Return vector a + b
    friend const LLColor4U& operator-=(LLColor4U& a, const LLColor4U& b); // Return vector a minus b
    friend const LLColor4U& operator*=(LLColor4U& a, U8 k);               // Return rgb times scaler k (no alpha change)
    friend const LLColor4U& operator%=(LLColor4U& a, U8 k);               // Return alpha times scaler k (no rgb change)

    LLColor4U addClampMax(const LLColor4U& color); // Add and clamp the max

    LLColor4U multAll(const F32 k); // Multiply ALL channels by scalar k

    inline void setVecScaleClamp(const LLColor3& color);
    inline void setVecScaleClamp(const LLColor4& color);

    static bool parseColor4U(const std::string& buf, LLColor4U* value);

    // conversion
    operator LLColor4() const { return LLColor4(*this); }

    U32  asRGBA() const;
    void fromRGBA(U32 aVal);

    static LLColor4U white;
    static LLColor4U black;
    static LLColor4U red;
    static LLColor4U green;
    static LLColor4U blue;
};

// Non-member functions
F32 distVec(const LLColor4U& a, const LLColor4U& b);         // Returns distance between a and b
F32 distVec_squared(const LLColor4U& a, const LLColor4U& b); // Returns distance squared between a and b

inline LLColor4U::LLColor4U()
{
    mV[VRED]   = 0;
    mV[VGREEN] = 0;
    mV[VBLUE]  = 0;
    mV[VALPHA] = 255;
}

inline LLColor4U::LLColor4U(U8 r, U8 g, U8 b)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = 255;
}

inline LLColor4U::LLColor4U(U8 r, U8 g, U8 b, U8 a)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = a;
}

inline LLColor4U::LLColor4U(const U8* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
}

inline const LLColor4U& LLColor4U::setToBlack(void)
{
    mV[VRED]   = 0;
    mV[VGREEN] = 0;
    mV[VBLUE]  = 0;
    mV[VALPHA] = 255;
    return (*this);
}

inline const LLColor4U& LLColor4U::setToWhite(void)
{
    mV[VRED]   = 255;
    mV[VGREEN] = 255;
    mV[VBLUE]  = 255;
    mV[VALPHA] = 255;
    return (*this);
}

inline const LLColor4U& LLColor4U::set(const U8 x, const U8 y, const U8 z)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;

    //  no change to alpha!
    //  mV[VALPHA] = 255;

    return (*this);
}

inline const LLColor4U& LLColor4U::set(const U8 r, const U8 g, const U8 b, U8 a)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = a;
    return (*this);
}

inline const LLColor4U& LLColor4U::set(const LLColor4U& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    mV[VALPHA] = vec.mV[VALPHA];
    return (*this);
}

inline const LLColor4U& LLColor4U::set(const U8* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
    return (*this);
}

// deprecated
inline const LLColor4U& LLColor4U::setVec(const U8 x, const U8 y, const U8 z)
{
    mV[VRED]   = x;
    mV[VGREEN] = y;
    mV[VBLUE]  = z;

    //  no change to alpha!
    //  mV[VALPHA] = 255;

    return (*this);
}

// deprecated
inline const LLColor4U& LLColor4U::setVec(const U8 r, const U8 g, const U8 b, U8 a)
{
    mV[VRED]   = r;
    mV[VGREEN] = g;
    mV[VBLUE]  = b;
    mV[VALPHA] = a;
    return (*this);
}

// deprecated
inline const LLColor4U& LLColor4U::setVec(const LLColor4U& vec)
{
    mV[VRED]   = vec.mV[VRED];
    mV[VGREEN] = vec.mV[VGREEN];
    mV[VBLUE]  = vec.mV[VBLUE];
    mV[VALPHA] = vec.mV[VALPHA];
    return (*this);
}

// deprecated
inline const LLColor4U& LLColor4U::setVec(const U8* vec)
{
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
    mV[VALPHA] = vec[VALPHA];
    return (*this);
}

inline const LLColor4U& LLColor4U::setAlpha(U8 a)
{
    mV[VALPHA] = a;
    return (*this);
}

// LLColor4U Magnitude and Normalization Functions

inline F32 LLColor4U::length() const
{
    return sqrt(((F32)mV[VRED]) * mV[VRED] + ((F32)mV[VGREEN]) * mV[VGREEN] + ((F32)mV[VBLUE]) * mV[VBLUE]);
}

inline F32 LLColor4U::lengthSquared() const
{
    return ((F32)mV[VRED]) * mV[VRED] + ((F32)mV[VGREEN]) * mV[VGREEN] + ((F32)mV[VBLUE]) * mV[VBLUE];
}

// deprecated
inline F32 LLColor4U::magVec() const
{
    return sqrt(((F32)mV[VRED]) * mV[VRED] + ((F32)mV[VGREEN]) * mV[VGREEN] + ((F32)mV[VBLUE]) * mV[VBLUE]);
}

// deprecated
inline F32 LLColor4U::magVecSquared() const
{
    return ((F32)mV[VRED]) * mV[VRED] + ((F32)mV[VGREEN]) * mV[VGREEN] + ((F32)mV[VBLUE]) * mV[VBLUE];
}

inline LLColor4U operator+(const LLColor4U& a, const LLColor4U& b)
{
    return LLColor4U(a.mV[VRED] + b.mV[VRED], a.mV[VGREEN] + b.mV[VGREEN], a.mV[VBLUE] + b.mV[VBLUE], a.mV[VALPHA] + b.mV[VALPHA]);
}

inline LLColor4U operator-(const LLColor4U& a, const LLColor4U& b)
{
    return LLColor4U(a.mV[VRED] - b.mV[VRED], a.mV[VGREEN] - b.mV[VGREEN], a.mV[VBLUE] - b.mV[VBLUE], a.mV[VALPHA] - b.mV[VALPHA]);
}

inline LLColor4U operator*(const LLColor4U& a, const LLColor4U& b)
{
    return LLColor4U(a.mV[VRED] * b.mV[VRED], a.mV[VGREEN] * b.mV[VGREEN], a.mV[VBLUE] * b.mV[VBLUE], a.mV[VALPHA] * b.mV[VALPHA]);
}

inline LLColor4U LLColor4U::addClampMax(const LLColor4U& color)
{
    return LLColor4U(llmin((S32)mV[VRED] + color.mV[VRED], 255),
                     llmin((S32)mV[VGREEN] + color.mV[VGREEN], 255),
                     llmin((S32)mV[VBLUE] + color.mV[VBLUE], 255),
                     llmin((S32)mV[VALPHA] + color.mV[VALPHA], 255));
}

inline LLColor4U LLColor4U::multAll(const F32 k)
{
    // Round to nearest
    return LLColor4U((U8)ll_round(mV[VRED] * k), (U8)ll_round(mV[VGREEN] * k), (U8)ll_round(mV[VBLUE] * k), (U8)ll_round(mV[VALPHA] * k));
}

inline bool operator==(const LLColor4U& a, const LLColor4U& b)
{
    return ((a.mV[VRED] == b.mV[VRED]) && (a.mV[VGREEN] == b.mV[VGREEN]) && (a.mV[VBLUE] == b.mV[VBLUE]) && (a.mV[VALPHA] == b.mV[VALPHA]));
}

inline bool operator!=(const LLColor4U& a, const LLColor4U& b)
{
    return ((a.mV[VRED] != b.mV[VRED]) || (a.mV[VGREEN] != b.mV[VGREEN]) || (a.mV[VBLUE] != b.mV[VBLUE]) || (a.mV[VALPHA] != b.mV[VALPHA]));
}

inline const LLColor4U& operator+=(LLColor4U& a, const LLColor4U& b)
{
    a.mV[VRED] += b.mV[VRED];
    a.mV[VGREEN] += b.mV[VGREEN];
    a.mV[VBLUE] += b.mV[VBLUE];
    a.mV[VALPHA] += b.mV[VALPHA];
    return a;
}

inline const LLColor4U& operator-=(LLColor4U& a, const LLColor4U& b)
{
    a.mV[VRED] -= b.mV[VRED];
    a.mV[VGREEN] -= b.mV[VGREEN];
    a.mV[VBLUE] -= b.mV[VBLUE];
    a.mV[VALPHA] -= b.mV[VALPHA];
    return a;
}

inline const LLColor4U& operator*=(LLColor4U& a, U8 k)
{
    // only affects rgb (not a!)
    a.mV[VRED] *= k;
    a.mV[VGREEN] *= k;
    a.mV[VBLUE] *= k;
    return a;
}

inline const LLColor4U& operator%=(LLColor4U& a, U8 k)
{
    // only affects alpha (not rgb!)
    a.mV[VALPHA] *= k;
    return a;
}

inline F32 distVec(const LLColor4U& a, const LLColor4U& b)
{
    LLColor4U vec = a - b;
    return (vec.length());
}

inline F32 distVec_squared(const LLColor4U& a, const LLColor4U& b)
{
    LLColor4U vec = a - b;
    return (vec.lengthSquared());
}

void LLColor4U::setVecScaleClamp(const LLColor4& color)
{
    F32 color_scale_factor = 255.f;
    F32 max_color          = llmax(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE]);
    if (max_color > 1.f)
    {
        color_scale_factor /= max_color;
    }
    constexpr S32 MAX_COLOR = 255;
    S32           r         = ll_round(color.mV[VRED] * color_scale_factor);
    if (r > MAX_COLOR)
    {
        r = MAX_COLOR;
    }
    else if (r < 0)
    {
        r = 0;
    }
    mV[VRED] = r;

    S32 g = ll_round(color.mV[VGREEN] * color_scale_factor);
    if (g > MAX_COLOR)
    {
        g = MAX_COLOR;
    }
    else if (g < 0)
    {
        g = 0;
    }
    mV[VGREEN] = g;

    S32 b = ll_round(color.mV[VBLUE] * color_scale_factor);
    if (b > MAX_COLOR)
    {
        b = MAX_COLOR;
    }
    else if (b < 0)
    {
        b = 0;
    }
    mV[VBLUE] = b;

    // Alpha shouldn't be scaled, just clamped...
    S32 a = ll_round(color.mV[VALPHA] * MAX_COLOR);
    if (a > MAX_COLOR)
    {
        a = MAX_COLOR;
    }
    else if (a < 0)
    {
        a = 0;
    }
    mV[VALPHA] = a;
}

void LLColor4U::setVecScaleClamp(const LLColor3& color)
{
    F32 color_scale_factor = 255.f;
    F32 max_color          = llmax(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE]);
    if (max_color > 1.f)
    {
        color_scale_factor /= max_color;
    }

    const S32 MAX_COLOR = 255;
    S32       r         = ll_round(color.mV[VRED] * color_scale_factor);
    if (r > MAX_COLOR)
    {
        r = MAX_COLOR;
    }
    else if (r < 0)
    {
        r = 0;
    }
    mV[VRED] = r;

    S32 g = ll_round(color.mV[VGREEN] * color_scale_factor);
    if (g > MAX_COLOR)
    {
        g = MAX_COLOR;
    }
    else if (g < 0)
    {
        g = 0;
    }
    mV[VGREEN] = g;

    S32 b = ll_round(color.mV[VBLUE] * color_scale_factor);
    if (b > MAX_COLOR)
    {
        b = MAX_COLOR;
    }
    if (b < 0)
    {
        b = 0;
    }
    mV[VBLUE] = b;

    mV[VALPHA] = 255;
}

inline U32 LLColor4U::asRGBA() const
{
    // Little endian: values are swapped in memory. The original code access the array like a U32, so we need to swap here

    return (mV[VALPHA] << 24) | (mV[VBLUE] << 16) | (mV[VGREEN] << 8) | mV[VRED];
}

inline void LLColor4U::fromRGBA(U32 aVal)
{
    // Little endian: values are swapped in memory. The original code access the array like a U32, so we need to swap here

    mV[VRED] = aVal & 0xFF;
    aVal >>= 8;
    mV[VGREEN] = aVal & 0xFF;
    aVal >>= 8;
    mV[VBLUE] = aVal & 0xFF;
    aVal >>= 8;
    mV[VALPHA] = aVal & 0xFF;
}

#endif
