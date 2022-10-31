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
LLDeadmanTimer::LLDeadmanTimer(F64 horizon, bool inc_cpu)
    : mHorizon(time_type(llmax(horizon, F64(0.0)) * get_timer_info().mClockFrequency)),
      mActive(false),           // If true, a timer is running.
      mDone(false),             // If true, timer has completed and can be read (once)
      mStarted(U64L(0)),
      mExpires(U64L(0)),
      mStopped(U64L(0)),
      mCount(U64L(0)),
      mIncCPU(inc_cpu),
      mUStartCPU(LLProcInfo::time_type(U64L(0))),
      mUEndCPU(LLProcInfo::time_type(U64L(0))),
      mSStartCPU(LLProcInfo::time_type(U64L(0))),
      mSEndCPU(LLProcInfo::time_type(U64L(0)))
{}


// static
LLDeadmanTimer::time_type LLDeadmanTimer::getNow()
{
    return LLTimer::getCurrentClockCount();
}


void LLDeadmanTimer::start(time_type now)
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
    if (mIncCPU)
    {
        LLProcInfo::getCPUUsage(mUStartCPU, mSStartCPU);
    }
}


void LLDeadmanTimer::stop(time_type now)
{
    if (! mActive)
    {
        return;
    }

    if (! now)
    {
        now = getNow();
    }
    mStopped = now;
    mActive = false;
    mDone = true;
    if (mIncCPU)
    {
        LLProcInfo::getCPUUsage(mUEndCPU, mSEndCPU);
    }
}


bool LLDeadmanTimer::isExpired(time_type now, F64 & started, F64 & stopped, U64 & count,
                               U64 & user_cpu, U64 & sys_cpu)
{
    const bool status(isExpired(now, started, stopped, count));
    if (status)
    {
        user_cpu = U64(mUEndCPU - mUStartCPU);
        sys_cpu = U64(mSEndCPU - mSStartCPU);
    }
    return status;
}

        
bool LLDeadmanTimer::isExpired(time_type now, F64 & started, F64 & stopped, U64 & count)
{
    if (mActive && ! mDone)
    {
        if (! now)
        {
            now = getNow();
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
    
    started = mStarted * get_timer_info().mClockFrequencyInv;
    stopped = mStopped * get_timer_info().mClockFrequencyInv;
    count = mCount;
    mDone = false;

    return true;
}

    
void LLDeadmanTimer::ringBell(time_type now, unsigned int count)
{
    if (! mActive)
    {
        return;
    }
    
    if (! now)
    {
        now = getNow();
    }

    if (now >= mExpires)
    {
        // Timer has expired, this event will be dropped
        mActive = false;
        mDone = true;
    }
    else
    {
        // Timer renewed, keep going
        mStopped = now;
        mExpires = now + mHorizon;
        mCount += count;
        if (mIncCPU)
        {
            LLProcInfo::getCPUUsage(mUEndCPU, mSEndCPU);
        }
    }
    
    return;
}

