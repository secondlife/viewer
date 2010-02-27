/** 
 * @file lltimer.h
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

#ifndef LL_TIMER_H					
#define LL_TIMER_H

#if LL_LINUX || LL_DARWIN || LL_SOLARIS
#include <sys/time.h>
#endif
#include <limits.h>

#include "stdtypes.h"
#include "lldate.h"
#include "llinstancetracker.h"

#include <string>
#include <list>
// units conversions
#ifndef USEC_PER_SEC
    const U32	USEC_PER_SEC	= 1000000;
#endif
const U32	SEC_PER_MIN		= 60;
const U32	MIN_PER_HOUR	= 60;
const U32	USEC_PER_MIN	= USEC_PER_SEC * SEC_PER_MIN;
const U32	USEC_PER_HOUR	= USEC_PER_MIN * MIN_PER_HOUR;
const U32	SEC_PER_HOUR	= SEC_PER_MIN * MIN_PER_HOUR;
const F64 	SEC_PER_USEC 	= 1.0 / (F64) USEC_PER_SEC;

class LL_COMMON_API LLTimer 
{
public:
	static LLTimer *sTimer;				// global timer
	
protected:	
	U64 mLastClockCount;
	U64 mExpirationTicks;
	BOOL mStarted;

public:
	LLTimer();
	~LLTimer();

	static void initClass() { if (!sTimer) sTimer = new LLTimer; }
	static void cleanupClass() { delete sTimer; sTimer = NULL; }

	// Return a high precision number of seconds since the start of
	// this application instance.
	static F64 getElapsedSeconds()
	{
		return sTimer->getElapsedTimeF64();
	}

	// Return a high precision usec since epoch
	static U64 getTotalTime();

	// Return a high precision seconds since epoch
	static F64 getTotalSeconds();


	// MANIPULATORS
	void start() { reset(); mStarted = TRUE; }
	void stop() { mStarted = FALSE; }
	void reset();								// Resets the timer
	void setLastClockCount(U64 current_count);		// Sets the timer so that the next elapsed call will be relative to this time
	void setTimerExpirySec(F32 expiration);
	BOOL checkExpirationAndReset(F32 expiration);
	BOOL hasExpired() const;
	F32 getElapsedTimeAndResetF32();	// Returns elapsed time in seconds with reset
	F64 getElapsedTimeAndResetF64();

	F32 getRemainingTimeF32() const;

	static BOOL knownBadTimer();

	// ACCESSORS
	F32 getElapsedTimeF32() const;			// Returns elapsed time in seconds
	F64 getElapsedTimeF64() const;			// Returns elapsed time in seconds

	BOOL getStarted() const { return mStarted; }


	static U64 getCurrentClockCount();		// Returns the raw clockticks
};

//
// Various functions for initializing/accessing clock and timing stuff.  Don't use these without REALLY knowing how they work.
//
LL_COMMON_API U64 get_clock_count();
LL_COMMON_API F64 calc_clock_frequency(U32 msecs);
LL_COMMON_API void update_clock_frequencies();

// Sleep for milliseconds
LL_COMMON_API void ms_sleep(U32 ms);
LL_COMMON_API U32 micro_sleep(U64 us, U32 max_yields = 0xFFFFFFFF);

// Returns the correct UTC time in seconds, like time(NULL).
// Useful on the viewer, which may have its local clock set wrong.
LL_COMMON_API time_t time_corrected();

static inline time_t time_min()
{
	if (sizeof(time_t) == 4)
	{
		return (time_t) INT_MIN;
	} else {
#ifdef LLONG_MIN
		return (time_t) LLONG_MIN;
#else
		return (time_t) LONG_MIN;
#endif
	}
}

static inline time_t time_max()
{
	if (sizeof(time_t) == 4)
	{
		return (time_t) INT_MAX;
	} else {
#ifdef LLONG_MAX
		return (time_t) LLONG_MAX;
#else
		return (time_t) LONG_MAX;
#endif
	}
}

// Correction factor used by time_corrected() above.
extern LL_COMMON_API S32 gUTCOffset;

// Is the current computer (in its current time zone)
// observing daylight savings time?
LL_COMMON_API BOOL is_daylight_savings();

// Converts internal "struct tm" time buffer to Pacific Standard/Daylight Time
// Usage:
// S32 utc_time;
// utc_time = time_corrected();
// struct tm* internal_time = utc_to_pacific_time(utc_time, gDaylight);
LL_COMMON_API struct tm* utc_to_pacific_time(time_t utc_time, BOOL pacific_daylight_time);

LL_COMMON_API void microsecondsToTimecodeString(U64 current_time, std::string& tcstring);
LL_COMMON_API void secondsToTimecodeString(F32 current_time, std::string& tcstring);

// class for scheduling a function to be called at a given frequency (approximate, inprecise)
class LL_COMMON_API LLEventTimer : protected LLInstanceTracker<LLEventTimer>
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
	bool mBusy;
};

U64 LL_COMMON_API totalTime();					// Returns current system time in microseconds

#endif
