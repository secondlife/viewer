/** 
 * @file llcriticaldamp.cpp
 * @brief Implementation of the critical damping functionality.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
