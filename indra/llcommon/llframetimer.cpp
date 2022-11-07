/** 
 * @file llframetimer.cpp
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

#include "u64.h"

#include "llframetimer.h"

// We don't bother building a stand alone lib; we just need to include the one source file for Tracy support
#if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY || LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY_FAST_TIMER
    #include "TracyClient.cpp"
#endif // LL_PROFILER_CONFIGURATION

// Static members
//LLTimer   LLFrameTimer::sInternalTimer;
U64 LLFrameTimer::sStartTotalTime = totalTime();
F64 LLFrameTimer::sFrameTime = 0.0;
U64 LLFrameTimer::sTotalTime = 0;
F64 LLFrameTimer::sTotalSeconds = 0.0;
S32 LLFrameTimer::sFrameCount = 0;
U64 LLFrameTimer::sFrameDeltaTime = 0;
const F64 USEC_TO_SEC_F64 = 0.000001;

// static
void LLFrameTimer::updateFrameTime()
{
    U64 total_time = totalTime();
    sFrameDeltaTime = total_time - sTotalTime;
    sTotalTime = total_time;
    sTotalSeconds = U64_to_F64(sTotalTime) * USEC_TO_SEC_F64;
    sFrameTime = U64_to_F64(sTotalTime - sStartTotalTime) * USEC_TO_SEC_F64;
} 

void LLFrameTimer::start()
{
    reset();
    mStarted = TRUE;
}

void LLFrameTimer::stop()
{
    mStarted = FALSE;
}

void LLFrameTimer::reset()
{
    mStartTime = sFrameTime;
    mExpiry = sFrameTime;
}

void LLFrameTimer::resetWithExpiry(F32 expiration)
{
    reset();
    setTimerExpirySec(expiration);
}

// Don't combine pause/unpause with start/stop
// Useage:
//  LLFrameTime foo; // starts automatically
//  foo.unpause(); // noop but safe
//  foo.pause(); // pauses timer
//  foo.unpause() // unpauses
//  F32 elapsed = foo.getElapsedTimeF32() // does not include time between pause() and unpause()
//  Note: elapsed would also be valid with no unpause() call (= time run until pause() called)
void LLFrameTimer::pause()
{
    if (mStarted)
        mStartTime = sFrameTime - mStartTime; // save dtime
    mStarted = FALSE;
}

void LLFrameTimer::unpause()
{
    if (!mStarted)
        mStartTime = sFrameTime - mStartTime; // restore dtime
    mStarted = TRUE;
}

void LLFrameTimer::setTimerExpirySec(F32 expiration)
{
    mExpiry = expiration + mStartTime;
}

void LLFrameTimer::setExpiryAt(F64 seconds_since_epoch)
{
    mStartTime = sFrameTime;
    mExpiry = seconds_since_epoch - (USEC_TO_SEC_F64 * sStartTotalTime);
}

F64 LLFrameTimer::expiresAt() const
{
    F64 expires_at = U64_to_F64(sStartTotalTime) * USEC_TO_SEC_F64;
    expires_at += mExpiry;
    return expires_at;
}

BOOL LLFrameTimer::checkExpirationAndReset(F32 expiration)
{
    //LL_INFOS() << "LLFrameTimer::checkExpirationAndReset()" << LL_ENDL;
    //LL_INFOS() << "  mStartTime:" << mStartTime << LL_ENDL;
    //LL_INFOS() << "  sFrameTime:" << sFrameTime << LL_ENDL;
    //LL_INFOS() << "  mExpiry:   " <<  mExpiry << LL_ENDL;

    if(hasExpired())
    {
        reset();
        setTimerExpirySec(expiration);
        return TRUE;
    }
    return FALSE;
}

// static
F32 LLFrameTimer::getFrameDeltaTimeF32()
{
    return (F32)(U64_to_F64(sFrameDeltaTime) * USEC_TO_SEC_F64); 
}


//  static 
// Return seconds since the current frame started
F32  LLFrameTimer::getCurrentFrameTime()
{
    U64 frame_time = totalTime() - sTotalTime;
    return (F32)(U64_to_F64(frame_time) * USEC_TO_SEC_F64); 
}

// Glue code to avoid full class .h file #includes
F32  getCurrentFrameTime()
{
    return (F32)(LLFrameTimer::getCurrentFrameTime());
}
