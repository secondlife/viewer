/** 
 * @file ctype_workaround.h
 * @brief The workaround is to create some legacy symbols that point
 * to the correct symbols, which avoids link errors.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef _CTYPE_WORKAROUND_H_
#define _CTYPE_WORKAROUND_H_

/**
 * the CTYPE_WORKAROUND is needed for linux dev stations that don't
 * have the broken libc6 packages needed by our out-of-date static
 * libs (such as libcrypto and libcurl).
 *
 * -- Leviathan 20060113
*/

#include <cctype>

__const unsigned short int *__ctype_b;
__const __int32_t *__ctype_tolower;
__const __int32_t *__ctype_toupper;

// call this function at the beginning of main() 
void ctype_workaround()
{
    __ctype_b = *(__ctype_b_loc());
    __ctype_toupper = *(__ctype_toupper_loc());
    __ctype_tolower = *(__ctype_tolower_loc());
}

#endif

