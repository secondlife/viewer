/** 
 * @file m3math.h
 * @brief LLMatrix3 class header file.
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

#ifndef LL_M3MATH_H
#define LL_M3MATH_H

#include "llerror.h"
#include "stdtypes.h"

class LLVector4;
class LLVector3;
class LLVector3d;
class LLQuaternion;

// NOTA BENE: Currently assuming a right-handed, z-up universe

//			     ji	
// LLMatrix3 = | 00 01 02 |
//			   | 10 11 12 |
//			   | 20 21 22 |

// LLMatrix3 = | fx fy fz |	forward-axis
//			   | lx ly lz |	left-axis
//			   | ux uy uz |	up-axis

// NOTE: The world of computer graphics uses column-vectors and matricies that 
// "operate to the left". 


static const U32 NUM_VALUES_IN_MAT3	= 3;
class LLMatrix3
{
	public:
		F32	mMatrix[NUM_VALUES_IN_MAT3][NUM_VALUES_IN_MAT3];

		LLMatrix3(void);							// Initializes Matrix to identity matrix
		explicit LLMatrix3(const F32 *mat);					// Initializes Matrix to values in mat
		explicit LLMatrix3(const LLQuaternion &q);			// Initializes Matrix with rotation q

		LLMatrix3(const F32 angle, const F32 x, const F32 y, const F32 z);	// Initializes Matrix with axis angle
		LLMatrix3(const F32 angle, const LLVector3 &vec);	// Initializes Matrix with axis angle
		LLMatrix3(const F32 angle, const LLVector3d &vec);	// Initializes Matrix with axis angle
		LLMatrix3(const F32 angle, const LLVector4 &vec);	// Initializes Matrix with axis angle
		LLMatrix3(const F32 roll, const F32 pitch, const F32 yaw);	// Initializes Matrix with Euler angles

		//////////////////////////////
		//
		// Matrix initializers - these replace any existing values in the matrix
		//

		// various useful matrix functions
		const LLMatrix3& setIdentity();				// Load identity matrix
		const LLMatrix3& clear();					// Clears Matrix to zero
		const LLMatrix3& setZero();					// Clears Matrix to zero

		///////////////////////////
		//
		// Matrix setters - set some properties without modifying others
		//

		// These functions take Rotation arguments
		const LLMatrix3& setRot(const F32 angle, const F32 x, const F32 y, const F32 z);	// Calculate rotation matrix for rotating angle radians about (x, y, z)
		const LLMatrix3& setRot(const F32 angle, const LLVector3 &vec);	// Calculate rotation matrix for rotating angle radians about vec
		const LLMatrix3& setRot(const F32 roll, const F32 pitch, const F32 yaw);	// Calculate rotation matrix from Euler angles
		const LLMatrix3& setRot(const LLQuaternion &q);			// Transform matrix by Euler angles and translating by pos

		const LLMatrix3& setRows(const LLVector3 &x_axis, const LLVector3 &y_axis, const LLVector3 &z_axis);
		const LLMatrix3& setRow( U32 rowIndex, const LLVector3& row );
		const LLMatrix3& setCol( U32 colIndex, const LLVector3& col );

		
		///////////////////////////
		//
		// Get properties of a matrix
		//
		LLQuaternion quaternion() const;		// Returns quaternion from mat
		void getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) const;	// Returns Euler angles, in radians

		// Axis extraction routines
		LLVector3 getFwdRow() const;
		LLVector3 getLeftRow() const;
		LLVector3 getUpRow() const;
		F32	 determinant() const;			// Return determinant


		///////////////////////////
		//
		// Operations on an existing matrix
		//
		const LLMatrix3& transpose();		// Transpose MAT4
		const LLMatrix3& orthogonalize();	// Orthogonalizes X, then Y, then Z
		void invert();			// Invert MAT4
		const LLMatrix3& adjointTranspose();// returns transpose of matrix adjoint, for multiplying normals

		
		// Rotate existing matrix  
		// Note: the two lines below are equivalent:
		//	foo.rotate(bar) 
		//	foo = foo * bar
		// That is, foo.rotate(bar) multiplies foo by bar FROM THE RIGHT
		const LLMatrix3& rotate(const F32 angle, const F32 x, const F32 y, const F32 z); 	// Rotate matrix by rotating angle radians about (x, y, z)
		const LLMatrix3& rotate(const F32 angle, const LLVector3 &vec);						// Rotate matrix by rotating angle radians about vec
		const LLMatrix3& rotate(const F32 roll, const F32 pitch, const F32 yaw); 			// Rotate matrix by roll (about x), pitch (about y), and yaw (about z)
		const LLMatrix3& rotate(const LLQuaternion &q);			// Transform matrix by Euler angles and translating by pos

		void add(const LLMatrix3& other_matrix);	// add other_matrix to this one

// This operator is misleading as to operation direction
//		friend LLVector3 operator*(const LLMatrix3 &a, const LLVector3 &b);			// Apply rotation a to vector b

		friend LLVector3 operator*(const LLVector3 &a, const LLMatrix3 &b);			// Apply rotation b to vector a
		friend LLVector3d operator*(const LLVector3d &a, const LLMatrix3 &b);			// Apply rotation b to vector a
		friend LLMatrix3 operator*(const LLMatrix3 &a, const LLMatrix3 &b);			// Return a * b

		friend bool operator==(const LLMatrix3 &a, const LLMatrix3 &b);				// Return a == b
		friend bool operator!=(const LLMatrix3 &a, const LLMatrix3 &b);				// Return a != b

		friend const LLMatrix3& operator*=(LLMatrix3 &a, const LLMatrix3 &b);				// Return a * b
		friend const LLMatrix3& operator*=(LLMatrix3 &a, F32 scalar );						// Return a * scalar

		friend std::ostream&	 operator<<(std::ostream& s, const LLMatrix3 &a);	// Stream a
};

inline LLMatrix3::LLMatrix3(void)
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
}

inline LLMatrix3::LLMatrix3(const F32 *mat)
{
	mMatrix[0][0] = mat[0];
	mMatrix[0][1] = mat[1];
	mMatrix[0][2] = mat[2];

	mMatrix[1][0] = mat[3];
	mMatrix[1][1] = mat[4];
	mMatrix[1][2] = mat[5];

	mMatrix[2][0] = mat[6];
	mMatrix[2][1] = mat[7];
	mMatrix[2][2] = mat[8];
}


#endif


// Rotation matrix hints...

// Inverse of Rotation Matrices
// ----------------------------
// If R is a rotation matrix that rotate vectors from Frame-A to Frame-B,
// then the transpose of R will rotate vectors from Frame-B to Frame-A.


// Creating Rotation Matricies From Object Axes
// --------------------------------------------
// Suppose you know the three axes of some object in some "absolute-frame".
// If you take those three vectors and throw them into the rows of 
// a rotation matrix what do you get?
//
// R = | X0  X1  X2 |
//     | Y0  Y1  Y2 |
//     | Z0  Z1  Z2 |
//
// Yeah, but what does it mean?
//
// Transpose the matrix and have it operate on a vector...
//
// V * R_transpose = [ V0  V1  V2 ] * | X0  Y0  Z0 | 
//                                    | X1  Y1  Z1 |                       
//                                    | X2  Y2  Z2 |
// 
//                 = [ V*X  V*Y  V*Z ] 
//
//                 = components of V that are parallel to the three object axes
//
//                 = transformation of V into object frame
//
// Since the transformation of a rotation matrix is its inverse, then
// R must rotate vectors from the object-frame into the absolute-frame.



