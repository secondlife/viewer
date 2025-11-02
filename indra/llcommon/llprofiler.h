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
#define LL_PROFILER_CONFIG_TRACY            1  // Profiling on: Only Tracy

#ifndef LL_PROFILER_CONFIGURATION
#define LL_PROFILER_CONFIGURATION LL_PROFILER_CONFIG_NONE
#endif

#if defined(LL_PROFILER_CONFIGURATION) && (LL_PROFILER_CONFIGURATION > LL_PROFILER_CONFIG_NONE)
    #if LL_PROFILER_CONFIGURATION == LL_PROFILER_CONFIG_TRACY
        #include "llpreprocessor.h"

#if defined(LL_GNUC) && GCC_VERSION >= 130000
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wnonnull"
#endif
        #include "tracy/Tracy.hpp"
#if defined(LL_GNUC) && GCC_VERSION >= 130000
#   pragma GCC diagnostic pop
#endif

        #define LL_PROFILER_FRAME_END                   FrameMark
        #define LL_PROFILER_SET_THREAD_NAME( name )     tracy::SetThreadName( name )
        #define LL_RECORD_BLOCK_TIME(name)              ZoneNamedN(___tracy_scoped_zone, name, true)
        #define LL_PROFILE_ZONE_NAMED(name)             ZoneNamedN( ___tracy_scoped_zone, name, true )
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
#else
    #define LL_PROFILER_FRAME_END
    #define LL_PROFILER_SET_THREAD_NAME( name ) (void)(name)

    #define LL_PROFILE_ZONE_NAMED(name)
    #define LL_PROFILE_ZONE_NAMED_COLOR(name, color)
    #define LL_PROFILE_ZONE_SCOPED

    #define LL_PROFILE_ZONE_NUM(val)
    #define LL_PROFILE_ZONE_TEXT(text, size)

    #define LL_PROFILE_ZONE_ERR(name)  LL_PROFILE_ZONE_NAMED_COLOR(name, 0XFF0000)  // RGB yellow
    #define LL_PROFILE_ZONE_INFO(name) LL_PROFILE_ZONE_NAMED_COLOR(name, 0X00FFFF)  // RGB cyan
    #define LL_PROFILE_ZONE_WARN(name) LL_PROFILE_ZONE_NAMED_COLOR(name, 0x0FFFF00) // RGB red

    #define LL_PROFILE_MUTEX(type, varname)                    type varname
    #define LL_PROFILE_MUTEX_NAMED(type, varname, desc)        type varname
    #define LL_PROFILE_MUTEX_SHARED(type, varname)             type varname
    #define LL_PROFILE_MUTEX_SHARED_NAMED(type, varname, desc) type varname
    #define LL_PROFILE_MUTEX_LOCK(varname)                     // LL_PROFILE_MUTEX_LOCK is a no-op when Tracy is disabled
#endif // LL_PROFILER

#if LL_PROFILER_ENABLE_TRACY_OPENGL
#define LL_PROFILE_GPU_ZONE(name)         TracyGpuZone(name)
#define LL_PROFILE_GPU_ZONEC(name,color)  TracyGpuZoneC(name,color)
#define LL_PROFILER_GPU_COLLECT           TracyGpuCollect
#define LL_PROFILER_GPU_CONTEXT           TracyGpuContext
#define LL_PROFILER_GPU_CONTEXT_NAMED     TracyGpuContextName
#else
#define LL_PROFILE_GPU_ZONE(name)           (void)name
#define LL_PROFILE_GPU_ZONEC(name,color)    (void)name;(void)color
#define LL_PROFILER_GPU_COLLECT
#define LL_PROFILER_GPU_CONTEXT
#define LL_PROFILER_GPU_CONTEXT_NAMED(name) (void)name
#endif // LL_PROFILER_ENABLE_TRACY_OPENGL

#if LL_PROFILER_CONFIGURATION >= LL_PROFILER_CONFIG_TRACY
#define LL_PROFILE_ALLOC(ptr, size)             TracyAlloc(ptr, size)
#define LL_PROFILE_FREE(ptr)                    TracyFree(ptr)
#else
#define LL_PROFILE_ALLOC(ptr, size)             (void)(ptr); (void)(size);
#define LL_PROFILE_FREE(ptr)                    (void)(ptr);
#endif

#if LL_PROFILER_ENABLE_RENDER_DOC
#define LL_LABEL_OBJECT_GL(type, name, length, label) glObjectLabel(type, name, length, label)
#else
#define LL_LABEL_OBJECT_GL(type, name, length, label)
#endif

#include "llprofilercategories.h"

#endif // LL_PROFILER_H
