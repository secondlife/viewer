/** 
 * @file u64.cpp
 * @brief Utilities to deal with U64s.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "u64.h"


U64 str_to_U64(const char *str)
{
	U64 result = 0;
	char *aptr = strpbrk(str,"0123456789");

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
