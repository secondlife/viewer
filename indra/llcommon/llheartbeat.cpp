/**
 * @file llheartbeat.cpp
 * @brief Class encapsulating logic for telling a watchdog that we live.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
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

#include <errno.h>
#include <signal.h>

#include "linden_common.h"
#include "llapp.h"

#include "llheartbeat.h"

LLHeartbeat::LLHeartbeat(F32 secs_between_heartbeat,
			 F32 aggressive_heartbeat_panic_secs,
			 F32 aggressive_heartbeat_max_blocking_secs)
	: mSecsBetweenHeartbeat(secs_between_heartbeat),
	  mAggressiveHeartbeatPanicSecs(aggressive_heartbeat_panic_secs),
	  mAggressiveHeartbeatMaxBlockingSecs(aggressive_heartbeat_max_blocking_secs),
	  mSuppressed(false)
{
	mBeatTimer.reset();
	mBeatTimer.setTimerExpirySec(mSecsBetweenHeartbeat);
	mPanicTimer.reset();
	mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
}

LLHeartbeat::~LLHeartbeat()
{
	// do nothing.
}

void
LLHeartbeat::setSuppressed(bool is_suppressed)
{
	mSuppressed = is_suppressed;
}

// returns 0 on success, -1 on permanent failure, 1 on temporary failure
int
LLHeartbeat::rawSend()
{
#if LL_WINDOWS
	return 0; // Pretend we succeeded.
#else
	if (mSuppressed)
		return 0; // Pretend we succeeded.

	union sigval dummy;
	int result = sigqueue(getppid(), LL_HEARTBEAT_SIGNAL, dummy);
	if (result == 0)
		return 0; // success

	int err = errno;
	if (err == EAGAIN)
		return 1; // failed to queue, try again

	return -1; // other failure.
#endif
}

int
LLHeartbeat::rawSendWithTimeout(F32 timeout_sec)
{
	int result = 0;

	// Spin tightly until our heartbeat is digested by the watchdog
	// or we time-out.  We don't really want to sleep because our
	// wake-up time might be undesirably synchronised to a hidden
	// clock by the system's scheduler.
	mTimeoutTimer.reset();
	mTimeoutTimer.setTimerExpirySec(timeout_sec);
	do {
		result = rawSend();
		//llinfos << " HEARTSENDc=" << result << llendl;
	} while (result==1 && !mTimeoutTimer.hasExpired());

	return result;
}

bool
LLHeartbeat::send(F32 timeout_sec)
{
	bool total_success = false;
	int result = 1;

	if (timeout_sec > 0.f) {
		// force a spin until success or timeout
		result = rawSendWithTimeout(timeout_sec);
	} else {
		if (mBeatTimer.hasExpired()) {
			// zero-timeout; we don't care too much whether our
			// heartbeat was digested.
			result = rawSend();
			//llinfos << " HEARTSENDb=" << result << llendl;
		}
	}

	if (result == -1) {
		// big failure.
	} else if (result == 0) {
		total_success = true;
	} else {
		// need to retry at some point
	}

	if (total_success) {
		mBeatTimer.reset();
		mBeatTimer.setTimerExpirySec(mSecsBetweenHeartbeat);
		// reset the time until we start panicking about lost
		// heartbeats again.
		mPanicTimer.reset();
		mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
	} else {
		// leave mBeatTimer as expired so we'll lazily poke the
		// watchdog again next time through.
	}

	if (mPanicTimer.hasExpired()) {
		// It's been ages since we successfully had a heartbeat
		// digested by the watchdog.  Sit here and spin a while
		// in the hope that we can force it through.
		llwarns << "Unable to deliver heartbeat to launcher for " << mPanicTimer.getElapsedTimeF32() << " seconds.  Going to try very hard for up to " << mAggressiveHeartbeatMaxBlockingSecs << " seconds." << llendl;
		result = rawSendWithTimeout(mAggressiveHeartbeatMaxBlockingSecs);
		if (result == 0) {
			total_success = true;
		} else {
			// we couldn't even force it through.  That's bad,
			// but we'll try again in a while.
			llwarns << "Could not deliver heartbeat to launcher even after trying very hard for " << mAggressiveHeartbeatMaxBlockingSecs << " seconds." << llendl;
		}
		
		// in any case, reset the panic timer.
		mPanicTimer.reset();
		mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
	}

	return total_success;
}
