/** 
 * @file llquaternion.h
 * @brief LLQuaternion class header file.
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

#ifndef LLQUATERNION_H
#define LLQUATERNION_H

#include <iostream>

#ifndef LLMATH_H //enforce specific include order to avoid tangling inline dependencies
#error "Please include llmath.h first."
#endif

class LLVector4;
class LLVector3;
class LLVector3d;
class LLMatrix4;
class LLMatrix3;

//	NOTA BENE: Quaternion code is written assuming Unit Quaternions!!!!
//			   Moreover, it is written assuming that all vectors and matricies
//			   passed as arguments are normalized and unitary respectively.
//			   VERY VERY VERY VERY BAD THINGS will happen if these assumptions fail.

static const U32 LENGTHOFQUAT = 4;

class LLQuaternion
{
public:
	F32 mQ[LENGTHOFQUAT];

	static const LLQuaternion DEFAULT;

	LLQuaternion();									// Initializes Quaternion to (0,0,0,1)
	explicit LLQuaternion(const LLMatrix4 &mat);				// Initializes Quaternion from Matrix4
	explicit LLQuaternion(const LLMatrix3 &mat);				// Initializes Quaternion from Matrix3
	LLQuaternion(F32 x, F32 y, F32 z, F32 w);		// Initializes Quaternion to normalize(x, y, z, w)
	LLQuaternion(F32 angle, const LLVector4 &vec);	// Initializes Quaternion to axis_angle2quat(angle, vec)
	LLQuaternion(F32 angle, const LLVector3 &vec);	// Initializes Quaternion to axis_angle2quat(angle, vec)
	LLQuaternion(const F32 *q);						// Initializes Quaternion to normalize(x, y, z, w)
	LLQuaternion(const LLVector3 &x_axis,
				 const LLVector3 &y_axis,
				 const LLVector3 &z_axis);			// Initializes Quaternion from Matrix3 = [x_axis ; y_axis ; z_axis]

	BOOL isIdentity() const;
	BOOL isNotIdentity() const;
	BOOL isFinite() const;									// checks to see if all values of LLQuaternion are finite
	void quantize16(F32 lower, F32 upper);					// changes the vector to reflect quatization
	void quantize8(F32 lower, F32 upper);							// changes the vector to reflect quatization
	void loadIdentity();											// Loads the quaternion that represents the identity rotation

	const LLQuaternion&	set(F32 x, F32 y, F32 z, F32 w);		// Sets Quaternion to normalize(x, y, z, w)
	const LLQuaternion&	set(const LLQuaternion &quat);			// Copies Quaternion
	const LLQuaternion&	set(const F32 *q);						// Sets Quaternion to normalize(quat[VX], quat[VY], quat[VZ], quat[VW])
	const LLQuaternion&	set(const LLMatrix3 &mat);				// Sets Quaternion to mat2quat(mat)
	const LLQuaternion&	set(const LLMatrix4 &mat);				// Sets Quaternion to mat2quat(mat)

	const LLQuaternion&	setAngleAxis(F32 angle, F32 x, F32 y, F32 z);	// Sets Quaternion to axis_angle2quat(angle, x, y, z)
	const LLQuaternion&	setAngleAxis(F32 angle, const LLVector3 &vec);	// Sets Quaternion to axis_angle2quat(angle, vec)
	const LLQuaternion&	setAngleAxis(F32 angle, const LLVector4 &vec);	// Sets Quaternion to axis_angle2quat(angle, vec)
	const LLQuaternion&	setEulerAngles(F32 roll, F32 pitch, F32 yaw);	// Sets Quaternion to euler2quat(pitch, yaw, roll)

	const LLQuaternion&	setQuatInit(F32 x, F32 y, F32 z, F32 w);	// deprecated
	const LLQuaternion&	setQuat(const LLQuaternion &quat);			// deprecated
	const LLQuaternion&	setQuat(const F32 *q);						// deprecated
	const LLQuaternion&	setQuat(const LLMatrix3 &mat);				// deprecated
	const LLQuaternion&	setQuat(const LLMatrix4 &mat);				// deprecated
	const LLQuaternion&	setQuat(F32 angle, F32 x, F32 y, F32 z);	// deprecated
	const LLQuaternion&	setQuat(F32 angle, const LLVector3 &vec);	// deprecated
	const LLQuaternion&	setQuat(F32 angle, const LLVector4 &vec);	// deprecated
	const LLQuaternion&	setQuat(F32 roll, F32 pitch, F32 yaw);		// deprecated

	LLMatrix4	getMatrix4(void) const;							// Returns the Matrix4 equivalent of Quaternion
	LLMatrix3	getMatrix3(void) const;							// Returns the Matrix3 equivalent of Quaternion
	void		getAngleAxis(F32* angle, F32* x, F32* y, F32* z) const;	// returns rotation in radians about axis x,y,z
	void		getAngleAxis(F32* angle, LLVector3 &vec) const;
	void		getEulerAngles(F32 *roll, F32* pitch, F32 *yaw) const;

	F32	normalize();	// Normalizes Quaternion and returns magnitude
	F32	normQuat();		// deprecated

	const LLQuaternion&	conjugate(void);	// Conjugates Quaternion and returns result
	const LLQuaternion&	conjQuat(void);		// deprecated

	// Other useful methods
	const LLQuaternion&	transpose();		// transpose (same as conjugate)
	const LLQuaternion&	transQuat();		// deprecated

	void			shortestArc(const LLVector3 &a, const LLVector3 &b);	// shortest rotation from a to b
	const LLQuaternion& constrain(F32 radians);						// constrains rotation to a cone angle specified in radians

	// Standard operators
	friend std::ostream& operator<<(std::ostream &s, const LLQuaternion &a);					// Prints a
	friend LLQuaternion operator+(const LLQuaternion &a, const LLQuaternion &b);	// Addition
	friend LLQuaternion operator-(const LLQuaternion &a, const LLQuaternion &b);	// Subtraction
	friend LLQuaternion operator-(const LLQuaternion &a);							// Negation
	friend LLQuaternion operator*(F32 a, const LLQuaternion &q);					// Scale
	friend LLQuaternion operator*(const LLQuaternion &q, F32 b);					// Scale
	friend LLQuaternion operator*(const LLQuaternion &a, const LLQuaternion &b);	// Returns a * b
	friend LLQuaternion operator~(const LLQuaternion &a);							// Returns a* (Conjugate of a)
	bool operator==(const LLQuaternion &b) const;			// Returns a == b
	bool operator!=(const LLQuaternion &b) const;			// Returns a != b

	friend const LLQuaternion& operator*=(LLQuaternion &a, const LLQuaternion &b);	// Returns a * b

	friend LLVector4 operator*(const LLVector4 &a, const LLQuaternion &rot);		// Rotates a by rot
	friend LLVector3 operator*(const LLVector3 &a, const LLQuaternion &rot);		// Rotates a by rot
	friend LLVector3d operator*(const LLVector3d &a, const LLQuaternion &rot);		// Rotates a by rot

	// Non-standard operators
	friend F32 dot(const LLQuaternion &a, const LLQuaternion &b);
	friend LLQuaternion lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q);		// linear interpolation (t = 0 to 1) from p to q
	friend LLQuaternion lerp(F32 t, const LLQuaternion &q);								// linear interpolation (t = 0 to 1) from identity to q
	friend LLQuaternion slerp(F32 t, const LLQuaternion &p, const LLQuaternion &q); 	// spherical linear interpolation from p to q
	friend LLQuaternion slerp(F32 t, const LLQuaternion &q);							// spherical linear interpolation from identity to q
	friend LLQuaternion nlerp(F32 t, const LLQuaternion &p, const LLQuaternion &q); 	// normalized linear interpolation from p to q
	friend LLQuaternion nlerp(F32 t, const LLQuaternion &q); 							// normalized linear interpolation from p to q

	LLVector3	packToVector3() const;						// Saves space by using the fact that our quaternions are normalized
	void		unpackFromVector3(const LLVector3& vec);	// Saves space by using the fact that our quaternions are normalized

	enum Order {
		XYZ = 0,
		YZX = 1,
		ZXY = 2,
		XZY = 3,
		YXZ = 4,
		ZYX = 5
	};
	// Creates a quaternions from maya's rotation representation,
	// which is 3 rotations (in DEGREES) in the specified order
	friend LLQuaternion mayaQ(F32 x, F32 y, F32 z, Order order);

	// Conversions between Order and strings like "xyz" or "ZYX"
	friend const char *OrderToString( const Order order );
	friend Order StringToOrder( const char *str );

	static BOOL parseQuat(const std::string& buf, LLQuaternion* value);

	// For debugging, only
	//static U32 mMultCount;
};

// checker
inline BOOL	LLQuaternion::isFinite() const
{
	return (llfinite(mQ[VX]) && llfinite(mQ[VY]) && llfinite(mQ[VZ]) && llfinite(mQ[VS]));
}

inline BOOL LLQuaternion::isIdentity() const
{
	return 
		( mQ[VX] == 0.f ) &&
		( mQ[VY] == 0.f ) &&
		( mQ[VZ] == 0.f ) &&
		( mQ[VS] == 1.f );
}

inline BOOL LLQuaternion::isNotIdentity() const
{
	return 
		( mQ[VX] != 0.f ) ||
		( mQ[VY] != 0.f ) ||
		( mQ[VZ] != 0.f ) ||
		( mQ[VS] != 1.f );
}



inline LLQuaternion::LLQuaternion(void)
{
	mQ[VX] = 0.f;
	mQ[VY] = 0.f;
	mQ[VZ] = 0.f;
	mQ[VS] = 1.f;
}

inline LLQuaternion::LLQuaternion(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;

	//RN: don't normalize this case as its used mainly for temporaries during calculations
	//normalize();
	/*
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);
	mag -= 1.f;
	mag = fabs(mag);
	llassert(mag < 10.f*FP_MAG_THRESHOLD);
	*/
}

inline LLQuaternion::LLQuaternion(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];

	normalize();
	/*
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);
	mag -= 1.f;
	mag = fabs(mag);
	llassert(mag < FP_MAG_THRESHOLD);
	*/
}


inline void LLQuaternion::loadIdentity()
{
	mQ[VX] = 0.0f;
	mQ[VY] = 0.0f;
	mQ[VZ] = 0.0f;
	mQ[VW] = 1.0f;
}


inline const LLQuaternion&	LLQuaternion::set(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;
	normalize();
	return (*this);
}

inline const LLQuaternion&	LLQuaternion::set(const LLQuaternion &quat)
{
	mQ[VX] = quat.mQ[VX];
	mQ[VY] = quat.mQ[VY];
	mQ[VZ] = quat.mQ[VZ];
	mQ[VW] = quat.mQ[VW];
	normalize();
	return (*this);
}

inline const LLQuaternion&	LLQuaternion::set(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];
	normalize();
	return (*this);
}


// deprecated
inline const LLQuaternion&	LLQuaternion::setQuatInit(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;
	normalize();
	return (*this);
}

// deprecated
inline const LLQuaternion&	LLQuaternion::setQuat(const LLQuaternion &quat)
{
	mQ[VX] = quat.mQ[VX];
	mQ[VY] = quat.mQ[VY];
	mQ[VZ] = quat.mQ[VZ];
	mQ[VW] = quat.mQ[VW];
	normalize();
	return (*this);
}

// deprecated
inline const LLQuaternion&	LLQuaternion::setQuat(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];
	normalize();
	return (*this);
}

// There may be a cheaper way that avoids the sqrt.
// Does sin_a = VX*VX + VY*VY + VZ*VZ?
// Copied from Matrix and Quaternion FAQ 1.12
inline void LLQuaternion::getAngleAxis(F32* angle, F32* x, F32* y, F32* z) const
{
	F32 cos_a = mQ[VW];
	if (cos_a > 1.0f) cos_a = 1.0f;
	if (cos_a < -1.0f) cos_a = -1.0f;

    F32 sin_a = (F32) sqrt( 1.0f - cos_a * cos_a );

    if ( fabs( sin_a ) < 0.0005f )
		sin_a = 1.0f;
	else
		sin_a = 1.f/sin_a;

    F32 temp_angle = 2.0f * (F32) acos( cos_a );
	if (temp_angle > F_PI)
	{
		// The (angle,axis) pair should never have angles outside [PI, -PI]
		// since we want the _shortest_ (angle,axis) solution.
		// Since acos is defined for [0, PI], and we multiply by 2.0, we
		// can push the angle outside the acceptible range.
		// When this happens we set the angle to the other portion of a 
		// full 2PI rotation, and negate the axis, which reverses the 
		// direction of the rotation (by the right-hand rule).
		*angle = 2.f * F_PI - temp_angle;
    	*x = - mQ[VX] * sin_a;
    	*y = - mQ[VY] * sin_a;
    	*z = - mQ[VZ] * sin_a;
	}
	else
	{
		*angle = temp_angle;
    	*x = mQ[VX] * sin_a;
    	*y = mQ[VY] * sin_a;
    	*z = mQ[VZ] * sin_a;
	}
}

inline const LLQuaternion& LLQuaternion::conjugate()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}

inline const LLQuaternion& LLQuaternion::conjQuat()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}

// Transpose
inline const LLQuaternion& LLQuaternion::transpose()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}

// deprecated
inline const LLQuaternion& LLQuaternion::transQuat()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}


inline LLQuaternion 	operator+(const LLQuaternion &a, const LLQuaternion &b)
{
	return LLQuaternion( 
		a.mQ[VX] + b.mQ[VX],
		a.mQ[VY] + b.mQ[VY],
		a.mQ[VZ] + b.mQ[VZ],
		a.mQ[VW] + b.mQ[VW] );
}


inline LLQuaternion 	operator-(const LLQuaternion &a, const LLQuaternion &b)
{
	return LLQuaternion( 
		a.mQ[VX] - b.mQ[VX],
		a.mQ[VY] - b.mQ[VY],
		a.mQ[VZ] - b.mQ[VZ],
		a.mQ[VW] - b.mQ[VW] );
}


inline LLQuaternion 	operator-(const LLQuaternion &a)
{
	return LLQuaternion(
		-a.mQ[VX],
		-a.mQ[VY],
		-a.mQ[VZ],
		-a.mQ[VW] );
}


inline LLQuaternion 	operator*(F32 a, const LLQuaternion &q)
{
	return LLQuaternion(
		a * q.mQ[VX],
		a * q.mQ[VY],
		a * q.mQ[VZ],
		a * q.mQ[VW] );
}


inline LLQuaternion 	operator*(const LLQuaternion &q, F32 a)
{
	return LLQuaternion(
		a * q.mQ[VX],
		a * q.mQ[VY],
		a * q.mQ[VZ],
		a * q.mQ[VW] );
}

inline LLQuaternion	operator~(const LLQuaternion &a)
{
	LLQuaternion q(a);
	q.conjQuat();
	return q;
}

inline bool	LLQuaternion::operator==(const LLQuaternion &b) const
{
	return (  (mQ[VX] == b.mQ[VX])
			&&(mQ[VY] == b.mQ[VY])
			&&(mQ[VZ] == b.mQ[VZ])
			&&(mQ[VS] == b.mQ[VS]));
}

inline bool	LLQuaternion::operator!=(const LLQuaternion &b) const
{
	return (  (mQ[VX] != b.mQ[VX])
			||(mQ[VY] != b.mQ[VY])
			||(mQ[VZ] != b.mQ[VZ])
			||(mQ[VS] != b.mQ[VS]));
}

inline const LLQuaternion&	operator*=(LLQuaternion &a, const LLQuaternion &b)
{
#if 1
	LLQuaternion q(
		b.mQ[3] * a.mQ[0] + b.mQ[0] * a.mQ[3] + b.mQ[1] * a.mQ[2] - b.mQ[2] * a.mQ[1],
		b.mQ[3] * a.mQ[1] + b.mQ[1] * a.mQ[3] + b.mQ[2] * a.mQ[0] - b.mQ[0] * a.mQ[2],
		b.mQ[3] * a.mQ[2] + b.mQ[2] * a.mQ[3] + b.mQ[0] * a.mQ[1] - b.mQ[1] * a.mQ[0],
		b.mQ[3] * a.mQ[3] - b.mQ[0] * a.mQ[0] - b.mQ[1] * a.mQ[1] - b.mQ[2] * a.mQ[2]
	);
	a = q;
#else
	a = a * b;
#endif
	return a;
}

const F32 ONE_PART_IN_A_MILLION = 0.000001f;

inline F32	LLQuaternion::normalize()
{
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);

	if (mag > FP_MAG_THRESHOLD)
	{
		// Floating point error can prevent some quaternions from achieving
		// exact unity length.  When trying to renormalize such quaternions we
		// can oscillate between multiple quantized states.  To prevent such
		// drifts we only renomalize if the length is far enough from unity.
		if (fabs(1.f - mag) > ONE_PART_IN_A_MILLION)
		{
			F32 oomag = 1.f/mag;
			mQ[VX] *= oomag;
			mQ[VY] *= oomag;
			mQ[VZ] *= oomag;
			mQ[VS] *= oomag;
		}
	}
	else
	{
		// we were given a very bad quaternion so we set it to identity
		mQ[VX] = 0.f;
		mQ[VY] = 0.f;
		mQ[VZ] = 0.f;
		mQ[VS] = 1.f;
	}

	return mag;
}

// deprecated
inline F32	LLQuaternion::normQuat()
{
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);

	if (mag > FP_MAG_THRESHOLD)
	{
		if (fabs(1.f - mag) > ONE_PART_IN_A_MILLION)
		{
			// only renormalize if length not close enough to 1.0 already
			F32 oomag = 1.f/mag;
			mQ[VX] *= oomag;
			mQ[VY] *= oomag;
			mQ[VZ] *= oomag;
			mQ[VS] *= oomag;
		}
	}
	else
	{
		mQ[VX] = 0.f;
		mQ[VY] = 0.f;
		mQ[VZ] = 0.f;
		mQ[VS] = 1.f;
	}

	return mag;
}

LLQuaternion::Order StringToOrder( const char *str );

// Some notes about Quaternions

// What is a Quaternion?
// ---------------------
// A quaternion is a point in 4-dimensional complex space.
// Q = { Qx, Qy, Qz, Qw }
// 
//
// Why Quaternions?
// ----------------
// The set of quaternions that make up the the 4-D unit sphere 
// can be mapped to the set of all rotations in 3-D space.  Sometimes
// it is easier to describe/manipulate rotations in quaternion space
// than rotation-matrix space.
//
//
// How Quaternions?
// ----------------
// In order to take advantage of quaternions we need to know how to
// go from rotation-matricies to quaternions and back.  We also have
// to agree what variety of rotations we're generating.
// 
// Consider the equation...   v' = v * R 
//
// There are two ways to think about rotations of vectors.
// 1) v' is the same vector in a different reference frame
// 2) v' is a new vector in the same reference frame
//
// bookmark -- which way are we using?
// 
// 
// Quaternion from Angle-Axis:
// ---------------------------
// Suppose we wanted to represent a rotation of some angle (theta) 
// about some axis ({Ax, Ay, Az})...
//
// axis of rotation = {Ax, Ay, Az} 
// angle_of_rotation = theta
//
// s = sin(0.5 * theta)
// c = cos(0.5 * theta)
// Q = { s * Ax, s * Ay, s * Az, c }
//
//
// 3x3 Matrix from Quaternion
// --------------------------
//
//     |                                                                    |
//     | 1 - 2 * (y^2 + z^2)   2 * (x * y + z * w)     2 * (y * w - x * z)  |
//     |                                                                    |
// M = | 2 * (x * y - z * w)   1 - 2 * (x^2 + z^2)     2 * (y * z + x * w)  |
//     |                                                                    |
//     | 2 * (x * z + y * w)   2 * (y * z - x * w)     1 - 2 * (x^2 + y^2)  |
//     |                                                                    |

#endif
