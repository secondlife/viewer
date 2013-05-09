/** 
 * @file lleventtimer.cpp
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

#include "linden_common.h"

#include "lleventtimer.h"

#include "u64.h"


//////////////////////////////////////////////////////////////////////////////
//
//		LLEventTimer Implementation
//
//////////////////////////////////////////////////////////////////////////////

LLEventTimer::LLEventTimer(F32 period)
: mEventTimer()
{
	mPeriod = period;
}

LLEventTimer::LLEventTimer(const LLDate& time)
: mEventTimer()
{
	mPeriod = (F32)(time.secondsSinceEpoch() - LLDate::now().secondsSinceEpoch());
}


LLEventTimer::~LLEventTimer()
{
}

//static
void LLEventTimer::updateClass() 
{
	std::list<LLEventTimer*> completed_timers;
	for (instance_iter iter = beginInstances(); iter != endInstances(); ) 
	{
		LLEventTimer& timer = *iter++;
		F32 et = timer.mEventTimer.getElapsedTimeF32();
		if (timer.mEventTimer.getStarted() && et > timer.mPeriod) {
			timer.mEventTimer.reset();
			if ( timer.tick() )
			{
				completed_timers.push_back( &timer );
			}
		}
	}

	if ( completed_timers.size() > 0 )
	{
		for (std::list<LLEventTimer*>::iterator completed_iter = completed_timers.begin(); 
			 completed_iter != completed_timers.end(); 
			 completed_iter++ ) 
		{
			delete *completed_iter;
		}
	}
}


