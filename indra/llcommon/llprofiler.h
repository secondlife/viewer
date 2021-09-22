/**
 * @file llprofiler.h
 * @brief Wrapper for Tracy and/or other profilers
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#ifndef LL_PROFILER_H
#define LL_PROFILER_H

#define LL_PROFILER_CONFIG_NONE             0  // No profiling
#define LL_PROFILER_CONFIG_FAST_TIMER       1  // Profiling on: Only Fast Timers
#define LL_PROFILER_CONFIG_TRACY            2  // Profiling on: Only Tracy
#define LL_PROFILER_CONFIG_TRACY_FAST_TIMER 3  // Profiling on: Fast Timers + Tracy

#ifndef LL_PROFILER_CONFIGURATION
#define LL_PROFILER_CONFIGURATION           LL_PROFILER_CONFIG_FAST_TIMER
#endif

#if defined(LL_PROFILER_CONFIGURATION) && (LL_PROFILER_CONFIGURATION > LL_PROFILER_CONFIG_NONE)
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY || LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY_FAST_TIMER
        #define TRACY_ENABLE         1
// Normally these would be enabled but we want to be able to build any viewer with Tracy enabled and run the Tracy server on another machine
// They must be undefined in order to work across multiple machines
//      #define TRACY_NO_BROADCAST   1
//      #define TRACY_ONLY_LOCALHOST 1
        #define TRACY_ONLY_IPV4      1
        #include "Tracy.hpp"

        // Mutually exclusive with detailed memory tracing
        #define LL_PROFILER_ENABLE_TRACY_OPENGL 0
    #endif

    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY
        #define LL_PROFILER_FRAME_END               FrameMark
        #define LL_PROFILER_SET_THREAD_NAME( name ) tracy::SetThreadName( name )
        #define LL_RECORD_BLOCK_TIME(name)          ZoneNamedN( ___tracy_scoped_zone, #name, true );
        #define LL_PROFILE_ZONE_NAMED(name)          ZoneNamedN( ___tracy_scoped_zone, name, true );  
        #define LL_PROFILE_ZONE_SCOPED              ZoneScoped
    #endif
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_FAST_TIMER
        #define LL_PROFILER_FRAME_END
        #define LL_PROFILER_SET_THREAD_NAME( name ) (void)(name)
        #define LL_RECORD_BLOCK_TIME(name)                                                                  const LLTrace::BlockTimer& LL_GLUE_TOKENS(block_time_recorder, __LINE__)(LLTrace::timeThisBlock(name)); (void)LL_GLUE_TOKENS(block_time_recorder, __LINE__);
        #define LL_PROFILE_ZONE_NAMED(name) // LL_PROFILE_ZONE_NAMED is a no-op when Tracy is disabled
        #define LL_PROFILE_ZONE_SCOPED      // LL_PROFILE_ZONE_SCOPED is a no-op when Tracy is disabled
    #endif
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY_FAST_TIMER
        #define LL_PROFILER_FRAME_END               FrameMark
        #define LL_PROFILER_SET_THREAD_NAME( name ) tracy::SetThreadName( name )
        #define LL_RECORD_BLOCK_TIME(name)          ZoneNamedN( ___tracy_scoped_zone, #timer_stat, true )   const LLTrace::BlockTimer& LL_GLUE_TOKENS(block_time_recorder, __LINE__)(LLTrace::timeThisBlock(name)); (void)LL_GLUE_TOKENS(block_time_recorder, __LINE__);
        #define LL_PROFILE_ZONE_NAMED(name)         ZoneNamedN( ___tracy_scoped_zone, #name, true );
        #define LL_PROFILE_ZONE_SCOPED              ZoneScoped
    #endif
#else
    #define LL_PROFILER_FRAME_END
    #define LL_PROFILER_SET_THREAD_NAME( name ) (void)(name)
#endif // LL_PROFILER

#endif // LL_PROFILER_H
