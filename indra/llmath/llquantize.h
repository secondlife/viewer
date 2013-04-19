/** 
 * @file llquantize.h
 * @brief useful routines for quantizing floats to various length ints
 * and back out again
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLQUANTIZE_H
#define LL_LLQUANTIZE_H

const U16 U16MAX = 65535;
LL_ALIGN_16( const F32 F_U16MAX_4A[4] ) = { 65535.f, 65535.f, 65535.f, 65535.f };

const F32 OOU16MAX = 1.f/(F32)(U16MAX);
LL_ALIGN_16( const F32 F_OOU16MAX_4A[4] ) = { OOU16MAX, OOU16MAX, OOU16MAX, OOU16MAX };

const U8 U8MAX = 255;
LL_ALIGN_16( const F32 F_U8MAX_4A[4] ) = { 255.f, 255.f, 255.f, 255.f };

const F32 OOU8MAX = 1.f/(F32)(U8MAX);
LL_ALIGN_16( const F32 F_OOU8MAX_4A[4] ) = { OOU8MAX, OOU8MAX, OOU8MAX, OOU8MAX };

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
