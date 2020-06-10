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

1. Uncomment #define LL_TELEMETRY below

2. Include our header file
    #include "lltelemetry.h"

3. Add zones to the functions you wish to profile
    void onFoo()
    {
        TELEMETRY_PROFILE("Foo");
    }
*/
//#define LL_TELEMETRY 1

// Default no telemetry
#ifndef LL_TELEMETRY
    #define LL_TELEMETRY             0
    #define TELEMETRY_PROFILE(  ...) {}
    #define TELEMETRY_SHUTDOWN( ...) {}
    #define TELEMETRY_STARTUP(  ...) {}
    #define TELEMETRY_UPDATE(   ...) {}
#else
    #include <rad_tm.h>

    #define TELEMETRY_PROFILE(name) tmZone(0,0,name)
    #define TELEMETRY_SHUTDOWN      telemetry_shutdown
    #define TELEMETRY_STARTUP       telemetry_startup
    #define TELEMETRY_UPDATE        telemetry_update
#endif // LL_TELEMETRY

//
// exported functionality
//

extern void telemetry_startup();
extern void telemetry_shutdown();
extern void telemetry_update(); // called after every frame update
