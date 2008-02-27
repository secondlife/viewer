/** 
 * @file llcoordframe.cpp
 * @brief LLCoordFrame class implementation.
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
#include "m3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "llcoordframe.h"

#ifndef X_AXIS
	#define X_AXIS 1.0f,0.0f,0.0f
	#define Y_AXIS 0.0f,1.0f,0.0f
	#define Z_AXIS 0.0f,0.0f,1.0f
#endif

// Constructors

LLCoordFrame::LLCoordFrame() :
	mOrigin(0.f, 0.f, 0.f),
	mXAxis(X_AXIS),
	mYAxis(Y_AXIS),
	mZAxis(Z_AXIS)
{
}

LLCoordFrame::LLCoordFrame(const LLVector3 &origin) :
	mOrigin(origin), 
	mXAxis(X_AXIS),
	mYAxis(Y_AXIS),
	mZAxis(Z_AXIS)
{
	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLVector3 &origin, const LLVector3 &direction) :
	mOrigin(origin)
{
	lookDir(direction);
	
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLVector3 &x_axis,
						   const LLVector3 &y_axis,
						   const LLVector3 &z_axis) : 
	mOrigin(0.f, 0.f, 0.f), 
	mXAxis(x_axis), 
	mYAxis(y_axis), 
	mZAxis(z_axis)
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLVector3 &origin,
						   const LLVector3 &x_axis,
						   const LLVector3 &y_axis,
						   const LLVector3 &z_axis) : 
	mOrigin(origin), 
	mXAxis(x_axis), 
	mYAxis(y_axis), 
	mZAxis(z_axis)
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}


LLCoordFrame::LLCoordFrame(const LLVector3 &origin, 
						   const LLMatrix3 &rotation) :
	mOrigin(origin),
	mXAxis(rotation.mMatrix[VX]),
	mYAxis(rotation.mMatrix[VY]),
	mZAxis(rotation.mMatrix[VZ])
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLQuaternion &q) :
	mOrigin(0.f, 0.f, 0.f)
{
	LLMatrix3 rotation_matrix(q);
	mXAxis.setVec(rotation_matrix.mMatrix[VX]);
	mYAxis.setVec(rotation_matrix.mMatrix[VY]);
	mZAxis.setVec(rotation_matrix.mMatrix[VZ]);

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLVector3 &origin, const LLQuaternion &q) :
	mOrigin(origin)
{
	LLMatrix3 rotation_matrix(q);
	mXAxis.setVec(rotation_matrix.mMatrix[VX]);
	mYAxis.setVec(rotation_matrix.mMatrix[VY]);
	mZAxis.setVec(rotation_matrix.mMatrix[VZ]);

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}

LLCoordFrame::LLCoordFrame(const LLMatrix4 &mat) :
	mOrigin(mat.mMatrix[VW]),
	mXAxis(mat.mMatrix[VX]),
	mYAxis(mat.mMatrix[VY]),
	mZAxis(mat.mMatrix[VZ])
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}


// The folowing two constructors are dangerous due to implicit casting and have been disabled - SJB
/*
LLCoordFrame::LLCoordFrame(const F32 *origin, const F32 *rotation) :
	mOrigin(origin),
	mXAxis(rotation+3*VX),
	mYAxis(rotation+3*VY),
	mZAxis(rotation+3*VZ)
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}
*/

/*
LLCoordFrame::LLCoordFrame(const F32 *origin_and_rotation) :
	mOrigin(origin_and_rotation),
	mXAxis(origin_and_rotation + 3*(VX+1)),
	mYAxis(origin_and_rotation + 3*(VY+1)),
	mZAxis(origin_and_rotation + 3*(VZ+1))
{
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::LLCoordFrame()" << llendl;
	}
}
*/


void LLCoordFrame::reset() 
{
	mOrigin.setVec(0.0f, 0.0f, 0.0f);
	resetAxes();
}


void LLCoordFrame::resetAxes()
{
	mXAxis.setVec(1.0f, 0.0f, 0.0f);
	mYAxis.setVec(0.0f, 1.0f, 0.0f);
	mZAxis.setVec(0.0f, 0.0f, 1.0f);
}

// setOrigin() member functions set mOrigin

void LLCoordFrame::setOrigin(F32 x, F32 y, F32 z) 
{
	mOrigin.setVec(x, y, z); 

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setOrigin()" << llendl;
	}
}

void LLCoordFrame::setOrigin(const LLVector3 &new_origin)
{
	mOrigin = new_origin; 
	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setOrigin()" << llendl;
	}
}

void LLCoordFrame::setOrigin(const F32 *origin)
{
	mOrigin.mV[VX] = *(origin + VX);
	mOrigin.mV[VY] = *(origin + VY);
	mOrigin.mV[VZ] = *(origin + VZ);

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setOrigin()" << llendl;
	}
}

void LLCoordFrame::setOrigin(const LLCoordFrame &frame)
{
	mOrigin = frame.getOrigin();

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setOrigin()" << llendl;
	}
}

// setAxes()  member functions set the axes, and assume that
// the arguments are orthogonal and normalized.

void LLCoordFrame::setAxes(const LLVector3 &x_axis,
						  const LLVector3 &y_axis,
						  const LLVector3 &z_axis)
{
	mXAxis = x_axis;
	mYAxis = y_axis;
	mZAxis = z_axis;
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setAxes()" << llendl;
	}
}


void LLCoordFrame::setAxes(const LLMatrix3 &rotation_matrix)
{
	mXAxis.setVec(rotation_matrix.mMatrix[VX]);
	mYAxis.setVec(rotation_matrix.mMatrix[VY]);
	mZAxis.setVec(rotation_matrix.mMatrix[VZ]);
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setAxes()" << llendl;
	}
}


void LLCoordFrame::setAxes(const LLQuaternion &q )
{
	LLMatrix3 rotation_matrix(q);
	setAxes(rotation_matrix);
	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setAxes()" << llendl;
	}
}


void LLCoordFrame::setAxes(  const F32 *rotation_matrix ) 
{
	mXAxis.mV[VX] = *(rotation_matrix + 3*VX + VX);
	mXAxis.mV[VY] = *(rotation_matrix + 3*VX + VY);
	mXAxis.mV[VZ] = *(rotation_matrix + 3*VX + VZ);
	mYAxis.mV[VX] = *(rotation_matrix + 3*VY + VX);
	mYAxis.mV[VY] = *(rotation_matrix + 3*VY + VY);
	mYAxis.mV[VZ] = *(rotation_matrix + 3*VY + VZ);
	mZAxis.mV[VX] = *(rotation_matrix + 3*VZ + VX);
	mZAxis.mV[VY] = *(rotation_matrix + 3*VZ + VY);
	mZAxis.mV[VZ] = *(rotation_matrix + 3*VZ + VZ);

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setAxes()" << llendl;
	}
}


void LLCoordFrame::setAxes(const LLCoordFrame &frame)
{
	mXAxis = frame.getXAxis();
	mYAxis = frame.getYAxis();
	mZAxis = frame.getZAxis();

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::setAxes()" << llendl;
	}
}


// translate() member functions move mOrigin to a relative position

void LLCoordFrame::translate(F32 x, F32 y, F32 z)
{
	mOrigin.mV[VX] += x;
	mOrigin.mV[VY] += y;
	mOrigin.mV[VZ] += z;

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::translate()" << llendl;
	}
}


void LLCoordFrame::translate(const LLVector3 &v)
{
	mOrigin += v;

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::translate()" << llendl;
	}
}


void LLCoordFrame::translate(const F32 *origin)
{
	mOrigin.mV[VX] += *(origin + VX);
	mOrigin.mV[VY] += *(origin + VY);
	mOrigin.mV[VZ] += *(origin + VZ);

	if( !mOrigin.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::translate()" << llendl;
	}
}


// Rotate move the axes to a relative rotation

void LLCoordFrame::rotate(F32 angle, F32 x, F32 y, F32 z)
{
	LLQuaternion q(angle, LLVector3(x,y,z));
	rotate(q);
}


void LLCoordFrame::rotate(F32 angle, const LLVector3 &rotation_axis)
{
	LLQuaternion q(angle, rotation_axis);
	rotate(q);
}


void LLCoordFrame::rotate(const LLQuaternion &q)
{
	LLMatrix3 rotation_matrix(q);
	rotate(rotation_matrix);
}


void LLCoordFrame::rotate(const LLMatrix3 &rotation_matrix)
{
	mXAxis.rotVec(rotation_matrix);
	mYAxis.rotVec(rotation_matrix);
	orthonormalize();

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::rotate()" << llendl;
	}
}


void LLCoordFrame::roll(F32 angle)
{
	LLQuaternion q(angle, mXAxis);
	LLMatrix3 rotation_matrix(q);
	rotate(rotation_matrix);

	if( !mYAxis.isFinite() || !mZAxis.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::roll()" << llendl;
	}
}

void LLCoordFrame::pitch(F32 angle)
{
	LLQuaternion q(angle, mYAxis);
	LLMatrix3 rotation_matrix(q);
	rotate(rotation_matrix);

	if( !mXAxis.isFinite() || !mZAxis.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::pitch()" << llendl;
	}
}

void LLCoordFrame::yaw(F32 angle)
{
	LLQuaternion q(angle, mZAxis);
	LLMatrix3 rotation_matrix(q);
	rotate(rotation_matrix);

	if( !mXAxis.isFinite() || !mYAxis.isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::yaw()" << llendl;
	}
}

// get*() routines


LLQuaternion LLCoordFrame::getQuaternion() const
{
	LLQuaternion quat(mXAxis, mYAxis, mZAxis);
	return quat;
}


void LLCoordFrame::getMatrixToLocal(LLMatrix4& mat) const
{
	mat.setFwdCol(mXAxis);
	mat.setLeftCol(mYAxis);
	mat.setUpCol(mZAxis);

	mat.mMatrix[3][0] = -(mOrigin * LLVector3(mat.mMatrix[0][0], mat.mMatrix[1][0], mat.mMatrix[2][0]));
	mat.mMatrix[3][1] = -(mOrigin * LLVector3(mat.mMatrix[0][1], mat.mMatrix[1][1], mat.mMatrix[2][1]));
	mat.mMatrix[3][2] = -(mOrigin * LLVector3(mat.mMatrix[0][2], mat.mMatrix[1][2], mat.mMatrix[2][2]));
}


void LLCoordFrame::getRotMatrixToParent(LLMatrix4& mat) const
{
	// Note: moves into CFR
	mat.setFwdRow(	-mYAxis );
	mat.setLeftRow(	 mZAxis );
	mat.setUpRow(	-mXAxis );
}

size_t LLCoordFrame::writeOrientation(char *buffer) const
{
	memcpy(buffer, mOrigin.mV, 3*sizeof(F32)); /*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(buffer, mXAxis.mV, 3*sizeof(F32)); /*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(buffer, mYAxis.mV, 3*sizeof(F32));/*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(buffer, mZAxis.mV, 3*sizeof(F32));	/*Flawfinder: ignore */
	return 12*sizeof(F32);
}


size_t LLCoordFrame::readOrientation(const char *buffer)
{
	memcpy(mOrigin.mV, buffer, 3*sizeof(F32));	/*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(mXAxis.mV, buffer, 3*sizeof(F32));	/*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(mYAxis.mV, buffer, 3*sizeof(F32));	/*Flawfinder: ignore */
	buffer += 3*sizeof(F32);
	memcpy(mZAxis.mV, buffer, 3*sizeof(F32));	/*Flawfinder: ignore */

	if( !isFinite() )
	{
		reset();
		llwarns << "Non Finite in LLCoordFrame::readOrientation()" << llendl;
	}

	return 12*sizeof(F32);
}


// rotation and transform vectors between reference frames

LLVector3 LLCoordFrame::rotateToLocal(const LLVector3 &absolute_vector) const
{
	LLVector3 local_vector(mXAxis * absolute_vector,
						   mYAxis * absolute_vector,
						   mZAxis * absolute_vector);
	return local_vector;
}


LLVector4 LLCoordFrame::rotateToLocal(const LLVector4 &absolute_vector) const
{
	LLVector4 local_vector;
	local_vector.mV[VX] = mXAxis.mV[VX] * absolute_vector.mV[VX] +
						  mXAxis.mV[VY] * absolute_vector.mV[VY] +
						  mXAxis.mV[VZ] * absolute_vector.mV[VZ];
	local_vector.mV[VY] = mYAxis.mV[VX] * absolute_vector.mV[VX] +
						  mYAxis.mV[VY] * absolute_vector.mV[VY] +
						  mYAxis.mV[VZ] * absolute_vector.mV[VZ];
	local_vector.mV[VZ] = mZAxis.mV[VX] * absolute_vector.mV[VX] +
						  mZAxis.mV[VY] * absolute_vector.mV[VY] +
						  mZAxis.mV[VZ] * absolute_vector.mV[VZ];
	local_vector.mV[VW] = absolute_vector.mV[VW];
	return local_vector;
}


LLVector3 LLCoordFrame::rotateToAbsolute(const LLVector3 &local_vector) const
{
	LLVector3 absolute_vector;
	absolute_vector.mV[VX] = mXAxis.mV[VX] * local_vector.mV[VX] +
							 mYAxis.mV[VX] * local_vector.mV[VY] +
							 mZAxis.mV[VX] * local_vector.mV[VZ];
	absolute_vector.mV[VY] = mXAxis.mV[VY] * local_vector.mV[VX] +
							 mYAxis.mV[VY] * local_vector.mV[VY] +
							 mZAxis.mV[VY] * local_vector.mV[VZ];
	absolute_vector.mV[VZ] = mXAxis.mV[VZ] * local_vector.mV[VX] +
							 mYAxis.mV[VZ] * local_vector.mV[VY] +
							 mZAxis.mV[VZ] * local_vector.mV[VZ];
	return absolute_vector;
}


LLVector4 LLCoordFrame::rotateToAbsolute(const LLVector4 &local_vector) const
{
	LLVector4 absolute_vector;
	absolute_vector.mV[VX] = mXAxis.mV[VX] * local_vector.mV[VX] +
							 mYAxis.mV[VX] * local_vector.mV[VY] +
							 mZAxis.mV[VX] * local_vector.mV[VZ];
	absolute_vector.mV[VY] = mXAxis.mV[VY] * local_vector.mV[VX] +
							 mYAxis.mV[VY] * local_vector.mV[VY] +
							 mZAxis.mV[VY] * local_vector.mV[VZ];
	absolute_vector.mV[VZ] = mXAxis.mV[VZ] * local_vector.mV[VX] +
							 mYAxis.mV[VZ] * local_vector.mV[VY] +
							 mZAxis.mV[VZ] * local_vector.mV[VZ];
	absolute_vector.mV[VW] = local_vector[VW];
	return absolute_vector;
}


void LLCoordFrame::orthonormalize()
// Makes sure the axes are orthogonal and normalized.
{
	mXAxis.normVec();						// X is renormalized
	mYAxis -= mXAxis * (mXAxis * mYAxis);	// Y remains in X-Y plane
	mYAxis.normVec();						// Y is normalized
	mZAxis = mXAxis % mYAxis;				// Z = X cross Y
}


LLVector3 LLCoordFrame::transformToLocal(const LLVector3 &absolute_vector) const
{
	return rotateToLocal(absolute_vector - mOrigin);
}


LLVector4 LLCoordFrame::transformToLocal(const LLVector4 &absolute_vector) const
{
	LLVector4 local_vector(absolute_vector);
	local_vector.mV[VX] -= mOrigin.mV[VX];
	local_vector.mV[VY] -= mOrigin.mV[VY];
	local_vector.mV[VZ] -= mOrigin.mV[VZ];
	return rotateToLocal(local_vector);
}


LLVector3 LLCoordFrame::transformToAbsolute(const LLVector3 &local_vector) const
{
	return (rotateToAbsolute(local_vector) + mOrigin);
}


LLVector4 LLCoordFrame::transformToAbsolute(const LLVector4 &local_vector) const
{
	LLVector4 absolute_vector;
	absolute_vector = rotateToAbsolute(local_vector);
	absolute_vector.mV[VX] += mOrigin.mV[VX];
	absolute_vector.mV[VY] += mOrigin.mV[VY];
	absolute_vector.mV[VZ] += mOrigin.mV[VZ];
	return absolute_vector;
}


// This is how you combine a translation and rotation of a 
// coordinate frame to get an OpenGL transformation matrix:
//
//     translation   *   rotation      =          transformation matrix
//
//     (i)->
// (j)| 1  0  0  0 |   | a  d  g  0 |     |     a            d            g          0 |
//  | | 0  1  0  0 | * | b  e  h  0 |  =  |     b            e            h          0 |
//  V | 0  0  1  0 |   | c  f  i  0 |     |     c            f            i          0 |
//    |-x -y -z  1 |   | 0  0  0  1 |     |-(ax+by+cz)  -(dx+ey+fz)  -(gx+hy+iz)     1 |
//
// where {a,b,c} = x-axis 
//       {d,e,f} = y-axis 
//       {g,h,i} = z-axis 
//       {x,y,z} = origin

void LLCoordFrame::getOpenGLTranslation(F32 *ogl_matrix) const
{
	*(ogl_matrix + 0)  = 1.0f;
	*(ogl_matrix + 1)  = 0.0f;
	*(ogl_matrix + 2)  = 0.0f;
	*(ogl_matrix + 3)  = 0.0f;

	*(ogl_matrix + 4)  = 0.0f;
	*(ogl_matrix + 5)  = 1.0f;
	*(ogl_matrix + 6)  = 0.0f;
	*(ogl_matrix + 7)  = 0.0f;

	*(ogl_matrix + 8)  = 0.0f;
	*(ogl_matrix + 9)  = 0.0f;
	*(ogl_matrix + 10) = 1.0f;
	*(ogl_matrix + 11) = 0.0f;

	*(ogl_matrix + 12) = -mOrigin.mV[VX];
	*(ogl_matrix + 13) = -mOrigin.mV[VY];
	*(ogl_matrix + 14) = -mOrigin.mV[VZ];
	*(ogl_matrix + 15) = 1.0f;
}


void LLCoordFrame::getOpenGLRotation(F32 *ogl_matrix) const
{
	*(ogl_matrix + 0)  = mXAxis.mV[VX];
	*(ogl_matrix + 4)  = mXAxis.mV[VY];
	*(ogl_matrix + 8)  = mXAxis.mV[VZ];

	*(ogl_matrix + 1)  = mYAxis.mV[VX];
	*(ogl_matrix + 5)  = mYAxis.mV[VY];
	*(ogl_matrix + 9)  = mYAxis.mV[VZ];

	*(ogl_matrix + 2)  = mZAxis.mV[VX];
	*(ogl_matrix + 6)  = mZAxis.mV[VY];
	*(ogl_matrix + 10) = mZAxis.mV[VZ];

	*(ogl_matrix + 3)  = 0.0f;
	*(ogl_matrix + 7)  = 0.0f;
	*(ogl_matrix + 11) = 0.0f;

	*(ogl_matrix + 12) = 0.0f;
	*(ogl_matrix + 13) = 0.0f;
	*(ogl_matrix + 14) = 0.0f;
	*(ogl_matrix + 15) = 1.0f;
}


void LLCoordFrame::getOpenGLTransform(F32 *ogl_matrix) const
{
	*(ogl_matrix + 0)  = mXAxis.mV[VX];
	*(ogl_matrix + 4)  = mXAxis.mV[VY];
	*(ogl_matrix + 8)  = mXAxis.mV[VZ];
	*(ogl_matrix + 12) = -mOrigin * mXAxis;

	*(ogl_matrix + 1)  = mYAxis.mV[VX];
	*(ogl_matrix + 5)  = mYAxis.mV[VY];
	*(ogl_matrix + 9)  = mYAxis.mV[VZ];
	*(ogl_matrix + 13) = -mOrigin * mYAxis;

	*(ogl_matrix + 2)  = mZAxis.mV[VX];
	*(ogl_matrix + 6)  = mZAxis.mV[VY];
	*(ogl_matrix + 10) = mZAxis.mV[VZ];
	*(ogl_matrix + 14) = -mOrigin * mZAxis;

	*(ogl_matrix + 3)  = 0.0f;
	*(ogl_matrix + 7)  = 0.0f;
	*(ogl_matrix + 11) = 0.0f;
	*(ogl_matrix + 15) = 1.0f;
}


// at and up_direction are presumed to be normalized
void LLCoordFrame::lookDir(const LLVector3 &at, const LLVector3 &up_direction)
{
	// Make sure 'at' and 'up_direction' are not parallel
	// and that neither are zero-length vectors
	LLVector3 left(up_direction % at);
	if (left.isNull()) 
	{
		//tweak lookat pos so we don't get a degenerate matrix
		LLVector3 tempat(at[VX] + 0.01f, at[VY], at[VZ]);
		tempat.normVec();
		left = (up_direction % tempat);
	}
	left.normVec();

	LLVector3 up = at % left;

	if (at.isFinite() && left.isFinite() && up.isFinite())
	{
		setAxes(at, left, up);
	}
}

void LLCoordFrame::lookDir(const LLVector3 &xuv)
{
	static LLVector3 up_direction(0.0f, 0.0f, 1.0f);
	lookDir(xuv, up_direction);
}

void LLCoordFrame::lookAt(const LLVector3 &origin, const LLVector3 &point_of_interest, const LLVector3 &up_direction)
{
	setOrigin(origin);
	LLVector3 at(point_of_interest - origin);
	at.normVec();
	lookDir(at, up_direction);
}

void LLCoordFrame::lookAt(const LLVector3 &origin, const LLVector3 &point_of_interest)
{
	static LLVector3 up_direction(0.0f, 0.0f, 1.0f);

	setOrigin(origin);
	LLVector3 at(point_of_interest - origin);
	at.normVec();
	lookDir(at, up_direction);
}


// Operators and friends

std::ostream& operator<<(std::ostream &s, const LLCoordFrame &C)
{
	s << "{ "
	  << " origin = " << C.mOrigin
	  << " x_axis = " << C.mXAxis
	  << " y_axis = " << C.mYAxis
	  << " z_axis = " << C.mZAxis
	<< " }";
	return s;
}



// Private member functions


//EOF
