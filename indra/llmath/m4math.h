/** 
 * @file m4math.h
 * @brief LLMatrix4 class header file.
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

#ifndef LL_M4MATH_H
#define LL_M4MATH_H

#include "v3math.h"

class LLVector4;
class LLMatrix3;
class LLQuaternion;

// NOTA BENE: Currently assuming a right-handed, x-forward, y-left, z-up universe

// Us versus OpenGL:

// Even though OpenGL uses column vectors and we use row vectors, we can plug our matrices
// directly into OpenGL.  This is because OpenGL numbers its matrices going columnwise:
//
// OpenGL indexing:          Our indexing:
// 0  4  8 12                [0][0] [0][1] [0][2] [0][3]
// 1  5  9 13                [1][0] [1][1] [1][2] [1][3]
// 2  6 10 14                [2][0] [2][1] [2][2] [2][3]
// 3  7 11 15                [3][0] [3][1] [3][2] [3][3]
//
// So when you're looking at OpenGL related matrices online, our matrices will be
// "transposed".  But our matrices can be plugged directly into OpenGL and work fine!
//

// We're using row vectors - [vx, vy, vz, vw]
//
// There are several different ways of thinking of matrices, if you mix them up, you'll get very confused.
//
// One way to think about it is a matrix that takes the origin frame A
// and rotates it into B': i.e. A*M = B
//
//		Vectors:
//		f - forward axis of B expressed in A
//		l - left axis of B expressed in A
//		u - up axis of B expressed in A
//
//		|  0: fx  1: fy  2: fz  3:0 |
//  M = |  4: lx  5: ly  6: lz  7:0 |
//      |  8: ux  9: uy 10: uz 11:0 |
//      | 12: 0  13: 0  14:  0 15:1 |
//		
//
//
//
// Another way to think of matrices is matrix that takes a point p in frame A, and puts it into frame B:
// This is used most commonly for the modelview matrix.
//
// so p*M = p'
//
//		Vectors:
//      f - forward of frame B in frame A
//      l - left of frame B in frame A
//      u - up of frame B in frame A
//      o - origin of frame frame B in frame A
//
//		|  0: fx  1: lx  2: ux  3:0 |
//  M = |  4: fy  5: ly  6: uy  7:0 |
//      |  8: fz  9: lz 10: uz 11:0 |
//      | 12:-of 13:-ol 14:-ou 15:1 |
//
//		of, ol, and ou mean the component of the "global" origin o in the f axis, l axis, and u axis.
//

static const U32 NUM_VALUES_IN_MAT4 = 4;

class LLMatrix4
{
public:
	F32	mMatrix[NUM_VALUES_IN_MAT4][NUM_VALUES_IN_MAT4];

	// Initializes Matrix to identity matrix
	LLMatrix4()
	{
		setIdentity();
	}
	explicit LLMatrix4(const F32 *mat);								// Initializes Matrix to values in mat
	explicit LLMatrix4(const LLMatrix3 &mat);						// Initializes Matrix to values in mat and sets position to (0,0,0)
	explicit LLMatrix4(const LLQuaternion &q);						// Initializes Matrix with rotation q and sets position to (0,0,0)

	LLMatrix4(const LLMatrix3 &mat, const LLVector4 &pos);	// Initializes Matrix to values in mat and pos

	// These are really, really, inefficient as implemented! - djs
	LLMatrix4(const LLQuaternion &q, const LLVector4 &pos);	// Initializes Matrix with rotation q and position pos
	LLMatrix4(F32 angle,
			  const LLVector4 &vec, 
			  const LLVector4 &pos);						// Initializes Matrix with axis-angle and position
	LLMatrix4(F32 angle, const LLVector4 &vec);				// Initializes Matrix with axis-angle and sets position to (0,0,0)
	LLMatrix4(const F32 roll, const F32 pitch, const F32 yaw, 
			  const LLVector4 &pos);						// Initializes Matrix with Euler angles
	LLMatrix4(const F32 roll, const F32 pitch, const F32 yaw);				// Initializes Matrix with Euler angles

	~LLMatrix4(void);										// Destructor

	LLSD getValue() const;
	void setValue(const LLSD&);

	//////////////////////////////
	//
	// Matrix initializers - these replace any existing values in the matrix
	//

	void initRows(const LLVector4 &row0,
				  const LLVector4 &row1,
				  const LLVector4 &row2,
				  const LLVector4 &row3);

	// various useful matrix functions
	const LLMatrix4& setIdentity();					// Load identity matrix
	bool isIdentity() const;
	const LLMatrix4& setZero();						// Clears matrix to all zeros.

	const LLMatrix4& initRotation(const F32 angle, const F32 x, const F32 y, const F32 z);	// Calculate rotation matrix by rotating angle radians about (x, y, z)
	const LLMatrix4& initRotation(const F32 angle, const LLVector4 &axis);	// Calculate rotation matrix for rotating angle radians about vec
	const LLMatrix4& initRotation(const F32 roll, const F32 pitch, const F32 yaw);		// Calculate rotation matrix from Euler angles
	const LLMatrix4& initRotation(const LLQuaternion &q);			// Set with Quaternion and position
	
	// Position Only
	const LLMatrix4& initMatrix(const LLMatrix3 &mat); //
	const LLMatrix4& initMatrix(const LLMatrix3 &mat, const LLVector4 &translation);

	// These operation create a matrix that will rotate and translate by the
	// specified amounts.
	const LLMatrix4& initRotTrans(const F32 angle,
								  const F32 rx, const F32 ry, const F32 rz,
								  const F32 px, const F32 py, const F32 pz);

	const LLMatrix4& initRotTrans(const F32 angle, const LLVector3 &axis, const LLVector3 &translation);	 // Rotation from axis angle + translation
	const LLMatrix4& initRotTrans(const F32 roll, const F32 pitch, const F32 yaw, const LLVector4 &pos); // Rotation from Euler + translation
	const LLMatrix4& initRotTrans(const LLQuaternion &q, const LLVector4 &pos);	// Set with Quaternion and position

	const LLMatrix4& initScale(const LLVector3 &scale);

	// Set all
	const LLMatrix4& initAll(const LLVector3 &scale, const LLQuaternion &q, const LLVector3 &pos);	


	///////////////////////////
	//
	// Matrix setters - set some properties without modifying others
	//

	const LLMatrix4& setTranslation(const F32 x, const F32 y, const F32 z);	// Sets matrix to translate by (x,y,z)

	void setFwdRow(const LLVector3 &row);
	void setLeftRow(const LLVector3 &row);
	void setUpRow(const LLVector3 &row);

	void setFwdCol(const LLVector3 &col);
	void setLeftCol(const LLVector3 &col);
	void setUpCol(const LLVector3 &col);

	const LLMatrix4& setTranslation(const LLVector4 &translation);
	const LLMatrix4& setTranslation(const LLVector3 &translation);

	///////////////////////////
	//
	// Get properties of a matrix
	//

	F32			 determinant(void) const;						// Return determinant
	LLQuaternion quaternion(void) const;			// Returns quaternion

	LLVector4 getFwdRow4() const;
	LLVector4 getLeftRow4() const;
	LLVector4 getUpRow4() const;

	LLMatrix3 getMat3() const;

	const LLVector3& getTranslation() const { return *(LLVector3*)&mMatrix[3][0]; }

	///////////////////////////
	//
	// Operations on an existing matrix
	//

	const LLMatrix4& transpose();						// Transpose LLMatrix4
	const LLMatrix4& invert();						// Invert LLMatrix4

	// Rotate existing matrix
	// These are really, really, inefficient as implemented! - djs
	const LLMatrix4& rotate(const F32 angle, const F32 x, const F32 y, const F32 z); 		// Rotate matrix by rotating angle radians about (x, y, z)
	const LLMatrix4& rotate(const F32 angle, const LLVector4 &vec);		// Rotate matrix by rotating angle radians about vec
	const LLMatrix4& rotate(const F32 roll, const F32 pitch, const F32 yaw);		// Rotate matrix by Euler angles
	const LLMatrix4& rotate(const LLQuaternion &q);				// Rotate matrix by Quaternion

	
	// Translate existing matrix
	const LLMatrix4& translate(const LLVector3 &vec);				// Translate matrix by (vec[VX], vec[VY], vec[VZ])
	



	///////////////////////
	//
	// Operators
	//

	//	friend inline LLMatrix4 operator*(const LLMatrix4 &a, const LLMatrix4 &b);		// Return a * b
	friend LLVector4 operator*(const LLVector4 &a, const LLMatrix4 &b);		// Return transform of vector a by matrix b
	friend const LLVector3 operator*(const LLVector3 &a, const LLMatrix4 &b);		// Return full transform of a by matrix b
	friend LLVector4 rotate_vector(const LLVector4 &a, const LLMatrix4 &b);	// Rotates a but does not translate
	friend LLVector3 rotate_vector(const LLVector3 &a, const LLMatrix4 &b);	// Rotates a but does not translate

	friend bool operator==(const LLMatrix4 &a, const LLMatrix4 &b);			// Return a == b
	friend bool operator!=(const LLMatrix4 &a, const LLMatrix4 &b);			// Return a != b
	friend bool operator<(const LLMatrix4 &a, const LLMatrix4& b);			// Return a < b

	friend const LLMatrix4& operator+=(LLMatrix4 &a, const LLMatrix4 &b);	// Return a + b
	friend const LLMatrix4& operator-=(LLMatrix4 &a, const LLMatrix4 &b);	// Return a - b
	friend const LLMatrix4& operator*=(LLMatrix4 &a, const LLMatrix4 &b);	// Return a * b
	friend const LLMatrix4& operator*=(LLMatrix4 &a, const F32 &b);			// Return a * b

	friend std::ostream&	 operator<<(std::ostream& s, const LLMatrix4 &a);	// Stream a
};

inline const LLMatrix4&	LLMatrix4::setIdentity()
{
	mMatrix[0][0] = 1.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;
	mMatrix[0][3] = 0.f;

	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 1.f;
	mMatrix[1][2] = 0.f;
	mMatrix[1][3] = 0.f;

	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 1.f;
	mMatrix[2][3] = 0.f;

	mMatrix[3][0] = 0.f;
	mMatrix[3][1] = 0.f;
	mMatrix[3][2] = 0.f;
	mMatrix[3][3] = 1.f;
	return (*this);
}

inline bool LLMatrix4::isIdentity() const
{
	return
		mMatrix[0][0] == 1.f &&
		mMatrix[0][1] == 0.f &&
		mMatrix[0][2] == 0.f &&
		mMatrix[0][3] == 0.f &&

		mMatrix[1][0] == 0.f &&
		mMatrix[1][1] == 1.f &&
		mMatrix[1][2] == 0.f &&
		mMatrix[1][3] == 0.f &&

		mMatrix[2][0] == 0.f &&
		mMatrix[2][1] == 0.f &&
		mMatrix[2][2] == 1.f &&
		mMatrix[2][3] == 0.f &&

		mMatrix[3][0] == 0.f &&
		mMatrix[3][1] == 0.f &&
		mMatrix[3][2] == 0.f &&
		mMatrix[3][3] == 1.f;
}


/*
inline LLMatrix4 operator*(const LLMatrix4 &a, const LLMatrix4 &b)
{
	U32		i, j;
	LLMatrix4	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][0] * b.mMatrix[0][i] + 
							    a.mMatrix[j][1] * b.mMatrix[1][i] + 
							    a.mMatrix[j][2] * b.mMatrix[2][i] +
								a.mMatrix[j][3] * b.mMatrix[3][i];
		}
	}
	return mat;
}
*/


inline const LLMatrix4& operator*=(LLMatrix4 &a, const LLMatrix4 &b)
{
	U32		i, j;
	LLMatrix4	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][0] * b.mMatrix[0][i] + 
							    a.mMatrix[j][1] * b.mMatrix[1][i] + 
							    a.mMatrix[j][2] * b.mMatrix[2][i] +
								a.mMatrix[j][3] * b.mMatrix[3][i];
		}
	}
	a = mat;
	return a;
}

inline const LLMatrix4& operator*=(LLMatrix4 &a, const F32 &b)
{
	U32		i, j;
	LLMatrix4	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] * b;
		}
	}
	a = mat;
	return a;
}

inline const LLMatrix4& operator+=(LLMatrix4 &a, const LLMatrix4 &b)
{
	LLMatrix4 mat;
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] + b.mMatrix[j][i];
		}
	}
	a = mat;
	return a;
}

inline const LLMatrix4& operator-=(LLMatrix4 &a, const LLMatrix4 &b)
{
	LLMatrix4 mat;
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] - b.mMatrix[j][i];
		}
	}
	a = mat;
	return a;
}

// Operates "to the left" on row-vector a
//
// When avatar vertex programs are off, this function is a hot spot in profiles
// due to software skinning in LLViewerJointMesh::updateGeometry().  JC
inline const LLVector3 operator*(const LLVector3 &a, const LLMatrix4 &b)
{
	// This is better than making a temporary LLVector3.  This eliminates an
	// unnecessary LLVector3() constructor and also helps the compiler to
	// realize that the output floats do not alias the input floats, hence
	// eliminating redundant loads of a.mV[0], etc.  JC
	return LLVector3(a.mV[VX] * b.mMatrix[VX][VX] + 
					 a.mV[VY] * b.mMatrix[VY][VX] + 
					 a.mV[VZ] * b.mMatrix[VZ][VX] +
					 b.mMatrix[VW][VX],
					 
					 a.mV[VX] * b.mMatrix[VX][VY] + 
					 a.mV[VY] * b.mMatrix[VY][VY] + 
					 a.mV[VZ] * b.mMatrix[VZ][VY] +
					 b.mMatrix[VW][VY],
					 
					 a.mV[VX] * b.mMatrix[VX][VZ] + 
					 a.mV[VY] * b.mMatrix[VY][VZ] + 
					 a.mV[VZ] * b.mMatrix[VZ][VZ] +
					 b.mMatrix[VW][VZ]);
}

#endif

