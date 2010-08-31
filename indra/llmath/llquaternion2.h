/** 
 * @file llquaternion2.h
 * @brief LLQuaternion2 class header file - SIMD-enabled quaternion class
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

#ifndef	LL_QUATERNION2_H
#define	LL_QUATERNION2_H

/////////////////////////////
// LLQuaternion2
/////////////////////////////
// This class stores a quaternion x*i + y*j + z*k + w in <x, y, z, w> order
// (i.e., w in high order element of vector)
/////////////////////////////
/////////////////////////////
// These classes are intentionally minimal right now. If you need additional
// functionality, please contact someone with SSE experience (e.g., Falcon or
// Huseby).
/////////////////////////////
#include "llquaternion.h"

class LLQuaternion2
{
public:

	//////////////////////////
	// Ctors
	//////////////////////////
	
	// Ctor
	LLQuaternion2() {}

	// Ctor from LLQuaternion
	explicit LLQuaternion2( const class LLQuaternion& quat );

	//////////////////////////
	// Get/Set
	//////////////////////////

	// Load from an LLQuaternion
	inline void operator=( const LLQuaternion& quat )
	{
		mQ.loadua( quat.mQ );
	}

	// Return the internal LLVector4a representation of the quaternion
	inline const LLVector4a& getVector4a() const;
	inline LLVector4a& getVector4aRw();

	/////////////////////////
	// Quaternion modification
	/////////////////////////
	
	// Set this quaternion to the conjugate of src
	inline void setConjugate(const LLQuaternion2& src);

	// Renormalizes the quaternion. Assumes it has nonzero length.
	inline void normalize();

	// Quantize this quaternion to 8 bit precision
	inline void quantize8();

	// Quantize this quaternion to 16 bit precision
	inline void quantize16();

	/////////////////////////
	// Quaternion inspection
	/////////////////////////

	// Return true if this quaternion is equal to 'rhs'. 
	// Note! Quaternions exhibit "double-cover", so any rotation has two equally valid
	// quaternion representations and they will NOT compare equal.
	inline bool equals(const LLQuaternion2& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;

	// Return true if all components are finite and the quaternion is normalized
	inline bool isOkRotation() const;

protected:

	LLVector4a mQ;

};

#endif
