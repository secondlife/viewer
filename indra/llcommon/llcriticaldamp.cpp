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
F32 LLCriticalDamp::sTimeDelta;
F32	LLCriticalDamp::sInterpolants[kNumCachedInterpolants];
F32 LLCriticalDamp::sInterpolatedValues[kNumCachedInterpolants];

//-----------------------------------------------------------------------------
// LLCriticalDamp()
//-----------------------------------------------------------------------------
LLCriticalDamp::LLCriticalDamp()
{
	sTimeDelta = 0.f;

	// Init the core interpolant values (to which many, many enums map)
	//
	setInterpolantConstant(InterpDelta_0_025, 0.025f);
	setInterpolantConstant(InterpDelta_0_05,  0.05f );
	setInterpolantConstant(InterpDelta_0_06,  0.06f);
	setInterpolantConstant(InterpDelta_0_10,  0.10f);
	setInterpolantConstant(InterpDelta_0_15,  0.15f);
	setInterpolantConstant(InterpDelta_0_20,  0.20f);
	setInterpolantConstant(InterpDelta_0_25,  0.25f);
	setInterpolantConstant(InterpDelta_0_30,  0.30f);
}

// static
//-----------------------------------------------------------------------------
// updateInterpolants()
//-----------------------------------------------------------------------------
void LLCriticalDamp::updateInterpolants()
{
	sTimeDelta = sInternalTimer.getElapsedTimeAndResetF32();

	U32 i;
	for (i = 0; i < kNumCachedInterpolants; i++)
	{
		sInterpolatedValues[i] = llclamp(sTimeDelta / sInterpolants[ i], 0.0f, 1.0f);
	}
}

