/**
 * @file llfasttimer.cpp
 * @brief Implementation of the fast timer.
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
#include "linden_common.h"

#include "llfasttimer.h"

#include "llmemory.h"
#include "llprocessor.h"
#include "llsingleton.h"
#include "llsdserialize.h"
#include "llunits.h"
#include "llsd.h"

#include <boost/bind.hpp>
#include <queue>

#if LL_WINDOWS
#include "lltimer.h"
#elif LL_LINUX
#include <sys/time.h>
#include <sched.h>
#include "lltimer.h"
#elif LL_DARWIN
#include <sys/time.h>
#include "lltimer.h"    // get_clock_count()
#else
#error "architecture not supported"
#endif

//////////////////////////////////////////////////////////////////////////////
// statics
#if LL_LINUX || (LL_DARWIN && LL_ARM64)
U64 LLFastTimer::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64 LLFastTimer::sClockResolution = 1000000; // Microsecond resolution
#endif

//static
#if (LL_DARWIN || LL_LINUX) && !(defined(__i386__) || defined(__amd64__))
U64 LLFastTimer::countsPerSecond()
{
    return sClockResolution;
}
#else // windows or x86-mac or x86-linux
U64 LLFastTimer::countsPerSecond()
{
#if !LL_WINDOWS || LL_FASTTIMER_USE_RDTSC
    //getCPUFrequency returns MHz and sCPUClockFrequency wants to be in Hz
    static LLUnit<U64, LLUnits::Hertz> sCPUClockFrequency = LLProcessorInfo().getCPUFrequency();
    return sCPUClockFrequency.value();
#else
    // If we're not using RDTSC, each fasttimer tick is just a performance counter tick.
    // Not redefining the clock frequency itself (in llprocessor.cpp/calculate_cpu_frequency())
    // since that would change displayed MHz stats for CPUs
    static bool firstcall = true;
    static U64 sCPUClockFrequency;
    if (firstcall)
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&sCPUClockFrequency);
        firstcall = false;
    }
    return sCPUClockFrequency.value();
#endif
}
#endif
