/** 
 * @file lleventtimer.h
 * @brief Cross-platform objects for doing timing 
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

#ifndef LL_EVENTTIMER_H					
#define LL_EVENTTIMER_H

#include "stdtypes.h"
#include "lldate.h"
#include "llinstancetracker.h"
#include "lltimer.h"

// class for scheduling a function to be called at a given frequency (approximate, inprecise)
class LL_COMMON_API LLEventTimer : public LLInstanceTracker<LLEventTimer>
{
public:
	LLEventTimer(F32 period);	// period is the amount of time between each call to tick() in seconds
	LLEventTimer(const LLDate& time);
	virtual ~LLEventTimer();
	
	//function to be called at the supplied frequency
	// Normally return FALSE; TRUE will delete the timer after the function returns.
	virtual BOOL tick() = 0;

	static void updateClass();

protected:
	LLTimer mEventTimer;
	F32 mPeriod;
};

#endif //LL_EVENTTIMER_H
