/** 
 * @file lltimer.cpp
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

#include "lltimer.h"

#include "u64.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#elif LL_LINUX || LL_SOLARIS || LL_DARWIN
#       include <errno.h>
#	include <sys/time.h>
#else 
#	error "architecture not supported"
#endif


//
// Locally used constants
//
const U32 SEC_PER_DAY = 86400;
const F64 SEC_TO_MICROSEC = 1000000.f;
const U64 SEC_TO_MICROSEC_U64 = 1000000;
const F64 USEC_TO_SEC_F64 = 0.000001;


//---------------------------------------------------------------------------
// Globals and statics
//---------------------------------------------------------------------------

S32 gUTCOffset = 0; // viewer's offset from server UTC, in seconds
LLTimer* LLTimer::sTimer = NULL;

F64 gClockFrequency = 0.0;
F64 gClockFrequencyInv = 0.0;
F64 gClocksToMicroseconds = 0.0;
U64 gTotalTimeClockCount = 0;
U64 gLastTotalTimeClockCount = 0;

//
// Forward declarations
//


//---------------------------------------------------------------------------
// Implementation
//---------------------------------------------------------------------------

#if LL_WINDOWS
void ms_sleep(U32 ms)
{
	Sleep(ms);
}

U32 micro_sleep(U64 us, U32 max_yields)
{
    // max_yields is unused; just fiddle with it to avoid warnings.
    max_yields = 0;
    ms_sleep(us / 1000);
    return 0;
}
#elif LL_LINUX || LL_SOLARIS || LL_DARWIN
static void _sleep_loop(struct timespec& thiswait)
{
	struct timespec nextwait;
	bool sleep_more = false;

	do {
		int result = nanosleep(&thiswait, &nextwait);

		// check if sleep was interrupted by a signal; unslept
		// remainder was written back into 't' and we just nanosleep
		// again.
		sleep_more = (result == -1 && EINTR == errno);

		if (sleep_more)
		{
			if ( nextwait.tv_sec > thiswait.tv_sec ||
			     (nextwait.tv_sec == thiswait.tv_sec &&
			      nextwait.tv_nsec >= thiswait.tv_nsec) )
			{
				// if the remaining time isn't actually going
				// down then we're being shafted by low clock
				// resolution - manually massage the sleep time
				// downward.
				if (nextwait.tv_nsec > 1000000) {
					// lose 1ms
					nextwait.tv_nsec -= 1000000;
				} else {
					if (nextwait.tv_sec == 0) {
						// already so close to finished
						sleep_more = false;
					} else {
						// lose up to 1ms
						nextwait.tv_nsec = 0;
					}
				}
			}
			thiswait = nextwait;
		}
	} while (sleep_more);
}

U32 micro_sleep(U64 us, U32 max_yields)
{
    U64 start = get_clock_count();
    // This is kernel dependent.  Currently, our kernel generates software clock
    // interrupts at 250 Hz (every 4,000 microseconds).
    const U64 KERNEL_SLEEP_INTERVAL_US = 4000;

    S32 num_sleep_intervals = (us - (KERNEL_SLEEP_INTERVAL_US >> 1)) / KERNEL_SLEEP_INTERVAL_US;
    if (num_sleep_intervals > 0)
    {
        U64 sleep_time = (num_sleep_intervals * KERNEL_SLEEP_INTERVAL_US) - (KERNEL_SLEEP_INTERVAL_US >> 1);
        struct timespec thiswait;
        thiswait.tv_sec = sleep_time / 1000000;
        thiswait.tv_nsec = (sleep_time % 1000000) * 1000l;
        _sleep_loop(thiswait);
    }

    U64 current_clock = get_clock_count();
    U32 yields = 0;
    while (    (yields < max_yields)
            && (current_clock - start < us) )
    {
        sched_yield();
        ++yields;
        current_clock = get_clock_count();
    }
    return yields;
}

void ms_sleep(U32 ms)
{
	long mslong = ms; // tv_nsec is a long
	struct timespec thiswait;
	thiswait.tv_sec = ms / 1000;
	thiswait.tv_nsec = (mslong % 1000) * 1000000l;
    _sleep_loop(thiswait);
}
#else
# error "architecture not supported"
#endif

//
// CPU clock/other clock frequency and count functions
//

#if LL_WINDOWS
U64 get_clock_count()
{
	static bool firstTime = true;
	static U64 offset;
		// ensures that callers to this function never have to deal with wrap

	// QueryPerformanceCounter implementation
	LARGE_INTEGER clock_count;
	QueryPerformanceCounter(&clock_count);
	if (firstTime) {
		offset = clock_count.QuadPart;
		firstTime = false;
	}
	return clock_count.QuadPart - offset;
}

F64 calc_clock_frequency(U32 uiMeasureMSecs)
{
	__int64 freq;
	QueryPerformanceFrequency((LARGE_INTEGER *) &freq);
	return (F64)freq;
}
#endif // LL_WINDOWS


#if LL_LINUX || LL_DARWIN || LL_SOLARIS
// Both Linux and Mac use gettimeofday for accurate time
F64 calc_clock_frequency(unsigned int uiMeasureMSecs)
{
	return 1000000.0; // microseconds, so 1 Mhz.
}

U64 get_clock_count()
{
	// Linux clocks are in microseconds
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*SEC_TO_MICROSEC_U64 + tv.tv_usec;
}
#endif


void update_clock_frequencies()
{
	gClockFrequency = calc_clock_frequency(50U);
	gClockFrequencyInv = 1.0/gClockFrequency;
	gClocksToMicroseconds = gClockFrequencyInv * SEC_TO_MICROSEC;
}


///////////////////////////////////////////////////////////////////////////////

// returns a U64 number that represents the number of 
// microseconds since the unix epoch - Jan 1, 1970
U64 totalTime()
{
	U64 current_clock_count = get_clock_count();
	if (!gTotalTimeClockCount)
	{
		update_clock_frequencies();
		gTotalTimeClockCount = current_clock_count;

#if LL_WINDOWS
		// Synch us up with local time (even though we PROBABLY don't need to, this is how it was implemented)
		// Unix platforms use gettimeofday so they are synced, although this probably isn't a good assumption to
		// make in the future.

		gTotalTimeClockCount = (U64)(time(NULL) * gClockFrequency);
#endif

		// Update the last clock count
		gLastTotalTimeClockCount = current_clock_count;
	}
	else
	{
		if (current_clock_count >= gLastTotalTimeClockCount)
		{
			// No wrapping, we're all okay.
			gTotalTimeClockCount += current_clock_count - gLastTotalTimeClockCount;
		}
		else
		{
			// We've wrapped.  Compensate correctly
			gTotalTimeClockCount += (0xFFFFFFFFFFFFFFFFULL - gLastTotalTimeClockCount) + current_clock_count;
		}

		// Update the last clock count
		gLastTotalTimeClockCount = current_clock_count;
	}

	// Return the total clock tick count in microseconds.
	return (U64)(gTotalTimeClockCount*gClocksToMicroseconds);
}


///////////////////////////////////////////////////////////////////////////////

LLTimer::LLTimer()
{
	if (!gClockFrequency)
	{
		update_clock_frequencies();
	}

	mStarted = TRUE;
	reset();
}

LLTimer::~LLTimer()
{
}

// static
U64 LLTimer::getTotalTime()
{
	// simply call into the implementation function.
	return totalTime();
}	

// static
F64 LLTimer::getTotalSeconds()
{
	return U64_to_F64(getTotalTime()) * USEC_TO_SEC_F64;
}

void LLTimer::reset()
{
	mLastClockCount = get_clock_count();
	mExpirationTicks = 0;
}

///////////////////////////////////////////////////////////////////////////////

U64 LLTimer::getCurrentClockCount()
{
	return get_clock_count();
}

///////////////////////////////////////////////////////////////////////////////

void LLTimer::setLastClockCount(U64 current_count)
{
	mLastClockCount = current_count;
}

///////////////////////////////////////////////////////////////////////////////

static
U64 getElapsedTimeAndUpdate(U64& lastClockCount)
{
	U64 current_clock_count = get_clock_count();
	U64 result;

	if (current_clock_count >= lastClockCount)
	{
		result = current_clock_count - lastClockCount;
	}
	else
	{
		// time has gone backward
		result = 0;
	}

	lastClockCount = current_clock_count;

	return result;
}


F64 LLTimer::getElapsedTimeF64() const
{
	U64 last = mLastClockCount;
	return (F64)getElapsedTimeAndUpdate(last) * gClockFrequencyInv;
}

F32 LLTimer::getElapsedTimeF32() const
{
	return (F32)getElapsedTimeF64();
}

F64 LLTimer::getElapsedTimeAndResetF64()
{
	return (F64)getElapsedTimeAndUpdate(mLastClockCount) * gClockFrequencyInv;
}

F32 LLTimer::getElapsedTimeAndResetF32()
{
	return (F32)getElapsedTimeAndResetF64();
}

///////////////////////////////////////////////////////////////////////////////

void  LLTimer::setTimerExpirySec(F32 expiration)
{
	mExpirationTicks = get_clock_count()
		+ (U64)((F32)(expiration * gClockFrequency));
}

F32 LLTimer::getRemainingTimeF32() const
{
	U64 cur_ticks = get_clock_count();
	if (cur_ticks > mExpirationTicks)
	{
		return 0.0f;
	}
	return F32((mExpirationTicks - cur_ticks) * gClockFrequencyInv);
}


BOOL  LLTimer::checkExpirationAndReset(F32 expiration)
{
	U64 cur_ticks = get_clock_count();
	if (cur_ticks < mExpirationTicks)
	{
		return FALSE;
	}

	mExpirationTicks = cur_ticks
		+ (U64)((F32)(expiration * gClockFrequency));
	return TRUE;
}


BOOL  LLTimer::hasExpired() const
{
	return (get_clock_count() >= mExpirationTicks)
		? TRUE : FALSE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL LLTimer::knownBadTimer()
{
	BOOL failed = FALSE;

#if LL_WINDOWS
	WCHAR bad_pci_list[][10] = {L"1039:0530",
						        L"1039:0620",
							    L"10B9:0533",
							    L"10B9:1533",
							    L"1106:0596",
							    L"1106:0686",
							    L"1166:004F",
							    L"1166:0050",
 							    L"8086:7110",
							    L"\0"
	};

	HKEY hKey = NULL;
	LONG nResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Enum\\PCI", 0,
								  KEY_EXECUTE | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey);
	
	WCHAR name[1024];
	DWORD name_len = 1024;
	FILETIME scrap;

	S32 key_num = 0;
	WCHAR pci_id[10];

	wcscpy(pci_id, L"0000:0000");	 /*Flawfinder: ignore*/

	while (nResult == ERROR_SUCCESS)
	{
		nResult = ::RegEnumKeyEx(hKey, key_num++, name, &name_len, NULL, NULL, NULL, &scrap);

		if (nResult == ERROR_SUCCESS)
		{
			memcpy(&pci_id[0],&name[4],4);		/* Flawfinder: ignore */
			memcpy(&pci_id[5],&name[13],4);		/* Flawfinder: ignore */

			for (S32 check = 0; bad_pci_list[check][0]; check++)
			{
				if (!wcscmp(pci_id, bad_pci_list[check]))
				{
//					llwarns << "unreliable PCI chipset found!! " << pci_id << endl;
					failed = TRUE;
					break;
				}
			}
//			llinfo << "PCI chipset found: " << pci_id << endl;
			name_len = 1024;
		}
	}
#endif
	return(failed);
}

///////////////////////////////////////////////////////////////////////////////
// 
// NON-MEMBER FUNCTIONS
//
///////////////////////////////////////////////////////////////////////////////

time_t time_corrected()
{
	return time(NULL) + gUTCOffset;
}


// Is the current computer (in its current time zone)
// observing daylight savings time?
BOOL is_daylight_savings()
{
	time_t now = time(NULL);

	// Internal buffer to local server time
	struct tm* internal_time = localtime(&now);

	// tm_isdst > 0  =>  daylight savings
	// tm_isdst = 0  =>  not daylight savings
	// tm_isdst < 0  =>  can't tell
	return (internal_time->tm_isdst > 0);
}


struct tm* utc_to_pacific_time(time_t utc_time, BOOL pacific_daylight_time)
{
	S32 pacific_offset_hours;
	if (pacific_daylight_time)
	{
		pacific_offset_hours = 7;
	}
	else
	{
		pacific_offset_hours = 8;
	}

	// We subtract off the PST/PDT offset _before_ getting
	// "UTC" time, because this will handle wrapping around
	// for 5 AM UTC -> 10 PM PDT of the previous day.
	utc_time -= pacific_offset_hours * MIN_PER_HOUR * SEC_PER_MIN;
 
	// Internal buffer to PST/PDT (see above)
	struct tm* internal_time = gmtime(&utc_time);

	/*
	// Don't do this, this won't correctly tell you if daylight savings is active in CA or not.
	if (pacific_daylight_time)
	{
		internal_time->tm_isdst = 1;
	}
	*/

	return internal_time;
}


void microsecondsToTimecodeString(U64 current_time, std::string& tcstring)
{
	U64 hours;
	U64 minutes;
	U64 seconds;
	U64 frames;
	U64 subframes;

	hours = current_time / (U64)3600000000ul;
	minutes = current_time / (U64)60000000;
	minutes %= 60;
	seconds = current_time / (U64)1000000;
	seconds %= 60;
	frames = current_time / (U64)41667;
	frames %= 24;
	subframes = current_time / (U64)42;
	subframes %= 100;

	tcstring = llformat("%3.3d:%2.2d:%2.2d:%2.2d.%2.2d",(int)hours,(int)minutes,(int)seconds,(int)frames,(int)subframes);
}


void secondsToTimecodeString(F32 current_time, std::string& tcstring)
{
	microsecondsToTimecodeString((U64)((F64)(SEC_TO_MICROSEC*current_time)), tcstring);
}


//////////////////////////////////////////////////////////////////////////////
//
//		LLEventTimer Implementation
//
//////////////////////////////////////////////////////////////////////////////

LLEventTimer::LLEventTimer(F32 period)
: mEventTimer()
{
	mPeriod = period;
	mBusy = false;
}

LLEventTimer::LLEventTimer(const LLDate& time)
: mEventTimer()
{
	mPeriod = (F32)(time.secondsSinceEpoch() - LLDate::now().secondsSinceEpoch());
	mBusy = false;
}


LLEventTimer::~LLEventTimer()
{
	llassert(!mBusy); // this LLEventTimer was destroyed from within its own tick() function - bad.  if you want tick() to cause destruction of its own timer, make it return true.
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
			timer.mBusy = true;
			if ( timer.tick() )
			{
				completed_timers.push_back( &timer );
			}
			timer.mBusy = false;
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


