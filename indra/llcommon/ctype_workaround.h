/** 
 * @file ctype_workaround.h
 * @brief The workaround is to create some legacy symbols that point
 * to the correct symbols, which avoids link errors.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

