 /**
 * @file lltelemetry.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lltelemetry.h"

#if LLPROFILE_USE_RAD_TELEMETRY_PROFILER
    #if LL_WINDOWS
        #include "llwin32headers.h"

        // build-vc120-64\packages\lib\release
        // build-vc150-64\packages\lib\release
        #ifdef _MSC_VER
            #pragma comment(lib,"rad_tm_win64.lib")
        #else
            #pragma message "NOTE: Rad GameTools Telemetry requested but non-MSVC compiler not yet supported on Windows"
        #endif
    #endif // LL_WINDOWS

    #if LL_DARWIN
        #pragma message "NOTE: Rad Game Tools Telemetry requested but not yet supported on Darwin"
    #endif

    #if LL_LINUX
        #pragma message "NOTE: Rad Game Tools Telemetry requested but not yet supported on Linux"
    #endif

//
// local consts
//
static const tm_int32 TELEMETRY_BUFFER_SIZE  = 8 * 1024 * 1024;

//
// local globals
//
static char *gTelemetryBufferPtr = NULL; // Telemetry

static const char *tm_status[ TMERR_INIT_NETWORKING_FAILED + 1 ] =
{
      "Telemetry pass: connected"                       // TM_OK
    , "Telemetry FAIL: disabled via #define NTELEMETRY" // TMERR_DISABLED
    , "Telemetry FAIL: invalid paramater"               // TMERR_INVALID_PARAM
    , "Telemetry FAIL: DLL not found"                   // TMERR_NULL_API
    , "Telemetry FAIL: out of resources"                // TMERR_OUT_OF_RESOURCES
    , "Telemetry FAIL: tmInitialize() not called"       // TMERR_UNINITIALIZED
    , "Telemetry FAIL: bad hostname"                    // TMERR_BAD_HOSTNAME
    , "Telemetry FAIL: couldn't connect to server"      // TMERR_COULD_NOT_CONNECT
    , "Telemetry FAIL: unknown network error"           // TMERR_UNKNOWN_NETWORK
    , "Telemetry FAIL: tmShutdown() already called"     // TMERR_ALREADY_SHUTDOWN
    , "Telemetry FAIL: memory buffer too small"         // TMERR_ARENA_TOO_SMALL
    , "Telemetry FAIL: server handshake error"          // TMERR_BAD_HANDSHAKE
    , "Telemetry FAIL: unaligned parameters"            // TMERR_UNALIGNED
    , "Telemetry FAIL: network not initialized"         // TMERR_NETWORK_NOT_INITIALIZED -- WSAStartup not called before tmOpen()
    , "Telemetry FAIL: bad version"                     // TMERR_BAD_VERSION
    , "Telemetry FAIL: timer too large"                 // TMERR_BAD_TIMER
    , "Telemetry FAIL: tmOpen() already called"         // TMERR_ALREADY_OPENED
    , "Telemetry FAIL: tmInitialize() already called"   // TMERR_ALREADY_INITIALIZED
    , "Telemetry FAIL: could't open file"               // TMERR_FILE_OPEN_FAILED
    , "Telemetry FAIL: tmOpen() failed networking"      // TMERR_INIT_NETWORKING_FAILED
};

//
// exported functionality
//

void telemetry_shutdown()
{
    #if LL_WINDOWS
        if (gTelemetryBufferPtr)
        {
            tmClose(0);
            tmShutdown();

            delete[] gTelemetryBufferPtr;
            gTelemetryBufferPtr = NULL;
        }
    #endif
}

void telemetry_startup()
{
    #if LL_WINDOWS
        tmLoadLibrary(TM_RELEASE); // Loads .dll

        gTelemetryBufferPtr = new char[ TELEMETRY_BUFFER_SIZE ];
        tmInitialize(TELEMETRY_BUFFER_SIZE, gTelemetryBufferPtr);

        tm_error telemetry_status = tmOpen(
            0,                     // unused
            "SecondLife",          // app name
            __DATE__ " " __TIME__, // build identifier
            "localhost",           // server name (or filename)
            TMCT_TCP,              // connection type (or TMCT_FILE)
            4719,                  // port
            TMOF_INIT_NETWORKING,  // open flags
            250 );                 // timeout ms

        if (telemetry_status == TMERR_UNKNOWN)
        {
            LL_ERRS() << "Telemetry FAIL: unknown error" << LL_ENDL;
        }
        else if (telemetry_status && (telemetry_status <= TMERR_INIT_NETWORKING_FAILED))
        {
            LL_INFOS() << tm_status[ telemetry_status ] << LL_ENDL;
            free(gTelemetryBufferPtr);
            gTelemetryBufferPtr = NULL;
        }
    #endif // LL_WINDOWS
}

// Called after we render a frame
void telemetry_update()
{
    #if LL_WINDOWS
        if (gTelemetryBufferPtr)
        {
            tmTick(0);
        }
    #endif
}
#endif // LLPROFILE_USE_RAD_TELEMETRY_PROFILER
