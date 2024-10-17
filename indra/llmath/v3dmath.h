/**
 * @file v3dmath.h
 * @brief High precision 3 dimensional vector.
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

#ifndef LL_V3DMATH_H
#define LL_V3DMATH_H

#include "llerror.h"
#include "v3math.h"

class LLVector3d
{
    public:
        F64 mdV[3];

        const static LLVector3d zero;
        const static LLVector3d x_axis;
        const static LLVector3d y_axis;
        const static LLVector3d z_axis;
        const static LLVector3d x_axis_neg;
        const static LLVector3d y_axis_neg;
        const static LLVector3d z_axis_neg;

        inline LLVector3d();                            // Initializes LLVector3d to (0, 0, 0)
        inline LLVector3d(const F64 x, const F64 y, const F64 z);           // Initializes LLVector3d to (x. y, z)
        inline explicit LLVector3d(const F64 *vec);             // Initializes LLVector3d to (vec[0]. vec[1], vec[2])
        inline explicit LLVector3d(const LLVector3 &vec);
        explicit LLVector3d(const LLSD& sd)
        {
            setValue(sd);
        }

        void setValue(const LLSD& sd)
        {
            mdV[VX] = sd[0].asReal();
            mdV[VY] = sd[1].asReal();
            mdV[VZ] = sd[2].asReal();
        }

        LLSD getValue() const
        {
            LLSD ret;
            ret[0] = mdV[VX];
            ret[1] = mdV[VY];
            ret[2] = mdV[VZ];
            return ret;
        }

        inline bool isFinite() const;                                   // checks to see if all values of LLVector3d are finite
        bool        clamp(const F64 min, const F64 max);        // Clamps all values to (min,max), returns true if data changed
        bool        abs();                      // sets all values to absolute value of original value (first octant), returns true if changed

        inline const LLVector3d&    clear();        // Clears LLVector3d to (0, 0, 0, 1)
        inline const LLVector3d&    clearVec();     // deprecated
        inline const LLVector3d&    setZero();      // Zero LLVector3d to (0, 0, 0, 0)
        inline const LLVector3d&    zeroVec();      // deprecated
        inline const LLVector3d&    set(const F64 x, const F64 y, const F64 z); // Sets LLVector3d to (x, y, z, 1)
        inline const LLVector3d&    set(const LLVector3d &vec); // Sets LLVector3d to vec
        inline const LLVector3d&    set(const F64 *vec);        // Sets LLVector3d to vec
        inline const LLVector3d&    set(const LLVector3 &vec);
        inline const LLVector3d&    setVec(const F64 x, const F64 y, const F64 z);  // deprecated
        inline const LLVector3d&    setVec(const LLVector3d &vec);  // deprecated
        inline const LLVector3d&    setVec(const F64 *vec);         // deprecated
        inline const LLVector3d&    setVec(const LLVector3 &vec);   // deprecated

        F64     magVec() const;             // deprecated
        F64     magVecSquared() const;      // deprecated
        inline F64      normVec();                  // deprecated

        F64 length() const;         // Returns magnitude of LLVector3d
        F64 lengthSquared() const;  // Returns magnitude squared of LLVector3d
        inline F64 normalize();     // Normalizes and returns the magnitude of LLVector3d

        const LLVector3d&   rotVec(const F64 angle, const LLVector3d &vec); // Rotates about vec by angle radians
        const LLVector3d&   rotVec(const F64 angle, const F64 x, const F64 y, const F64 z);     // Rotates about x,y,z by angle radians
        const LLVector3d&   rotVec(const LLMatrix3 &mat);               // Rotates by LLMatrix4 mat
        const LLVector3d&   rotVec(const LLQuaternion &q);              // Rotates by LLQuaternion q

        bool isNull() const;            // Returns true if vector has a _very_small_ length
        bool isExactlyZero() const      { return !mdV[VX] && !mdV[VY] && !mdV[VZ]; }

        const LLVector3d&   operator=(const LLVector4 &a);

        F64 operator[](int idx) const { return mdV[idx]; }
        F64 &operator[](int idx) { return mdV[idx]; }

        friend LLVector3d operator+(const LLVector3d& a, const LLVector3d& b);  // Return vector a + b
        friend LLVector3d operator-(const LLVector3d& a, const LLVector3d& b);  // Return vector a minus b
        friend F64 operator*(const LLVector3d& a, const LLVector3d& b);     // Return a dot b
        friend LLVector3d operator%(const LLVector3d& a, const LLVector3d& b);  // Return a cross b
        friend LLVector3d operator*(const LLVector3d& a, const F64 k);              // Return a times scaler k
        friend LLVector3d operator/(const LLVector3d& a, const F64 k);              // Return a divided by scaler k
        friend LLVector3d operator*(const F64 k, const LLVector3d& a);              // Return a times scaler k
        friend bool operator==(const LLVector3d& a, const LLVector3d& b);       // Return a == b
        friend bool operator!=(const LLVector3d& a, const LLVector3d& b);       // Return a != b

        friend const LLVector3d& operator+=(LLVector3d& a, const LLVector3d& b);    // Return vector a + b
        friend const LLVector3d& operator-=(LLVector3d& a, const LLVector3d& b);    // Return vector a minus b
        friend const LLVector3d& operator%=(LLVector3d& a, const LLVector3d& b);    // Return a cross b
        friend const LLVector3d& operator*=(LLVector3d& a, const F64 k);                // Return a times scaler k
        friend const LLVector3d& operator/=(LLVector3d& a, const F64 k);                // Return a divided by scaler k

        friend LLVector3d operator-(const LLVector3d& a);                   // Return vector -a

        friend std::ostream&     operator<<(std::ostream& s, const LLVector3d& a);      // Stream a

        static bool parseVector3d(const std::string& buf, LLVector3d* value);

};

typedef LLVector3d LLGlobalVec;

inline const LLVector3d &LLVector3d::set(const LLVector3 &vec)
{
    mdV[VX] = vec.mV[VX];
    mdV[VY] = vec.mV[VY];
    mdV[VZ] = vec.mV[VZ];
    return *this;
}

inline const LLVector3d &LLVector3d::setVec(const LLVector3 &vec)
{
    mdV[VX] = vec.mV[VX];
    mdV[VY] = vec.mV[VY];
    mdV[VZ] = vec.mV[VZ];
    return *this;
}


inline LLVector3d::LLVector3d(void)
{
    mdV[VX] = 0.f;
    mdV[VY] = 0.f;
    mdV[VZ] = 0.f;
}

inline LLVector3d::LLVector3d(const F64 x, const F64 y, const F64 z)
{
    mdV[VX] = x;
    mdV[VY] = y;
    mdV[VZ] = z;
}

inline LLVector3d::LLVector3d(const F64 *vec)
{
    mdV[VX] = vec[VX];
    mdV[VY] = vec[VY];
    mdV[VZ] = vec[VZ];
}

inline LLVector3d::LLVector3d(const LLVector3 &vec)
{
    mdV[VX] = vec.mV[VX];
    mdV[VY] = vec.mV[VY];
    mdV[VZ] = vec.mV[VZ];
}

/*
inline LLVector3d::LLVector3d(const LLVector3d &copy)
{
    mdV[VX] = copy.mdV[VX];
    mdV[VY] = copy.mdV[VY];
    mdV[VZ] = copy.mdV[VZ];
}
*/

// Destructors

// checker
inline bool LLVector3d::isFinite() const
{
    return (llfinite(mdV[VX]) && llfinite(mdV[VY]) && llfinite(mdV[VZ]));
}


// Clear and Assignment Functions

inline const LLVector3d&    LLVector3d::clear(void)
{
    mdV[VX] = 0.f;
    mdV[VY] = 0.f;
    mdV[VZ] = 0.f;
    return (*this);
}

inline const LLVector3d&    LLVector3d::clearVec(void)
{
    mdV[VX] = 0.f;
    mdV[VY] = 0.f;
    mdV[VZ] = 0.f;
    return (*this);
}

inline const LLVector3d&    LLVector3d::setZero(void)
{
    mdV[VX] = 0.f;
    mdV[VY] = 0.f;
    mdV[VZ] = 0.f;
    return (*this);
}

inline const LLVector3d&    LLVector3d::zeroVec(void)
{
    mdV[VX] = 0.f;
    mdV[VY] = 0.f;
    mdV[VZ] = 0.f;
    return (*this);
}

inline const LLVector3d&    LLVector3d::set(const F64 x, const F64 y, const F64 z)
{
    mdV[VX] = x;
    mdV[VY] = y;
    mdV[VZ] = z;
    return (*this);
}

inline const LLVector3d&    LLVector3d::set(const LLVector3d &vec)
{
    mdV[VX] = vec.mdV[VX];
    mdV[VY] = vec.mdV[VY];
    mdV[VZ] = vec.mdV[VZ];
    return (*this);
}

inline const LLVector3d&    LLVector3d::set(const F64 *vec)
{
    mdV[VX] = vec[0];
    mdV[VY] = vec[1];
    mdV[VZ] = vec[2];
    return (*this);
}

inline const LLVector3d&    LLVector3d::setVec(const F64 x, const F64 y, const F64 z)
{
    mdV[VX] = x;
    mdV[VY] = y;
    mdV[VZ] = z;
    return (*this);
}

inline const LLVector3d&    LLVector3d::setVec(const LLVector3d& vec)
{
    mdV[VX] = vec.mdV[VX];
    mdV[VY] = vec.mdV[VY];
    mdV[VZ] = vec.mdV[VZ];
    return (*this);
}

inline const LLVector3d&    LLVector3d::setVec(const F64* vec)
{
    mdV[VX] = vec[VX];
    mdV[VY] = vec[VY];
    mdV[VZ] = vec[VZ];
    return (*this);
}

inline F64 LLVector3d::normVec()
{
    F64 mag = sqrt(mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ]);
    F64 oomag;

    if (mag > FP_MAG_THRESHOLD)
    {
        oomag = 1.f/mag;
        mdV[VX] *= oomag;
        mdV[VY] *= oomag;
        mdV[VZ] *= oomag;
    }
    else
    {
        mdV[VX] = 0.f;
        mdV[VY] = 0.f;
        mdV[VZ] = 0.f;
        mag = 0;
    }
    return (mag);
}

inline F64 LLVector3d::normalize()
{
    F64 mag = sqrt(mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ]);
    F64 oomag;

    if (mag > FP_MAG_THRESHOLD)
    {
        oomag = 1.f/mag;
        mdV[VX] *= oomag;
        mdV[VY] *= oomag;
        mdV[VZ] *= oomag;
    }
    else
    {
        mdV[VX] = 0.f;
        mdV[VY] = 0.f;
        mdV[VZ] = 0.f;
        mag = 0;
    }
    return (mag);
}

// LLVector3d Magnitude and Normalization Functions

inline F64  LLVector3d::magVec() const
{
    return sqrt(mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ]);
}

inline F64  LLVector3d::magVecSquared() const
{
    return mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ];
}

inline F64  LLVector3d::length() const
{
    return sqrt(mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ]);
}

inline F64  LLVector3d::lengthSquared() const
{
    return mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ];
}

inline LLVector3d operator+(const LLVector3d& a, const LLVector3d& b)
{
    LLVector3d c(a);
    return c += b;
}

inline LLVector3d operator-(const LLVector3d& a, const LLVector3d& b)
{
    LLVector3d c(a);
    return c -= b;
}

inline F64  operator*(const LLVector3d& a, const LLVector3d& b)
{
    return (a.mdV[VX]*b.mdV[VX] + a.mdV[VY]*b.mdV[VY] + a.mdV[VZ]*b.mdV[VZ]);
}

inline LLVector3d operator%(const LLVector3d& a, const LLVector3d& b)
{
    return LLVector3d( a.mdV[VY]*b.mdV[VZ] - b.mdV[VY]*a.mdV[VZ], a.mdV[VZ]*b.mdV[VX] - b.mdV[VZ]*a.mdV[VX], a.mdV[VX]*b.mdV[VY] - b.mdV[VX]*a.mdV[VY] );
}

inline LLVector3d operator/(const LLVector3d& a, const F64 k)
{
    F64 t = 1.f / k;
    return LLVector3d( a.mdV[VX] * t, a.mdV[VY] * t, a.mdV[VZ] * t );
}

inline LLVector3d operator*(const LLVector3d& a, const F64 k)
{
    return LLVector3d( a.mdV[VX] * k, a.mdV[VY] * k, a.mdV[VZ] * k );
}

inline LLVector3d operator*(F64 k, const LLVector3d& a)
{
    return LLVector3d( a.mdV[VX] * k, a.mdV[VY] * k, a.mdV[VZ] * k );
}

inline bool operator==(const LLVector3d& a, const LLVector3d& b)
{
    return (  (a.mdV[VX] == b.mdV[VX])
            &&(a.mdV[VY] == b.mdV[VY])
            &&(a.mdV[VZ] == b.mdV[VZ]));
}

inline bool operator!=(const LLVector3d& a, const LLVector3d& b)
{
    return (  (a.mdV[VX] != b.mdV[VX])
            ||(a.mdV[VY] != b.mdV[VY])
            ||(a.mdV[VZ] != b.mdV[VZ]));
}

inline const LLVector3d& operator+=(LLVector3d& a, const LLVector3d& b)
{
    a.mdV[VX] += b.mdV[VX];
    a.mdV[VY] += b.mdV[VY];
    a.mdV[VZ] += b.mdV[VZ];
    return a;
}

inline const LLVector3d& operator-=(LLVector3d& a, const LLVector3d& b)
{
    a.mdV[VX] -= b.mdV[VX];
    a.mdV[VY] -= b.mdV[VY];
    a.mdV[VZ] -= b.mdV[VZ];
    return a;
}

inline const LLVector3d& operator%=(LLVector3d& a, const LLVector3d& b)
{
    LLVector3d ret( a.mdV[VY]*b.mdV[VZ] - b.mdV[VY]*a.mdV[VZ], a.mdV[VZ]*b.mdV[VX] - b.mdV[VZ]*a.mdV[VX], a.mdV[VX]*b.mdV[VY] - b.mdV[VX]*a.mdV[VY]);
    a = ret;
    return a;
}

inline const LLVector3d& operator*=(LLVector3d& a, const F64 k)
{
    a.mdV[VX] *= k;
    a.mdV[VY] *= k;
    a.mdV[VZ] *= k;
    return a;
}

inline const LLVector3d& operator/=(LLVector3d& a, const F64 k)
{
    F64 t = 1.f / k;
    a.mdV[VX] *= t;
    a.mdV[VY] *= t;
    a.mdV[VZ] *= t;
    return a;
}

inline LLVector3d operator-(const LLVector3d& a)
{
    return LLVector3d( -a.mdV[VX], -a.mdV[VY], -a.mdV[VZ] );
}

inline F64  dist_vec(const LLVector3d& a, const LLVector3d& b)
{
    F64 x = a.mdV[VX] - b.mdV[VX];
    F64 y = a.mdV[VY] - b.mdV[VY];
    F64 z = a.mdV[VZ] - b.mdV[VZ];
    return (F32) sqrt( x*x + y*y + z*z );
}

inline F64  dist_vec_squared(const LLVector3d& a, const LLVector3d& b)
{
    F64 x = a.mdV[VX] - b.mdV[VX];
    F64 y = a.mdV[VY] - b.mdV[VY];
    F64 z = a.mdV[VZ] - b.mdV[VZ];
    return x*x + y*y + z*z;
}

inline F64  dist_vec_squared2D(const LLVector3d& a, const LLVector3d& b)
{
    F64 x = a.mdV[VX] - b.mdV[VX];
    F64 y = a.mdV[VY] - b.mdV[VY];
    return x*x + y*y;
}

inline LLVector3d lerp(const LLVector3d& a, const LLVector3d& b, const F64 u)
{
    return LLVector3d(
        a.mdV[VX] + (b.mdV[VX] - a.mdV[VX]) * u,
        a.mdV[VY] + (b.mdV[VY] - a.mdV[VY]) * u,
        a.mdV[VZ] + (b.mdV[VZ] - a.mdV[VZ]) * u);
}


inline bool LLVector3d::isNull() const
{
    if ( F_APPROXIMATELY_ZERO > mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ] )
    {
        return true;
    }
    return false;
}


inline F64 angle_between(const LLVector3d& a, const LLVector3d& b)
{
    LLVector3d an = a;
    LLVector3d bn = b;
    an.normalize();
    bn.normalize();
    F64 cosine = an * bn;
    F64 angle = (cosine >= 1.0f) ? 0.0f :
                (cosine <= -1.0f) ? F_PI :
                acos(cosine);
    return angle;
}

inline bool are_parallel(const LLVector3d& a, const LLVector3d& b, const F64 epsilon)
{
    LLVector3d an = a;
    LLVector3d bn = b;
    an.normalize();
    bn.normalize();
    F64 dot = an * bn;
    if ( (1.0f - fabs(dot)) < epsilon)
    {
        return true;
    }
    return false;
}

inline LLVector3d projected_vec(const LLVector3d& a, const LLVector3d& b)
{
    LLVector3d project_axis = b;
    project_axis.normalize();
    return project_axis * (a * project_axis);
}

inline LLVector3d inverse_projected_vec(const LLVector3d& a, const LLVector3d& b)
{
    LLVector3d normalized_a = a;
    normalized_a.normalize();
    LLVector3d normalized_b = b;
    F64 b_length = normalized_b.normalize();

    F64 dot_product = normalized_a * normalized_b;
    return normalized_a * (b_length / dot_product);
}

#endif // LL_V3DMATH_H
