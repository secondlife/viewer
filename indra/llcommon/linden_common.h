/** 
 * @file linden_common.h
 * @brief Includes common headers that are always safe to include
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

#ifndef LL_LINDEN_COMMON_H
#define LL_LINDEN_COMMON_H

#if defined(LL_WINDOWS) && defined(_DEBUG)
# if _MSC_VER >= 1400 // Visual C++ 2005 or later
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
# endif
#endif

#include "llpreprocessor.h"

#include <cstring>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>

// Work Microsoft compiler warnings
#ifdef LL_WINDOWS
#pragma warning (disable : 4702) // unreachable code
#pragma warning (disable : 4244) // conversion from time_t to S32
#endif	//	LL_WINDOWS

#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include <string>

#ifdef LL_WINDOWS
#pragma warning (3 : 4702) // we like level 3, not 4
// level 4 warnings that we need to disable:
#pragma warning (disable : 4100) // unreferenced formal parameter
#pragma warning (disable : 4127) // conditional expression is constant (e.g. while(1) )
#pragma warning (disable : 4244) // possible loss of data on conversions
#pragma warning (disable : 4396) // the inline specifier cannot be used when a friend declaration refers to a specialization of a function template
#pragma warning (disable : 4512) // assignment operator could not be generated
#pragma warning (disable : 4706) // assignment within conditional (even if((x = y)) )
#endif	//	LL_WINDOWS

// Linden only libs in alpha-order other than stdtypes.h
#include "stdtypes.h"
#include "lldefs.h"
#include "llerror.h"
#include "llextendedstatus.h"
#include "llfasttimer.h"
#include "llfile.h"
#include "llformat.h"
#include "llstring.h"
#include "llsys.h"
#include "lltimer.h"

#endif
