/** 
 * @file llcriticaldamp.cpp
 * @brief Implementation of the critical damping functionality.
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

#include "linden_common.h"

#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// static members
//-----------------------------------------------------------------------------
LLFrameTimer LLCriticalDamp::sInternalTimer;
std::map<F32, F32> LLCriticalDamp::sInterpolants;
F32 LLCriticalDamp::sTimeDelta;

//-----------------------------------------------------------------------------
// LLCriticalDamp()
//-----------------------------------------------------------------------------
LLCriticalDamp::LLCriticalDamp()
{
	sTimeDelta = 0.f;
}

// static
//-----------------------------------------------------------------------------
// updateInterpolants()
//-----------------------------------------------------------------------------
void LLCriticalDamp::updateInterpolants()
{
	sTimeDelta = sInternalTimer.getElapsedTimeAndResetF32();

	F32 time_constant;

	for (std::map<F32, F32>::iterator iter = sInterpolants.begin();
		 iter != sInterpolants.end(); iter++)
	{
		time_constant = iter->first;
		F32 new_interpolant = 1.f - pow(2.f, -sTimeDelta / time_constant);
		new_interpolant = llclamp(new_interpolant, 0.f, 1.f);
		sInterpolants[time_constant] = new_interpolant;
	}
} 

//-----------------------------------------------------------------------------
// getInterpolant()
//-----------------------------------------------------------------------------
F32 LLCriticalDamp::getInterpolant(const F32 time_constant, BOOL use_cache)
{
	if (time_constant == 0.f)
	{
		return 1.f;
	}

	if (use_cache && sInterpolants.count(time_constant))
	{
		return sInterpolants[time_constant];
	}
	
	F32 interpolant = 1.f - pow(2.f, -sTimeDelta / time_constant);
	interpolant = llclamp(interpolant, 0.f, 1.f);
	if (use_cache)
	{
		sInterpolants[time_constant] = interpolant;
	}

	return interpolant;
}
