/** 
 * @file v2math.cpp
 * @brief LLVector2 class implementation.
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
	return fsqrtf( x*x + y*y );
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

