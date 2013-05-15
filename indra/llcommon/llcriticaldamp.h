/** 
 * @file llcriticaldamp.h
 * @brief A lightweight class that calculates critical damping constants once
 * per frame.  
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLCRITICALDAMP_H
#define LL_LLCRITICALDAMP_H

#include <map>

#include "llframetimer.h"

class LL_COMMON_API LLCriticalDamp 
{
public:
	LLCriticalDamp();

	// MANIPULATORS
	static void updateInterpolants();

	// ACCESSORS
	static F32 getInterpolant(const F32 time_constant, BOOL use_cache = TRUE);

protected:	
	static LLFrameTimer sInternalTimer;	// frame timer for calculating deltas

	static std::map<F32, F32> 	sInterpolants;
	static F32					sTimeDelta;
};

#endif  // LL_LLCRITICALDAMP_H
