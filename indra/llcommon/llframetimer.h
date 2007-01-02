/** 
 * @file llframetimer.h
 * @brief A lightweight timer that measures seconds and is only
 * updated once per frame.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	// Call this method once per frame to update the current frame
	// time.
	static void updateFrameTime();

	static S32  getFrameCount()						{ return sFrameCount; }

	static F32	getFrameDeltaTimeF32();

	// MANIPULATORS
	void start()									{ reset(); mStarted = TRUE; }
	void stop()										{ mStarted = FALSE; }
	void reset()									{ mStartTime = sFrameTime; mExpiry = sFrameTime; }
	void setTimerExpirySec(F32 expiration)			{ mExpiry = expiration + mStartTime; }
	void setExpiryAt(F64 seconds_since_epoch);
	BOOL checkExpirationAndReset(F32 expiration);
	F32 getElapsedTimeAndResetF32() 				{ F32 t = F32(sFrameTime - mStartTime); reset(); return t; }

	void setAge(const F64 age)						{ mStartTime = sFrameTime - age; }

	// ACCESSORS
	BOOL hasExpired() const							{ return (sFrameTime >= mExpiry); }
	F32  getTimeToExpireF32() const					{ return (F32)(mExpiry - sFrameTime); }
	F32  getElapsedTimeF32() const					{ return (F32)(sFrameTime - mStartTime); }
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

#endif  // LL_LLFRAMETIMER_H
