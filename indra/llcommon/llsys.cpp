/**
 * @file llsys.cpp
 * @brief Implementation of the basic system query functions.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

#include "linden_common.h"

#include "llsys.h"

#include <iostream>
#ifdef LL_USESYSTEMLIBS
# include <zlib.h>
#else
# include "zlib-ng/zlib.h"
#endif

#include "llprocessor.h"
#include "llerrorcontrol.h"
#include "llevents.h"
#include "llformat.h"
#include "llregex.h"
#include "lltimer.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_float.hpp>
#include "llfasttimer.h"

using namespace llsd;

#if LL_WINDOWS
#   include "llwin32headers.h"
#   include <psapi.h>               // GetPerformanceInfo() et al.
#   include <VersionHelpers.h>
#elif LL_DARWIN
#   include "llsys_objc.h"
#   include <errno.h>
#   include <sys/sysctl.h>
#   include <sys/utsname.h>
#   include <stdint.h>
#   include <CoreServices/CoreServices.h>
#   include <stdexcept>
#   include <mach/host_info.h>
#   include <mach/mach_host.h>
#   include <mach/task.h>
#   include <mach/task_info.h>
#   include <sys/types.h>
#   include <mach/mach_init.h>
#elif LL_LINUX
#   include <errno.h>
#   include <sys/utsname.h>
#   include <unistd.h>
#   include <sys/sysinfo.h>
#   include <stdexcept>
const char MEMINFO_FILE[] = "/proc/meminfo";
#   include <gnu/libc-version.h>
#endif

LLCPUInfo gSysCPU;
LLMemoryInfo gSysMemory;

// Don't log memory info any more often than this. It also serves as our
// framerate sample size.
static const F32 MEM_INFO_THROTTLE = 20;
// Sliding window of samples. We intentionally limit the length of time we
// remember "the slowest" framerate because framerate is very slow at login.
// If we only triggered FrameWatcher logging when the session framerate
// dropped below the login framerate, we'd have very little additional data.
static const F32 MEM_INFO_WINDOW = 10*60;

LLOSInfo::LLOSInfo() :
    mMajorVer(0), mMinorVer(0), mBuild(0), mOSVersionString("")
{

#if LL_WINDOWS

    if (IsWindows10OrGreater())
    {
        mMajorVer = 10;
        mMinorVer = 0;
        mOSStringSimple = "Microsoft Windows 10 ";
    }
    else if (IsWindows8Point1OrGreater())
    {
        mMajorVer = 6;
        mMinorVer = 3;
        if (IsWindowsServer())
        {
            mOSStringSimple = "Windows Server 2012 R2 ";
        }
        else
        {
            mOSStringSimple = "Microsoft Windows 8.1 ";
        }
    }
    else if (IsWindows8OrGreater())
    {
        mMajorVer = 6;
        mMinorVer = 2;
        if (IsWindowsServer())
        {
            mOSStringSimple = "Windows Server 2012 ";
        }
        else
        {
            mOSStringSimple = "Microsoft Windows 8 ";
        }
    }
    else if (IsWindows7SP1OrGreater())
    {
        mMajorVer = 6;
        mMinorVer = 1;
        if (IsWindowsServer())
        {
            mOSStringSimple = "Windows Server 2008 R2 SP1 ";
        }
        else
        {
            mOSStringSimple = "Microsoft Windows 7 SP1 ";
        }
    }
    else if (IsWindows7OrGreater())
    {
        mMajorVer = 6;
        mMinorVer = 1;
        if (IsWindowsServer())
        {
            mOSStringSimple = "Windows Server 2008 R2 ";
        }
        else
        {
            mOSStringSimple = "Microsoft Windows 7 ";
        }
    }
    else if (IsWindowsVistaSP2OrGreater())
    {
        mMajorVer = 6;
        mMinorVer = 0;
        if (IsWindowsServer())
        {
            mOSStringSimple = "Windows Server 2008 SP2 ";
        }
        else
        {
            mOSStringSimple = "Microsoft Windows Vista SP2 ";
        }
    }
    else
    {
        mOSStringSimple = "Unsupported Windows version ";
    }

    ///get native system info if available..
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO); ///function pointer for loading GetNativeSystemInfo
    SYSTEM_INFO si; //System Info object file contains architecture info
    PGNSI pGNSI; //pointer object
    ZeroMemory(&si, sizeof(SYSTEM_INFO)); //zero out the memory in information
    pGNSI = (PGNSI)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo"); //load kernel32 get function
    if (NULL != pGNSI) //check if it has failed
        pGNSI(&si); //success
    else
        GetSystemInfo(&si); //if it fails get regular system info
    //(Warning: If GetSystemInfo it may result in incorrect information in a WOW64 machine, if the kernel fails to load)

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((OSVERSIONINFO *)&osvi))
    {
        mBuild = osvi.dwBuildNumber & 0xffff;
    }
    else
    {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx((OSVERSIONINFO *)&osvi))
        {
            mBuild = osvi.dwBuildNumber & 0xffff;
        }
    }

    S32 ubr = 0; // Windows 10 Update Build Revision, can be retrieved from a registry
    if (mMajorVer == 10)
    {
        DWORD cbData(sizeof(DWORD));
        DWORD data(0);
        HKEY key;
        LSTATUS ret_code = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &key);
        if (ERROR_SUCCESS == ret_code)
        {
            ret_code = RegQueryValueExW(key, L"UBR", 0, NULL, reinterpret_cast<LPBYTE>(&data), &cbData);
            if (ERROR_SUCCESS == ret_code)
            {
                ubr = data;
            }
        }

        if (mBuild >= 22000)
        {
            // At release Windows 11 version was 10.0.22000.194
            // According to microsoft win 10 won't ever get that far.
            mOSStringSimple = "Microsoft Windows 11 ";
        }
    }

    //msdn microsoft finds 32 bit and 64 bit flavors this way..
    //http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx (example code that contains quite a few more flavors
    //of windows than this code does (in case it is needed for the future)
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) //check for 64 bit
    {
        mOSStringSimple += "64-bit ";
    }
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        mOSStringSimple += "32-bit ";
    }

    mOSString = mOSStringSimple;
    if (mBuild > 0)
    {
        mOSString += llformat("(Build %d", mBuild);
        if (ubr > 0)
        {
            mOSString += llformat(".%d", ubr);
        }
        mOSString += ")";
    }

    LLStringUtil::trim(mOSStringSimple);
    LLStringUtil::trim(mOSString);

#elif LL_DARWIN

    // Initialize mOSStringSimple to something like:
    // "macOS 10.13.1"
    {
        const char * DARWIN_PRODUCT_NAME = "macOS";

        int64_t major_version, minor_version, bugfix_version = 0;

        if (LLGetDarwinOSInfo(major_version, minor_version, bugfix_version))
        {
            mMajorVer = major_version;
            mMinorVer = minor_version;
            mBuild = bugfix_version;

            std::stringstream os_version_string;
            os_version_string << DARWIN_PRODUCT_NAME << " " << mMajorVer << "." << mMinorVer << "." << mBuild;

            // Put it in the OS string we are compiling
            mOSStringSimple.append(os_version_string.str());
        }
        else
        {
            mOSStringSimple.append("Unable to collect OS info");
        }
    }

    // Initialize mOSString to something like:
    // "macOS 10.13.1 Darwin Kernel Version 10.7.0: Sat Jan 29 15:17:16 PST 2011; root:xnu-1504.9.37~1/RELEASE_I386 i386"
    struct utsname un;
    if(uname(&un) != -1)
    {
        mOSString = mOSStringSimple;
        mOSString.append(" ");
        mOSString.append(un.sysname);
        mOSString.append(" ");
        mOSString.append(un.release);
        mOSString.append(" ");
        mOSString.append(un.version);
        mOSString.append(" ");
        mOSString.append(un.machine);
    }
    else
    {
        mOSString = mOSStringSimple;
    }

#elif LL_LINUX

    struct utsname un;
    if(uname(&un) != -1)
    {
        mOSStringSimple.append(un.sysname);
        mOSStringSimple.append(" ");
        mOSStringSimple.append(un.release);

        mOSString = mOSStringSimple;
        mOSString.append(" ");
        mOSString.append(un.version);
        mOSString.append(" ");
        mOSString.append(un.machine);

        // Simplify 'Simple'
        std::string ostype = mOSStringSimple.substr(0, mOSStringSimple.find_first_of(" ", 0));
        if (ostype == "Linux")
        {
            // Only care about major and minor Linux versions, truncate at second '.'
            std::string::size_type idx1 = mOSStringSimple.find_first_of(".", 0);
            std::string::size_type idx2 = (idx1 != std::string::npos) ? mOSStringSimple.find_first_of(".", idx1+1) : std::string::npos;
            std::string simple = mOSStringSimple.substr(0, idx2);
            if (simple.length() > 0)
                mOSStringSimple = simple;
        }
    }
    else
    {
        mOSStringSimple.append("Unable to collect OS info");
        mOSString = mOSStringSimple;
    }

    const char OS_VERSION_MATCH_EXPRESSION[] = "([0-9]+)\\.([0-9]+)(\\.([0-9]+))?";
    boost::regex os_version_parse(OS_VERSION_MATCH_EXPRESSION);
    boost::smatch matched;

    std::string glibc_version(gnu_get_libc_version());
    if ( ll_regex_match(glibc_version, matched, os_version_parse) )
    {
        LL_INFOS("AppInit") << "Using glibc version '" << glibc_version << "' as OS version" << LL_ENDL;

        std::string version_value;

        if ( matched[1].matched ) // Major version
        {
            version_value.assign(matched[1].first, matched[1].second);
            if (sscanf(version_value.c_str(), "%d", &mMajorVer) != 1)
            {
              LL_WARNS("AppInit") << "failed to parse major version '" << version_value << "' as a number" << LL_ENDL;
            }
        }
        else
        {
            LL_ERRS("AppInit")
                << "OS version regex '" << OS_VERSION_MATCH_EXPRESSION
                << "' returned true, but major version [1] did not match"
                << LL_ENDL;
        }

        if ( matched[2].matched ) // Minor version
        {
            version_value.assign(matched[2].first, matched[2].second);
            if (sscanf(version_value.c_str(), "%d", &mMinorVer) != 1)
            {
              LL_ERRS("AppInit") << "failed to parse minor version '" << version_value << "' as a number" << LL_ENDL;
            }
        }
        else
        {
            LL_ERRS("AppInit")
                << "OS version regex '" << OS_VERSION_MATCH_EXPRESSION
                << "' returned true, but minor version [1] did not match"
                << LL_ENDL;
        }

        if ( matched[4].matched ) // Build version (optional) - note that [3] includes the '.'
        {
            version_value.assign(matched[4].first, matched[4].second);
            if (sscanf(version_value.c_str(), "%d", &mBuild) != 1)
            {
              LL_ERRS("AppInit") << "failed to parse build version '" << version_value << "' as a number" << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS("AppInit")
                << "OS build version not provided; using zero"
                << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("AppInit") << "glibc version '" << glibc_version << "' cannot be parsed to three numbers; using all zeros" << LL_ENDL;
    }

#else

    struct utsname un;
    if(uname(&un) != -1)
    {
        mOSStringSimple.append(un.sysname);
        mOSStringSimple.append(" ");
        mOSStringSimple.append(un.release);

        mOSString = mOSStringSimple;
        mOSString.append(" ");
        mOSString.append(un.version);
        mOSString.append(" ");
        mOSString.append(un.machine);

        // Simplify 'Simple'
        std::string ostype = mOSStringSimple.substr(0, mOSStringSimple.find_first_of(" ", 0));
        if (ostype == "Linux")
        {
            // Only care about major and minor Linux versions, truncate at second '.'
            std::string::size_type idx1 = mOSStringSimple.find_first_of(".", 0);
            std::string::size_type idx2 = (idx1 != std::string::npos) ? mOSStringSimple.find_first_of(".", idx1+1) : std::string::npos;
            std::string simple = mOSStringSimple.substr(0, idx2);
            if (simple.length() > 0)
                mOSStringSimple = simple;
        }
    }
    else
    {
        mOSStringSimple.append("Unable to collect OS info");
        mOSString = mOSStringSimple;
    }

#endif

    std::stringstream dotted_version_string;
    dotted_version_string << mMajorVer << "." << mMinorVer << "." << mBuild;
    mOSVersionString.append(dotted_version_string.str());

    mOSBitness = is64Bit() ? 64 : 32;
    LL_INFOS("LLOSInfo") << "OS bitness: " << mOSBitness << LL_ENDL;
}

#ifndef LL_WINDOWS
// static
long LLOSInfo::getMaxOpenFiles()
{
    const long OPEN_MAX_GUESS = 256;

#ifdef  OPEN_MAX
    static long open_max = OPEN_MAX;
#else
    static long open_max = 0;
#endif

    if (0 == open_max)
    {
        // First time through.
        errno = 0;
        if ( (open_max = sysconf(_SC_OPEN_MAX)) < 0)
        {
            if (0 == errno)
            {
                // Indeterminate.
                open_max = OPEN_MAX_GUESS;
            }
            else
            {
                LL_ERRS() << "LLOSInfo::getMaxOpenFiles: sysconf error for _SC_OPEN_MAX" << LL_ENDL;
            }
        }
    }
    return open_max;
}
#endif

void LLOSInfo::stream(std::ostream& s) const
{
    s << mOSString;
}

const std::string& LLOSInfo::getOSString() const
{
    return mOSString;
}

const std::string& LLOSInfo::getOSStringSimple() const
{
    return mOSStringSimple;
}

const std::string& LLOSInfo::getOSVersionString() const
{
    return mOSVersionString;
}

const S32 LLOSInfo::getOSBitness() const
{
    return mOSBitness;
}

namespace {

    U32 readFromProcStat( std::string entryName )
    {
        U32 val{};
#if LL_LINUX
        constexpr U32 STATUS_SIZE  = 2048;

        LLFILE* status_filep = LLFile::fopen("/proc/self/status", "rb");
        if (status_filep)
        {
            char buff[STATUS_SIZE];     /* Flawfinder: ignore */

            size_t nbytes = fread(buff, 1, STATUS_SIZE-1, status_filep);
            buff[nbytes] = '\0';

            // All these guys return numbers in KB
            char *memp = strstr(buff, entryName.c_str());
            if (memp)
            {
                (void) sscanf(memp, "%*s %u", &val);
            }
            fclose(status_filep);
        }
#endif
        return val;
    }

}

//static
U32 LLOSInfo::getProcessVirtualSizeKB()
{
    return readFromProcStat( "VmSize:" );
}

//static
U32 LLOSInfo::getProcessResidentSizeKB()
{
    return readFromProcStat( "VmRSS:" );
}

//static
bool LLOSInfo::is64Bit()
{
#if LL_WINDOWS
#if defined(_WIN64)
    return true;
#elif defined(_WIN32)
    // 32-bit viewer may be run on both 32-bit and 64-bit Windows, need to elaborate
    bool f64 = false;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
    return false;
#endif
#else // ! LL_WINDOWS
    // we only build a 64-bit mac viewer and currently we don't build for linux at all
    return true;
#endif
}

LLCPUInfo::LLCPUInfo()
{
    std::ostringstream out;
    LLProcessorInfo proc;
    // proc.WriteInfoTextFile("procInfo.txt");
    mHasSSE = proc.hasSSE();
    mHasSSE2 = proc.hasSSE2();
    mHasSSE3 = proc.hasSSE3();
    mHasSSE3S = proc.hasSSE3S();
    mHasSSE41 = proc.hasSSE41();
    mHasSSE42 = proc.hasSSE42();
    mHasSSE4a = proc.hasSSE4a();
    mHasAltivec = proc.hasAltivec();
    mCPUMHz = (F64)proc.getCPUFrequency();
    mFamily = proc.getCPUFamilyName();
    mCPUString = "Unknown";

    out << proc.getCPUBrandName();
    if (200 < mCPUMHz && mCPUMHz < 10000)           // *NOTE: cpu speed is often way wrong, do a sanity check
    {
        out << " (" << mCPUMHz << " MHz)";
    }
    mCPUString = out.str();
    LLStringUtil::trim(mCPUString);

    if (mHasSSE)
    {
        mSSEVersions.append("1");
    }
    if (mHasSSE2)
    {
        mSSEVersions.append("2");
    }
    if (mHasSSE3)
    {
        mSSEVersions.append("3");
    }
    if (mHasSSE3S)
    {
        mSSEVersions.append("3S");
    }
    if (mHasSSE41)
    {
        mSSEVersions.append("4.1");
    }
    if (mHasSSE42)
    {
        mSSEVersions.append("4.2");
    }
    if (mHasSSE4a)
    {
        mSSEVersions.append("4a");
    }
}

bool LLCPUInfo::hasAltivec() const
{
    return mHasAltivec;
}

bool LLCPUInfo::hasSSE() const
{
    return mHasSSE;
}

bool LLCPUInfo::hasSSE2() const
{
    return mHasSSE2;
}

bool LLCPUInfo::hasSSE3() const
{
    return mHasSSE3;
}

bool LLCPUInfo::hasSSE3S() const
{
    return mHasSSE3S;
}

bool LLCPUInfo::hasSSE41() const
{
    return mHasSSE41;
}

bool LLCPUInfo::hasSSE42() const
{
    return mHasSSE42;
}

bool LLCPUInfo::hasSSE4a() const
{
    return mHasSSE4a;
}

F64 LLCPUInfo::getMHz() const
{
    return mCPUMHz;
}

std::string LLCPUInfo::getCPUString() const
{
    return mCPUString;
}

const LLSD& LLCPUInfo::getSSEVersions() const
{
    return mSSEVersions;
}

void LLCPUInfo::stream(std::ostream& s) const
{
    // gather machine information.
    s << LLProcessorInfo().getCPUFeatureDescription();

    // These are interesting as they reflect our internal view of the
    // CPU's attributes regardless of platform
    s << "->mHasSSE:     " << (U32)mHasSSE << std::endl;
    s << "->mHasSSE2:    " << (U32)mHasSSE2 << std::endl;
    s << "->mHasSSE3:    " << (U32)mHasSSE3 << std::endl;
    s << "->mHasSSE3S:    " << (U32)mHasSSE3S << std::endl;
    s << "->mHasSSE41:    " << (U32)mHasSSE41 << std::endl;
    s << "->mHasSSE42:    " << (U32)mHasSSE42 << std::endl;
    s << "->mHasSSE4a:    " << (U32)mHasSSE4a << std::endl;
    s << "->mHasAltivec: " << (U32)mHasAltivec << std::endl;
    s << "->mCPUMHz:     " << mCPUMHz << std::endl;
    s << "->mCPUString:  " << mCPUString << std::endl;
}

// Helper class for LLMemoryInfo: accumulate stats in the form we store for
// LLMemoryInfo::getStatsMap().
class Stats
{
public:
    Stats():
        mStats(LLSD::emptyMap())
    {}

    // Store every integer type as LLSD::Integer.
    template <class T>
    void add(const LLSD::String& name, const T& value,
             typename boost::enable_if<boost::is_integral<T> >::type* = 0)
    {
        mStats[name] = LLSD::Integer(value);
    }

    // Store every floating-point type as LLSD::Real.
    template <class T>
    void add(const LLSD::String& name, const T& value,
             typename boost::enable_if<boost::is_float<T> >::type* = 0)
    {
        mStats[name] = LLSD::Real(value);
    }

    // Hope that LLSD::Date values are sufficiently unambiguous.
    void add(const LLSD::String& name, const LLSD::Date& value)
    {
        mStats[name] = value;
    }

    LLSD get() const { return mStats; }

private:
    LLSD mStats;
};

LLMemoryInfo::LLMemoryInfo()
{
    refresh();
}

#if LL_WINDOWS
static U32Kilobytes LLMemoryAdjustKBResult(U32Kilobytes inKB)
{
    // Moved this here from llfloaterabout.cpp

    //! \bug
    // For some reason, the reported amount of memory is always wrong.
    // The original adjustment assumes it's always off by one meg, however
    // errors of as much as 2520 KB have been observed in the value
    // returned from the GetMemoryStatusEx function.  Here we keep the
    // original adjustment from llfoaterabout.cpp until this can be
    // fixed somehow.
    inKB += U32Megabytes(1);

    return inKB;
}
#endif

#if LL_DARWIN
// static
U32Kilobytes LLMemoryInfo::getHardwareMemSize()
{
    // This might work on Linux as well.  Someone check...
    uint64_t phys = 0;
    int mib[2] = { CTL_HW, HW_MEMSIZE };

    size_t len = sizeof(phys);
    sysctl(mib, 2, &phys, &len, NULL, 0);

    return U64Bytes(phys);
}
#endif

U32Kilobytes LLMemoryInfo::getPhysicalMemoryKB() const
{
#if LL_WINDOWS
    return LLMemoryAdjustKBResult(U32Kilobytes(mStatsMap["Total Physical KB"].asInteger()));

#elif LL_DARWIN
    return getHardwareMemSize();

#elif LL_LINUX
    U64 phys = 0;
    phys = (U64)(getpagesize()) * (U64)(get_phys_pages());
    return U64Bytes(phys);

#else
    return 0;

#endif
}

//static
void LLMemoryInfo::getAvailableMemoryKB(U32Kilobytes& avail_mem_kb)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEMORY;
#if LL_WINDOWS
    // Sigh, this shouldn't be a static method, then we wouldn't have to
    // reload this data separately from refresh()
    LLSD statsMap(loadStatsMap());

    avail_mem_kb = (U32Kilobytes)statsMap["Avail Physical KB"].asInteger();

#elif LL_DARWIN
    // use host_statistics64 to get memory info
    vm_statistics64_data_t vmstat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    mach_port_t host = mach_host_self();
    vm_size_t page_size;
    host_page_size(host, &page_size);
    kern_return_t result = host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info_t>(&vmstat), &count);
    if (result == KERN_SUCCESS)
    {
        avail_mem_kb = U64Bytes((vmstat.free_count + vmstat.inactive_count) * page_size);
    }
    else
    {
        avail_mem_kb = (U32Kilobytes)-1;
    }

#elif LL_LINUX
    // mStatsMap is derived from MEMINFO_FILE:
    // $ cat /proc/meminfo
    // MemTotal:        4108424 kB
    // MemFree:         1244064 kB
    // Buffers:           85164 kB
    // Cached:          1990264 kB
    // SwapCached:            0 kB
    // Active:          1176648 kB
    // Inactive:        1427532 kB
    // Active(anon):     529152 kB
    // Inactive(anon):    15924 kB
    // Active(file):     647496 kB
    // Inactive(file):  1411608 kB
    // Unevictable:          16 kB
    // Mlocked:              16 kB
    // HighTotal:       3266316 kB
    // HighFree:         721308 kB
    // LowTotal:         842108 kB
    // LowFree:          522756 kB
    // SwapTotal:       6384632 kB
    // SwapFree:        6384632 kB
    // Dirty:                28 kB
    // Writeback:             0 kB
    // AnonPages:        528820 kB
    // Mapped:            89472 kB
    // Shmem:             16324 kB
    // Slab:             159624 kB
    // SReclaimable:     145168 kB
    // SUnreclaim:        14456 kB
    // KernelStack:        2560 kB
    // PageTables:         5560 kB
    // NFS_Unstable:          0 kB
    // Bounce:                0 kB
    // WritebackTmp:          0 kB
    // CommitLimit:     8438844 kB
    // Committed_AS:    1271596 kB
    // VmallocTotal:     122880 kB
    // VmallocUsed:       65252 kB
    // VmallocChunk:      52356 kB
    // HardwareCorrupted:     0 kB
    // HugePages_Total:       0
    // HugePages_Free:        0
    // HugePages_Rsvd:        0
    // HugePages_Surp:        0
    // Hugepagesize:       2048 kB
    // DirectMap4k:      434168 kB
    // DirectMap2M:      477184 kB
    // (could also run 'free', but easier to read a file than run a program)
    LLSD statsMap(loadStatsMap());

    avail_mem_kb = (U32Kilobytes)statsMap["MemFree"].asInteger();
#else
    //do not know how to collect available memory info for other systems.
    //leave it blank here for now.

    avail_mem_kb = (U32Kilobytes)-1 ;
#endif
}

void LLMemoryInfo::stream(std::ostream& s) const
{
    // We want these memory stats to be easy to grep from the log, along with
    // the timestamp. So preface each line with the timestamp and a
    // distinctive marker. Without that, we'd have to search the log for the
    // introducer line, then read subsequent lines, etc...
    std::string pfx(LLError::utcTime() + " <mem> ");

    // Max key length
    size_t key_width(0);
    for (const auto& [key, value] : inMap(mStatsMap))
    {
        size_t len(key.length());
        if (len > key_width)
        {
            key_width = len;
        }
    }

    // Now stream stats
    for (const auto& [key, value] : inMap(mStatsMap))
    {
        s << pfx << std::setw(narrow<size_t>(key_width+1)) << (key + ':') << ' ';
        if (value.isInteger())
            s << std::setw(12) << value.asInteger();
        else if (value.isReal())
            s << std::fixed << std::setprecision(1) << value.asReal();
        else if (value.isDate())
            value.asDate().toStream(s);
        else
            s << value;           // just use default LLSD formatting
        s << std::endl;
    }
}

LLSD LLMemoryInfo::getStatsMap() const
{
    return mStatsMap;
}

LLMemoryInfo& LLMemoryInfo::refresh()
{
    LL_PROFILE_ZONE_SCOPED;
    mStatsMap = loadStatsMap();

    LL_DEBUGS("LLMemoryInfo") << "Populated mStatsMap:\n";
    LLSDSerialize::toPrettyXML(mStatsMap, LL_CONT);
    LL_ENDL;

    return *this;
}

LLSD LLMemoryInfo::loadStatsMap()
{
    LL_PROFILE_ZONE_SCOPED;

    // This implementation is derived from stream() code (as of 2011-06-29).
    Stats stats;

    // associate timestamp for analysis over time
    stats.add("timestamp", LLDate::now());

#if LL_WINDOWS
    MEMORYSTATUSEX state;
    state.dwLength = sizeof(state);
    GlobalMemoryStatusEx(&state);

    DWORDLONG div = 1024;

    stats.add("Percent Memory use", state.dwMemoryLoad/div);
    stats.add("Total Physical KB",  state.ullTotalPhys/div);
    stats.add("Avail Physical KB",  state.ullAvailPhys/div);
    stats.add("Total page KB",      state.ullTotalPageFile/div);
    stats.add("Avail page KB",      state.ullAvailPageFile/div);
    stats.add("Total Virtual KB",   state.ullTotalVirtual/div);
    stats.add("Avail Virtual KB",   state.ullAvailVirtual/div);

    // SL-12122 - Call to GetPerformanceInfo() was removed here. Took
    // on order of 10 ms, causing unacceptable frame time spike every
    // second, and results were never used. If this is needed in the
    // future, must find a way to avoid frame time impact (e.g. move
    // to another thread, call much less often).

    PROCESS_MEMORY_COUNTERS_EX pmem;
    pmem.cb = sizeof(pmem);
    // GetProcessMemoryInfo() is documented to accept either
    // PROCESS_MEMORY_COUNTERS* or PROCESS_MEMORY_COUNTERS_EX*, presumably
    // using the redundant size info to distinguish. But its prototype
    // specifically accepts PROCESS_MEMORY_COUNTERS*, and since this is a
    // classic-C API, PROCESS_MEMORY_COUNTERS_EX isn't a subclass. Cast the
    // pointer.
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &pmem, sizeof(pmem));

    stats.add("Page Fault Count",              pmem.PageFaultCount);
    stats.add("PeakWorkingSetSize KB",         pmem.PeakWorkingSetSize/div);
    stats.add("WorkingSetSize KB",             pmem.WorkingSetSize/div);
    stats.add("QutaPeakPagedPoolUsage KB",     pmem.QuotaPeakPagedPoolUsage/div);
    stats.add("QuotaPagedPoolUsage KB",        pmem.QuotaPagedPoolUsage/div);
    stats.add("QuotaPeakNonPagedPoolUsage KB", pmem.QuotaPeakNonPagedPoolUsage/div);
    stats.add("QuotaNonPagedPoolUsage KB",     pmem.QuotaNonPagedPoolUsage/div);
    stats.add("PagefileUsage KB",              pmem.PagefileUsage/div);
    stats.add("PeakPagefileUsage KB",          pmem.PeakPagefileUsage/div);
    stats.add("PrivateUsage KB",               pmem.PrivateUsage/div);

#elif LL_DARWIN

    const vm_size_t pagekb(vm_page_size / 1024);

    //
    // Collect the vm_stat's
    //

    {
        vm_statistics64_data_t vmstat;
        mach_msg_type_number_t vmstatCount = HOST_VM_INFO64_COUNT;

        if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) &vmstat, &vmstatCount) != KERN_SUCCESS)
    {
            LL_WARNS("LLMemoryInfo") << "Unable to collect memory information" << LL_ENDL;
        }
        else
        {
            stats.add("Pages free KB",      pagekb * vmstat.free_count);
            stats.add("Pages active KB",    pagekb * vmstat.active_count);
            stats.add("Pages inactive KB",  pagekb * vmstat.inactive_count);
            stats.add("Pages wired KB",     pagekb * vmstat.wire_count);

            stats.add("Pages zero fill",        vmstat.zero_fill_count);
            stats.add("Page reactivations",     vmstat.reactivations);
            stats.add("Page-ins",               vmstat.pageins);
            stats.add("Page-outs",              vmstat.pageouts);

            stats.add("Faults",                 vmstat.faults);
            stats.add("Faults copy-on-write",   vmstat.cow_faults);

            stats.add("Cache lookups",          vmstat.lookups);
            stats.add("Cache hits",             vmstat.hits);

            stats.add("Page purgeable count",   vmstat.purgeable_count);
            stats.add("Page purges",            vmstat.purges);

            stats.add("Page speculative reads", vmstat.speculative_count);
        }
    }

    //
    // Collect the misc task info
    //

        {
        task_events_info_data_t taskinfo;
        unsigned taskinfoSize = sizeof(taskinfo);

        if (task_info(mach_task_self(), TASK_EVENTS_INFO, (task_info_t) &taskinfo, &taskinfoSize) != KERN_SUCCESS)
                    {
            LL_WARNS("LLMemoryInfo") << "Unable to collect task information" << LL_ENDL;
            }
            else
            {
            stats.add("Task page-ins",                  taskinfo.pageins);
            stats.add("Task copy-on-write faults",      taskinfo.cow_faults);
            stats.add("Task messages sent",             taskinfo.messages_sent);
            stats.add("Task messages received",         taskinfo.messages_received);
            stats.add("Task mach system call count",    taskinfo.syscalls_mach);
            stats.add("Task unix system call count",    taskinfo.syscalls_unix);
            stats.add("Task context switch count",      taskinfo.csw);
            }
    }

    //
    // Collect the basic task info
    //

        {
            mach_task_basic_info_data_t taskinfo;
            mach_msg_type_number_t task_count = MACH_TASK_BASIC_INFO_COUNT;
            if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &taskinfo, &task_count) != KERN_SUCCESS)
            {
                LL_WARNS("LLMemoryInfo") << "Unable to collect task information" << LL_ENDL;
            }
            else
            {
                stats.add("Basic virtual memory KB", taskinfo.virtual_size / 1024);
                stats.add("Basic resident memory KB", taskinfo.resident_size / 1024);
                stats.add("Basic max resident memory KB", taskinfo.resident_size_max / 1024);
                stats.add("Basic new thread policy", taskinfo.policy);
                stats.add("Basic suspend count", taskinfo.suspend_count);
            }
    }

#elif LL_LINUX
    std::ifstream meminfo(MEMINFO_FILE);
    if (meminfo.is_open())
    {
        // MemTotal:        4108424 kB
        // MemFree:         1244064 kB
        // Buffers:           85164 kB
        // Cached:          1990264 kB
        // SwapCached:            0 kB
        // Active:          1176648 kB
        // Inactive:        1427532 kB
        // ...
        // VmallocTotal:     122880 kB
        // VmallocUsed:       65252 kB
        // VmallocChunk:      52356 kB
        // HardwareCorrupted:     0 kB
        // HugePages_Total:       0
        // HugePages_Free:        0
        // HugePages_Rsvd:        0
        // HugePages_Surp:        0
        // Hugepagesize:       2048 kB
        // DirectMap4k:      434168 kB
        // DirectMap2M:      477184 kB

        // Intentionally don't pass the boost::no_except flag. This
        // boost::regex object is constructed with a string literal, so it
        // should be valid every time. If it becomes invalid, we WANT an
        // exception, hopefully even before the dev checks in.
        boost::regex stat_rx("(.+): +([0-9]+)( kB)?");
        boost::smatch matched;

        std::string line;
        while (std::getline(meminfo, line))
        {
            LL_DEBUGS("LLMemoryInfo") << line << LL_ENDL;
            if (ll_regex_match(line, matched, stat_rx))
            {
                // e.g. "MemTotal:      4108424 kB"
                LLSD::String key(matched[1].first, matched[1].second);
                LLSD::String value_str(matched[2].first, matched[2].second);
                LLSD::Integer value(0);

                // Skip over VmallocTotal. It's just a fixed and huge number on (modern) systems. "34359738367 kB"
                // https://unix.stackexchange.com/questions/700724/why-is-vmalloctotal-34359738367-kb
                // If not skipped converting it to a LLSD::integer (32 bit) will fail and spam the logs (this function
                // is called quite frequently).
                if( key == "VmallocTotal")
                    continue;

                try
                {
                    value = boost::lexical_cast<LLSD::Integer>(value_str);
                }
                catch (const boost::bad_lexical_cast&)
                {
                    LL_WARNS("LLMemoryInfo") << "couldn't parse '" << value_str
                                             << "' in " << MEMINFO_FILE << " line: "
                                             << line << LL_ENDL;
                    continue;
                }
                // Store this statistic.
                stats.add(key, value);
            }
            else
            {
                LL_WARNS("LLMemoryInfo") << "unrecognized " << MEMINFO_FILE << " line: "
                                         << line << LL_ENDL;
            }
        }
    }
    else
    {
        LL_WARNS("LLMemoryInfo") << "Unable to collect memory information" << LL_ENDL;
    }

#else
    LL_WARNS("LLMemoryInfo") << "Unknown system; unable to collect memory information" << LL_ENDL;

#endif

    return stats.get();
}

std::ostream& operator<<(std::ostream& s, const LLOSInfo& info)
{
    info.stream(s);
    return s;
}

std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info)
{
    info.stream(s);
    return s;
}

std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info)
{
    info.stream(s);
    return s;
}

class FrameWatcher
{
public:
    FrameWatcher():
        // Hooking onto the "mainloop" event pump gets us one call per frame.
        mConnection(LLEventPumps::instance()
                    .obtain("mainloop")
                    .listen("FrameWatcher", boost::bind(&FrameWatcher::tick, this, _1))),
        // Initializing mSampleStart to an invalid timestamp alerts us to skip
        // trying to compute framerate on the first call.
        mSampleStart(-1),
        // Initializing mSampleEnd to 0 ensures that we treat the first call
        // as the completion of a sample window.
        mSampleEnd(0),
        mFrames(0),
        // Both MEM_INFO_WINDOW and MEM_INFO_THROTTLE are in seconds. We need
        // the number of integer MEM_INFO_THROTTLE sample slots that will fit
        // in MEM_INFO_WINDOW. Round up.
        mSamples(int((MEM_INFO_WINDOW / MEM_INFO_THROTTLE) + 0.7)),
        // Initializing to F32_MAX means that the first real frame will become
        // the slowest ever, which sounds like a good idea.
        mSlowest(F32_MAX)
    {}

    bool tick(const LLSD&)
    {
        F32 timestamp(mTimer.getElapsedTimeF32());

        // Count this frame in the interval just completed.
        ++mFrames;

        // Have we finished a sample window yet?
        if (timestamp < mSampleEnd)
        {
            // no, just keep waiting
            return false;
        }

        // Set up for next sample window. Capture values for previous frame in
        // local variables and reset data members.
        U32 frames(mFrames);
        F32 sampleStart(mSampleStart);
        // No frames yet in next window
        mFrames = 0;
        // which starts right now
        mSampleStart = timestamp;
        // and ends MEM_INFO_THROTTLE seconds in the future
        mSampleEnd = mSampleStart + MEM_INFO_THROTTLE;

        // On the very first call, that's all we can do, no framerate
        // computation is possible.
        if (sampleStart < 0)
        {
            return false;
        }

        // How long did this actually take? As framerate slows, the duration
        // of the frame we just finished could push us WELL beyond our desired
        // sample window size.
        F32 elapsed(timestamp - sampleStart);
        F32 framerate(frames/elapsed);

        // Remember previous slowest framerate because we're just about to
        // update it.
        F32 slowest(mSlowest);
        // Remember previous number of samples.
        boost::circular_buffer<F32>::size_type prevSize(mSamples.size());

        // Capture new framerate in our samples buffer. Once the buffer is
        // full (after MEM_INFO_WINDOW seconds), this will displace the oldest
        // sample. ("So they all rolled over, and one fell out...")
        mSamples.push_back(framerate);

        // Calculate the new minimum framerate. I know of no way to update a
        // rolling minimum without ever rescanning the buffer. But since there
        // are only a few tens of items in this buffer, rescanning it is
        // probably cheaper (and certainly easier to reason about) than
        // attempting to optimize away some of the scans.
        mSlowest = framerate;       // pick an arbitrary entry to start
        for (boost::circular_buffer<F32>::const_iterator si(mSamples.begin()), send(mSamples.end());
             si != send; ++si)
        {
            if (*si < mSlowest)
            {
                mSlowest = *si;
            }
        }

        // We're especially interested in memory as framerate drops. Only log
        // when framerate drops below the slowest framerate we remember.
        // (Should always be true for the end of the very first sample
        // window.)
        if (framerate >= slowest)
        {
            return false;
        }
        // Congratulations, we've hit a new low.  :-P

        LL_INFOS("FrameWatcher") << ' ';
        if (! prevSize)
        {
            LL_CONT << "initial framerate ";
        }
        else
        {
            LL_CONT << "slowest framerate for last " << int(prevSize * MEM_INFO_THROTTLE)
                    << " seconds ";
        }

    auto precision = LL_CONT.precision();

        LL_CONT << std::fixed << std::setprecision(1) << framerate << '\n'
                << LLMemoryInfo();

    LL_CONT.precision(precision);
    LL_CONT << LL_ENDL;
        return false;
    }

private:
    // Storing the connection in an LLTempBoundListener ensures it will be
    // disconnected when we're destroyed.
    LLTempBoundListener mConnection;
    // Track elapsed time
    LLTimer mTimer;
    // Some of what you see here is in fact redundant with functionality you
    // can get from LLTimer. Unfortunately the LLTimer API is missing the
    // feature we need: has at least the stated interval elapsed, and if so,
    // exactly how long has passed? So we have to do it by hand, sigh.
    // Time at start, end of sample window
    F32 mSampleStart, mSampleEnd;
    // Frames this sample window
    U32 mFrames;
    // Sliding window of framerate samples
    boost::circular_buffer<F32> mSamples;
    // Slowest framerate in mSamples
    F32 mSlowest;
};

// Need an instance of FrameWatcher before it does any good
static FrameWatcher sFrameWatcher;

bool gunzip_file(const std::string& srcfile, const std::string& dstfile)
{
    std::string tmpfile;
    const S32 UNCOMPRESS_BUFFER_SIZE = 32768;
    bool retval = false;
    gzFile src = NULL;
    U8 buffer[UNCOMPRESS_BUFFER_SIZE];
    LLFILE *dst = NULL;
    S32 bytes = 0;
    tmpfile = dstfile + ".t";
#ifdef LL_WINDOWS
    llutf16string utf16filename = utf8str_to_utf16str(srcfile);
    src = gzopen_w(utf16filename.c_str(), "rb");
#else
    src = gzopen(srcfile.c_str(), "rb");
#endif
    if (! src) goto err;
    dst = LLFile::fopen(tmpfile, "wb");     /* Flawfinder: ignore */
    if (! dst) goto err;
    do
    {
        bytes = gzread(src, buffer, UNCOMPRESS_BUFFER_SIZE);
        size_t nwrit = fwrite(buffer, sizeof(U8), bytes, dst);
        if (nwrit < (size_t) bytes)
        {
            LL_WARNS() << "Short write on " << tmpfile << ": Wrote " << nwrit << " of " << bytes << " bytes." << LL_ENDL;
            goto err;
        }
    } while(gzeof(src) == 0);
    fclose(dst);
    dst = NULL;
#if LL_WINDOWS
    // Rename in windows needs the dstfile to not exist.
    LLFile::remove(dstfile, ENOENT);
#endif
    if (LLFile::rename(tmpfile, dstfile) == -1) goto err;       /* Flawfinder: ignore */
    retval = true;
err:
    if (src != NULL) gzclose(src);
    if (dst != NULL) fclose(dst);
    return retval;
}

bool gzip_file(const std::string& srcfile, const std::string& dstfile)
{
    const S32 COMPRESS_BUFFER_SIZE = 32768;
    std::string tmpfile;
    bool retval = false;
    U8 buffer[COMPRESS_BUFFER_SIZE];
    gzFile dst = NULL;
    LLFILE *src = NULL;
    S32 bytes = 0;
    tmpfile = dstfile + ".t";

#ifdef LL_WINDOWS
    llutf16string utf16filename = utf8str_to_utf16str(tmpfile);
    dst = gzopen_w(utf16filename.c_str(), "wb");
#else
    dst = gzopen(tmpfile.c_str(), "wb");
#endif

    if (! dst) goto err;
    src = LLFile::fopen(srcfile, "rb");     /* Flawfinder: ignore */
    if (! src) goto err;

    while ((bytes = (S32)fread(buffer, sizeof(U8), COMPRESS_BUFFER_SIZE, src)) > 0)
    {
        if (gzwrite(dst, buffer, bytes) <= 0)
        {
            LL_WARNS() << "gzwrite failed: " << gzerror(dst, NULL) << LL_ENDL;
            goto err;
        }
    }

    if (ferror(src))
    {
        LL_WARNS() << "Error reading " << srcfile << LL_ENDL;
        goto err;
    }

    gzclose(dst);
    dst = NULL;
#if LL_WINDOWS
    // Rename in windows needs the dstfile to not exist.
    LLFile::remove(dstfile);
#endif
    if (LLFile::rename(tmpfile, dstfile) == -1) goto err;       /* Flawfinder: ignore */
    retval = true;
 err:
    if (src != NULL) fclose(src);
    if (dst != NULL) gzclose(dst);
    return retval;
}
