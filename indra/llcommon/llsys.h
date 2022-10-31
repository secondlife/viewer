/** 
 * @file llsys.h
 * @brief System information debugging classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_SYS_H
#define LL_SYS_H

//
// The LLOSInfo, LLCPUInfo, and LLMemoryInfo classes are essentially
// the same, but query different machine subsystems. Here's how you
// use an LLCPUInfo object:
//
//  LLCPUInfo info;
//  LL_INFOS() << info << LL_ENDL;
//

#include "llsd.h"
#include "llsingleton.h"
#include <iosfwd>
#include <string>

class LL_COMMON_API LLOSInfo : public LLSingleton<LLOSInfo>
{
    LLSINGLETON(LLOSInfo);
public:
    void stream(std::ostream& s) const;

    const std::string& getOSString() const;
    const std::string& getOSStringSimple() const;

    const std::string& getOSVersionString() const;

    const S32 getOSBitness() const;
    
    S32 mMajorVer;
    S32 mMinorVer;
    S32 mBuild;

#ifndef LL_WINDOWS
    static S32 getMaxOpenFiles();
#endif
    static bool is64Bit();

    static U32 getProcessVirtualSizeKB();
    static U32 getProcessResidentSizeKB();
private:
    std::string mOSString;
    std::string mOSStringSimple;
    std::string mOSVersionString;
    S32 mOSBitness;
};


class LL_COMMON_API LLCPUInfo
{
public:
    LLCPUInfo();    
    void stream(std::ostream& s) const;

    std::string getCPUString() const;
    const LLSD& getSSEVersions() const;

    bool hasAltivec() const;
    bool hasSSE() const;
    bool hasSSE2() const;
    bool hasSSE3() const;
    bool hasSSE3S() const;
    bool hasSSE41() const;
    bool hasSSE42() const;
    bool hasSSE4a() const;
    F64 getMHz() const;

    // Family is "AMD Duron" or "Intel Pentium Pro"
    const std::string& getFamily() const { return mFamily; }

private:
    bool mHasSSE;
    bool mHasSSE2;
    bool mHasSSE3;
    bool mHasSSE3S;
    bool mHasSSE41;
    bool mHasSSE42;
    bool mHasSSE4a;
    bool mHasAltivec;
    F64 mCPUMHz;
    std::string mFamily;
    std::string mCPUString;
    LLSD mSSEVersions;
};

//=============================================================================
//
//  CLASS       LLMemoryInfo

class LL_COMMON_API LLMemoryInfo

/*! @brief      Class to query the memory subsystem

    @details
        Here's how you use an LLMemoryInfo:
        
        LLMemoryInfo info;
<br>    LL_INFOS() << info << LL_ENDL;
*/
{
public:
    LLMemoryInfo(); ///< Default constructor
    void stream(std::ostream& s) const; ///< output text info to s

    U32Kilobytes getPhysicalMemoryKB() const; 

    //get the available memory infomation in KiloBytes.
    static void getAvailableMemoryKB(U32Kilobytes& avail_physical_mem_kb, U32Kilobytes& avail_virtual_mem_kb);

    // Retrieve a map of memory statistics. The keys of the map are platform-
    // dependent. The values are in kilobytes to try to avoid integer overflow.
    LLSD getStatsMap() const;

    // Re-fetch memory data (as reported by stream() and getStatsMap()) from the
    // system. Normally this is fetched at construction time. Return (*this)
    // to permit usage of the form:
    // @code
    // LLMemoryInfo info;
    // ...
    // info.refresh().getStatsMap();
    // @endcode
    LLMemoryInfo& refresh();

private:
    // set mStatsMap
    static LLSD loadStatsMap();

    // Memory stats for getStatsMap().
    LLSD mStatsMap;
};


LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLOSInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info);

// gunzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gunzip_file(const std::string& srcfile, const std::string& dstfile);
// gzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gzip_file(const std::string& srcfile, const std::string& dstfile);

extern LL_COMMON_API LLCPUInfo gSysCPU;

#endif // LL_LLSYS_H
