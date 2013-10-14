/** 
 * @file timing.h
 * @brief Cross-platform routines for doing timing.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_TIMING_H					
#define LL_TIMING_H


#if LL_LINUX || LL_DARWIN || LL_SOLARIS
#include <sys/time.h>
#endif


const F32 SEC_TO_MICROSEC = 1000000.f;
const U64 SEC_TO_MICROSEC_U64 = 1000000;
const U32 SEC_PER_DAY = 86400;

// functionality has been moved lltimer.{cpp,h}.  This file will be deprecated in the future.

#endif
