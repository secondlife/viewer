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

#pragma once

class LLColor4;
class LLVector4;

#include "llerror.h"
#include "llmath.h"
#include "llsd.h"
#include "v3math.h" // needed for linearColor3v implemtation below
#include <string.h>

#include <glm/vec3.hpp>
#include <glm/common.hpp>           // glm::clamp
#include <glm/exponential.hpp>      // glm::exp / pow / sqrt

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
    explicit LLColor3(const F32* vec);             // Initializes LLColor3 to (vec[0]. vec[1], vec[2])
    explicit LLColor3(const char* color_string);   // html format color ie "#FFDDEE"
    explicit LLColor3(const LLColor4& color4);   // "explicit" to avoid automatic conversion
    explicit LLColor3(const LLVector4& vector4); // "explicit" to avoid automatic conversion
    explicit LLColor3(const LLSD& sd);

    LLSD getValue() const
    {
        LLSD ret;
        ret[VRED]   = mV[VRED];
        ret[VGREEN] = mV[VGREEN];
        ret[VBLUE]  = mV[VBLUE];
        return ret;
    }

    void setValue(const LLSD& sd)
    {
        mV[VRED]   = static_cast<F32>(sd[VRED].asReal());
        mV[VGREEN] = static_cast<F32>(sd[VGREEN].asReal());
        mV[VBLUE]  = static_cast<F32>(sd[VBLUE].asReal());
    }

    void setHSL(F32 hue, F32 saturation, F32 luminance);
    void calcHSL(F32* hue, F32* saturation, F32* luminance) const;

    const LLColor3& setToBlack(); // Clears LLColor3 to (0, 0, 0)
    const LLColor3& setToWhite(); // Zero LLColor3 to (0, 0, 0)

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

static_assert(std::is_trivially_copyable<LLColor3>::value, "LLColor3 must be trivial copy");
static_assert(std::is_trivially_move_assignable<LLColor3>::value, "LLColor3 must be trivial move");
static_assert(std::is_standard_layout<LLColor3>::value, "LLColor3 must be a standard layout type");

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
    mV[VBLUE]  = static_cast<F32>(strtol(&tempstr[4], nullptr, 16)) / 255.f;
    tempstr[4] = '\0';
    mV[VGREEN] = static_cast<F32>(strtol(&tempstr[2], nullptr, 16)) / 255.f;
    tempstr[2] = '\0';
    mV[VRED]   = static_cast<F32>(strtol(&tempstr[0], nullptr, 16)) / 255.f;
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
    mV[VRED]   = vec[VRED];
    mV[VGREEN] = vec[VGREEN];
    mV[VBLUE]  = vec[VBLUE];
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

inline void LLColor3::exp()
{
    mV[VRED]   = static_cast<F32>(LL_FAST_EXP(mV[VRED]));
    mV[VGREEN] = static_cast<F32>(LL_FAST_EXP(mV[VGREEN]));
    mV[VBLUE]  = static_cast<F32>(LL_FAST_EXP(mV[VBLUE]));
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
    linearColor.mV[VRED]   = sRGBtoLinear(v[VRED]);
    linearColor.mV[VGREEN] = sRGBtoLinear(v[VGREEN]);
    linearColor.mV[VBLUE]  = sRGBtoLinear(v[VBLUE]);

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
        mV[i] = static_cast<F32>(v[i]);
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

// ============================================================================
// glm::vec3-flavoured helpers for the LLColor3 -> glm::vec3 migration.
//
// GLM has no native color type — glm::vec3 is the canonical RGB
// representation. The helpers below preserve the LL semantics that
// glm doesn't replicate directly:
//
//   * color3_inverse:  LL `-c` returns 1-rgb, NOT component negation.
//                      Use this instead of unary minus on glm::vec3.
//   * color3_brightness: LL brightness() == (r+g+b)/3, NAIVE average,
//                        NOT Rec.709 luminance. Migration must
//                        preserve this exact formula.
//   * color3_from_html: parses "RRGGBB" hex strings to glm::vec3.
//   * color3_setHSL / color3_calcHSL: HSL conversion as free
//                                     functions on glm::vec3.
//
// See v3color_glm_equivalence_test.cpp tests #8-#14 for the
// semantics each helper preserves.
// ============================================================================

namespace ll_color3
{
    // color3_inverse: LL `-c` semantics. Returns (1-r, 1-g, 1-b),
    // saturated nowhere — caller is responsible if c has values
    // outside [0, 1].
    inline glm::vec3 inverse(const glm::vec3& c)
    {
        return glm::vec3(1.f) - c;
    }

    // brightness == (r+g+b)/3. NOT Rec.709 luminance. Use a different
    // helper if you actually want luminance.
    inline F32 brightness(const glm::vec3& c)
    {
        return (c.x + c.y + c.z) / 3.0f;
    }

    // Parse "RRGGBB" or "#RRGGBB" hex strings to a normalized RGB
    // glm::vec3. Returns black on a malformed string. Mirrors the
    // LLColor3(const char*) ctor body.
    inline glm::vec3 from_html(const char* color_string)
    {
        if (color_string == nullptr || strlen(color_string) < 6) /* Flawfinder: ignore */
        {
            return glm::vec3(0.f);
        }
        // Skip a leading '#' if present.
        const char* p = (color_string[0] == '#') ? color_string + 1 : color_string;
        if (strlen(p) < 6) /* Flawfinder: ignore */
        {
            return glm::vec3(0.f);
        }
        char tempstr[7];
        strncpy(tempstr, p, 6); /* Flawfinder: ignore */
        tempstr[6] = '\0';
        const F32 b = static_cast<F32>(strtol(&tempstr[4], nullptr, 16)) / 255.f;
        tempstr[4] = '\0';
        const F32 g = static_cast<F32>(strtol(&tempstr[2], nullptr, 16)) / 255.f;
        tempstr[2] = '\0';
        const F32 r = static_cast<F32>(strtol(&tempstr[0], nullptr, 16)) / 255.f;
        return glm::vec3(r, g, b);
    }

    // HSL -> RGB. Bit-for-bit equivalent to LLColor3::setHSL — uses
    // the same hueToRgb helper logic, written as a static lambda.
    inline glm::vec3 from_hsl(F32 h, F32 s, F32 l)
    {
        if (s < 0.00001f)
        {
            return glm::vec3(l);
        }
        const F32 v2 = (l < 0.5f) ? l * (1.0f + s) : (l + s) - (s * l);
        const F32 v1 = 2.0f * l - v2;
        auto hue_to_rgb = [](F32 a, F32 b, F32 hue)
        {
            if (hue < 0.f) hue += 1.f;
            if (hue > 1.f) hue -= 1.f;
            if ((6.f * hue) < 1.f) return a + (b - a) * 6.f * hue;
            if ((2.f * hue) < 1.f) return b;
            if ((3.f * hue) < 2.f) return a + (b - a) * ((2.f / 3.f) - hue) * 6.f;
            return a;
        };
        return glm::vec3(
            hue_to_rgb(v1, v2, h + (1.f / 3.f)),
            hue_to_rgb(v1, v2, h),
            hue_to_rgb(v1, v2, h - (1.f / 3.f)));
    }

    // RGB -> HSL. Bit-for-bit equivalent to LLColor3::calcHSL.
    inline void to_hsl(const glm::vec3& c, F32* hue, F32* saturation, F32* luminance)
    {
        const F32 var_R = c.x;
        const F32 var_G = c.y;
        const F32 var_B = c.z;
        const F32 var_Min = (var_R < (var_G < var_B ? var_G : var_B)) ? var_R : (var_G < var_B ? var_G : var_B);
        const F32 var_Max = (var_R > (var_G > var_B ? var_G : var_B)) ? var_R : (var_G > var_B ? var_G : var_B);
        const F32 del_Max = var_Max - var_Min;
        const F32 L = (var_Max + var_Min) / 2.0f;
        F32 H = 0.f;
        F32 S = 0.f;
        if (del_Max != 0.f)
        {
            S = (L < 0.5f) ? (del_Max / (var_Max + var_Min))
                           : (del_Max / (2.f - var_Max - var_Min));
            const F32 del_R = (((var_Max - var_R) / 6.f) + (del_Max / 2.f)) / del_Max;
            const F32 del_G = (((var_Max - var_G) / 6.f) + (del_Max / 2.f)) / del_Max;
            const F32 del_B = (((var_Max - var_B) / 6.f) + (del_Max / 2.f)) / del_Max;
            if (var_R >= var_Max)        H = del_B - del_G;
            else if (var_G >= var_Max)   H = (1.f / 3.f) + del_R - del_B;
            else if (var_B >= var_Max)   H = (2.f / 3.f) + del_G - del_R;
            if (H < 0.f) H += 1.f;
            if (H > 1.f) H -= 1.f;
        }
        if (hue) *hue = H;
        if (saturation) *saturation = S;
        if (luminance) *luminance = L;
    }

    // sRGB / linear gamma helpers — glm::vec3 overloads matching
    // the existing LLColor3 versions. These call the same scalar
    // linearTosRGB / sRGBtoLinear helpers from llmath.h that the
    // LLColor3 versions use, so the values match bit-for-bit.
    inline glm::vec3 to_srgb(const glm::vec3& a)
    {
        return glm::vec3(linearTosRGB(a.x), linearTosRGB(a.y), linearTosRGB(a.z));
    }

    inline glm::vec3 to_linear(const glm::vec3& a)
    {
        return glm::vec3(sRGBtoLinear(a.x), sRGBtoLinear(a.y), sRGBtoLinear(a.z));
    }

    // Common color constants as glm::vec3, available as inline
    // variables for header-only constexpr-style use. (constexpr
    // doesn't work because GLM_FORCE_DEFAULT_ALIGNED_GENTYPES makes
    // glm::vec3 not literal in this build.)
    inline const glm::vec3 white(1.f, 1.f, 1.f);
    inline const glm::vec3 black(0.f, 0.f, 0.f);
    inline const glm::vec3 grey(0.5f, 0.5f, 0.5f);
}

