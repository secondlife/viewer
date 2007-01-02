/** 
 * @file ctype_workaround.h
 * @brief The workaround is to create some legacy symbols that point
 * to the correct symbols, which avoids link errors.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#include <ctype.h>

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

