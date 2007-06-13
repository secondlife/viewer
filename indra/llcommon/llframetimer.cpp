/** 
 * @file llframetimer.cpp
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "u64.h"

#include "llframetimer.h"

// Static members
//LLTimer	LLFrameTimer::sInternalTimer;
U64 LLFrameTimer::sStartTotalTime = totalTime();
F64 LLFrameTimer::sFrameTime = 0.0;
U64 LLFrameTimer::sTotalTime = 0;
F64 LLFrameTimer::sTotalSeconds = 0.0;
S32 LLFrameTimer::sFrameCount = 0;
U64 LLFrameTimer::sFrameDeltaTime = 0;
const F64 USEC_PER_SECOND = 1000000.0;
const F64 USEC_TO_SEC_F64 = 0.000001;

// static
void LLFrameTimer::updateFrameTime()
{
	U64 total_time = totalTime();
	sFrameDeltaTime = total_time - sTotalTime;
	sTotalTime = total_time;
	sTotalSeconds = U64_to_F64(sTotalTime) * USEC_TO_SEC_F64;
	sFrameTime = U64_to_F64(sTotalTime - sStartTotalTime) * USEC_TO_SEC_F64;
	sFrameCount++;
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
	//llinfos << "LLFrameTimer::checkExpirationAndReset()" << llendl;
	//llinfos << "  mStartTime:" << mStartTime << llendl;
	//llinfos << "  sFrameTime:" << sFrameTime << llendl;
	//llinfos << "  mExpiry:   " <<  mExpiry << llendl;

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
