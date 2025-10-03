/**
 * @file llsimdtypes.h
 * @brief Declaration of basic SIMD math related types
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_SIMD_TYPES_H
#define LL_SIMD_TYPES_H

#ifndef LL_SIMD_MATH_H
#error "Please include llmath.h before this file."
#endif

typedef __m128  LLQuad;

class LLBool32
{
public:
    inline LLBool32() {}
    inline LLBool32(int rhs) : m_bool(rhs) {}
    inline LLBool32(unsigned int rhs) : m_bool(rhs) {}
    inline LLBool32(bool rhs) { m_bool = static_cast<const int>(rhs); }
    inline LLBool32& operator= (bool rhs) { m_bool = (int)rhs; return *this; }
    inline bool operator== (bool rhs) const { return static_cast<const bool&>(m_bool) == rhs; }
    inline bool operator!= (bool rhs) const { return !operator==(rhs); }
    inline operator bool() const { return static_cast<const bool&>(m_bool); }

private:
    int m_bool{ 0 };
};

class LLSimdScalar
{
public:
    inline LLSimdScalar() {}
    inline LLSimdScalar(LLQuad q)
    {
        mQ = q;
    }

    inline LLSimdScalar(F32 f)
    {
        mQ = _mm_set_ss(f);
    }

    static inline const LLSimdScalar& getZero()
    {
        extern const LLQuad F_ZERO_4A;
        return reinterpret_cast<const LLSimdScalar&>(F_ZERO_4A);
    }

    inline F32 getF32() const;

    inline LLBool32 isApproximatelyEqual(const LLSimdScalar& rhs, F32 tolerance = F_APPROXIMATELY_ZERO) const;

    inline LLSimdScalar getAbs() const;

    inline void setMax( const LLSimdScalar& a, const LLSimdScalar& b );

    inline void setMin( const LLSimdScalar& a, const LLSimdScalar& b );

    inline LLSimdScalar& operator=(F32 rhs);

    inline LLSimdScalar& operator+=(const LLSimdScalar& rhs);

    inline LLSimdScalar& operator-=(const LLSimdScalar& rhs);

    inline LLSimdScalar& operator*=(const LLSimdScalar& rhs);

    inline LLSimdScalar& operator/=(const LLSimdScalar& rhs);

    inline operator LLQuad() const
    {
        return mQ;
    }

    inline const LLQuad& getQuad() const
    {
        return mQ;
    }

private:
    LLQuad mQ{};
};

#endif //LL_SIMD_TYPES_H
