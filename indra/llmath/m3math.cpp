/** 
 * @file m3math.cpp
 * @brief LLMatrix3 class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

//#include "vmath.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"

// LLMatrix3

//              ji  
// LLMatrix3 = |00 01 02 |
//             |10 11 12 |
//             |20 21 22 |

// LLMatrix3 = |fx fy fz |  forward-axis
//             |lx ly lz |  left-axis
//             |ux uy uz |  up-axis


// Constructors


LLMatrix3::LLMatrix3(const LLQuaternion &q)
{
	setRot(q);
}


LLMatrix3::LLMatrix3(const F32 angle, const LLVector3 &vec)
{
	LLQuaternion	quat(angle, vec);
	setRot(quat);
}

LLMatrix3::LLMatrix3(const F32 angle, const LLVector3d &vec)
{
	LLVector3 vec_f;
	vec_f.setVec(vec);
	LLQuaternion	quat(angle, vec_f);
	setRot(quat);
}

LLMatrix3::LLMatrix3(const F32 angle, const LLVector4 &vec)
{
	LLQuaternion	quat(angle, vec);
	setRot(quat);
}

LLMatrix3::LLMatrix3(const F32 angle, const F32 x, const F32 y, const F32 z)
{
	LLVector3 vec(x, y, z);
	LLQuaternion	quat(angle, vec);
	setRot(quat);
}

LLMatrix3::LLMatrix3(const F32 roll, const F32 pitch, const F32 yaw)
{
	setRot(roll,pitch,yaw);
}

// From Matrix and Quaternion FAQ
void LLMatrix3::getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) const
{
	F64 angle_x, angle_y, angle_z;
	F64 cx, cy, cz;					// cosine of angle_x, angle_y, angle_z
	F64 sx,     sz;					// sine of angle_x, angle_y, angle_z

	angle_y = asin(llclamp(mMatrix[2][0], -1.f, 1.f));
	cy = cos(angle_y);

	if (fabs(cy) > 0.005)		// non-zero
	{
		// no gimbal lock
		cx = mMatrix[2][2] / cy;
		sx = - mMatrix[2][1] / cy;

		angle_x = (F32) atan2(sx, cx);

		cz = mMatrix[0][0] / cy;
		sz = - mMatrix[1][0] / cy;

		angle_z = (F32) atan2(sz, cz);
	}
	else
	{
		// yup, gimbal lock
		angle_x = 0;

		// some tricky math thereby avoided, see article

		cz = mMatrix[1][1];
		sz = mMatrix[0][1];

		angle_z = atan2(sz, cz);
	}

	*roll = (F32)angle_x;
	*pitch = (F32)angle_y;
	*yaw = (F32)angle_z;
}
	

// Clear and Assignment Functions

const LLMatrix3&	LLMatrix3::setIdentity()
{
	mMatrix[0][0] = 1.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;

	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 1.f;
	mMatrix[1][2] = 0.f;

	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 1.f;
	return (*this);
}

const LLMatrix3&	LLMatrix3::clear()
{
	mMatrix[0][0] = 0.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;

	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 0.f;
	mMatrix[1][2] = 0.f;

	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 0.f;
	return (*this);
}

const LLMatrix3&	LLMatrix3::setZero()
{
	mMatrix[0][0] = 0.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;

	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 0.f;
	mMatrix[1][2] = 0.f;

	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 0.f;
	return (*this);
}

// various useful mMatrix functions

const LLMatrix3&	LLMatrix3::transpose() 
{
	// transpose the matrix
	F32 temp;
	temp = mMatrix[VX][VY]; mMatrix[VX][VY] = mMatrix[VY][VX]; mMatrix[VY][VX] = temp;
	temp = mMatrix[VX][VZ]; mMatrix[VX][VZ] = mMatrix[VZ][VX]; mMatrix[VZ][VX] = temp;
	temp = mMatrix[VY][VZ]; mMatrix[VY][VZ] = mMatrix[VZ][VY]; mMatrix[VZ][VY] = temp;
	return *this;
}


F32		LLMatrix3::determinant() const
{
	// Is this a useful method when we assume the matrices are valid rotation
	// matrices throughout this implementation?
	return	mMatrix[0][0] * (mMatrix[1][1] * mMatrix[2][2] - mMatrix[1][2] * mMatrix[2][1]) +
		  	mMatrix[0][1] * (mMatrix[1][2] * mMatrix[2][0] - mMatrix[1][0] * mMatrix[2][2]) +
		  	mMatrix[0][2] * (mMatrix[1][0] * mMatrix[2][1] - mMatrix[1][1] * mMatrix[2][0]); 
}

// inverts this matrix
void LLMatrix3::invert()
{
	// fails silently if determinant is zero too small
	F32 det = determinant();
	const F32 VERY_SMALL_DETERMINANT = 0.000001f;
	if (fabs(det) > VERY_SMALL_DETERMINANT)
	{
		// invertiable
		LLMatrix3 t(*this);
		mMatrix[VX][VX] = ( t.mMatrix[VY][VY] * t.mMatrix[VZ][VZ] - t.mMatrix[VY][VZ] * t.mMatrix[VZ][VY] ) / det;
		mMatrix[VY][VX] = ( t.mMatrix[VY][VZ] * t.mMatrix[VZ][VX] - t.mMatrix[VY][VX] * t.mMatrix[VZ][VZ] ) / det;
		mMatrix[VZ][VX] = ( t.mMatrix[VY][VX] * t.mMatrix[VZ][VY] - t.mMatrix[VY][VY] * t.mMatrix[VZ][VX] ) / det;
		mMatrix[VX][VY] = ( t.mMatrix[VZ][VY] * t.mMatrix[VX][VZ] - t.mMatrix[VZ][VZ] * t.mMatrix[VX][VY] ) / det;
		mMatrix[VY][VY] = ( t.mMatrix[VZ][VZ] * t.mMatrix[VX][VX] - t.mMatrix[VZ][VX] * t.mMatrix[VX][VZ] ) / det;
		mMatrix[VZ][VY] = ( t.mMatrix[VZ][VX] * t.mMatrix[VX][VY] - t.mMatrix[VZ][VY] * t.mMatrix[VX][VX] ) / det;
		mMatrix[VX][VZ] = ( t.mMatrix[VX][VY] * t.mMatrix[VY][VZ] - t.mMatrix[VX][VZ] * t.mMatrix[VY][VY] ) / det;
		mMatrix[VY][VZ] = ( t.mMatrix[VX][VZ] * t.mMatrix[VY][VX] - t.mMatrix[VX][VX] * t.mMatrix[VY][VZ] ) / det;
		mMatrix[VZ][VZ] = ( t.mMatrix[VX][VX] * t.mMatrix[VY][VY] - t.mMatrix[VX][VY] * t.mMatrix[VY][VX] ) / det;
	}
}

// does not assume a rotation matrix, and does not divide by determinant, assuming results will be renormalized
const LLMatrix3&	LLMatrix3::adjointTranspose()
{
	LLMatrix3 adjoint_transpose;
	adjoint_transpose.mMatrix[VX][VX] = mMatrix[VY][VY] * mMatrix[VZ][VZ] - mMatrix[VY][VZ] * mMatrix[VZ][VY] ;
	adjoint_transpose.mMatrix[VY][VX] = mMatrix[VY][VZ] * mMatrix[VZ][VX] - mMatrix[VY][VX] * mMatrix[VZ][VZ] ;
	adjoint_transpose.mMatrix[VZ][VX] = mMatrix[VY][VX] * mMatrix[VZ][VY] - mMatrix[VY][VY] * mMatrix[VZ][VX] ;
	adjoint_transpose.mMatrix[VX][VY] = mMatrix[VZ][VY] * mMatrix[VX][VZ] - mMatrix[VZ][VZ] * mMatrix[VX][VY] ;
	adjoint_transpose.mMatrix[VY][VY] = mMatrix[VZ][VZ] * mMatrix[VX][VX] - mMatrix[VZ][VX] * mMatrix[VX][VZ] ;
	adjoint_transpose.mMatrix[VZ][VY] = mMatrix[VZ][VX] * mMatrix[VX][VY] - mMatrix[VZ][VY] * mMatrix[VX][VX] ;
	adjoint_transpose.mMatrix[VX][VZ] = mMatrix[VX][VY] * mMatrix[VY][VZ] - mMatrix[VX][VZ] * mMatrix[VY][VY] ;
	adjoint_transpose.mMatrix[VY][VZ] = mMatrix[VX][VZ] * mMatrix[VY][VX] - mMatrix[VX][VX] * mMatrix[VY][VZ] ;
	adjoint_transpose.mMatrix[VZ][VZ] = mMatrix[VX][VX] * mMatrix[VY][VY] - mMatrix[VX][VY] * mMatrix[VY][VX] ;

	*this = adjoint_transpose;
	return *this;
}

// SJB: This code is correct for a logicly stored (non-transposed) matrix;
//		Our matrices are stored transposed, OpenGL style, so this generates the
//		INVERSE quaternion (-x, -y, -z, w)!
//		Because we use similar logic in LLQuaternion::getMatrix3,
//		we are internally consistant so everything works OK :)
LLQuaternion	LLMatrix3::quaternion() const
{
	LLQuaternion	quat;
	F32		tr, s, q[4];
	U32		i, j, k;
	U32		nxt[3] = {1, 2, 0};

	tr = mMatrix[0][0] + mMatrix[1][1] + mMatrix[2][2];

	// check the diagonal
	if (tr > 0.f) 
	{
		s = (F32)sqrt (tr + 1.f);
		quat.mQ[VS] = s / 2.f;
		s = 0.5f / s;
		quat.mQ[VX] = (mMatrix[1][2] - mMatrix[2][1]) * s;
		quat.mQ[VY] = (mMatrix[2][0] - mMatrix[0][2]) * s;
		quat.mQ[VZ] = (mMatrix[0][1] - mMatrix[1][0]) * s;
	} 
	else
	{		
		// diagonal is negative
		i = 0;
		if (mMatrix[1][1] > mMatrix[0][0]) 
			i = 1;
		if (mMatrix[2][2] > mMatrix[i][i]) 
			i = 2;

		j = nxt[i];
		k = nxt[j];


		s = (F32)sqrt ((mMatrix[i][i] - (mMatrix[j][j] + mMatrix[k][k])) + 1.f);

		q[i] = s * 0.5f;

		if (s != 0.f) 
			s = 0.5f / s;

		q[3] = (mMatrix[j][k] - mMatrix[k][j]) * s;
		q[j] = (mMatrix[i][j] + mMatrix[j][i]) * s;
		q[k] = (mMatrix[i][k] + mMatrix[k][i]) * s;

		quat.setQuat(q);
	}
	return quat;
}


// These functions take Rotation arguments
const LLMatrix3&	LLMatrix3::setRot(const F32 angle, const F32 x, const F32 y, const F32 z)
{
	setRot(LLQuaternion(angle,x,y,z));
	return *this;
}

const LLMatrix3&	LLMatrix3::setRot(const F32 angle, const LLVector3 &vec)
{
	setRot(LLQuaternion(angle, vec));
	return *this;
}

const LLMatrix3&	LLMatrix3::setRot(const F32 roll, const F32 pitch, const F32 yaw)
{
	// Rotates RH about x-axis by 'roll'  then
	// rotates RH about the old y-axis by 'pitch' then
	// rotates RH about the original z-axis by 'yaw'.
	//                .
	//               /|\ yaw axis
	//                |     __.
	//   ._        ___|      /| pitch axis
	//  _||\       \\ |-.   /
	//  \|| \_______\_|__\_/_______
	//   | _ _   o o o_o_o_o o   /_\_  ________\ roll axis
	//   //  /_______/    /__________>         /   
	//  /_,-'       //   /
	//             /__,-'

	F32		cx, sx, cy, sy, cz, sz;
	F32		cxsy, sxsy;

    cx = (F32)cos(roll); //A
    sx = (F32)sin(roll); //B
    cy = (F32)cos(pitch); //C
    sy = (F32)sin(pitch); //D
    cz = (F32)cos(yaw); //E
    sz = (F32)sin(yaw); //F

    cxsy = cx * sy; //AD
    sxsy = sx * sy; //BD 

    mMatrix[0][0] =  cy * cz;
    mMatrix[1][0] = -cy * sz;
    mMatrix[2][0] = sy;
    mMatrix[0][1] = sxsy * cz + cx * sz;
    mMatrix[1][1] = -sxsy * sz + cx * cz;
    mMatrix[2][1] = -sx * cy;
    mMatrix[0][2] =  -cxsy * cz + sx * sz;
    mMatrix[1][2] =  cxsy * sz + sx * cz;
    mMatrix[2][2] =  cx * cy;
	return *this;
}


const LLMatrix3&	LLMatrix3::setRot(const LLQuaternion &q)
{
	*this = q.getMatrix3();
	return *this;
}

const LLMatrix3&	LLMatrix3::setRows(const LLVector3 &fwd, const LLVector3 &left, const LLVector3 &up)
{
	mMatrix[0][0] = fwd.mV[0];
	mMatrix[0][1] = fwd.mV[1];
	mMatrix[0][2] = fwd.mV[2];

	mMatrix[1][0] = left.mV[0];
	mMatrix[1][1] = left.mV[1];
	mMatrix[1][2] = left.mV[2];

	mMatrix[2][0] = up.mV[0];
	mMatrix[2][1] = up.mV[1];
	mMatrix[2][2] = up.mV[2];

	return *this;
}

const LLMatrix3& LLMatrix3::setRow( U32 rowIndex, const LLVector3& row )
{
	llassert( rowIndex >= 0 && rowIndex < NUM_VALUES_IN_MAT3 );

	mMatrix[rowIndex][0] = row[0];
	mMatrix[rowIndex][1] = row[1];
	mMatrix[rowIndex][2] = row[2];

	return *this;
}

const LLMatrix3& LLMatrix3::setCol( U32 colIndex, const LLVector3& col )
{
	llassert( colIndex >= 0 && colIndex < NUM_VALUES_IN_MAT3 );

	mMatrix[0][colIndex] = col[0];
	mMatrix[1][colIndex] = col[1];
	mMatrix[2][colIndex] = col[2];

	return *this;
}
		
// Rotate exisitng mMatrix
const LLMatrix3&	LLMatrix3::rotate(const F32 angle, const F32 x, const F32 y, const F32 z)
{
	LLMatrix3	mat(angle, x, y, z);
	*this *= mat;
	return *this;
}


const LLMatrix3&	LLMatrix3::rotate(const F32 angle, const LLVector3 &vec)
{
	LLMatrix3	mat(angle, vec);
	*this *= mat;
	return *this;
}


const LLMatrix3&	LLMatrix3::rotate(const F32 roll, const F32 pitch, const F32 yaw)
{
	LLMatrix3	mat(roll, pitch, yaw); 
	*this *= mat;
	return *this;
}


const LLMatrix3&	LLMatrix3::rotate(const LLQuaternion &q)
{
	LLMatrix3	mat(q);
	*this *= mat;
	return *this;
}

void LLMatrix3::add(const LLMatrix3& other_matrix)
{
	for (S32 i = 0; i < 3; ++i)
	{
		for (S32 j = 0; j < 3; ++j)
		{
			mMatrix[i][j] += other_matrix.mMatrix[i][j];
		}
	}
}

LLVector3	LLMatrix3::getFwdRow() const
{
	return LLVector3(mMatrix[VX]);
}

LLVector3	LLMatrix3::getLeftRow() const
{
	return LLVector3(mMatrix[VY]);
}

LLVector3	LLMatrix3::getUpRow() const
{
	return LLVector3(mMatrix[VZ]);
}



const LLMatrix3&	LLMatrix3::orthogonalize()
{
	LLVector3 x_axis(mMatrix[VX]);
	LLVector3 y_axis(mMatrix[VY]);
	LLVector3 z_axis(mMatrix[VZ]);

	x_axis.normVec();
	y_axis -= x_axis * (x_axis * y_axis);
	y_axis.normVec();
	z_axis = x_axis % y_axis;
	setRows(x_axis, y_axis, z_axis);
	return (*this);
}


// LLMatrix3 Operators

LLMatrix3 operator*(const LLMatrix3 &a, const LLMatrix3 &b)
{
	U32		i, j;
	LLMatrix3	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT3; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT3; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][0] * b.mMatrix[0][i] + 
							    a.mMatrix[j][1] * b.mMatrix[1][i] + 
							    a.mMatrix[j][2] * b.mMatrix[2][i];
		}
	}
	return mat;
}

/* Not implemented to help enforce code consistency with the syntax of 
   row-major notation.  This is a Good Thing.
LLVector3 operator*(const LLMatrix3 &a, const LLVector3 &b)
{
	LLVector3	vec;
	// matrix operates "from the left" on column vector
	vec.mV[VX] = a.mMatrix[VX][VX] * b.mV[VX] + 
				 a.mMatrix[VX][VY] * b.mV[VY] + 
				 a.mMatrix[VX][VZ] * b.mV[VZ];
	
	vec.mV[VY] = a.mMatrix[VY][VX] * b.mV[VX] + 
				 a.mMatrix[VY][VY] * b.mV[VY] + 
				 a.mMatrix[VY][VZ] * b.mV[VZ];
	
	vec.mV[VZ] = a.mMatrix[VZ][VX] * b.mV[VX] + 
				 a.mMatrix[VZ][VY] * b.mV[VY] + 
				 a.mMatrix[VZ][VZ] * b.mV[VZ];
	return vec;
}
*/


LLVector3 operator*(const LLVector3 &a, const LLMatrix3 &b)
{
	// matrix operates "from the right" on row vector
	return LLVector3(
				a.mV[VX] * b.mMatrix[VX][VX] + 
				a.mV[VY] * b.mMatrix[VY][VX] + 
				a.mV[VZ] * b.mMatrix[VZ][VX],
	
				a.mV[VX] * b.mMatrix[VX][VY] + 
				a.mV[VY] * b.mMatrix[VY][VY] + 
				a.mV[VZ] * b.mMatrix[VZ][VY],
	
				a.mV[VX] * b.mMatrix[VX][VZ] + 
				a.mV[VY] * b.mMatrix[VY][VZ] + 
				a.mV[VZ] * b.mMatrix[VZ][VZ] );
}

LLVector3d operator*(const LLVector3d &a, const LLMatrix3 &b)
{
	// matrix operates "from the right" on row vector
	return LLVector3d(
				a.mdV[VX] * b.mMatrix[VX][VX] + 
				a.mdV[VY] * b.mMatrix[VY][VX] + 
				a.mdV[VZ] * b.mMatrix[VZ][VX],
	
				a.mdV[VX] * b.mMatrix[VX][VY] + 
				a.mdV[VY] * b.mMatrix[VY][VY] + 
				a.mdV[VZ] * b.mMatrix[VZ][VY],
	
				a.mdV[VX] * b.mMatrix[VX][VZ] + 
				a.mdV[VY] * b.mMatrix[VY][VZ] + 
				a.mdV[VZ] * b.mMatrix[VZ][VZ] );
}

bool operator==(const LLMatrix3 &a, const LLMatrix3 &b)
{
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT3; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT3; j++)
		{
			if (a.mMatrix[j][i] != b.mMatrix[j][i])
				return FALSE;
		}
	}
	return TRUE;
}

bool operator!=(const LLMatrix3 &a, const LLMatrix3 &b)
{
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT3; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT3; j++)
		{
			if (a.mMatrix[j][i] != b.mMatrix[j][i])
				return TRUE;
		}
	}
	return FALSE;
}

const LLMatrix3& operator*=(LLMatrix3 &a, const LLMatrix3 &b)
{
	U32		i, j;
	LLMatrix3	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT3; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT3; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][0] * b.mMatrix[0][i] + 
							    a.mMatrix[j][1] * b.mMatrix[1][i] + 
							    a.mMatrix[j][2] * b.mMatrix[2][i];
		}
	}
	a = mat;
	return a;
}

const LLMatrix3& operator*=(LLMatrix3 &a, F32 scalar )
{
	for( U32 i = 0; i < NUM_VALUES_IN_MAT3; ++i )
	{
		for( U32 j = 0; j < NUM_VALUES_IN_MAT3; ++j )
		{
			a.mMatrix[i][j] *= scalar;
		}
	}

	return a;
}

std::ostream& operator<<(std::ostream& s, const LLMatrix3 &a) 
{
	s << "{ " 
		<< a.mMatrix[VX][VX] << ", " << a.mMatrix[VX][VY] << ", " << a.mMatrix[VX][VZ] << "; "
		<< a.mMatrix[VY][VX] << ", " << a.mMatrix[VY][VY] << ", " << a.mMatrix[VY][VZ] << "; "
		<< a.mMatrix[VZ][VX] << ", " << a.mMatrix[VZ][VY] << ", " << a.mMatrix[VZ][VZ] 
	  << " }";
	return s;
}

