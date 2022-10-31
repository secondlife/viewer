/** 
 * @file u64.cpp
 * @brief Utilities to deal with U64s.
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

#include "linden_common.h"

#include "u64.h"


U64 str_to_U64(const std::string& str)
{
    U64 result = 0;
    const char *aptr = strpbrk(str.c_str(),"0123456789");

    if (!aptr)
    {
        LL_WARNS() << "str_to_U64: Bad string to U64 conversion attempt: format\n" << LL_ENDL;
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


std::string U64_to_str(U64 value) 
{
    std::string res;
    U32 part1,part2,part3;
    
    part3 = (U32)(value % (U64)10000000);
    
    value /= 10000000;
    part2 = (U32)(value % (U64)10000000);
    
    value /= 10000000;
    part1 = (U32)(value % (U64)10000000);
    
    // three cases to avoid leading zeroes unless necessary
    
    if (part1)
    {
        res = llformat("%u%07u%07u",part1,part2,part3);
    }
    else if (part2)
    {
        res = llformat("%u%07u",part2,part3);
    }
    else
    {
        res = llformat("%u",part3); 
    }
    return res;
} 

char* U64_to_str(U64 value, char* result, S32 result_size) 
{
    std::string res = U64_to_str(value);
    LLStringUtil::copy(result, res.c_str(), result_size);
    return result;
}

F64 U64_to_F64(const U64 value)
{
    S64 top_bits = (S64)(value >> 1);
    F64 result = (F64)top_bits;
    result *= 2.f;
    result += (U32)(value & 0x01);
    return result;
}

U64 llstrtou64(const char* str, char** end, S32 base)
{
#ifdef LL_WINDOWS
    return _strtoui64(str,end,base);
#else
    return strtoull(str,end,base);
#endif
}
