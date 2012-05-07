/** 
 * @file linden_common.h
 * @brief Includes common headers that are always safe to include
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

#ifndef LL_LINDEN_COMMON_H
#define LL_LINDEN_COMMON_H

// *NOTE:  Please keep includes here to a minimum!
//
// Files included here are included in every library .cpp file and
// are not precompiled.

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
#include <iosfwd>

// Work around Microsoft compiler warnings in STL headers
#ifdef LL_WINDOWS
#pragma warning (disable : 4702) // unreachable code
#pragma warning (disable : 4244) // conversion from time_t to S32
#endif	//	LL_WINDOWS

// *TODO: Eliminate these, most library .cpp files don't need them.
// Add them to llviewerprecompiledheaders.h if necessary.
#include <list>
#include <map>
#include <vector>
#include <string>

#ifdef LL_WINDOWS
// Reenable warnings we disabled above
#pragma warning (3 : 4702) // unreachable code, we like level 3, not 4
// moved msvc warnings to llpreprocessor.h  *TODO - delete this comment after merge conflicts are unlikely -brad
#endif	//	LL_WINDOWS

// Linden only libs in alpha-order other than stdtypes.h
// *NOTE: Please keep includes here to a minimum, see above.
#include "stdtypes.h"
#include "lldefs.h"
#include "llerror.h"
#include "llextendedstatus.h"
// Don't do this, adds 15K lines of header code to every library file.
//#include "llfasttimer.h"
#include "llfile.h"
#include "llformat.h"

// Boost 1.45 had version 2 as the default for the filesystem library,
// 1.48 has version 3 as the default.  Keep compatibility for now.
#define BOOST_FILESYSTEM_VERSION		2

#endif
