/**
 * @file llfasttimer.h
 * @brief Inline implementations of fast timers.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_FASTTIMER_H
#define LL_FASTTIMER_H

// pull in the actual class definition
#include "llfasttimer_class.h"

//
// Important note: These implementations must be FAST!
//

#if LL_WINDOWS
//
// Windows implementation of CPU clock
//

//
// NOTE: put back in when we aren't using platform sdk anymore
//
// because MS has different signatures for these functions in winnt.h
// need to rename them to avoid conflicts
//#define _interlockedbittestandset _renamed_interlockedbittestandset
//#define _interlockedbittestandreset _renamed_interlockedbittestandreset
//#include <intrin.h>
//#undef _interlockedbittestandset
//#undef _interlockedbittestandreset

//inline U32 LLFastTimer::getCPUClockCount32()
//{
//	U64 time_stamp = __rdtsc();
//	return (U32)(time_stamp >> 8);
//}
//
//// return full timer value, *not* shifted by 8 bits
//inline U64 LLFastTimer::getCPUClockCount64()
//{
//	return __rdtsc();
//}

// shift off lower 8 bits for lower resolution but longer term timing
// on 1Ghz machine, a 32-bit word will hold ~1000 seconds of timing
#ifdef USE_RDTSC
inline U32 LLFastTimer::getCPUClockCount32()
{
	U32 ret_val;
	__asm
	{
        _emit   0x0f
        _emit   0x31
		shr eax,8
		shl edx,24
		or eax, edx
		mov dword ptr [ret_val], eax
	}
    return ret_val;
}

// return full timer value, *not* shifted by 8 bits
inline U64 LLFastTimer::getCPUClockCount64()
{
	U64 ret_val;
	__asm
	{
        _emit   0x0f
        _emit   0x31
		mov eax,eax
		mov edx,edx
		mov dword ptr [ret_val+4], edx
		mov dword ptr [ret_val], eax
	}
    return ret_val;
}
#else
LL_COMMON_API U64 get_clock_count(); // in lltimer.cpp
// These use QueryPerformanceCounter, which is arguably fine and also works on amd architectures.
inline U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}

inline U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif

#endif


#if (LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
//
// Linux and Solaris implementation of CPU clock - non-x86.
// This is accurate but SLOW!  Only use out of desperation.
//
// Try to use the MONOTONIC clock if available, this is a constant time counter
// with nanosecond resolution (but not necessarily accuracy) and attempts are
// made to synchronize this value between cores at kernel start. It should not
// be affected by CPU frequency. If not available use the REALTIME clock, but
// this may be affected by NTP adjustments or other user activity affecting
// the system time.
inline U64 LLFastTimer::getCPUClockCount64()
{
	struct timespec tp;
	
#ifdef CLOCK_MONOTONIC // MONOTONIC supported at build-time?
	if (-1 == clock_gettime(CLOCK_MONOTONIC,&tp)) // if MONOTONIC isn't supported at runtime then ouch, try REALTIME
#endif
		clock_gettime(CLOCK_REALTIME,&tp);

	return (tp.tv_sec*LLFastTimer::sClockResolution)+tp.tv_nsec;        
}

inline U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(LLFastTimer::getCPUClockCount64() >> 8);
}
#endif // (LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))


#if (LL_LINUX || LL_SOLARIS || LL_DARWIN) && (defined(__i386__) || defined(__amd64__))
//
// Mac+Linux+Solaris FAST x86 implementation of CPU clock
inline U32 LLFastTimer::getCPUClockCount32()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return (U32)(x >> 8);
}

inline U64 LLFastTimer::getCPUClockCount64()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return x;
}
#endif


#if ( LL_DARWIN && !(defined(__i386__) || defined(__amd64__)))
//
// Mac PPC (deprecated) implementation of CPU clock
//
// Just use gettimeofday implementation for now

inline U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}

inline U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif

#endif // LL_LLFASTTIMER_H
