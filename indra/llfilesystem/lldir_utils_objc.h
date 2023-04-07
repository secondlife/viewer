/** 
 * @file lldir_utils_objc.h
 * @brief Definition of directory utilities class for Mac OS X
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#if !LL_DARWIN
#error This header must not be included when compiling for any target other than Mac OS. Consider including lldir.h instead.
#endif // !LL_DARWIN

#ifndef LL_LLDIR_UTILS_OBJC_H
#define LL_LLDIR_UTILS_OBJC_H

#include <iostream>

std::string getSystemTempFolder();
std::string getSystemCacheFolder();
std::string getSystemApplicationSupportFolder();
std::string getSystemResourceFolder();
std::string getSystemExecutableFolder();


#endif // LL_LLDIR_UTILS_OBJC_H
