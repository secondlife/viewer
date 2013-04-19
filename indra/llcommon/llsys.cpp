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
#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib/zlib.h"
#endif

#include "llprocessor.h"
#include "llerrorcontrol.h"
#include "llevents.h"
#include "lltimer.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_float.hpp>

using namespace llsd;

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#   include <psapi.h>               // GetPerformanceInfo() et al.
#elif LL_DARWIN
#	include <errno.h>
#	include <sys/sysctl.h>
#	include <sys/utsname.h>
#	include <stdint.h>
#	include <Carbon/Carbon.h>
#   include <stdexcept>
#	include <mach/host_info.h>
#	include <mach/mach_host.h>
#	include <mach/task.h>
#	include <mach/task_info.h>
#elif LL_LINUX
#	include <errno.h>
#	include <sys/utsname.h>
#	include <unistd.h>
#	include <sys/sysinfo.h>
#   include <stdexcept>
const char MEMINFO_FILE[] = "/proc/meminfo";
#elif LL_SOLARIS
#	include <stdio.h>
#	include <unistd.h>
#	include <sys/utsname.h>
#	define _STRUCTURED_PROC 1
#	include <sys/procfs.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <errno.h>
extern int errno;
#endif


static const S32 CPUINFO_BUFFER_SIZE = 16383;
LLCPUInfo gSysCPU;

// Don't log memory info any more often than this. It also serves as our
// framerate sample size.
static const F32 MEM_INFO_THROTTLE = 20;
// Sliding window of samples. We intentionally limit the length of time we
// remember "the slowest" framerate because framerate is very slow at login.
// If we only triggered FrameWatcher logging when the session framerate
// dropped below the login framerate, we'd have very little additional data.
static const F32 MEM_INFO_WINDOW = 10*60;

#if LL_WINDOWS
#ifndef DLLVERSIONINFO
typedef struct _DllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
}DLLVERSIONINFO;
#endif

#ifndef DLLGETVERSIONPROC
typedef int (FAR WINAPI *DLLGETVERSIONPROC) (DLLVERSIONINFO *);
#endif

bool get_shell32_dll_version(DWORD& major, DWORD& minor, DWORD& build_number)
{
	bool result = false;
	const U32 BUFF_SIZE = 32767;
	WCHAR tempBuf[BUFF_SIZE];
	if(GetSystemDirectory((LPWSTR)&tempBuf, BUFF_SIZE))
	{
		
		std::basic_string<WCHAR> shell32_path(tempBuf);

		// Shell32.dll contains the DLLGetVersion function. 
		// according to msdn its not part of the API
		// so you have to go in and get it.
		// http://msdn.microsoft.com/en-us/library/bb776404(VS.85).aspx
		shell32_path += TEXT("\\shell32.dll");

		HMODULE hDllInst = LoadLibrary(shell32_path.c_str());   //load the DLL
		if(hDllInst) 
		{  // Could successfully load the DLL
			DLLGETVERSIONPROC pDllGetVersion;
			/*
			You must get this function explicitly because earlier versions of the DLL
			don't implement this function. That makes the lack of implementation of the
			function a version marker in itself.
			*/
			pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hDllInst, 
																"DllGetVersion");

			if(pDllGetVersion) 
			{    
				// DLL supports version retrieval function
				DLLVERSIONINFO    dvi;

				ZeroMemory(&dvi, sizeof(dvi));
				dvi.cbSize = sizeof(dvi);
				HRESULT hr = (*pDllGetVersion)(&dvi);

				if(SUCCEEDED(hr)) 
				{ // Finally, the version is at our hands
					major = dvi.dwMajorVersion;
					minor = dvi.dwMinorVersion;
					build_number = dvi.dwBuildNumber;
					result = true;
				} 
			} 

			FreeLibrary(hDllInst);  // Release DLL
		} 
	}
	return result;
}
#endif // LL_WINDOWS

LLOSInfo::LLOSInfo() :
	mMajorVer(0), mMinorVer(0), mBuild(0)
{

#if LL_WINDOWS
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;

	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if(!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi)))
	{
		// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if(!GetVersionEx( (OSVERSIONINFO *) &osvi))
			return;
	}
	mMajorVer = osvi.dwMajorVersion;
	mMinorVer = osvi.dwMinorVersion;
	mBuild = osvi.dwBuildNumber;

	DWORD shell32_major, shell32_minor, shell32_build;
	bool got_shell32_version = get_shell32_dll_version(shell32_major, 
													   shell32_minor, 
													   shell32_build);

	switch(osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		{
			// Test for the product.
			if(osvi.dwMajorVersion <= 4)
			{
				mOSStringSimple = "Microsoft Windows NT ";
			}
			else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
			{
				mOSStringSimple = "Microsoft Windows 2000 ";
			}
			else if(osvi.dwMajorVersion ==5 && osvi.dwMinorVersion == 1)
			{
				mOSStringSimple = "Microsoft Windows XP ";
			}
			else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			{
				 if(osvi.wProductType == VER_NT_WORKSTATION)
					mOSStringSimple = "Microsoft Windows XP x64 Edition ";
				 else
					mOSStringSimple = "Microsoft Windows Server 2003 ";
			}
			else if(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion <= 2)
			{
				if(osvi.dwMinorVersion == 0)
				{
					if(osvi.wProductType == VER_NT_WORKSTATION)
						mOSStringSimple = "Microsoft Windows Vista ";
					else
						mOSStringSimple = "Windows Server 2008 ";
				}
				else if(osvi.dwMinorVersion == 1)
				{
					if(osvi.wProductType == VER_NT_WORKSTATION)
						mOSStringSimple = "Microsoft Windows 7 ";
					else
						mOSStringSimple = "Windows Server 2008 R2 ";
				}
				else if(osvi.dwMinorVersion == 2)
				{
					if(osvi.wProductType == VER_NT_WORKSTATION)
						mOSStringSimple = "Microsoft Windows 8 ";
					else
						mOSStringSimple = "Windows Server 2012 ";
				}

				///get native system info if available..
				typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO); ///function pointer for loading GetNativeSystemInfo
				SYSTEM_INFO si; //System Info object file contains architecture info
				PGNSI pGNSI; //pointer object
				ZeroMemory(&si, sizeof(SYSTEM_INFO)); //zero out the memory in information
				pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),  "GetNativeSystemInfo"); //load kernel32 get function
				if(NULL != pGNSI) //check if it has failed
					pGNSI(&si); //success
				else 
					GetSystemInfo(&si); //if it fails get regular system info 
				//(Warning: If GetSystemInfo it may result in incorrect information in a WOW64 machine, if the kernel fails to load)

				//msdn microsoft finds 32 bit and 64 bit flavors this way..
				//http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx (example code that contains quite a few more flavors
				//of windows than this code does (in case it is needed for the future)
				if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 ) //check for 64 bit
				{
					mOSStringSimple += "64-bit ";
				}
				else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
				{
					mOSStringSimple += "32-bit ";
				}
			}
			else   // Use the registry on early versions of Windows NT.
			{
				mOSStringSimple = "Microsoft Windows (unrecognized) ";

				HKEY hKey;
				WCHAR szProductType[80];
				DWORD dwBufLen;
				RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
							0, KEY_QUERY_VALUE, &hKey );
				RegQueryValueEx( hKey, L"ProductType", NULL, NULL,
								(LPBYTE) szProductType, &dwBufLen);
				RegCloseKey( hKey );
				if ( lstrcmpi( L"WINNT", szProductType) == 0 )
				{
					mOSStringSimple += "Professional ";
				}
				else if ( lstrcmpi( L"LANMANNT", szProductType) == 0 )
				{
					mOSStringSimple += "Server ";
				}
				else if ( lstrcmpi( L"SERVERNT", szProductType) == 0 )
				{
					mOSStringSimple += "Advanced Server ";
				}
			}

			std::string csdversion = utf16str_to_utf8str(osvi.szCSDVersion);
			// Display version, service pack (if any), and build number.
			std::string tmpstr;
			if(osvi.dwMajorVersion <= 4)
			{
				tmpstr = llformat("version %d.%d %s (Build %d)",
								  osvi.dwMajorVersion,
								  osvi.dwMinorVersion,
								  csdversion.c_str(),
								  (osvi.dwBuildNumber & 0xffff));
			}
			else
			{
				tmpstr = llformat("%s (Build %d)",
								  csdversion.c_str(),
								  (osvi.dwBuildNumber & 0xffff));
			}

			mOSString = mOSStringSimple + tmpstr;
		}
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
		// Test for the Windows 95 product family.
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			mOSStringSimple = "Microsoft Windows 95 ";
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
			{
                mOSStringSimple += "OSR2 ";
			}
		} 
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			mOSStringSimple = "Microsoft Windows 98 ";
			if ( osvi.szCSDVersion[1] == 'A' )
			{
                mOSStringSimple += "SE ";
			}
		} 
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
		{
			mOSStringSimple = "Microsoft Windows Millennium Edition ";
		}
		mOSString = mOSStringSimple;
		break;
	}

	std::string compatibility_mode;
	if(got_shell32_version)
	{
		if(osvi.dwMajorVersion != shell32_major || osvi.dwMinorVersion != shell32_minor)
		{
			compatibility_mode = llformat(" compatibility mode. real ver: %d.%d (Build %d)", 
											shell32_major,
											shell32_minor,
											shell32_build);
		}
	}
	mOSString += compatibility_mode;

#elif LL_DARWIN
	
	// Initialize mOSStringSimple to something like:
	// "Mac OS X 10.6.7"
	{
		const char * DARWIN_PRODUCT_NAME = "Mac OS X";
		
		SInt32 major_version, minor_version, bugfix_version;
		OSErr r1 = Gestalt(gestaltSystemVersionMajor, &major_version);
		OSErr r2 = Gestalt(gestaltSystemVersionMinor, &minor_version);
		OSErr r3 = Gestalt(gestaltSystemVersionBugFix, &bugfix_version);

		if((r1 == noErr) && (r2 == noErr) && (r3 == noErr))
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
	// "Mac OS X 10.6.7 Darwin Kernel Version 10.7.0: Sat Jan 29 15:17:16 PST 2011; root:xnu-1504.9.37~1/RELEASE_I386 i386"
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

}

#ifndef LL_WINDOWS
// static
S32 LLOSInfo::getMaxOpenFiles()
{
	const S32 OPEN_MAX_GUESS = 256;

#ifdef	OPEN_MAX
	static S32 open_max = OPEN_MAX;
#else
	static S32 open_max = 0;
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
				llerrs << "LLOSInfo::getMaxOpenFiles: sysconf error for _SC_OPEN_MAX" << llendl;
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

const S32 STATUS_SIZE = 8192;

//static
U32 LLOSInfo::getProcessVirtualSizeKB()
{
	U32 virtual_size = 0;
#if LL_WINDOWS
#endif
#if LL_LINUX
	LLFILE* status_filep = LLFile::fopen("/proc/self/status", "rb");
	if (status_filep)
	{
		S32 numRead = 0;		
		char buff[STATUS_SIZE];		/* Flawfinder: ignore */

		size_t nbytes = fread(buff, 1, STATUS_SIZE-1, status_filep);
		buff[nbytes] = '\0';

		// All these guys return numbers in KB
		char *memp = strstr(buff, "VmSize:");
		if (memp)
		{
			numRead += sscanf(memp, "%*s %u", &virtual_size);
		}
		fclose(status_filep);
	}
#elif LL_SOLARIS
	char proc_ps[LL_MAX_PATH];
	sprintf(proc_ps, "/proc/%d/psinfo", (int)getpid());
	int proc_fd = -1;
	if((proc_fd = open(proc_ps, O_RDONLY)) == -1){
		llwarns << "unable to open " << proc_ps << llendl;
		return 0;
	}
	psinfo_t proc_psinfo;
	if(read(proc_fd, &proc_psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)){
		llwarns << "Unable to read " << proc_ps << llendl;
		close(proc_fd);
		return 0;
	}

	close(proc_fd);

	virtual_size = proc_psinfo.pr_size;
#endif
	return virtual_size;
}

//static
U32 LLOSInfo::getProcessResidentSizeKB()
{
	U32 resident_size = 0;
#if LL_WINDOWS
#endif
#if LL_LINUX
	LLFILE* status_filep = LLFile::fopen("/proc/self/status", "rb");
	if (status_filep != NULL)
	{
		S32 numRead = 0;
		char buff[STATUS_SIZE];		/* Flawfinder: ignore */

		size_t nbytes = fread(buff, 1, STATUS_SIZE-1, status_filep);
		buff[nbytes] = '\0';

		// All these guys return numbers in KB
		char *memp = strstr(buff, "VmRSS:");
		if (memp)
		{
			numRead += sscanf(memp, "%*s %u", &resident_size);
		}
		fclose(status_filep);
	}
#elif LL_SOLARIS
	char proc_ps[LL_MAX_PATH];
	sprintf(proc_ps, "/proc/%d/psinfo", (int)getpid());
	int proc_fd = -1;
	if((proc_fd = open(proc_ps, O_RDONLY)) == -1){
		llwarns << "unable to open " << proc_ps << llendl;
		return 0;
	}
	psinfo_t proc_psinfo;
	if(read(proc_fd, &proc_psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)){
		llwarns << "Unable to read " << proc_ps << llendl;
		close(proc_fd);
		return 0;
	}

	close(proc_fd);

	resident_size = proc_psinfo.pr_rssize;
#endif
	return resident_size;
}

LLCPUInfo::LLCPUInfo()
{
	std::ostringstream out;
	LLProcessorInfo proc;
	// proc.WriteInfoTextFile("procInfo.txt");
	mHasSSE = proc.hasSSE();
	mHasSSE2 = proc.hasSSE2();
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

F64 LLCPUInfo::getMHz() const
{
	return mCPUMHz;
}

std::string LLCPUInfo::getCPUString() const
{
	return mCPUString;
}

void LLCPUInfo::stream(std::ostream& s) const
{
	// gather machine information.
	s << LLProcessorInfo().getCPUFeatureDescription();

	// These are interesting as they reflect our internal view of the
	// CPU's attributes regardless of platform
	s << "->mHasSSE:     " << (U32)mHasSSE << std::endl;
	s << "->mHasSSE2:    " << (U32)mHasSSE2 << std::endl;
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

// Wrap boost::regex_match() with a function that doesn't throw.
template <typename S, typename M, typename R>
static bool regex_match_no_exc(const S& string, M& match, const R& regex)
{
    try
    {
        return boost::regex_match(string, match, regex);
    }
    catch (const std::runtime_error& e)
    {
        LL_WARNS("LLMemoryInfo") << "error matching with '" << regex.str() << "': "
                                 << e.what() << ":\n'" << string << "'" << LL_ENDL;
        return false;
    }
}

// Wrap boost::regex_search() with a function that doesn't throw.
template <typename S, typename M, typename R>
static bool regex_search_no_exc(const S& string, M& match, const R& regex)
{
    try
    {
        return boost::regex_search(string, match, regex);
    }
    catch (const std::runtime_error& e)
    {
        LL_WARNS("LLMemoryInfo") << "error searching with '" << regex.str() << "': "
                                 << e.what() << ":\n'" << string << "'" << LL_ENDL;
        return false;
    }
}

LLMemoryInfo::LLMemoryInfo()
{
	refresh();
}

#if LL_WINDOWS
static U32 LLMemoryAdjustKBResult(U32 inKB)
{
	// Moved this here from llfloaterabout.cpp

	//! \bug
	// For some reason, the reported amount of memory is always wrong.
	// The original adjustment assumes it's always off by one meg, however
	// errors of as much as 2520 KB have been observed in the value
	// returned from the GetMemoryStatusEx function.  Here we keep the
	// original adjustment from llfoaterabout.cpp until this can be
	// fixed somehow.
	inKB += 1024;

	return inKB;
}
#endif

U32 LLMemoryInfo::getPhysicalMemoryKB() const
{
#if LL_WINDOWS
	return LLMemoryAdjustKBResult(mStatsMap["Total Physical KB"].asInteger());

#elif LL_DARWIN
	// This might work on Linux as well.  Someone check...
	uint64_t phys = 0;
	int mib[2] = { CTL_HW, HW_MEMSIZE };

	size_t len = sizeof(phys);	
	sysctl(mib, 2, &phys, &len, NULL, 0);
	
	return (U32)(phys >> 10);

#elif LL_LINUX
	U64 phys = 0;
	phys = (U64)(getpagesize()) * (U64)(get_phys_pages());
	return (U32)(phys >> 10);

#elif LL_SOLARIS
	U64 phys = 0;
	phys = (U64)(getpagesize()) * (U64)(sysconf(_SC_PHYS_PAGES));
	return (U32)(phys >> 10);

#else
	return 0;

#endif
}

U32 LLMemoryInfo::getPhysicalMemoryClamped() const
{
	// Return the total physical memory in bytes, but clamp it
	// to no more than U32_MAX
	
	U32 phys_kb = getPhysicalMemoryKB();
	if (phys_kb >= 4194304 /* 4GB in KB */)
	{
		return U32_MAX;
	}
	else
	{
		return phys_kb << 10;
	}
}

//static
void LLMemoryInfo::getAvailableMemoryKB(U32& avail_physical_mem_kb, U32& avail_virtual_mem_kb)
{
#if LL_WINDOWS
	// Sigh, this shouldn't be a static method, then we wouldn't have to
	// reload this data separately from refresh()
	LLSD statsMap(loadStatsMap());

	avail_physical_mem_kb = statsMap["Avail Physical KB"].asInteger();
	avail_virtual_mem_kb  = statsMap["Avail Virtual KB"].asInteger();

#elif LL_DARWIN
	// mStatsMap is derived from vm_stat, look for (e.g.) "kb free":
	// $ vm_stat
	// Mach Virtual Memory Statistics: (page size of 4096 bytes)
	// Pages free:                   462078.
	// Pages active:                 142010.
	// Pages inactive:               220007.
	// Pages wired down:             159552.
	// "Translation faults":      220825184.
	// Pages copy-on-write:         2104153.
	// Pages zero filled:         167034876.
	// Pages reactivated:             65153.
	// Pageins:                     2097212.
	// Pageouts:                      41759.
	// Object cache: 841598 hits of 7629869 lookups (11% hit rate)
	avail_physical_mem_kb = -1 ;
	avail_virtual_mem_kb = -1 ;

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
	avail_physical_mem_kb = -1 ;
	avail_virtual_mem_kb = -1 ;

#else
	//do not know how to collect available memory info for other systems.
	//leave it blank here for now.

	avail_physical_mem_kb = -1 ;
	avail_virtual_mem_kb = -1 ;
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
	BOOST_FOREACH(const MapEntry& pair, inMap(mStatsMap))
	{
		size_t len(pair.first.length());
		if (len > key_width)
		{
			key_width = len;
		}
	}

	// Now stream stats
	BOOST_FOREACH(const MapEntry& pair, inMap(mStatsMap))
	{
		s << pfx << std::setw(key_width+1) << (pair.first + ':') << ' ';
		LLSD value(pair.second);
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
	mStatsMap = loadStatsMap();

	LL_DEBUGS("LLMemoryInfo") << "Populated mStatsMap:\n";
	LLSDSerialize::toPrettyXML(mStatsMap, LL_CONT);
	LL_ENDL;

	return *this;
}

LLSD LLMemoryInfo::loadStatsMap()
{
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

	PERFORMANCE_INFORMATION perf;
	perf.cb = sizeof(perf);
	GetPerformanceInfo(&perf, sizeof(perf));

	SIZE_T pagekb(perf.PageSize/1024);
	stats.add("CommitTotal KB",     perf.CommitTotal * pagekb);
	stats.add("CommitLimit KB",     perf.CommitLimit * pagekb);
	stats.add("CommitPeak KB",      perf.CommitPeak * pagekb);
	stats.add("PhysicalTotal KB",   perf.PhysicalTotal * pagekb);
	stats.add("PhysicalAvail KB",   perf.PhysicalAvailable * pagekb);
	stats.add("SystemCache KB",     perf.SystemCache * pagekb);
	stats.add("KernelTotal KB",     perf.KernelTotal * pagekb);
	stats.add("KernelPaged KB",     perf.KernelPaged * pagekb);
	stats.add("KernelNonpaged KB",  perf.KernelNonpaged * pagekb);
	stats.add("PageSize KB",        pagekb);
	stats.add("HandleCount",        perf.HandleCount);
	stats.add("ProcessCount",       perf.ProcessCount);
	stats.add("ThreadCount",        perf.ThreadCount);

	PROCESS_MEMORY_COUNTERS_EX pmem;
	pmem.cb = sizeof(pmem);
	// GetProcessMemoryInfo() is documented to accept either
	// PROCESS_MEMORY_COUNTERS* or PROCESS_MEMORY_COUNTERS_EX*, presumably
	// using the redundant size info to distinguish. But its prototype
	// specifically accepts PROCESS_MEMORY_COUNTERS*, and since this is a
	// classic-C API, PROCESS_MEMORY_COUNTERS_EX isn't a subclass. Cast the
	// pointer.
	GetProcessMemoryInfo(GetCurrentProcess(), PPROCESS_MEMORY_COUNTERS(&pmem), sizeof(pmem));

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
		vm_statistics_data_t vmstat;
		mach_msg_type_number_t vmstatCount = HOST_VM_INFO_COUNT;

		if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t) &vmstat, &vmstatCount) != KERN_SUCCESS)
	{
			LL_WARNS("LLMemoryInfo") << "Unable to collect memory information" << LL_ENDL;
		}
		else
		{
			stats.add("Pages free KB",		pagekb * vmstat.free_count);
			stats.add("Pages active KB",	pagekb * vmstat.active_count);
			stats.add("Pages inactive KB",	pagekb * vmstat.inactive_count);
			stats.add("Pages wired KB",		pagekb * vmstat.wire_count);

			stats.add("Pages zero fill",		vmstat.zero_fill_count);
			stats.add("Page reactivations",		vmstat.reactivations);
			stats.add("Page-ins",				vmstat.pageins);
			stats.add("Page-outs",				vmstat.pageouts);
			
			stats.add("Faults",					vmstat.faults);
			stats.add("Faults copy-on-write",	vmstat.cow_faults);
			
			stats.add("Cache lookups",			vmstat.lookups);
			stats.add("Cache hits",				vmstat.hits);
			
			stats.add("Page purgeable count",	vmstat.purgeable_count);
			stats.add("Page purges",			vmstat.purges);
			
			stats.add("Page speculative reads",	vmstat.speculative_count);
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
			stats.add("Task page-ins",					taskinfo.pageins);
			stats.add("Task copy-on-write faults",		taskinfo.cow_faults);
			stats.add("Task messages sent",				taskinfo.messages_sent);
			stats.add("Task messages received",			taskinfo.messages_received);
			stats.add("Task mach system call count",	taskinfo.syscalls_mach);
			stats.add("Task unix system call count",	taskinfo.syscalls_unix);
			stats.add("Task context switch count",		taskinfo.csw);
			}
	}	
	
	//
	// Collect the basic task info
	//

		{
		task_basic_info_64_data_t taskinfo;
		unsigned taskinfoSize = sizeof(taskinfo);
		
		if (task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t) &taskinfo, &taskinfoSize) != KERN_SUCCESS)
			{
			LL_WARNS("LLMemoryInfo") << "Unable to collect task information" << LL_ENDL;
				}
				else
				{
			stats.add("Basic suspend count",					taskinfo.suspend_count);
			stats.add("Basic virtual memory KB",				taskinfo.virtual_size / 1024);
			stats.add("Basic resident memory KB",				taskinfo.resident_size / 1024);
			stats.add("Basic new thread policy",				taskinfo.policy);
		}
	}

#elif LL_SOLARIS
	U64 phys = 0;

	phys = (U64)(sysconf(_SC_PHYS_PAGES)) * (U64)(sysconf(_SC_PAGESIZE)/1024);

	stats.add("Total Physical KB", phys);

#elif LL_LINUX
	std::ifstream meminfo(MEMINFO_FILE);
	if (meminfo.is_open())
	{
		// MemTotal:		4108424 kB
		// MemFree:			1244064 kB
		// Buffers:			  85164 kB
		// Cached:			1990264 kB
		// SwapCached:			  0 kB
		// Active:			1176648 kB
		// Inactive:		1427532 kB
		// ...
		// VmallocTotal:	 122880 kB
		// VmallocUsed:		  65252 kB
		// VmallocChunk:	  52356 kB
		// HardwareCorrupted:	  0 kB
		// HugePages_Total:		  0
		// HugePages_Free:		  0
		// HugePages_Rsvd:		  0
		// HugePages_Surp:		  0
		// Hugepagesize:	   2048 kB
		// DirectMap4k:		 434168 kB
		// DirectMap2M:		 477184 kB

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
			if (regex_match_no_exc(line, matched, stat_rx))
			{
				// e.g. "MemTotal:		4108424 kB"
				LLSD::String key(matched[1].first, matched[1].second);
				LLSD::String value_str(matched[2].first, matched[2].second);
				LLSD::Integer value(0);
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
        LL_CONT << std::fixed << std::setprecision(1) << framerate << '\n'
                << LLMemoryInfo() << LL_ENDL;

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

BOOL gunzip_file(const std::string& srcfile, const std::string& dstfile)
{
	std::string tmpfile;
	const S32 UNCOMPRESS_BUFFER_SIZE = 32768;
	BOOL retval = FALSE;
	gzFile src = NULL;
	U8 buffer[UNCOMPRESS_BUFFER_SIZE];
	LLFILE *dst = NULL;
	S32 bytes = 0;
	tmpfile = dstfile + ".t";
	src = gzopen(srcfile.c_str(), "rb");
	if (! src) goto err;
	dst = LLFile::fopen(tmpfile, "wb");		/* Flawfinder: ignore */
	if (! dst) goto err;
	do
	{
		bytes = gzread(src, buffer, UNCOMPRESS_BUFFER_SIZE);
		size_t nwrit = fwrite(buffer, sizeof(U8), bytes, dst);
		if (nwrit < (size_t) bytes)
		{
			llwarns << "Short write on " << tmpfile << ": Wrote " << nwrit << " of " << bytes << " bytes." << llendl;
			goto err;
		}
	} while(gzeof(src) == 0);
	fclose(dst); 
	dst = NULL;	
	if (LLFile::rename(tmpfile, dstfile) == -1) goto err;		/* Flawfinder: ignore */
	retval = TRUE;
err:
	if (src != NULL) gzclose(src);
	if (dst != NULL) fclose(dst);
	return retval;
}

BOOL gzip_file(const std::string& srcfile, const std::string& dstfile)
{
	const S32 COMPRESS_BUFFER_SIZE = 32768;
	std::string tmpfile;
	BOOL retval = FALSE;
	U8 buffer[COMPRESS_BUFFER_SIZE];
	gzFile dst = NULL;
	LLFILE *src = NULL;
	S32 bytes = 0;
	tmpfile = dstfile + ".t";
	dst = gzopen(tmpfile.c_str(), "wb");		/* Flawfinder: ignore */
	if (! dst) goto err;
	src = LLFile::fopen(srcfile, "rb");		/* Flawfinder: ignore */
	if (! src) goto err;

	while ((bytes = (S32)fread(buffer, sizeof(U8), COMPRESS_BUFFER_SIZE, src)) > 0)
	{
		if (gzwrite(dst, buffer, bytes) <= 0)
		{
			llwarns << "gzwrite failed: " << gzerror(dst, NULL) << llendl;
			goto err;
		}
	}

	if (ferror(src))
	{
		llwarns << "Error reading " << srcfile << llendl;
		goto err;
	}

	gzclose(dst);
	dst = NULL;
#if LL_WINDOWS
	// Rename in windows needs the dstfile to not exist.
	LLFile::remove(dstfile);
#endif
	if (LLFile::rename(tmpfile, dstfile) == -1) goto err;		/* Flawfinder: ignore */
	retval = TRUE;
 err:
	if (src != NULL) fclose(src);
	if (dst != NULL) gzclose(dst);
	return retval;
}
