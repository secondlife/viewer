/** 
 * @file llsys.cpp
 * @brief Impelementation of the basic system query functions.
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

#include "linden_common.h"

#include "llsys.h"

#include <iostream>
#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib/zlib.h"
#endif

#include "llprocessor.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#elif LL_DARWIN
#	include <errno.h>
#	include <sys/sysctl.h>
#	include <sys/utsname.h>
#	include <stdint.h>
#elif LL_LINUX
#	include <errno.h>
#	include <sys/utsname.h>
#	include <unistd.h>
#	include <sys/sysinfo.h>
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
			else if(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion <= 1)
			{
				if(osvi.dwMinorVersion == 0)
				{
					mOSStringSimple = "Microsoft Windows Vista ";
				}
				else if(osvi.dwMinorVersion == 1)
				{
					mOSStringSimple = "Microsoft Windows 7 ";
				}

				if(osvi.wProductType != VER_NT_WORKSTATION)
				{
					mOSStringSimple += "Server ";
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
		if(osvi.dwMajorVersion != shell32_major 
			|| osvi.dwMinorVersion != shell32_minor)
		{
			compatibility_mode = llformat(" compatibility mode. real ver: %d.%d (Build %d)", 
											shell32_major,
											shell32_minor,
											shell32_build);
		}
	}
	mOSString += compatibility_mode;

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
		if (ostype == "Darwin")
		{
			// Only care about major Darwin versions, truncate at first '.'
			S32 idx1 = mOSStringSimple.find_first_of(".", 0);
			std::string simple = mOSStringSimple.substr(0, idx1);
			if (simple.length() > 0)
				mOSStringSimple = simple;
		}
		else if (ostype == "Linux")
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

LLMemoryInfo::LLMemoryInfo()
{
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
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);

	return LLMemoryAdjustKBResult((U32)(state.ullTotalPhys >> 10));

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
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);

	avail_physical_mem_kb = (U32)(state.ullAvailPhys/1024) ;
	avail_virtual_mem_kb = (U32)(state.ullAvailVirtual/1024) ;

#else
	//do not know how to collect available memory info for other systems.
	//leave it blank here for now.

	avail_physical_mem_kb = -1 ;
	avail_virtual_mem_kb = -1 ;
#endif
}

void LLMemoryInfo::stream(std::ostream& s) const
{
#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);

	s << "Percent Memory use: " << (U32)state.dwMemoryLoad << '%' << std::endl;
	s << "Total Physical KB:  " << (U32)(state.ullTotalPhys/1024) << std::endl;
	s << "Avail Physical KB:  " << (U32)(state.ullAvailPhys/1024) << std::endl;
	s << "Total page KB:      " << (U32)(state.ullTotalPageFile/1024) << std::endl;
	s << "Avail page KB:      " << (U32)(state.ullAvailPageFile/1024) << std::endl;
	s << "Total Virtual KB:   " << (U32)(state.ullTotalVirtual/1024) << std::endl;
	s << "Avail Virtual KB:   " << (U32)(state.ullAvailVirtual/1024) << std::endl;
#elif LL_DARWIN
	uint64_t phys = 0;

	size_t len = sizeof(phys);	
	
	if(sysctlbyname("hw.memsize", &phys, &len, NULL, 0) == 0)
	{
		s << "Total Physical KB:  " << phys/1024 << std::endl;
	}
	else
	{
		s << "Unable to collect memory information";
	}
#elif LL_SOLARIS
        U64 phys = 0;

        phys = (U64)(sysconf(_SC_PHYS_PAGES)) * (U64)(sysconf(_SC_PAGESIZE)/1024);

        s << "Total Physical KB:  " << phys << std::endl;
#else
	// *NOTE: This works on linux. What will it do on other systems?
	LLFILE* meminfo = LLFile::fopen(MEMINFO_FILE,"rb");
	if(meminfo)
	{
		char line[MAX_STRING];		/* Flawfinder: ignore */
		memset(line, 0, MAX_STRING);
		while(fgets(line, MAX_STRING, meminfo))
		{
			line[strlen(line)-1] = ' ';		 /*Flawfinder: ignore*/
			s << line;
		}
		fclose(meminfo);
	}
	else
	{
		s << "Unable to collect memory information";
	}
#endif
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

	do
	{
		bytes = (S32)fread(buffer, sizeof(U8), COMPRESS_BUFFER_SIZE,src);
		gzwrite(dst, buffer, bytes);
	} while(feof(src) == 0);
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
