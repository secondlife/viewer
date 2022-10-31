/** 
* @file   llprocinfo.h
* @brief  Interface to process/cpu/resource information services.
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

#ifndef LL_PROCINFO_H
#define LL_PROCINFO_H


#include "linden_common.h"

/// @file llprocinfo.h
///
/// Right now, this is really a namespace disguised as a class.
/// It wraps some types and functions to return information about
/// process resource consumption in a non-OS-specific manner.
///
/// Threading:  No instances so that's thread-safe.  Implementations
/// of static functions should be thread-safe, they mostly involve
/// direct syscall invocations.
///
/// Allocation:  Not instantiatable.

class LL_COMMON_API LLProcInfo
{
public:
    /// Public types

    typedef U64 time_type;                              /// Relative microseconds
    
private:
    LLProcInfo();                                       // Not defined
    ~LLProcInfo();                                      // Not defined
    LLProcInfo(const LLProcInfo &);                     // Not defined
    void operator=(const LLProcInfo &);                 // Not defined

public:
    /// Get accumulated system and user CPU time in
    /// microseconds.  Syscalls involved in every invocation.
    ///
    /// Threading:  expected to be safe.
    static void getCPUUsage(time_type & user_time, time_type & system_time);
};
    

#endif  // LL_PROCINFO_H
