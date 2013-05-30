/** 
 * @file llquaternion.cpp
 * @brief LLQuaternion class implementation.
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

#include "linden_common.h"

#include "llmath.h"	// for F_PI

#include "llquaternion.h"

//#include "vmath.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquantize.h"

// WARNING: Don't use this for global const definitions!  using this
// at the top of a *.cpp file might not give you what you think.
const LLQuaternion LLQuaternion::DEFAULT;
 
// Constructors

LLQuaternion::LLQuaternion(const LLMatrix4 &mat)
{
	*this = mat.quaternion();
	normalize();
}

LLQuaternion::LLQuaternion(const LLMatrix3 &mat)
{
	*this = mat.quaternion();
	normalize();
}

LLQuaternion::LLQuaternion(F32 angle, const LLVector4 &vec)
{
	LLVector3 v(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
	v.normalize();

	F32 c, s;
	c = cosf(angle*0.5f);
	s = sinf(angle*0.5f);

	mQ[VX] = v.mV[VX] * s;
	mQ[VY] = v.mV[VY] * s;
	mQ[VZ] = v.mV[VZ] * s;
	mQ[VW] = c;
	normalize();
}

LLQuaternion::LLQuaternion(F32 angle, const LLVector3 &vec)
{
	LLVector3 v(vec);
	v.normalize();

	F32 c, s;
	c = cosf(angle*0.5f);
	s = sinf(angle*0.5f);

	mQ[VX] = v.mV[VX] * s;
	mQ[VY] = v.mV[VY] * s;
	mQ[VZ] = v.mV[VZ] * s;
	mQ[VW] = c;
	normalize();
}

LLQuaternion::LLQuaternion(const LLVector3 &x_axis,
						   const LLVector3 &y_axis,
						   const LLVector3 &z_axis)
{
	LLMatrix3 mat;
	mat.setRows(x_axis, y_axis, z_axis);
	*this = mat.quaternion();
	normalize();
}

// Quatizations
void	LLQuaternion::quantize16(F32 lower, F32 upper)
{
	F32 x = mQ[VX];
	F32 y = mQ[VY];
	F32 z = mQ[VZ];
	F32 s = mQ[VS];

	x = U16_to_F32(F32_to_U16_ROUND(x, lower, upper), lower, upper);
	y = U16_to_F32(F32_to_U16_ROUND(y, lower, upper), lower, upper);
	z = U16_to_F32(F32_to_U16_ROUND(z, lower, upper), lower, upper);
	s = U16_to_F32(F32_to_U16_ROUND(s, lower, upper), lower, upper);

	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = s;

	normalize();
}

void	LLQuaternion::quantize8(F32 lower, F32 upper)
{
	mQ[VX] = U8_to_F32(F32_to_U8_ROUND(mQ[VX], lower, upper), lower, upper);
	mQ[VY] = U8_to_F32(F32_to_U8_ROUND(mQ[VY], lower, upper), lower, upper);
	mQ[VZ] = U8_to_F32(F32_to_U8_ROUND(mQ[VZ], lower, upper), lower, upper);
	mQ[VS] = U8_to_F32(F32_to_U8_ROUND(mQ[VS], lower, upper), lower, upper);

	normalize();
}

// LLVector3 Magnitude and Normalization Functions


// Set LLQuaternion routines

const LLQuaternion&	LLQuaternion::setAngleAxis(F32 angle, F32 x, F32 y, F32 z)
{
	LLVector3 vec(x, y, z);
	vec.normalize();

	angle *= 0.5f;
	F32 c, s;
	c = cosf(angle);
	s = sinf(angle);

	mQ[VX] = vec.mV[VX]*s;
	mQ[VY] = vec.mV[VY]*s;
	mQ[VZ] = vec.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setAngleAxis(F32 angle, const LLVector3 &vec)
{
	LLVector3 v(vec);
	v.normalize();

	angle *= 0.5f;
	F32 c, s;
	c = cosf(angle);
	s = sinf(angle);

	mQ[VX] = v.mV[VX]*s;
	mQ[VY] = v.mV[VY]*s;
	mQ[VZ] = v.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setAngleAxis(F32 angle, const LLVector4 &vec)
{
	LLVector3 v(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
	v.normalize();

	F32 c, s;
	c = cosf(angle*0.5f);
	s = sinf(angle*0.5f);

	mQ[VX] = v.mV[VX]*s;
	mQ[VY] = v.mV[VY]*s;
	mQ[VZ] = v.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setEulerAngles(F32 roll, F32 pitch, F32 yaw)
{
	LLMatrix3 rot_mat(roll, pitch, yaw);
	rot_mat.orthogonalize();
	*this = rot_mat.quaternion();
		
	normalize();
	return (*this);
}

// deprecated
const LLQuaternion&	LLQuaternion::set(const LLMatrix3 &mat)
{
	*this = mat.quaternion();
	normalize();
	return (*this);
}

// deprecated
const LLQuaternion&	LLQuaternion::set(const LLMatrix4 &mat)
{
	*this = mat.quaternion();
	normalize();
	return (*this);
}

// deprecated
const LLQuaternion&	LLQuaternion::setQuat(F32 angle, F32 x, F32 y, F32 z)
{
	LLVector3 vec(x, y, z);
	vec.normalize();

	angle *= 0.5f;
	F32 c, s;
	c = cosf(angle);
	s = sinf(angle);

	mQ[VX] = vec.mV[VX]*s;
	mQ[VY] = vec.mV[VY]*s;
	mQ[VZ] = vec.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

// deprecated
const LLQuaternion&	LLQuaternion::setQuat(F32 angle, const LLVector3 &vec)
{
	LLVector3 v(vec);
	v.normalize();

	angle *= 0.5f;
	F32 c, s;
	c = cosf(angle);
	s = sinf(angle);

	mQ[VX] = v.mV[VX]*s;
	mQ[VY] = v.mV[VY]*s;
	mQ[VZ] = v.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setQuat(F32 angle, const LLVector4 &vec)
{
	LLVector3 v(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
	v.normalize();

	F32 c, s;
	c = cosf(angle*0.5f);
	s = sinf(angle*0.5f);

	mQ[VX] = v.mV[VX]*s;
	mQ[VY] = v.mV[VY]*s;
	mQ[VZ] = v.mV[VZ]*s;
	mQ[VW] = c;

	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setQuat(F32 roll, F32 pitch, F32 yaw)
{
	LLMatrix3 rot_mat(roll, pitch, yaw);
	rot_mat.orthogonalize();
	*this = rot_mat.quaternion();
		
	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setQuat(const LLMatrix3 &mat)
{
	*this = mat.quaternion();
	normalize();
	return (*this);
}

const LLQuaternion&	LLQuaternion::setQuat(const LLMatrix4 &mat)
{
	*this = mat.quaternion();
	normalize();
	return (*this);
//#if 1
//	// NOTE: LLQuaternion's are actually inverted with respect to
//	// the matrices, so this code also assumes inverted quaternions
//	// (-x, -y, -z, w). The result is that roll,pitch,yaw are applied
//	// in reverse order (yaw,pitch,roll).
//	F64 cosX = cos(roll);
//    F64 cosY = cos(pitch);
//    F64 cosZ = cos(yaw);
//
//    F64 sinX = sin(roll);
//    F64 sinY = sin(pitch);
//    F64 sinZ = sin(yaw);
//
//    mQ[VW] = (F32)sqrt(cosY*cosZ - sinX*sinY*sinZ + cosX*cosZ + cosX*cosY + 1.0)*.5;
//	if (fabs(mQ[VW]) < F_APPROXIMATELY_ZERO)
//	{
//		// null rotation, any axis will do
//		mQ[VX] = 0.0f;
//		mQ[VY] = 1.0f;
//		mQ[VZ] = 0.0f;
//	}
//	else
//	{
//		F32 inv_s = 1.0f / (4.0f * mQ[VW]);
//		mQ[VX] = (F32)-(-sinX*cosY - cosX*sinY*sinZ - sinX*cosZ) * inv_s;
//		mQ[VY] = (F32)-(-cosX*sinY*cosZ + sinX*sinZ - sinY) * inv_s;
//		mQ[VZ] = (F32)-(-cosY*sinZ - sinX*sinY*cosZ - cosX*sinZ) * inv_s;		
//	}
//
//#else // This only works on a certain subset of roll/pitch/yaw
//	
//	F64 cosX = cosf(roll/2.0);
//    F64 cosY = cosf(pitch/2.0);
//    F64 cosZ = cosf(yaw/2.0);
//
//    F64 sinX = sinf(roll/2.0);
//    F64 sinY = sinf(pitch/2.0);
//    F64 sinZ = sinf(yaw/2.0);
//
//    mQ[VW] = (F32)(cosX*cosY*cosZ + sinX*sinY*sinZ);
//    mQ[VX] = (F32)(sinX*cosY*cosZ - cosX*sinY*sinZ);
//    mQ[VY] = (F32)(cosX*sinY*cosZ + sinX*cosY*sinZ);
//    mQ[VZ] = (F32)(cosX*cosY*sinZ - sinX*sinY*cosZ);
//#endif
//
//	normalize();
//	return (*this);
}

// SJB: This code is correct for a logicly stored (non-transposed) matrix;
//		Our matrices are stored transposed, OpenGL style, so this generates the
//		INVERSE matrix, or the CORRECT matrix form an INVERSE quaternion.
//		Because we use similar logic in LLMatrix3::quaternion(),
//		we are internally consistant so everything works OK :)
LLMatrix3	LLQuaternion::getMatrix3(void) const
{
	LLMatrix3	mat;
	F32		xx, xy, xz, xw, yy, yz, yw, zz, zw;

    xx      = mQ[VX] * mQ[VX];
    xy      = mQ[VX] * mQ[VY];
    xz      = mQ[VX] * mQ[VZ];
    xw      = mQ[VX] * mQ[VW];

    yy      = mQ[VY] * mQ[VY];
    yz      = mQ[VY] * mQ[VZ];
    yw      = mQ[VY] * mQ[VW];

    zz      = mQ[VZ] * mQ[VZ];
    zw      = mQ[VZ] * mQ[VW];

    mat.mMatrix[0][0]  = 1.f - 2.f * ( yy + zz );
    mat.mMatrix[0][1]  =	   2.f * ( xy + zw );
    mat.mMatrix[0][2]  =	   2.f * ( xz - yw );

    mat.mMatrix[1][0]  =	   2.f * ( xy - zw );
    mat.mMatrix[1][1]  = 1.f - 2.f * ( xx + zz );
    mat.mMatrix[1][2]  =	   2.f * ( yz + xw );

    mat.mMatrix[2][0]  =	   2.f * ( xz + yw );
    mat.mMatrix[2][1]  =	   2.f * ( yz - xw );
    mat.mMatrix[2][2]  = 1.f - 2.f * ( xx + yy );

	return mat;
}

LLMatrix4	LLQuaternion::getMatrix4(void) const
{
	LLMatrix4	mat;
	F32		xx, xy, xz, xw, yy, yz, yw, zz, zw;

    xx      = mQ[VX] * mQ[VX];
    xy      = mQ[VX] * mQ[VY];
    xz      = mQ[VX] * mQ[VZ];
    xw      = mQ[VX] * mQ[VW];

    yy      = mQ[VY] * mQ[VY];
    yz      = mQ[VY] * mQ[VZ];
    yw      = mQ[VY] * mQ[VW];

    zz      = mQ[VZ] * mQ[VZ];
    zw      = mQ[VZ] * mQ[VW];

    mat.mMatrix[0][0]  = 1.f - 2.f * ( yy + zz );
    mat.mMatrix[0][1]  =	   2.f * ( xy + zw );
    mat.mMatrix[0][2]  =	   2.f * ( xz - yw );

    mat.mMatrix[1][0]  =	   2.f * ( xy - zw );
    mat.mMatrix[1][1]  = 1.f - 2.f * ( xx + zz );
    mat.mMatrix[1][2]  =	   2.f * ( yz + xw );

    mat.mMatrix[2][0]  =	   2.f * ( xz + yw );
    mat.mMatrix[2][1]  =	   2.f * ( yz - xw );
    mat.mMatrix[2][2]  = 1.f - 2.f * ( xx + yy );

	// TODO -- should we set the translation portion to zero?

	return mat;
}




// Other useful methods


// calculate the shortest rotation from a to b
void LLQuaternion::shortestArc(const LLVector3 &a, const LLVector3 &b)
{
	// Make a local copy of both vectors.
	LLVector3 vec_a = a;
	LLVector3 vec_b = b;

	// Make sure neither vector is zero length.  Also normalize
	// the vectors while we are at it.
	F32 vec_a_mag = vec_a.normalize();
	F32 vec_b_mag = vec_b.normalize();
	if (vec_a_mag < F_APPROXIMATELY_ZERO ||
		vec_b_mag < F_APPROXIMATELY_ZERO)
	{
		// Can't calculate a rotation from this.
		// Just return ZERO_ROTATION instead.
		loadIdentity();
		return;
	}

	// Create an axis to rotate around, and the cos of the angle to rotate.
	LLVector3 axis = vec_a % vec_b;
	F32 cos_theta  = vec_a * vec_b;

	// Check the angle between the vectors to see if they are parallel or anti-parallel.
	if (cos_theta > 1.0 - F_APPROXIMATELY_ZERO)
	{
		// a and b are parallel.  No rotation is necessary.
		loadIdentity();
	}
	else if (cos_theta < -1.0 + F_APPROXIMATELY_ZERO)
	{
		// a and b are anti-parallel.
		// Rotate 180 degrees around some orthogonal axis.
		// Find the projection of the x-axis onto a, and try
		// using the vector between the projection and the x-axis
		// as the orthogonal axis.
		LLVector3 proj = vec_a.mV[VX] / (vec_a * vec_a) * vec_a;
		LLVector3 ortho_axis(1.f, 0.f, 0.f);
		ortho_axis -= proj;
		
		// Turn this into an orthonormal axis.
		F32 ortho_length = ortho_axis.normalize();
		// If the axis' length is 0, then our guess at an orthogonal axis
		// was wrong (a is parallel to the x-axis).
		if (ortho_length < F_APPROXIMATELY_ZERO)
		{
			// Use the z-axis instead.
			ortho_axis.setVec(0.f, 0.f, 1.f);
		}

		// Construct a quaternion from this orthonormal axis.
		mQ[VX] = ortho_axis.mV[VX];
		mQ[VY] = ortho_axis.mV[VY];
		mQ[VZ] = ortho_axis.mV[VZ];
		mQ[VW] = 0.f;
	}
	else
	{
		// a and b are NOT parallel or anti-parallel.
		// Return the rotation between these vectors.
		F32 theta = (F32)acos(cos_theta);

		setAngleAxis(theta, axis);
	}
}

// constrains rotation to a cone angle specified in radians
const LLQuaternion &LLQuaternion::constrain(F32 radians)
{
	const F32 cos_angle_lim = cosf( radians/2 );	// mQ[VW] limit
	const F32 sin_angle_lim = sinf( radians/2 );	// rotation axis length	limit

	if (mQ[VW] < 0.f)
	{
		mQ[VX] *= -1.f;
		mQ[VY] *= -1.f;
		mQ[VZ] *= -1.f;
		mQ[VW] *= -1.f;
	}

	// if rotation angle is greater than limit (cos is less than limit)
	if( mQ[VW] < cos_angle_lim )
	{
		mQ[VW] = cos_angle_lim;
		F32 axis_len = sqrtf( mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] ); // sin(theta/2)
		F32 axis_mult_fact = sin_angle_lim / axis_len;
		mQ[VX] *= axis_mult_fact;
		mQ[VY] *= axis_mult_fact;
		mQ[VZ] *= axis_mult_fact;
	}

	return *this;
}

// Operators

std::ostream& operator<<(std::ostream &s, const LLQuaternion &a)
{
	s << "{ " 
		<< a.mQ[VX] << ", " << a.mQ[VY] << ", " << a.mQ[VZ] << ", " << a.mQ[VW] 
	<< " }";
	return s;
}


// Does NOT renormalize the result
LLQuaternion	operator*(const LLQuaternion &a, const LLQuaternion &b)
{
//	LLQuaternion::mMultCount++;

	LLQuaternion q(
		b.mQ[3] * a.mQ[0] + b.mQ[0] * a.mQ[3] + b.mQ[1] * a.mQ[2] - b.mQ[2] * a.mQ[1],
		b.mQ[3] * a.mQ[1] + b.mQ[1] * a.mQ[3] + b.mQ[2] * a.mQ[0] - b.mQ[0] * a.mQ[2],
		b.mQ[3] * a.mQ[2] + b.mQ[2] * a.mQ[3] + b.mQ[0] * a.mQ[1] - b.mQ[1] * a.mQ[0],
		b.mQ[3] * a.mQ[3] - b.mQ[0] * a.mQ[0] - b.mQ[1] * a.mQ[1] - b.mQ[2] * a.mQ[2]
	);
	return q;
}

/*
LLMatrix4	operator*(const LLMatrix4 &m, const LLQuaternion &q)
{
	LLMatrix4 qmat(q);
	return (m*qmat);
}
*/



LLVector4		operator*(const LLVector4 &a, const LLQuaternion &rot)
{
    F32 rw = - rot.mQ[VX] * a.mV[VX] - rot.mQ[VY] * a.mV[VY] - rot.mQ[VZ] * a.mV[VZ];
    F32 rx =   rot.mQ[VW] * a.mV[VX] + rot.mQ[VY] * a.mV[VZ] - rot.mQ[VZ] * a.mV[VY];
    F32 ry =   rot.mQ[VW] * a.mV[VY] + rot.mQ[VZ] * a.mV[VX] - rot.mQ[VX] * a.mV[VZ];
    F32 rz =   rot.mQ[VW] * a.mV[VZ] + rot.mQ[VX] * a.mV[VY] - rot.mQ[VY] * a.mV[VX];

    F32 nx = - rw * rot.mQ[VX] +  rx * rot.mQ[VW] - ry * rot.mQ[VZ] + rz * rot.mQ[VY];
    F32 ny = - rw * rot.mQ[VY] +  ry * rot.mQ[VW] - rz * rot.mQ[VX] + rx * rot.mQ[VZ];
    F32 nz = - rw * rot.mQ[VZ] +  rz * rot.mQ[VW] - rx * rot.mQ[VY] + ry * rot.mQ[VX];

    return LLVector4(nx, ny, nz, a.mV[VW]);
}

LLVector3		operator*(const LLVector3 &a, const LLQuaternion &rot)
{
    F32 rw = - rot.mQ[VX] * a.mV[VX] - rot.mQ[VY] * a.mV[VY] - rot.mQ[VZ] * a.mV[VZ];
    F32 rx =   rot.mQ[VW] * a.mV[VX] + rot.mQ[VY] * a.mV[VZ] - rot.mQ[VZ] * a.mV[VY];
    F32 ry =   rot.mQ[VW] * a.mV[VY] + rot.mQ[VZ] * a.mV[VX] - rot.mQ[VX] * a.mV[VZ];
    F32 rz =   rot.mQ[VW] * a.mV[VZ] + rot.mQ[VX] * a.mV[VY] - rot.mQ[VY] * a.mV[VX];

    F32 nx = - rw * rot.mQ[VX] +  rx * rot.mQ[VW] - ry * rot.mQ[VZ] + rz * rot.mQ[VY];
    F32 ny = - rw * rot.mQ[VY] +  ry * rot.mQ[VW] - rz * rot.mQ[VX] + rx * rot.mQ[VZ];
    F32 nz = - rw * rot.mQ[VZ] +  rz * rot.mQ[VW] - rx * rot.mQ[VY] + ry * rot.mQ[VX];

    return LLVector3(nx, ny, nz);
}

LLVector3d		operator*(const LLVector3d &a, const LLQuaternion &rot)
{
    F64 rw = - rot.mQ[VX] * a.mdV[VX] - rot.mQ[VY] * a.mdV[VY] - rot.mQ[VZ] * a.mdV[VZ];
    F64 rx =   rot.mQ[VW] * a.mdV[VX] + rot.mQ[VY] * a.mdV[VZ] - rot.mQ[VZ] * a.mdV[VY];
    F64 ry =   rot.mQ[VW] * a.mdV[VY] + rot.mQ[VZ] * a.mdV[VX] - rot.mQ[VX] * a.mdV[VZ];
    F64 rz =   rot.mQ[VW] * a.mdV[VZ] + rot.mQ[VX] * a.mdV[VY] - rot.mQ[VY] * a.mdV[VX];

    F64 nx = - rw * rot.mQ[VX] +  rx * rot.mQ[VW] - ry * rot.mQ[VZ] + rz * rot.mQ[VY];
    F64 ny = - rw * rot.mQ[VY] +  ry * rot.mQ[VW] - rz * rot.mQ[VX] + rx * rot.mQ[VZ];
    F64 nz = - rw * rot.mQ[VZ] +  rz * rot.mQ[VW] - rx * rot.mQ[VY] + ry * rot.mQ[VX];

    return LLVector3d(nx, ny, nz);
}

F32 dot(const LLQuaternion &a, const LLQuaternion &b)
{
	return a.mQ[VX] * b.mQ[VX] + 
		   a.mQ[VY] * b.mQ[VY] + 
		   a.mQ[VZ] * b.mQ[VZ] + 
		   a.mQ[VW] * b.mQ[VW]; 
}

// DEMO HACK: This lerp is probably inocrrect now due intermediate normalization
// it should look more like the lerp below
#if 0
// linear interpolation
LLQuaternion lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q)
{
	LLQuaternion r;
	r = t * (q - p) + p;
	r.normalize();
	return r;
}
#endif

// lerp from identity to q
LLQuaternion lerp(F32 t, const LLQuaternion &q)
{
	LLQuaternion r;
	r.mQ[VX] = t * q.mQ[VX];
	r.mQ[VY] = t * q.mQ[VY];
	r.mQ[VZ] = t * q.mQ[VZ];
	r.mQ[VW] = t * (q.mQ[VZ] - 1.f) + 1.f;
	r.normalize();
	return r;
}

LLQuaternion lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q)
{
	LLQuaternion r;
	F32 inv_t;

	inv_t = 1.f - t;

	r.mQ[VX] = t * q.mQ[VX] + (inv_t * p.mQ[VX]);
	r.mQ[VY] = t * q.mQ[VY] + (inv_t * p.mQ[VY]);
	r.mQ[VZ] = t * q.mQ[VZ] + (inv_t * p.mQ[VZ]);
	r.mQ[VW] = t * q.mQ[VW] + (inv_t * p.mQ[VW]);
	r.normalize();
	return r;
}


// spherical linear interpolation
LLQuaternion slerp( F32 u, const LLQuaternion &a, const LLQuaternion &b )
{
	// cosine theta = dot product of a and b
	F32 cos_t = a.mQ[0]*b.mQ[0] + a.mQ[1]*b.mQ[1] + a.mQ[2]*b.mQ[2] + a.mQ[3]*b.mQ[3];
	
	// if b is on opposite hemisphere from a, use -a instead
	int bflip;
 	if (cos_t < 0.0f)
	{
		cos_t = -cos_t;
		bflip = TRUE;
	}
	else
		bflip = FALSE;

	// if B is (within precision limits) the same as A,
	// just linear interpolate between A and B.
	F32 alpha;	// interpolant
	F32 beta;		// 1 - interpolant
	if (1.0f - cos_t < 0.00001f)
	{
		beta = 1.0f - u;
		alpha = u;
 	}
	else
	{
 		F32 theta = acosf(cos_t);
 		F32 sin_t = sinf(theta);
 		beta = sinf(theta - u*theta) / sin_t;
 		alpha = sinf(u*theta) / sin_t;
 	}

	if (bflip)
		beta = -beta;

	// interpolate
	LLQuaternion ret;
	ret.mQ[0] = beta*a.mQ[0] + alpha*b.mQ[0];
 	ret.mQ[1] = beta*a.mQ[1] + alpha*b.mQ[1];
 	ret.mQ[2] = beta*a.mQ[2] + alpha*b.mQ[2];
 	ret.mQ[3] = beta*a.mQ[3] + alpha*b.mQ[3];

	return ret;
}

// lerp whenever possible
LLQuaternion nlerp(F32 t, const LLQuaternion &a, const LLQuaternion &b)
{
	if (dot(a, b) < 0.f)
	{
		return slerp(t, a, b);
	}
	else
	{
		return lerp(t, a, b);
	}
}

LLQuaternion nlerp(F32 t, const LLQuaternion &q)
{
	if (q.mQ[VW] < 0.f)
	{
		return slerp(t, q);
	}
	else
	{
		return lerp(t, q);
	}
}

// slerp from identity quaternion to another quaternion
LLQuaternion slerp(F32 t, const LLQuaternion &q)
{
	F32 c = q.mQ[VW];
	if (1.0f == t  ||  1.0f == c)
	{
		// the trivial cases
		return q;
	}

	LLQuaternion r;
	F32 s, angle, stq, stp;

	s = (F32) sqrt(1.f - c*c);

    if (c < 0.0f)
    {
        // when c < 0.0 then theta > PI/2 
        // since quat and -quat are the same rotation we invert one of  
        // p or q to reduce unecessary spins
        // A equivalent way to do it is to convert acos(c) as if it had 
		// been negative, and to negate stp 
        angle   = (F32) acos(-c); 
        stp     = -(F32) sin(angle * (1.f - t));
        stq     = (F32) sin(angle * t);
    }   
    else
    {
		angle 	= (F32) acos(c);
        stp     = (F32) sin(angle * (1.f - t));
        stq     = (F32) sin(angle * t);
    }

	r.mQ[VX] = (q.mQ[VX] * stq) / s;
	r.mQ[VY] = (q.mQ[VY] * stq) / s;
	r.mQ[VZ] = (q.mQ[VZ] * stq) / s;
	r.mQ[VW] = (stp + q.mQ[VW] * stq) / s;

	return r;
}

LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order)
{
	LLQuaternion xQ( xRot*DEG_TO_RAD, LLVector3(1.0f, 0.0f, 0.0f) );
	LLQuaternion yQ( yRot*DEG_TO_RAD, LLVector3(0.0f, 1.0f, 0.0f) );
	LLQuaternion zQ( zRot*DEG_TO_RAD, LLVector3(0.0f, 0.0f, 1.0f) );
	LLQuaternion ret;
	switch( order )
	{
	case LLQuaternion::XYZ:
		ret = xQ * yQ * zQ;
		break;
	case LLQuaternion::YZX:
		ret = yQ * zQ * xQ;
		break;
	case LLQuaternion::ZXY:
		ret = zQ * xQ * yQ;
		break;
	case LLQuaternion::XZY:
		ret = xQ * zQ * yQ;
		break;
	case LLQuaternion::YXZ:
		ret = yQ * xQ * zQ;
		break;
	case LLQuaternion::ZYX:
		ret = zQ * yQ * xQ;
		break;
	}
	return ret;
}

const char *OrderToString( const LLQuaternion::Order order )
{
	const char *p = NULL;
	switch( order )
	{
	default:
	case LLQuaternion::XYZ:
		p = "XYZ";
		break;
	case LLQuaternion::YZX:
		p = "YZX";
		break;
	case LLQuaternion::ZXY:
		p = "ZXY";
		break;
	case LLQuaternion::XZY:
		p = "XZY";
		break;
	case LLQuaternion::YXZ:
		p = "YXZ";
		break;
	case LLQuaternion::ZYX:
		p = "ZYX";
		break;
	}
	return p;
}

LLQuaternion::Order StringToOrder( const char *str )
{
	if (strncmp(str, "XYZ", 3)==0 || strncmp(str, "xyz", 3)==0)
		return LLQuaternion::XYZ;

	if (strncmp(str, "YZX", 3)==0 || strncmp(str, "yzx", 3)==0)
		return LLQuaternion::YZX;

	if (strncmp(str, "ZXY", 3)==0 || strncmp(str, "zxy", 3)==0)
		return LLQuaternion::ZXY;

	if (strncmp(str, "XZY", 3)==0 || strncmp(str, "xzy", 3)==0)
		return LLQuaternion::XZY;

	if (strncmp(str, "YXZ", 3)==0 || strncmp(str, "yxz", 3)==0)
		return LLQuaternion::YXZ;

	if (strncmp(str, "ZYX", 3)==0 || strncmp(str, "zyx", 3)==0)
		return LLQuaternion::ZYX;

	return LLQuaternion::XYZ;
}

void LLQuaternion::getAngleAxis(F32* angle, LLVector3 &vec) const
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
    	vec.mV[VX] = - mQ[VX] * sin_a;
    	vec.mV[VY] = - mQ[VY] * sin_a;
    	vec.mV[VZ] = - mQ[VZ] * sin_a;
	}
	else
	{
		*angle = temp_angle;
    	vec.mV[VX] = mQ[VX] * sin_a;
    	vec.mV[VY] = mQ[VY] * sin_a;
    	vec.mV[VZ] = mQ[VZ] * sin_a;
	}
}


// quaternion does not need to be normalized
void LLQuaternion::getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) const
{
	LLMatrix3 rot_mat(*this);
	rot_mat.orthogonalize();
	rot_mat.getEulerAngles(roll, pitch, yaw);

//	// NOTE: LLQuaternion's are actually inverted with respect to
//	// the matrices, so this code also assumes inverted quaternions
//	// (-x, -y, -z, w). The result is that roll,pitch,yaw are applied
//	// in reverse order (yaw,pitch,roll).
//	F32 x = -mQ[VX], y = -mQ[VY], z = -mQ[VZ], w = mQ[VW];
//	F64 m20 = 2.0*(x*z-y*w);
//	if (1.0f - fabsf(m20) < F_APPROXIMATELY_ZERO)
//	{
//		*roll = 0.0f;
//		*pitch = (F32)asin(m20);
//		*yaw = (F32)atan2(2.0*(x*y-z*w), 1.0 - 2.0*(x*x+z*z));
//	}
//	else
//	{
//		*roll  = (F32)atan2(-2.0*(y*z+x*w), 1.0-2.0*(x*x+y*y));
//		*pitch = (F32)asin(m20);
//		*yaw   = (F32)atan2(-2.0*(x*y+z*w), 1.0-2.0*(y*y+z*z));
//	}
}

// Saves space by using the fact that our quaternions are normalized
LLVector3 LLQuaternion::packToVector3() const
{
	if( mQ[VW] >= 0 )
	{
		return LLVector3( mQ[VX], mQ[VY], mQ[VZ] );
	}
	else
	{
		return LLVector3( -mQ[VX], -mQ[VY], -mQ[VZ] );
	}
}

// Saves space by using the fact that our quaternions are normalized
void LLQuaternion::unpackFromVector3( const LLVector3& vec )
{
	mQ[VX] = vec.mV[VX];
	mQ[VY] = vec.mV[VY];
	mQ[VZ] = vec.mV[VZ];
	F32 t = 1.f - vec.magVecSquared();
	if( t > 0 )
	{
		mQ[VW] = sqrt( t );
	}
	else
	{
		// Need this to avoid trying to find the square root of a negative number due
		// to floating point error.
		mQ[VW] = 0;
	}
}

BOOL LLQuaternion::parseQuat(const std::string& buf, LLQuaternion* value)
{
	if( buf.empty() || value == NULL)
	{
		return FALSE;
	}

	LLQuaternion quat;
	S32 count = sscanf( buf.c_str(), "%f %f %f %f", quat.mQ + 0, quat.mQ + 1, quat.mQ + 2, quat.mQ + 3 );
	if( 4 == count )
	{
		value->set( quat );
		return TRUE;
	}

	return FALSE;
}


// End
