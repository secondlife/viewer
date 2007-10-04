/** 
 * @file llcriticaldamp.cpp
 * @brief Implementation of the critical damping functionality.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
