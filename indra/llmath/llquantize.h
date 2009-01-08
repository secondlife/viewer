/** 
 * @file llquantize.h
 * @brief useful routines for quantizing floats to various length ints
 * and back out again
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLQUANTIZE_H
#define LL_LLQUANTIZE_H

const U16 U16MAX = 65535;
const F32 OOU16MAX = 1.f/(F32)(U16MAX);

const U8 U8MAX = 255;
const F32 OOU8MAX = 1.f/(F32)(U8MAX);

const U8 FIRSTVALIDCHAR = 54;
const U8 MAXSTRINGVAL = U8MAX - FIRSTVALIDCHAR; //we don't allow newline or null 


inline U16 F32_to_U16_ROUND(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// round the value.   Sreturn the U16
	return (U16)(llround(val*U16MAX));
}


inline U16 F32_to_U16(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// return the U16
	return (U16)(llfloor(val*U16MAX));
}

inline F32 U16_to_F32(U16 ival, F32 lower, F32 upper)
{
	F32 val = ival*OOU16MAX;
	F32 delta = (upper - lower);
	val *= delta;
	val += lower;

	F32 max_error = delta*OOU16MAX;

	// make sure that zero's come through as zero
	if (fabsf(val) < max_error)
		val = 0.f;

	return val;
}


inline U8 F32_to_U8_ROUND(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// return the rounded U8
	return (U8)(llround(val*U8MAX));
}


inline U8 F32_to_U8(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper);
	// make sure that the value is positive and normalized to <0, 1>
	val -= lower;
	val /= (upper - lower);

	// return the U8
	return (U8)(llfloor(val*U8MAX));
}

inline F32 U8_to_F32(U8 ival, F32 lower, F32 upper)
{
	F32 val = ival*OOU8MAX;
	F32 delta = (upper - lower);
	val *= delta;
	val += lower;

	F32 max_error = delta*OOU8MAX;

	// make sure that zero's come through as zero
	if (fabsf(val) < max_error)
		val = 0.f;

	return val;
}

inline U8 F32_TO_STRING(F32 val, F32 lower, F32 upper)
{
	val = llclamp(val, lower, upper); //[lower, upper]
	// make sure that the value is positive and normalized to <0, 1>
	val -= lower;					//[0, upper-lower]
	val /= (upper - lower);			//[0,1]
	val = val * MAXSTRINGVAL;		//[0, MAXSTRINGVAL]
	val = floor(val + 0.5f);		//[0, MAXSTRINGVAL]

	U8 stringVal = (U8)(val) + FIRSTVALIDCHAR;			//[FIRSTVALIDCHAR, MAXSTRINGVAL + FIRSTVALIDCHAR]
	return stringVal;
}

inline F32 STRING_TO_F32(U8 ival, F32 lower, F32 upper)
{
	// remove empty space left for NULL, newline, etc.
	ival -= FIRSTVALIDCHAR;								//[0, MAXSTRINGVAL]

	F32 val = (F32)ival * (1.f / (F32)MAXSTRINGVAL);	//[0, 1]
	F32 delta = (upper - lower);
	val *= delta;										//[0, upper - lower]
	val += lower;										//[lower, upper]

	return val;
}

#endif
