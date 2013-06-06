/** 
 * @file lldeleteutils.h
 * @brief Utility functions to simplify some common pointer-munging idioms.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LL_DELETE_UTILS_H
#define LL_DELETE_UTILS_H

// Simple utility functions to eventually replace the common 2-line
// idiom scattered throughout the viewer codebase.  Note that where
// possible we would rather be using smart pointers of some sort.

template <class T>
inline void deleteAndClear(T*& ptr)
{
	delete ptr;
	ptr = NULL;
}

template <class T>
inline void deleteAndClearArray(T*& array_ptr)
{
	delete[] array_ptr;
	array_ptr = NULL;
}

#endif
