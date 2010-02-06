/** 
 * @file lleventtimer.cpp
 * @brief Cross-platform objects for doing timing 
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "lleventtimer.h"

#include "u64.h"


//////////////////////////////////////////////////////////////////////////////
//
//		LLEventTimer Implementation
//
//////////////////////////////////////////////////////////////////////////////
bool LLEventTimer::sInTickLoop = false;

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
	llassert(!LLEventTimer::sInTickLoop); // this LLEventTimer was destroyed from within its own tick() function - bad.  if you want tick() to cause destruction of its own timer, make it return true.
}

//static
void LLEventTimer::updateClass() 
{
	std::list<LLEventTimer*> completed_timers;
	LLEventTimer::sInTickLoop = true;
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
	LLEventTimer::sInTickLoop = false;

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


