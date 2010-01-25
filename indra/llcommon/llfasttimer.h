/**
 * @file llfasttimer.h
 * @brief Inline implementations of fast timers.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_FASTTIMER_H
#define LL_FASTTIMER_H

// pull in the actual class definition
#include "llfasttimer_class.h"

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
#endif


#if LL_LINUX || LL_SOLARIS
//
// Linux and Solaris implementation of CPU clock - all architectures.
//
// Try to use the MONOTONIC clock if available, this is a constant time counter
// with nanosecond resolution (but not necessarily accuracy) and attempts are made
// to synchronize this value between cores at kernel start. It should not be affected
// by CPU frequency. If not available use the REALTIME clock, but this may be affected by
// NTP adjustments or other user activity affecting the system time.
inline U64 LLFastTimer::getCPUClockCount64()
{
	struct timespec tp;
	
#ifdef CLOCK_MONOTONIC // MONOTONIC supported at build-time?
	if (-1 == clock_gettime(CLOCK_MONOTONIC,&tp)) // if MONOTONIC isn't supported at runtime, try REALTIME
#endif
		clock_gettime(CLOCK_REALTIME,&tp);

	return (tp.tv_sec*LLFastTimer::sClockResolution)+tp.tv_nsec;        
}

inline U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)LLFastTimer::getCPUClockCount64();
}
#endif // (LL_LINUX || LL_SOLARIS))


#if (LL_DARWIN) && (defined(__i386__) || defined(__amd64__))
//
// Mac x86 implementation of CPU clock
inline U32 LLFastTimer::getCPUClockCount32()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return (U32)x >> 8;
}

inline U64 LLFastTimer::getCPUClockCount64()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return x >> 8;
}
#endif


#if ( LL_DARWIN && !(defined(__i386__) || defined(__amd64__)))
//
// Mac PPC (deprecated) implementation of CPU clock
//
// Just use gettimeofday implementation for now

inline U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)get_clock_count();
}

inline U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif

#endif // LL_LLFASTTIMER_H
