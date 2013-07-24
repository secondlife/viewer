/** 
 * @file v2math.cpp
 * @brief LLVector2 class implementation.
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

//#include "vmath.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"

// LLVector2

LLVector2 LLVector2::zero(0,0);


// Non-member functions

// Sets all values to absolute value of their original values
// Returns TRUE if data changed
BOOL LLVector2::abs()
{
	BOOL ret = FALSE;

	if (mV[0] < 0.f) { mV[0] = -mV[0]; ret = TRUE; }
	if (mV[1] < 0.f) { mV[1] = -mV[1]; ret = TRUE; }
	
	return ret;
}


F32 angle_between(const LLVector2& a, const LLVector2& b)
{
	LLVector2 an = a;
	LLVector2 bn = b;
	an.normVec();
	bn.normVec();
	F32 cosine = an * bn;
	F32 angle = (cosine >= 1.0f) ? 0.0f :
				(cosine <= -1.0f) ? F_PI :
				acos(cosine);
	return angle;
}

BOOL are_parallel(const LLVector2 &a, const LLVector2 &b, float epsilon)
{
	LLVector2 an = a;
	LLVector2 bn = b;
	an.normVec();
	bn.normVec();
	F32 dot = an * bn;
	if ( (1.0f - fabs(dot)) < epsilon)
	{
		return TRUE;
	}
	return FALSE;
}


F32	dist_vec(const LLVector2 &a, const LLVector2 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return (F32) sqrt( x*x + y*y );
}

F32	dist_vec_squared(const LLVector2 &a, const LLVector2 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return x*x + y*y;
}

F32	dist_vec_squared2D(const LLVector2 &a, const LLVector2 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return x*x + y*y;
}

LLVector2 lerp(const LLVector2 &a, const LLVector2 &b, F32 u)
{
	return LLVector2(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u );
}

LLSD LLVector2::getValue() const
{
	LLSD ret;
	ret[0] = mV[0];
	ret[1] = mV[1];
	return ret;
}

void LLVector2::setValue(LLSD& sd)
{
	mV[0] = (F32) sd[0].asReal();
	mV[1] = (F32) sd[1].asReal();
}

