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

// If you use the default macros LL_PROFILE_ZONE_SCOPED and LL_PROFILE_ZONE_NAMED to profile code ...
//
//     void foo()
//     {
//         LL_PROFILE_ZONE_SCOPED;
//         :
//
//         {
//             LL_PROFILE_ZONE_NAMED("widget bar");
//             :
//         }
//         {
//             LL_PROFILE_ZONE_NAMED("widget qux");
//             :
//         }
//     }
//
// ... please be aware that ALL these will show up in a Tracy capture which can quickly exhaust memory.
// Instead, use LL_PROFILE_ZONE_SCOPED_CATEGORY_* and LL_PROFILE_ZONE_NAMED_CATEGORY_* to profile code ...
//
//     void foo()
//     {
//         LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
//         :
//
//         {
//             LL_PROFILE_ZONE_NAMED_CATEGORY_UI("widget bar");
//             :
//         }
//         {
//             LL_PROFILE_ZONE_NAMED_CATEGORY_UI("widget qux");
//             :
//         }
//     }
//
// ... as these can be selectively turned on/off.  This will minimize memory usage and visual clutter in a Tracy capture.
// See llprofiler_categories.h for more details on profiling categories.

#define LL_PROFILER_CONFIG_NONE             0  // No profiling
#define LL_PROFILER_CONFIG_FAST_TIMER       1  // Profiling on: Only Fast Timers
#define LL_PROFILER_CONFIG_TRACY            2  // Profiling on: Only Tracy
#define LL_PROFILER_CONFIG_TRACY_FAST_TIMER 3  // Profiling on: Fast Timers + Tracy

#ifndef LL_PROFILER_CONFIGURATION
#define LL_PROFILER_CONFIGURATION           LL_PROFILER_CONFIG_FAST_TIMER
#endif

#if defined(LL_PROFILER_CONFIGURATION) && (LL_PROFILER_CONFIGURATION > LL_PROFILER_CONFIG_NONE)
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY || LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY_FAST_TIMER
        #include "tracy/Tracy.hpp"

        // Enable RenderDoc labeling
        //#define LL_PROFILER_ENABLE_RENDER_DOC 0

    #endif

    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY
        #define LL_PROFILER_FRAME_END                   FrameMark
        #define LL_PROFILER_SET_THREAD_NAME( name )     tracy::SetThreadName( name );
        #define LL_RECORD_BLOCK_TIME(name)              ZoneScoped // Want descriptive names; was: ZoneNamedN( ___tracy_scoped_zone, #name, true );
        #define LL_PROFILE_ZONE_NAMED(name)             ZoneNamedN( ___tracy_scoped_zone, name, true );
        #define LL_PROFILE_ZONE_NAMED_COLOR(name,color) ZoneNamedNC( ___tracy_scopped_zone, name, color, true ) // RGB
        #define LL_PROFILE_ZONE_SCOPED                  ZoneScoped

        #define LL_PROFILE_ZONE_NUM( val )              ZoneValue( val )
        #define LL_PROFILE_ZONE_TEXT( text, size )      ZoneText( text, size )

        #define LL_PROFILE_ZONE_ERR(name)               LL_PROFILE_ZONE_NAMED_COLOR( name, 0XFF0000  )  // RGB yellow
        #define LL_PROFILE_ZONE_INFO(name)              LL_PROFILE_ZONE_NAMED_COLOR( name, 0X00FFFF  )  // RGB cyan
        #define LL_PROFILE_ZONE_WARN(name)              LL_PROFILE_ZONE_NAMED_COLOR( name, 0x0FFFF00 )  // RGB red

        #define LL_PROFILE_MUTEX(type, varname)                     TracyLockable(type, varname)
        #define LL_PROFILE_MUTEX_NAMED(type, varname, desc)         TracyLockableN(type, varname, desc)
        #define LL_PROFILE_MUTEX_SHARED(type, varname)              TracySharedLockable(type, varname)
        #define LL_PROFILE_MUTEX_SHARED_NAMED(type, varname, desc)  TracySharedLockableN(type, varname, desc)
        #define LL_PROFILE_MUTEX_LOCK(varname) { auto& mutex = varname; LockMark(mutex); }
    #endif
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_FAST_TIMER
        #define LL_PROFILER_FRAME_END
        #define LL_PROFILER_SET_THREAD_NAME( name )      (void)(name)
        #define LL_RECORD_BLOCK_TIME(name)                                                                  const LLTrace::BlockTimer& LL_GLUE_TOKENS(block_time_recorder, __LINE__)(LLTrace::timeThisBlock(name)); (void)LL_GLUE_TOKENS(block_time_recorder, __LINE__);
        #define LL_PROFILE_ZONE_NAMED(name)             // LL_PROFILE_ZONE_NAMED is a no-op when Tracy is disabled
        #define LL_PROFILE_ZONE_NAMED_COLOR(name,color) // LL_PROFILE_ZONE_NAMED_COLOR is a no-op when Tracy is disabled
        #define LL_PROFILE_ZONE_SCOPED                  // LL_PROFILE_ZONE_SCOPED is a no-op when Tracy is disabled
        #define LL_PROFILE_ZONE_COLOR(name,color)       // LL_RECORD_BLOCK_TIME(name)

        #define LL_PROFILE_ZONE_NUM( val )              (void)( val );                // Not supported
        #define LL_PROFILE_ZONE_TEXT( text, size )      (void)( text ); void( size ); // Not supported

        #define LL_PROFILE_ZONE_ERR(name)               (void)(name); // Not supported
        #define LL_PROFILE_ZONE_INFO(name)              (void)(name); // Not supported
        #define LL_PROFILE_ZONE_WARN(name)              (void)(name); // Not supported

        #define LL_PROFILE_MUTEX(type, varname) type varname
        #define LL_PROFILE_MUTEX_NAMED(type, varname, desc) type varname
        #define LL_PROFILE_MUTEX_SHARED(type, varname) type varname
        #define LL_PROFILE_MUTEX_SHARED_NAMED(type, varname, desc) type varname
        #define LL_PROFILE_MUTEX_LOCK(varname) // LL_PROFILE_MUTEX_LOCK is a no-op when Tracy is disabled
    #endif
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY_FAST_TIMER
        #define LL_PROFILER_FRAME_END                   FrameMark
        #define LL_PROFILER_SET_THREAD_NAME( name )     tracy::SetThreadName( name );
        #define LL_RECORD_BLOCK_TIME(name)              ZoneNamedN(___tracy_scoped_zone, #name, true);   const LLTrace::BlockTimer& LL_GLUE_TOKENS(block_time_recorder, __LINE__)(LLTrace::timeThisBlock(name)); (void)LL_GLUE_TOKENS(block_time_recorder, __LINE__);
        #define LL_PROFILE_ZONE_NAMED(name)             ZoneNamedN( ___tracy_scoped_zone, #name, true );
        #define LL_PROFILE_ZONE_NAMED_COLOR(name,color) ZoneNamedNC( ___tracy_scopped_zone, name, color, true ) // RGB
        #define LL_PROFILE_ZONE_SCOPED                  ZoneScoped

        #define LL_PROFILE_ZONE_NUM( val )              ZoneValue( val )
        #define LL_PROFILE_ZONE_TEXT( text, size )      ZoneText( text, size )

        #define LL_PROFILE_ZONE_ERR(name)               LL_PROFILE_ZONE_NAMED_COLOR( name, 0XFF0000  )  // RGB yellow
        #define LL_PROFILE_ZONE_INFO(name)              LL_PROFILE_ZONE_NAMED_COLOR( name, 0X00FFFF  )  // RGB cyan
        #define LL_PROFILE_ZONE_WARN(name)              LL_PROFILE_ZONE_NAMED_COLOR( name, 0x0FFFF00 )  // RGB red

        #define LL_PROFILE_MUTEX(type, varname)                     TracyLockable(type, varname)
        #define LL_PROFILE_MUTEX_NAMED(type, varname, desc)         TracyLockableN(type, varname, desc)
        #define LL_PROFILE_MUTEX_SHARED(type, varname)              TracySharedLockable(type, varname)
        #define LL_PROFILE_MUTEX_SHARED_NAMED(type, varname, desc)  TracySharedLockableN(type, varname, desc)
        #define LL_PROFILE_MUTEX_LOCK(varname) { auto& mutex = varname; LockMark(mutex); } // see https://github.com/wolfpld/tracy/issues/575
    #endif
#else
    #define LL_PROFILER_FRAME_END
    #define LL_PROFILER_SET_THREAD_NAME( name ) (void)(name)
#endif // LL_PROFILER

#if LL_PROFILER_ENABLE_TRACY_OPENGL
#define LL_PROFILE_GPU_ZONE(name)         TracyGpuZone(name)
#define LL_PROFILE_GPU_ZONEC(name,color)  TracyGpuZoneC(name,color)
#define LL_PROFILER_GPU_COLLECT           TracyGpuCollect
#define LL_PROFILER_GPU_CONTEXT           TracyGpuContext
#define LL_PROFILER_GPU_CONTEXT_NAMED     TracyGpuContextName
#else
#define LL_PROFILE_GPU_ZONE(name)           (void)name;
#define LL_PROFILE_GPU_ZONEC(name,color)    (void)name;(void)color;
#define LL_PROFILER_GPU_COLLECT
#define LL_PROFILER_GPU_CONTEXT
#define LL_PROFILER_GPU_CONTEXT_NAMED(name) (void)name;
#endif // LL_PROFILER_ENABLE_TRACY_OPENGL

#if LL_PROFILER_CONFIGURATION > 1
#define LL_PROFILE_ALLOC(ptr, size)             TracyAlloc(ptr, size)
#define LL_PROFILE_FREE(ptr)                    TracyFree(ptr)
#else
#define LL_PROFILE_ALLOC(ptr, size)             (void)(ptr); (void)(size);
#define LL_PROFILE_FREE(ptr)                    (void)(ptr);
#endif

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
#define LL_LABEL_OBJECT_GL(type, name, length, label) glObjectLabel(type, name, length, label)
#else
#define LL_LABEL_OBJECT_GL(type, name, length, label)
#endif

#include "llprofilercategories.h"

#endif // LL_PROFILER_H
