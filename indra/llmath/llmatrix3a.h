/** 
 * @file llmatrix3a.h
 * @brief LLMatrix3a class header file - memory aligned and vectorized 3x3 matrix
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef	LL_LLMATRIX3A_H
#define	LL_LLMATRIX3A_H

/////////////////////////////
// LLMatrix3a, LLRotation
/////////////////////////////
// This class stores a 3x3 (technically 4x3) matrix in column-major order
/////////////////////////////
/////////////////////////////
// These classes are intentionally minimal right now. If you need additional
// functionality, please contact someone with SSE experience (e.g., Falcon or
// Huseby).
/////////////////////////////

// LLMatrix3a is the base class for LLRotation, which should be used instead any time you're dealing with a 
// rotation matrix.
class LLMatrix3a
{
public:

	// Utility function for quickly transforming an array of LLVector4a's
	// For transforming a single LLVector4a, see LLVector4a::setRotated
	static void batchTransform( const LLMatrix3a& xform, const LLVector4a* src, int numVectors, LLVector4a* dst );

	// Utility function to obtain the identity matrix
	static inline const LLMatrix3a& getIdentity();

	//////////////////////////
	// Ctors
	//////////////////////////
	
	// Ctor
	LLMatrix3a() {}

	// Ctor for setting by columns
	inline LLMatrix3a( const LLVector4a& c0, const LLVector4a& c1, const LLVector4a& c2 );

	//////////////////////////
	// Get/Set
	//////////////////////////

	// Loads from an LLMatrix3
	inline void loadu(const LLMatrix3& src);
	
	// Set rows
	inline void setRows(const LLVector4a& r0, const LLVector4a& r1, const LLVector4a& r2);
	
	// Set columns
	inline void setColumns(const LLVector4a& c0, const LLVector4a& c1, const LLVector4a& c2);

	// Get the read-only access to a specified column. Valid columns are 0-2, but the 
	// function is unchecked. You've been warned.
	inline const LLVector4a& getColumn(const U32 column) const;

	/////////////////////////
	// Matrix modification
	/////////////////////////
	
	// Set this matrix to the product of lhs and rhs ( this = lhs * rhs )
	void setMul( const LLMatrix3a& lhs, const LLMatrix3a& rhs );

	// Set this matrix to the transpose of src
	inline void setTranspose(const LLMatrix3a& src);

	// Set this matrix to a*w + b*(1-w)
	inline void setLerp(const LLMatrix3a& a, const LLMatrix3a& b, F32 w);

	/////////////////////////
	// Matrix inspection
	/////////////////////////

	// Sets all 4 elements in 'dest' to the determinant of this matrix.
	// If you will be using the determinant in subsequent ops with LLVector4a, use this version
	inline void getDeterminant( LLVector4a& dest ) const;

	// Returns the determinant as an LLSimdScalar. Use this if you will be using the determinant
	// primary for scalar operations.
	inline LLSimdScalar getDeterminant() const;

	// Returns nonzero if rows 0-2 and colums 0-2 contain no NaN or INF values. Row 3 is ignored
	inline LLBool32 isFinite() const;

	// Returns true if this matrix is equal to 'rhs' up to 'tolerance'
	inline bool isApproximatelyEqual( const LLMatrix3a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;

protected:

	LLVector4a mColumns[3];

};

class LLRotation : public LLMatrix3a
{
public:
	
	LLRotation() {}
	
	// Returns true if this rotation is orthonormal with det ~= 1
	inline bool isOkRotation() const;		
};

#endif
