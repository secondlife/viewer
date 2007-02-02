/** 
 * @file llsys.cpp
 * @brief Impelementation of the basic system query functions.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llsys.h"

#include <iostream>
#include <zlib/zlib.h>
#include "processor.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#elif LL_DARWIN
#	include <sys/sysctl.h>
#	include <sys/utsname.h>
#elif LL_LINUX
#	include <sys/utsname.h>
const char MEMINFO_FILE[] = "/proc/meminfo";
const char CPUINFO_FILE[] = "/proc/cpuinfo";
#endif


static const S32 CPUINFO_BUFFER_SIZE = 16383;
LLCPUInfo gSysCPU;

LLOSInfo::LLOSInfo() :
	mMajorVer(0), mMinorVer(0), mBuild(0),
	mOSString("")
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

	switch(osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		{
			// Test for the product.
			if(osvi.dwMajorVersion <= 4)
			{
				mOSString = "Microsoft Windows NT ";
			}
			else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
			{
				mOSString = "Microsoft Windows 2000 ";
			}
			else if(osvi.dwMajorVersion ==5 && osvi.dwMinorVersion == 1)
			{
				mOSString = "Microsoft Windows XP ";
			}
			else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			{
				 if(osvi.wProductType == VER_NT_WORKSTATION)
					mOSString = "Microsoft Windows XP x64 Edition ";
				 else mOSString = "Microsoft Windows Server 2003 ";
			}
			else if(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
			{
				 if(osvi.wProductType == VER_NT_WORKSTATION)
					mOSString = "Microsoft Windows Vista ";
				 else mOSString = "Microsoft Windows Vista Server ";
			}
			else   // Use the registry on early versions of Windows NT.
			{
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
					mOSString += "Professional ";
				}
				else if ( lstrcmpi( L"LANMANNT", szProductType) == 0 )
				{
					mOSString += "Server ";
				}
				else if ( lstrcmpi( L"SERVERNT", szProductType) == 0 )
				{
					mOSString += "Advanced Server ";
				}
			}

			std::string csdversion = utf16str_to_utf8str(osvi.szCSDVersion);
			// Display version, service pack (if any), and build number.
			char tmp[MAX_STRING];		/* Flawfinder: ignore */
			if(osvi.dwMajorVersion <= 4)
			{
				snprintf(	/* Flawfinder: ignore */
					tmp,
					sizeof(tmp),
					"version %d.%d %s (Build %d)",
					osvi.dwMajorVersion,
					osvi.dwMinorVersion,
					csdversion.c_str(),
					(osvi.dwBuildNumber & 0xffff));
			}
			else
			{
				snprintf( /* Flawfinder: ignore */
					tmp,
					sizeof(tmp),
					"%s (Build %d)",
					csdversion.c_str(),
					(osvi.dwBuildNumber & 0xffff));	 
			}
			mOSString += tmp;
		}
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
		// Test for the Windows 95 product family.
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			mOSString = "Microsoft Windows 95 ";
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
			{
                mOSString += "OSR2 ";
			}
		} 
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			mOSString = "Microsoft Windows 98 ";
			if ( osvi.szCSDVersion[1] == 'A' )
			{
                mOSString += "SE ";
			}
		} 
		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
		{
			mOSString = "Microsoft Windows Millennium Edition ";
		} 
		break;
	}
#else
	struct utsname un;
	if(0==uname(&un))
	{
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
		mOSString.append("Unable to collect OS info");
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

const S32 STATUS_SIZE = 8192;

//static
U32 LLOSInfo::getProcessVirtualSizeKB()
{
	U32 virtual_size = 0;
#if LL_WINDOWS
#endif
#if LL_LINUX
	FILE* status_filep = LLFile::fopen("/proc/self/status", "r");	/* Flawfinder: ignore */
	S32 numRead = 0;		
	char buff[STATUS_SIZE];		/* Flawfinder: ignore */
	bzero(buff, STATUS_SIZE);

	rewind(status_filep);
	fread(buff, 1, STATUS_SIZE-2, status_filep);

	// All these guys return numbers in KB
	char *memp = strstr(buff, "VmSize:");
	if (memp)
	{
		numRead += sscanf(memp, "%*s %u", &virtual_size);
	}
	fclose(status_filep);
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
	FILE* status_filep = LLFile::fopen("/proc/self/status", "r");	/* Flawfinder: ignore */
	if (status_filep != NULL)
	{
		S32 numRead = 0;
		char buff[STATUS_SIZE];		/* Flawfinder: ignore */
		bzero(buff, STATUS_SIZE);

		rewind(status_filep);
		fread(buff, 1, STATUS_SIZE-2, status_filep);

		// All these guys return numbers in KB
		char *memp = strstr(buff, "VmRSS:");
		if (memp)
		{
			numRead += sscanf(memp, "%*s %u", &resident_size);
		}
		fclose(status_filep);
	}
#endif
	return resident_size;
}

LLCPUInfo::LLCPUInfo()
{
	CProcessor proc;
	const ProcessorInfo* info = proc.GetCPUInfo();
	mHasSSE = (info->_Ext.SSE_StreamingSIMD_Extensions != 0);
	mHasSSE2 = (info->_Ext.SSE2_StreamingSIMD2_Extensions != 0);
	mCPUMhz = (S32)(proc.GetCPUFrequency(50)/1000000.0);
	mFamily.assign( info->strFamily );
}

std::string LLCPUInfo::getCPUString() const
{
	std::string cpu_string;

#if LL_WINDOWS || LL_DARWIN
	// gather machine information.
	char proc_buf[CPUINFO_BUFFER_SIZE];		/* Flawfinder: ignore */
	CProcessor proc;
	if(proc.CPUInfoToText(proc_buf, CPUINFO_BUFFER_SIZE))
	{
		cpu_string.append(proc_buf);
	}
#else
	cpu_string.append("Can't get CPU information");
#endif

	return cpu_string;
}

std::string LLCPUInfo::getCPUStringTerse() const
{
	std::string cpu_string;

#if LL_WINDOWS || LL_DARWIN
	CProcessor proc;
	const ProcessorInfo *info = proc.GetCPUInfo();

	cpu_string.append(info->strBrandID);
	
	F64 freq = (F64)(S64)proc.GetCPUFrequency(50) / 1000000.f;

	// cpu speed is often way wrong, do a sanity check
	if (freq < 10000.f && freq > 200.f )
	{
		char tmp[MAX_STRING];		/* Flawfinder: ignore */
		snprintf(tmp, sizeof(tmp), " (%.0f Mhz)", freq);		/* Flawfinder: ignore */

		cpu_string.append(tmp);
	}
#else
	cpu_string.append("Can't get terse CPU information");
#endif

	return cpu_string;
}

void LLCPUInfo::stream(std::ostream& s) const
{
#if LL_WINDOWS || LL_DARWIN
	// gather machine information.
	char proc_buf[CPUINFO_BUFFER_SIZE];		/* Flawfinder: ignore */
	CProcessor proc;
	if(proc.CPUInfoToText(proc_buf, CPUINFO_BUFFER_SIZE))
	{
		s << proc_buf;
	}
	else
	{
		s << "Unable to collect processor info";
	}
#else
	// *NOTE: This works on linux. What will it do on other systems?
	FILE* cpuinfo = LLFile::fopen(CPUINFO_FILE, "r");		/* Flawfinder: ignore */
	if(cpuinfo)
	{
		char line[MAX_STRING];		/* Flawfinder: ignore */
		memset(line, 0, MAX_STRING);
		while(fgets(line, MAX_STRING, cpuinfo))
		{
			line[strlen(line)-1] = ' ';		 /*Flawfinder: ignore*/
			s << line;
		}
		fclose(cpuinfo);
	}
	else
	{
		s << "Unable to collect memory information";
	}
#endif
}

LLMemoryInfo::LLMemoryInfo()
{
}

#if LL_LINUX
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

U32 LLMemoryInfo::getPhysicalMemory() const
{
#if LL_WINDOWS
	MEMORYSTATUS state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatus(&state);

	return (U32)state.dwTotalPhys;

#elif LL_DARWIN
	// This might work on Linux as well.  Someone check...
	unsigned int phys = 0;
	int mib[2] = { CTL_HW, HW_PHYSMEM };

	size_t len = sizeof(phys);	
	sysctl(mib, 2, &phys, &len, NULL, 0);
	
	return phys;
#elif LL_LINUX

	return getpagesize() * get_phys_pages();

#else
	return 0;

#endif
}

void LLMemoryInfo::stream(std::ostream& s) const
{
#if LL_WINDOWS
	MEMORYSTATUS state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatus(&state);

	s << "Percent Memory use: " << (U32)state.dwMemoryLoad << '%' << std::endl;
	s << "Total Physical Kb:  " << (U32)state.dwTotalPhys/1024 << std::endl;
	s << "Avail Physical Kb:  " << (U32)state.dwAvailPhys/1024 << std::endl;
	s << "Total page Kb:      " << (U32)state.dwTotalPageFile/1024 << std::endl;
	s << "Avail page Kb:      " << (U32)state.dwAvailPageFile/1024 << std::endl;
	s << "Total Virtual Kb:   " << (U32)state.dwTotalVirtual/1024 << std::endl;
	s << "Avail Virtual Kb:   " << (U32)state.dwAvailVirtual/1024 << std::endl;
#elif LL_DARWIN
	U64 phys = 0;

	size_t len = sizeof(phys);	
	
	if(sysctlbyname("hw.memsize", &phys, &len, NULL, 0) == 0)
	{
		s << "Total Physical Kb:  " << phys/1024 << std::endl;
	}
	else
	{
		s << "Unable to collect memory information";
	}
	
#else
	// *NOTE: This works on linux. What will it do on other systems?
	FILE* meminfo = LLFile::fopen(MEMINFO_FILE,"r");		/* Flawfinder: ignore */
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

BOOL gunzip_file(const char *srcfile, const char *dstfile)
{
	char tmpfile[LL_MAX_PATH];		/* Flawfinder: ignore */
	const S32 UNCOMPRESS_BUFFER_SIZE = 32768;
	BOOL retval = FALSE;
	gzFile src = NULL;
	U8 buffer[UNCOMPRESS_BUFFER_SIZE];
	FILE *dst = NULL;
	S32 bytes = 0;
	(void *) strcpy(tmpfile, dstfile);		/* Flawfinder: ignore */
	(void *) strncat(tmpfile, ".t", sizeof(tmpfile) - strlen(tmpfile) -1);		/* Flawfinder: ignore */
	src = gzopen(srcfile, "rb");
	if (! src) goto err;
	dst = LLFile::fopen(tmpfile, "wb");		/* Flawfinder: ignore */
	if (! dst) goto err;
	do
	{
		bytes = gzread(src, buffer, UNCOMPRESS_BUFFER_SIZE);
		fwrite(buffer, sizeof(U8), bytes, dst);
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

BOOL gzip_file(const char *srcfile, const char *dstfile)
{
	const S32 COMPRESS_BUFFER_SIZE = 32768;
	char tmpfile[LL_MAX_PATH];		/* Flawfinder: ignore */
	BOOL retval = FALSE;
	U8 buffer[COMPRESS_BUFFER_SIZE];
	gzFile dst = NULL;
	FILE *src = NULL;
	S32 bytes = 0;
	(void *) strcpy(tmpfile, dstfile);		/* Flawfinder: ignore */
	(void *) strncat(tmpfile, ".t", sizeof(tmpfile) - strlen(tmpfile) -1);		/* Flawfinder: ignore */
	dst = gzopen(tmpfile, "wb");		/* Flawfinder: ignore */
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
