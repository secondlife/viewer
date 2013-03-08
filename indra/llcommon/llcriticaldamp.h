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

// These enums each represent one fixed-time delta value
// that we interpolate once given the actual sTimeDelta time
// that has passed. This allows us to calculate the interp portion
// of those values once and then look them up repeatedly per frame.
//
enum InterpDelta
{	
	InterpDelta_0_025,		// 0.025
	InterpDeltaTeenier					= InterpDelta_0_025,
	InterpDeltaFolderOpenTime			= InterpDelta_0_025,
	InterpDeltaFolderCloseTime			= InterpDelta_0_025,
	InterpDeltaCameraFocusHalfLife		= InterpDelta_0_025,	// USED TO BE ZERO....

	InterpDelta_0_05,		// 0.05
	InterpDeltaTeeny		= InterpDelta_0_05,

	InterpDelta_0_06,		// 0.06
	InterpDeltaObjectDampingConstant	= InterpDelta_0_06,	
	InterpDeltaCameraZoomHalfLife		= InterpDelta_0_06,
	InterpDeltaFovZoomHalfLife			= InterpDelta_0_06,
	InterpDeltaManipulatorScaleHalfLife	= InterpDelta_0_06,
	InterpDeltaContextFadeTime			= InterpDelta_0_06,

	InterpDelta_0_10,		// 0.10
	InterpDeltaSmaller					= InterpDelta_0_10,
	InterpDeltaTargetLagHalfLife		= InterpDelta_0_10,
	InterpDeltaSpeedAdjustTime			= InterpDelta_0_10,

	InterpDelta_0_15,		// 0.15
	InterpDeltaFadeWeight				= InterpDelta_0_15,
	InterpDeltaHeadLookAtLagHalfLife	= InterpDelta_0_15,

	InterpDelta_0_20,		// 0.20
	InterpDeltaSmall					= InterpDelta_0_20,
	InterpDeltaTorsoLagHalfLife			= InterpDelta_0_20,
	InterpDeltaPositionDampingTC		= InterpDelta_0_20,

	InterpDelta_0_25,		// 0.25
	InterpDeltaCameraLagHalfLife		= InterpDelta_0_25,
	InterpDeltaTorsoTargetLagHalfLife	= InterpDelta_0_25,
	InterpDeltaTorsoLookAtLagHalfLife	= InterpDelta_0_25,

	InterpDelta_0_30,		// 0.3
	InterpDeltaSmallish		= InterpDelta_0_30,

	// Dynamically set interpolants which use setInterpolantConstant
	//
	InterpDeltaCameraSmoothingHalfLife,	
	InterpDeltaBehindnessLag,
	InterpDeltaFocusLag,
	InterpDeltaPositionLag,
	InterpDeltaOpenTime,
	InterpDeltaCloseTime,		

	kNumCachedInterpolants
};

class LL_COMMON_API LLCriticalDamp 
{
public:
	LLCriticalDamp();

	// Updates all the known interp delta values for fast lookup in calls to getInterpolant(InterpDelta)
	//
	static void updateInterpolants();

	static inline void setInterpolantConstant(InterpDelta whichDelta, const F32 time_constant)
	{
		llassert(whichDelta < kNumCachedInterpolants);
		sInterpolants[whichDelta] = time_constant;
	}

	// ACCESSORS
	static inline F32 getInterpolant(InterpDelta whichDelta)
	{
		llassert(whichDelta < kNumCachedInterpolants);
		return sInterpolatedValues[whichDelta];
	}

	static inline F32 getInterpolant(const F32 time_constant)
	{
		return llclamp((sTimeDelta / time_constant), 0.0f, 1.0f);
	}

protected:	
	static LLFrameTimer sInternalTimer;	// frame timer for calculating deltas

	//static std::map<F32, F32> 	sInterpolants;
	static F32					sInterpolants[kNumCachedInterpolants];
	static F32					sInterpolatedValues[kNumCachedInterpolants];
	static F32					sTimeDelta;
};

#endif  // LL_LLCRITICALDAMP_H

