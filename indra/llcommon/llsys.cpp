/** 
 * @file llsys.cpp
 * @brief Impelementation of the basic system query functions.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
const char CPUINFO_FILE[] = "/proc/cpuinfo";
#endif


static const S32 CPUINFO_BUFFER_SIZE = 16383;
LLCPUInfo gSysCPU;

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
			else if(osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
			{
				 if(osvi.wProductType == VER_NT_WORKSTATION)
					mOSStringSimple = "Microsoft Windows Vista ";
				 else mOSStringSimple = "Microsoft Windows Vista Server ";
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
				snprintf(	/* Flawfinder: ignore */
					tmp,
					sizeof(tmp),
					"%s (Build %d)",
					csdversion.c_str(),
					(osvi.dwBuildNumber & 0xffff));	 
			}
			mOSString = mOSStringSimple + tmp;
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
			S32 idx1 = mOSStringSimple.find_first_of(".", 0);
			S32 idx2 = (idx1 != std::string::npos) ? mOSStringSimple.find_first_of(".", idx1+1) : std::string::npos;
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
	CProcessor proc;
	const ProcessorInfo* info = proc.GetCPUInfo();
	// proc.WriteInfoTextFile("procInfo.txt");
	mHasSSE = info->_Ext.SSE_StreamingSIMD_Extensions;
	mHasSSE2 = info->_Ext.SSE2_StreamingSIMD2_Extensions;
	mHasAltivec = info->_Ext.Altivec_Extensions;
	mCPUMhz = (S32)(proc.GetCPUFrequency(50)/1000000.0);
	mFamily.assign( info->strFamily );
	mCPUString = "Unknown";

#if LL_WINDOWS || LL_DARWIN || LL_SOLARIS
	out << proc.strCPUName;
	if (200 < mCPUMhz && mCPUMhz < 10000)           // *NOTE: cpu speed is often way wrong, do a sanity check
	{
		out << " (" << mCPUMhz << " MHz)";
	}
	mCPUString = out.str();
	
#elif LL_LINUX
	std::map< LLString, LLString > cpuinfo;
	LLFILE* cpuinfo_fp = LLFile::fopen(CPUINFO_FILE, "rb");
	if(cpuinfo_fp)
	{
		char line[MAX_STRING];
		memset(line, 0, MAX_STRING);
		while(fgets(line, MAX_STRING, cpuinfo_fp))
		{
			// /proc/cpuinfo on Linux looks like:
			// name\t*: value\n
			char* tabspot = strchr( line, '\t' );
			if (tabspot == NULL)
				continue;
			char* colspot = strchr( tabspot, ':' );
			if (colspot == NULL)
				continue;
			char* spacespot = strchr( colspot, ' ' );
			if (spacespot == NULL)
				continue;
			char* nlspot = strchr( line, '\n' );
			if (nlspot == NULL)
				nlspot = line + strlen( line ); // Fallback to terminating NUL
			std::string linename( line, tabspot );
			LLString llinename(linename);
			LLString::toLower(llinename);
			std::string lineval( spacespot + 1, nlspot );
			cpuinfo[ llinename ] = lineval;
		}
		fclose(cpuinfo_fp);
	}
# if LL_X86
	LLString flags = " " + cpuinfo["flags"] + " ";
	LLString::toLower(flags);
	mHasSSE = ( flags.find( " sse " ) != std::string::npos );
	mHasSSE2 = ( flags.find( " sse2 " ) != std::string::npos );
	
	F64 mhz;
	if (LLString::convertToF64(cpuinfo["cpu mhz"], mhz)
	    && 200.0 < mhz && mhz < 10000.0)
	{
		mCPUMhz = (S32)llrint(mhz);
	}
	if (!cpuinfo["model name"].empty())
		mCPUString = cpuinfo["model name"];
# endif // LL_X86
#endif // LL_LINUX
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

S32 LLCPUInfo::getMhz() const
{
	return mCPUMhz;
}

std::string LLCPUInfo::getCPUString() const
{
	return mCPUString;
}

void LLCPUInfo::stream(std::ostream& s) const
{
#if LL_WINDOWS || LL_DARWIN || LL_SOLARIS
	// gather machine information.
	char proc_buf[CPUINFO_BUFFER_SIZE];		/* Flawfinder: ignore */
	CProcessor proc;
	if(proc.CPUInfoToText(proc_buf, CPUINFO_BUFFER_SIZE))
	{
		s << proc_buf;
	}
	else
	{
		s << "Unable to collect processor information" << std::endl;
	}
#else
	// *NOTE: This works on linux. What will it do on other systems?
	LLFILE* cpuinfo = LLFile::fopen(CPUINFO_FILE, "rb");
	if(cpuinfo)
	{
		char line[MAX_STRING];
		memset(line, 0, MAX_STRING);
		while(fgets(line, MAX_STRING, cpuinfo))
		{
			line[strlen(line)-1] = ' ';
			s << line;
		}
		fclose(cpuinfo);
		s << std::endl;
	}
	else
	{
		s << "Unable to collect processor information" << std::endl;
	}
#endif
	// These are interesting as they reflect our internal view of the
	// CPU's attributes regardless of platform
	s << "->mHasSSE:     " << (U32)mHasSSE << std::endl;
	s << "->mHasSSE2:    " << (U32)mHasSSE2 << std::endl;
	s << "->mHasAltivec: " << (U32)mHasAltivec << std::endl;
	s << "->mCPUMhz:     " << mCPUMhz << std::endl;
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

BOOL gunzip_file(const char *srcfile, const char *dstfile)
{
	char tmpfile[LL_MAX_PATH];		/* Flawfinder: ignore */
	const S32 UNCOMPRESS_BUFFER_SIZE = 32768;
	BOOL retval = FALSE;
	gzFile src = NULL;
	U8 buffer[UNCOMPRESS_BUFFER_SIZE];
	LLFILE *dst = NULL;
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

BOOL gzip_file(const char *srcfile, const char *dstfile)
{
	const S32 COMPRESS_BUFFER_SIZE = 32768;
	char tmpfile[LL_MAX_PATH];		/* Flawfinder: ignore */
	BOOL retval = FALSE;
	U8 buffer[COMPRESS_BUFFER_SIZE];
	gzFile dst = NULL;
	LLFILE *src = NULL;
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
