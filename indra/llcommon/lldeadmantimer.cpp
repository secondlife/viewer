/** 
* @file lldeadmantimer.cpp
* @brief Simple deadman-switch timer.
* @author monty@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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


#include "lldeadmantimer.h"


// *TODO:  Currently, this uses lltimer functions for its time
// aspects and this leaks into the apis in the U64s/F64s.  Would
// like to perhaps switch this over to TSC register-based timers
// sometime and drop the overhead some more.


//  Flag states and their meaning:
//  mActive  mDone   Meaning
//   false   false   Nothing running, no result available
//    true   false   Timer running, no result available
//   false    true   Timer finished, result can be read once
//    true    true   Not allowed
//
LLDeadmanTimer::LLDeadmanTimer(F64 horizon)
	: mHorizon(U64(llmax(horizon, F64(0.0)) * gClockFrequency)),
	  mActive(false),			// If true, a timer is running.
	  mDone(false),				// If true, timer has completed and can be read (once)
	  mStarted(U64L(0)),
	  mExpires(U64L(0)),
	  mStopped(U64L(0)),
	  mCount(U64L(0))
{}


void LLDeadmanTimer::start(U64 now)
{
	// *TODO:  If active, let's complete an existing timer and save
	// the result to the side.  I think this will be useful later.
	// For now, wipe out anything in progress, start fresh.
	
	if (! now)
	{
		now = LLTimer::getCurrentClockCount();
	}
	mActive = true;
	mDone = false;
	mStarted = now;
	mExpires = now + mHorizon;
	mStopped = now;
	mCount = U64L(0);
}


void LLDeadmanTimer::stop(U64 now)
{
	if (! mActive)
	{
		return;
	}

	if (! now)
	{
		now = LLTimer::getCurrentClockCount();
	}
	mStopped = now;
	mActive = false;
	mDone = true;
}


bool LLDeadmanTimer::isExpired(F64 & started, F64 & stopped, U64 & count, U64 now)
{
	if (mActive && ! mDone)
	{
		if (! now)
		{
			now = LLTimer::getCurrentClockCount();
		}

		if (now >= mExpires)
		{
			// mStopped from ringBell() is the value we want
			mActive = false;
			mDone = true;
		}
	}

	if (! mDone)
	{
		return false;
	}
	
	started = mStarted * gClockFrequencyInv;
	stopped = mStopped * gClockFrequencyInv;
	count = mCount;
	mDone = false;

	return true;
}

	
void LLDeadmanTimer::ringBell(U64 now)
{
	if (! mActive)
	{
		return;
	}
	
	if (! now)
	{
		now = LLTimer::getCurrentClockCount();
	}

	if (now >= mExpires)
	{
		mActive = false;
		mDone = true;
	}
	else
	{
		mStopped = now;
		mExpires = now + mHorizon;
		++mCount;
	}
	
	return;
}

