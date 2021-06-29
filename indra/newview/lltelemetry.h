/**
 * @file lltelemetry.h
 * @brief Wrapper for Rad Game Tools Telemetry
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

/*
To use:

1. Uncomment #define LLPROFILE_USE_RAD_TELEMETRY_PROFILER below

2. Include this header file
    #include "lltelemetry.h"

3. Add zones to the functions you wish to profile
    void onFoo()
    {
        LLPROFILE_ZONE("Foo");
    }
*/
//#define LLPROFILE_USE_RAD_TELEMETRY_PROFILER 1

// Default NO local telemetry profiling
#ifndef LLPROFILE_USE_RAD_TELEMETRY_PROFILER
    #define LLPROFILE_USE_RAD_TELEMETRY_PROFILER 0
    #define LLPROFILE_SHUTDOWN( ...) {}
    #define LLPROFILE_STARTUP(  ...) {}
    #define LLPROFILE_UPDATE(   ...) {}

    #define LLPROFILE_AUTO_CPU_MARKER_COLOR(r, g, b)
    #define LLPROFILE_ENTER(name)
    #define LLPROFILE_ENTER_FORMAT(format, ...)
    #define LLPROFILE_FUNCTION
    #define LLPROFILE_LEAVE()
    #define LLPROFILE_THREAD_NAME(name)
    #define LLPROFILE_ZONE(name)
    #define LLPROFILE_ZONE_FORMAT(format, ...)
#else
    #include <rad_tm.h>

    #define LLPROFILE_SHUTDOWN                       telemetry_shutdown
    #define LLPROFILE_STARTUP                        telemetry_startup
    #define LLPROFILE_UPDATE                         telemetry_update

    #define LLPROFILE_AUTO_CPU_MARKER_COLOR(r, g, b) tmZoneColor(r, g, b)
    #define LLPROFILE_ENTER(name)                    tmEnter(0, 0, name)
    #define LLPROFILE_ENTER_FORMAT(format, ...)      tmEnter(0, 0, format, __VA_ARGS__)
    #define LLPROFILE_FUNCTION                       tmFunction(0, 0)
    #define LLPROFILE_LEAVE()                        tmLeave(0)
    #define LLPROFILE_THREAD_NAME(name)              tmThreadName(0, 0, name)
    #define LLPROFILE_ZONE(name)                     tmZone(0, 0, name)
    #define LLPROFILE_ZONE_FORMAT(format, ...)       tmZone(0, 0, format, __VA_ARGS__)
#endif // LLPROFILE_USE_RAD_TELEMETRY_PROFILER

//
// exported functionality
//

extern void telemetry_startup();
extern void telemetry_shutdown();
extern void telemetry_update(); // called after every frame update
