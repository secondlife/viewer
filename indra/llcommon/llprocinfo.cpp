/** 
* @file llprocinfo.cpp
* @brief Process, cpu and resource usage information APIs.
* @author monty@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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


#include "llprocinfo.h"

#if LL_WINDOWS

#define PSAPI_VERSION   1
#include "windows.h"
#include "psapi.h"

#elif LL_DARWIN

#include <sys/resource.h>
#include <mach/mach.h>
    
#else

#include <sys/time.h>
#include <sys/resource.h>

#endif // LL_WINDOWS/LL_DARWIN


// static
void LLProcInfo::getCPUUsage(time_type & user_time, time_type & system_time)
{
#if LL_WINDOWS

    HANDLE self(GetCurrentProcess());           // Does not have to be closed
    FILETIME ft_dummy, ft_system, ft_user;

    GetProcessTimes(self, &ft_dummy, &ft_dummy, &ft_system, &ft_user);
    ULARGE_INTEGER uli;
    uli.u.LowPart = ft_system.dwLowDateTime;
    uli.u.HighPart = ft_system.dwHighDateTime;
    system_time = uli.QuadPart / U64L(10);      // Convert to uS
    uli.u.LowPart = ft_user.dwLowDateTime;
    uli.u.HighPart = ft_user.dwHighDateTime;
    user_time = uli.QuadPart / U64L(10);
    
#elif LL_DARWIN

    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage))
    {
        user_time = system_time = time_type(0U);
        return;
    }
    user_time = U64(usage.ru_utime.tv_sec) * U64L(1000000) + usage.ru_utime.tv_usec;
    system_time = U64(usage.ru_stime.tv_sec) * U64L(1000000) + usage.ru_stime.tv_usec;

#else // Linux

    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage))
    {
        user_time = system_time = time_type(0U);
        return;
    }
    user_time = U64(usage.ru_utime.tv_sec) * U64L(1000000) + usage.ru_utime.tv_usec;
    system_time = U64(usage.ru_stime.tv_sec) * U64L(1000000) + usage.ru_stime.tv_usec;
    
#endif // LL_WINDOWS/LL_DARWIN/Linux
}


