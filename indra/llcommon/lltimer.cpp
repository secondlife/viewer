/** 
 * @file lltimer.cpp
 * @brief Cross-platform objects for doing timing 
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "lltimer.h"

#include "u64.h"

#include <chrono>
#include <thread>

#if LL_WINDOWS
#	include "llwin32headerslean.h"
#elif LL_LINUX || LL_DARWIN
#   include <errno.h>
#	include <sys/time.h>
#else 
#	error "architecture not supported"
#endif

//
// Locally used constants
//
const U64 SEC_TO_MICROSEC_U64 = 1000000;

//---------------------------------------------------------------------------
// Globals and statics
//---------------------------------------------------------------------------

S32 gUTCOffset = 0; // viewer's offset from server UTC, in seconds
LLTimer* LLTimer::sTimer = NULL;


//
// Forward declarations
//


//---------------------------------------------------------------------------
// Implementation
//---------------------------------------------------------------------------

#if LL_WINDOWS


#if 0
void ms_sleep(U32 ms)
{
    LL_PROFILE_ZONE_SCOPED;
    using TimePoint = std::chrono::steady_clock::time_point;
    auto resume_time = TimePoint::clock::now() + std::chrono::milliseconds(ms);
    while (TimePoint::clock::now() < resume_time)
    {
        std::this_thread::yield(); //note: don't use LLThread::yield here to avoid yielding for too long
    }
}

U32 micro_sleep(U64 us, U32 max_yields)
{
    // max_yields is unused; just fiddle with it to avoid warnings.
    max_yields = 0;
	ms_sleep((U32)(us / 1000));
    return 0;
}

#else

U32 micro_sleep(U64 us, U32 max_yields)
{
    LL_PROFILE_ZONE_SCOPED
#if 0
    LARGE_INTEGER ft;
    ft.QuadPart = -static_cast<S64>(us * 10);  // '-' using relative time

    HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    Sleep(us / 1000);
#endif

    return 0;
}

void ms_sleep(U32 ms)
{
    LL_PROFILE_ZONE_SCOPED
    micro_sleep(ms * 1000, 0);
}

#endif

#elif LL_LINUX || LL_DARWIN
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
    const S64 KERNEL_SLEEP_INTERVAL_US = 4000;

    // Use signed arithmetic to discover whether a sleep is even necessary. If
    // either 'us' or KERNEL_SLEEP_INTERVAL_US is unsigned, the compiler
    // promotes the difference to unsigned. If 'us' is less than half
    // KERNEL_SLEEP_INTERVAL_US, the unsigned difference will be hugely
    // positive, resulting in a crazy long wait.
    auto num_sleep_intervals = (S64(us) - (KERNEL_SLEEP_INTERVAL_US >> 1)) / KERNEL_SLEEP_INTERVAL_US;
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

F64 calc_clock_frequency()
{
	__int64 freq;
	QueryPerformanceFrequency((LARGE_INTEGER *) &freq);
	return (F64)freq;
}
#endif // LL_WINDOWS


#if LL_LINUX || LL_DARWIN
// Both Linux and Mac use gettimeofday for accurate time
F64 calc_clock_frequency()
{
	return 1000000.0; // microseconds, so 1 MHz.
}

U64 get_clock_count()
{
	// Linux clocks are in microseconds
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*SEC_TO_MICROSEC_U64 + tv.tv_usec;
}
#endif


TimerInfo::TimerInfo()
:	mClockFrequency(0.0),
	mTotalTimeClockCount(0),
	mLastTotalTimeClockCount(0)
{}

void TimerInfo::update()
{
	mClockFrequency = calc_clock_frequency();
	mClockFrequencyInv = 1.0/mClockFrequency;
	mClocksToMicroseconds = mClockFrequencyInv;
}

TimerInfo& get_timer_info()
{
	static TimerInfo sTimerInfo;
	return sTimerInfo;
}

///////////////////////////////////////////////////////////////////////////////

// returns a U64 number that represents the number of 
// microseconds since the Unix epoch - Jan 1, 1970
U64MicrosecondsImplicit totalTime()
{
	U64 current_clock_count = get_clock_count();
	if (!get_timer_info().mTotalTimeClockCount || get_timer_info().mClocksToMicroseconds.value() == 0)
	{
		get_timer_info().update();
		get_timer_info().mTotalTimeClockCount = current_clock_count;

#if LL_WINDOWS
		// Sync us up with local time (even though we PROBABLY don't need to, this is how it was implemented)
		// Unix platforms use gettimeofday so they are synced, although this probably isn't a good assumption to
		// make in the future.

		get_timer_info().mTotalTimeClockCount = (U64)(time(NULL) * get_timer_info().mClockFrequency);
#endif

		// Update the last clock count
		get_timer_info().mLastTotalTimeClockCount = current_clock_count;
	}
	else
	{
		if (current_clock_count >= get_timer_info().mLastTotalTimeClockCount)
		{
			// No wrapping, we're all okay.
			get_timer_info().mTotalTimeClockCount += current_clock_count - get_timer_info().mLastTotalTimeClockCount;
		}
		else
		{
			// We've wrapped.  Compensate correctly
			get_timer_info().mTotalTimeClockCount += (0xFFFFFFFFFFFFFFFFULL - get_timer_info().mLastTotalTimeClockCount) + current_clock_count;
		}

		// Update the last clock count
		get_timer_info().mLastTotalTimeClockCount = current_clock_count;
	}

	// Return the total clock tick count in microseconds.
	U64Microseconds time(get_timer_info().mTotalTimeClockCount*get_timer_info().mClocksToMicroseconds);
	return time;
}


///////////////////////////////////////////////////////////////////////////////

LLTimer::LLTimer()
{
	if (!get_timer_info().mClockFrequency)
	{
		get_timer_info().update();
	}

	mStarted = TRUE;
	reset();
}

LLTimer::~LLTimer()
{}

// static
void LLTimer::initClass()
{
	if (!sTimer) sTimer = new LLTimer;
}

// static
void LLTimer::cleanupClass()
{
	delete sTimer; sTimer = NULL;
}

// static
U64MicrosecondsImplicit LLTimer::getTotalTime()
{
	// simply call into the implementation function.
	U64MicrosecondsImplicit total_time = totalTime();
	return total_time;
}	

// static
F64SecondsImplicit LLTimer::getTotalSeconds()
{
	return F64Microseconds(U64_to_F64(getTotalTime()));
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


F64SecondsImplicit LLTimer::getElapsedTimeF64() const
{
	U64 last = mLastClockCount;
	return (F64)getElapsedTimeAndUpdate(last) * get_timer_info().mClockFrequencyInv;
}

F32SecondsImplicit LLTimer::getElapsedTimeF32() const
{
	return (F32)getElapsedTimeF64();
}

F64SecondsImplicit LLTimer::getElapsedTimeAndResetF64()
{
	return (F64)getElapsedTimeAndUpdate(mLastClockCount) * get_timer_info().mClockFrequencyInv;
}

F32SecondsImplicit LLTimer::getElapsedTimeAndResetF32()
{
	return (F32)getElapsedTimeAndResetF64();
}

///////////////////////////////////////////////////////////////////////////////

void  LLTimer::setTimerExpirySec(F32SecondsImplicit expiration)
{
	mExpirationTicks = get_clock_count()
		+ (U64)((F32)(expiration * get_timer_info().mClockFrequency.value()));
}

F32SecondsImplicit LLTimer::getRemainingTimeF32() const
{
	U64 cur_ticks = get_clock_count();
	if (cur_ticks > mExpirationTicks)
	{
		return 0.0f;
	}
	return F32((mExpirationTicks - cur_ticks) * get_timer_info().mClockFrequencyInv);
}


BOOL  LLTimer::checkExpirationAndReset(F32 expiration)
{
	U64 cur_ticks = get_clock_count();
	if (cur_ticks < mExpirationTicks)
	{
		return FALSE;
	}

	mExpirationTicks = cur_ticks
		+ (U64)((F32)(expiration * get_timer_info().mClockFrequency));
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
//					LL_WARNS() << "unreliable PCI chipset found!! " << pci_id << endl;
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
	S32Hours pacific_offset_hours;
	if (pacific_daylight_time)
	{
		pacific_offset_hours = S32Hours(7);
	}
	else
	{
		pacific_offset_hours = S32Hours(8);
	}

	// We subtract off the PST/PDT offset _before_ getting
	// "UTC" time, because this will handle wrapping around
	// for 5 AM UTC -> 10 PM PDT of the previous day.
	utc_time -= S32SecondsImplicit(pacific_offset_hours);
 
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


void microsecondsToTimecodeString(U64MicrosecondsImplicit current_time, std::string& tcstring)
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


void secondsToTimecodeString(F32SecondsImplicit current_time, std::string& tcstring)
{
	microsecondsToTimecodeString(current_time, tcstring);
}


