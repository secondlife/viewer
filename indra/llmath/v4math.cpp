/** 
 * @file v4math.cpp
 * @brief LLVector4 class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "linden_common.h"

//#include "vmath.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"

// LLVector4

// Axis-Angle rotations

/*
const LLVector4&	LLVector4::rotVec(F32 angle, const LLVector4 &vec)
{
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLMatrix4(angle, vec);
	}
	return *this;
}

const LLVector4&	LLVector4::rotVec(F32 angle, F32 x, F32 y, F32 z)
{
	LLVector3 vec(x, y, z);
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLMatrix4(angle, vec);
	}
	return *this;
}
*/

const LLVector4&	LLVector4::rotVec(const LLMatrix4 &mat)
{
	*this = *this * mat;
	return *this;
}

const LLVector4&	LLVector4::rotVec(const LLQuaternion &q)
{
	*this = *this * q;
	return *this;
}

const LLVector4&	LLVector4::scaleVec(const LLVector4& vec)
{
	mV[VX] *= vec.mV[VX];
	mV[VY] *= vec.mV[VY];
	mV[VZ] *= vec.mV[VZ];
	mV[VW] *= vec.mV[VW];

	return *this;
}

// Sets all values to absolute value of their original values
// Returns TRUE if data changed
BOOL LLVector4::abs()
{
	BOOL ret = FALSE;

	if (mV[0] < 0.f) { mV[0] = -mV[0]; ret = TRUE; }
	if (mV[1] < 0.f) { mV[1] = -mV[1]; ret = TRUE; }
	if (mV[2] < 0.f) { mV[2] = -mV[2]; ret = TRUE; }
	if (mV[3] < 0.f) { mV[3] = -mV[3]; ret = TRUE; }

	return ret;
}


std::ostream& operator<<(std::ostream& s, const LLVector4 &a) 
{
	s << "{ " << a.mV[VX] << ", " << a.mV[VY] << ", " << a.mV[VZ] << ", " << a.mV[VW] << " }";
	return s;
}


// Non-member functions

F32 angle_between( const LLVector4& a, const LLVector4& b )
{
	LLVector4 an = a;
	LLVector4 bn = b;
	an.normalize();
	bn.normalize();
	F32 cosine = an * bn;
	F32 angle = (cosine >= 1.0f) ? 0.0f :
		(cosine <= -1.0f) ? F_PI :
		acos(cosine);
	return angle;
}

BOOL are_parallel(const LLVector4 &a, const LLVector4 &b, F32 epsilon)
{
	LLVector4 an = a;
	LLVector4 bn = b;
	an.normalize();
	bn.normalize();
	F32 dot = an * bn;
	if ( (1.0f - fabs(dot)) < epsilon)
		return TRUE;
	return FALSE;
}


LLVector3 vec4to3(const LLVector4 &vec)
{
	return LLVector3( vec.mV[VX], vec.mV[VY], vec.mV[VZ] );
}

LLVector4 vec3to4(const LLVector3 &vec)
{
	return LLVector4(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
}

