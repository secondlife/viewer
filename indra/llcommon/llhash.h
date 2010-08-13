/** 
 * @file llhash.h
 * @brief Wrapper for a hash function.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLHASH_H
#define LL_LLHASH_H

#include "llpreprocessor.h" // for GCC_VERSION

#if (LL_WINDOWS)
#include <hash_map>
#include <algorithm>
#elif LL_DARWIN || LL_LINUX
#  if GCC_VERSION >= 40300 // gcc 4.3 and up
#    include <backward/hashtable.h>
#  elif GCC_VERSION >= 30400 // gcc 3.4 and up
#    include <ext/hashtable.h>
#  elif __GNUC__ >= 3
#    include <ext/stl_hashtable.h>
#  else
#    include <hashtable.h>
#  endif
#elif LL_SOLARIS
#include <ext/hashtable.h>
#else
#error Please define your platform.
#endif

// Warning - an earlier template-based version of this routine did not do
// the correct thing on Windows.   Since this is only used to get
// a string hash, it was converted to a regular routine and
// unit tests added.

inline size_t llhash( const char * value )
{
#if LL_WINDOWS
	return stdext::hash_value(value);
#elif ( (defined _STLPORT_VERSION) || ((LL_LINUX) && (__GNUC__ <= 2)) )
	std::hash<const char *> H;
	return H(value);
#elif LL_DARWIN || LL_LINUX || LL_SOLARIS
	__gnu_cxx::hash<const char *> H;
	return H(value);
#else
#error Please define your platform.
#endif
}

#endif

