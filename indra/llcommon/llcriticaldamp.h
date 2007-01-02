/** 
 * @file llcriticaldamp.h
 * @brief A lightweight class that calculates critical damping constants once
 * per frame.  
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCRITICALDAMP_H
#define LL_LLCRITICALDAMP_H

#include <map>

#include "llframetimer.h"

class LLCriticalDamp 
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
