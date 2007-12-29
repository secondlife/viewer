/** 
 * @file u64.cpp
 * @brief Utilities to deal with U64s.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "u64.h"


U64 str_to_U64(const char *str)
{
	U64 result = 0;
	const char *aptr = strpbrk(str,"0123456789");

	if (!aptr)
	{
		llwarns << "str_to_U64: Bad string to U64 conversion attempt: format\n" << llendl;
	}
	else
	{
		while ((*aptr >= '0') && (*aptr <= '9'))
		{
			result = result*10 + (*aptr++ - '0');
		}
	}
	return (result);
}


char* U64_to_str(U64 value, char* result, S32 result_size) 
{										
	U32 part1,part2,part3;
	
	part3 = (U32)(value % (U64)10000000);
	
	value /= 10000000;
	part2 = (U32)(value % (U64)10000000);
	
	value /= 10000000;
	part1 = (U32)(value % (U64)10000000);
	
	// three cases to avoid leading zeroes unless necessary
	
	if (part1)
	{
		snprintf(	/* Flawfinder: ignore */
			result,
			result_size,
			"%u%07u%07u",
			part1,part2,part3);
	}
	else if (part2)
	{
		snprintf(	/* Flawfinder: ignore */
			result,
			result_size,
			"%u%07u",
			part2,part3);
	}
	else
	{
		snprintf(	/* Flawfinder: ignore */
			result,
			result_size,
			"%u",
			part3);		
	}
	return (result);
} 

F64 U64_to_F64(const U64 value)
{
	S64 top_bits = (S64)(value >> 1);
	F64 result = (F64)top_bits;
	result *= 2.f;
	result += (U32)(value & 0x01);
	return result;
}

U64	llstrtou64(const char* str, char** end, S32 base)
{
#ifdef LL_WINDOWS
	return _strtoui64(str,end,base);
#else
	return strtoull(str,end,base);
#endif
}
