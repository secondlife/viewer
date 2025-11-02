/**
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
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

#include "llprocessor.h"

#if LL_X86 || LL_X86_64
#if LL_WINDOWS
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#define LL_FASTTIMER_USE_RDTSC 1

class LLFastTimer
{
public:
    LLFastTimer()                                    = delete;
    LLFastTimer(const LLFastTimer& other)            = delete;
    LLFastTimer& operator=(const LLFastTimer& other) = delete;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Important note: These implementations must be FAST!
    //
#if LL_WINDOWS
    //
    // Windows implementation of CPU clock
    //
#if LL_FASTTIMER_USE_RDTSC

    // shift off lower 8 bits for lower resolution but longer term timing
    // on 1Ghz machine, a 32-bit word will hold ~1000 seconds of timing
    static U32 getCPUClockCount32()
    {
        unsigned __int64 val = __rdtsc();
        val = val >> 8;
        return static_cast<U32>(val);
    }

    // return full timer value, *not* shifted by 8 bits
    static U64 getCPUClockCount64()
    {
        return static_cast<U64>( __rdtsc() );
    }

#else
    //U64 get_clock_count(); // in lltimer.cpp
    // These use QueryPerformanceCounter, which is arguably fine and also works on AMD architectures.
    static U32 getCPUClockCount32()
    {
        return (U32)(get_clock_count()>>8);
    }

    static U64 getCPUClockCount64()
    {
        return get_clock_count();
    }

#endif

#endif


#if (LL_LINUX) && !(defined(__i386__) || defined(__amd64__))
    //
    // Linux implementation of CPU clock - non-x86.
    // This is accurate but SLOW!  Only use out of desperation.
    //
    // Try to use the MONOTONIC clock if available, this is a constant time counter
    // with nanosecond resolution (but not necessarily accuracy) and attempts are
    // made to synchronize this value between cores at kernel start. It should not
    // be affected by CPU frequency. If not available use the REALTIME clock, but
    // this may be affected by NTP adjustments or other user activity affecting
    // the system time.
    static U64 getCPUClockCount64()
    {
        struct timespec tp;

#ifdef CLOCK_MONOTONIC // MONOTONIC supported at build-time?
        if (-1 == clock_gettime(CLOCK_MONOTONIC,&tp)) // if MONOTONIC isn't supported at runtime then ouch, try REALTIME
#endif
            clock_gettime(CLOCK_REALTIME,&tp);

        return (tp.tv_sec*sClockResolution)+tp.tv_nsec;
    }

    static U32 getCPUClockCount32()
    {
        return (U32)(getCPUClockCount64() >> 8);
    }

#endif // (LL_LINUX) && !(defined(__i386__) || defined(__amd64__))


#if LL_DARWIN && LL_ARM64
    //
    // Mac implementation of CPU clock - non-x86.
    //
    static U64 getCPUClockCount64()
    {
        return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    }

    static U32 getCPUClockCount32()
    {
        return (U32)(getCPUClockCount64() >> 8);
    }
#endif // LL_DARWIN && LL_ARM64

#if (LL_LINUX || LL_DARWIN) && (LL_X86 || LL_X86_64)
    //
    // Mac+Linux FAST x86 implementation of CPU clock
    //
#if LL_FASTTIMER_USE_RDTSC
    static U32 getCPUClockCount32()
    {
        U64 time_stamp = __rdtsc() >> 8U;
        return static_cast<U32>(time_stamp);
    }

    static U64 getCPUClockCount64()
    {
        return static_cast<U64>(__rdtsc());
    }
#endif
#endif

    static U64 countsPerSecond();

public:
    // statics
    static U64              sClockResolution;
};

#endif // LL_LLFASTTIMER_H
