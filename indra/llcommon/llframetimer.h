/** 
 * @file llframetimer.h
 * @brief A lightweight timer that measures seconds and is only
 * updated once per frame.
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

#ifndef LL_LLFRAMETIMER_H
#define LL_LLFRAMETIMER_H

/**
 * *NOTE: Because of limitations on linux which we do not really have
 * time to explore, the total time is derived from the frame time
 * and is recsynchronized on every frame.
 */

#include "lltimer.h"
#include "timing.h"

class LLFrameTimer 
{
public:
	LLFrameTimer() : mStartTime( sFrameTime ), mExpiry(0), mStarted(TRUE) {}

	// Return the number of seconds since the start of this
	// application instance.
	static F64 getElapsedSeconds()
	{
		// Loses msec precision after ~4.5 hours...
		return sFrameTime;
	} 

	// Return a low precision usec since epoch
	static U64 getTotalTime()
	{
		return sTotalTime ? sTotalTime : totalTime();
	}

	// Return a low precision seconds since epoch
	static F64 getTotalSeconds()
	{
		return sTotalSeconds;
	}

	// Call this method once per frame to update the current frame time.   This is actually called
	// at some other times as well
	static void updateFrameTime();

	// Call this method once, and only once, per frame to update the current frame count.
	static void updateFrameCount()					{ sFrameCount++; }

	static U32  getFrameCount()						{ return sFrameCount; }

	static F32	getFrameDeltaTimeF32();

	// Return seconds since the current frame started
	static F32  getCurrentFrameTime();

	// MANIPULATORS
	void start();
	void stop();
	void reset();
	void resetWithExpiry(F32 expiration);
	void pause();
	void unpause();
	void setTimerExpirySec(F32 expiration);
	void setExpiryAt(F64 seconds_since_epoch);
	BOOL checkExpirationAndReset(F32 expiration);
	F32 getElapsedTimeAndResetF32() 				{ F32 t = F32(sFrameTime - mStartTime); reset(); return t; }

	void setAge(const F64 age)						{ mStartTime = sFrameTime - age; }

	// ACCESSORS
	BOOL hasExpired() const							{ return (sFrameTime >= mExpiry); }
	F32  getTimeToExpireF32() const					{ return (F32)(mExpiry - sFrameTime); }
	F32  getElapsedTimeF32() const					{ return mStarted ? (F32)(sFrameTime - mStartTime) : (F32)mStartTime; }
	BOOL getStarted() const							{ return mStarted; }

	// return the seconds since epoch when this timer will expire.
	F64 expiresAt() const;

protected:	
	// A single, high resolution timer that drives all LLFrameTimers
	// *NOTE: no longer used.
	//static LLTimer sInternalTimer;		

	//
	// Aplication constants
	//

	// Start time of opp in usec since epoch
	static U64 sStartTotalTime;	

	// 
	// Data updated per frame
	//

	// Seconds since application start
	static F64 sFrameTime;

	// Time that has elapsed since last call to updateFrameTime()
	static U64 sFrameDeltaTime;

	// Total microseconds since epoch.
	static U64 sTotalTime;			

	// Seconds since epoch.
	static F64 sTotalSeconds;

	// Total number of frames elapsed in application
	static S32 sFrameCount;

	//
	// Member data
	//

	// Number of seconds after application start when this timer was
	// started. Set equal to sFrameTime when reset.
	F64 mStartTime;

	// Timer expires this many seconds after application start time.
	F64 mExpiry;

	// Useful bit of state usually associated with timers, but does
	// not affect actual functionality
	BOOL mStarted;
};

// Glue code for Havok (or anything else that doesn't want the full .h files)
extern F32  getCurrentFrameTime();

#endif  // LL_LLFRAMETIMER_H
