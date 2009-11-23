FOO TO BREAK BUILD/** 
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#if defined(LL_WINDOWS)
# include <windows.h>
# include <psapi.h>
#elif defined(LL_DARWIN)
# include <sys/types.h>
# include <mach/task.h>
# include <mach/mach_init.h>
#elif LL_LINUX || LL_SOLARIS
# include <unistd.h>
#endif

#include "llmemory.h"

//----------------------------------------------------------------------------

//static
char* LLMemory::reserveMem = 0;

//static
void LLMemory::initClass()
{
	if (!reserveMem)
	{
		reserveMem = new char[16*1024]; // reserve 16K for out of memory error handling
	}
}

//static
void LLMemory::cleanupClass()
{
	delete [] reserveMem;
	reserveMem = NULL;
}

//static
void LLMemory::freeReserve()
{
	delete [] reserveMem;
	reserveMem = NULL;
}

void* ll_allocate (size_t size)
{
	if (size == 0)
	{
		llwarns << "Null allocation" << llendl;
	}
	void *p = malloc(size);
	if (p == NULL)
	{
		LLMemory::freeReserve();
		llerrs << "Out of memory Error" << llendl;
	}
	return p;
}

void ll_release (void *p)
{
	free(p);
}

//----------------------------------------------------------------------------

#if defined(LL_WINDOWS)

U64 LLMemory::getCurrentRSS()
{
	HANDLE self = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS counters;
	
	if (!GetProcessMemoryInfo(self, &counters, sizeof(counters)))
	{
		llwarns << "GetProcessMemoryInfo failed" << llendl;
		return 0;
	}

	return counters.WorkingSetSize;
}

#elif defined(LL_DARWIN)

/* 
	The API used here is not capable of dealing with 64-bit memory sizes, but is available before 10.4.
	
	Once we start requiring 10.4, we can use the updated API, which looks like this:
	
	task_basic_info_64_data_t basicInfo;
	mach_msg_type_number_t  basicInfoCount = TASK_BASIC_INFO_64_COUNT;
	if (task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&basicInfo, &basicInfoCount) == KERN_SUCCESS)
	
	Of course, this doesn't gain us anything unless we start building the viewer as a 64-bit executable, since that's the only way
	for our memory allocation to exceed 2^32.
*/

// 	if (sysctl(ctl, 2, &page_size, &size, NULL, 0) == -1)
// 	{
// 		llwarns << "Couldn't get page size" << llendl;
// 		return 0;
// 	} else {
// 		return page_size;
// 	}
// }

U64 LLMemory::getCurrentRSS()
{
	U64 residentSize = 0;
	task_basic_info_data_t basicInfo;
	mach_msg_type_number_t  basicInfoCount = TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&basicInfo, &basicInfoCount) == KERN_SUCCESS)
	{
		residentSize = basicInfo.resident_size;

		// If we ever wanted it, the process virtual size is also available as:
		// virtualSize = basicInfo.virtual_size;
		
//		llinfos << "resident size is " << residentSize << llendl;
	}
	else
	{
		llwarns << "task_info failed" << llendl;
	}

	return residentSize;
}

#elif defined(LL_LINUX)

U64 LLMemory::getCurrentRSS()
{
	static const char statPath[] = "/proc/self/stat";
	LLFILE *fp = LLFile::fopen(statPath, "r");
	U64 rss = 0;

	if (fp == NULL)
	{
		llwarns << "couldn't open " << statPath << llendl;
		goto bail;
	}

	// Eee-yew!	 See Documentation/filesystems/proc.txt in your
	// nearest friendly kernel tree for details.
	
	{
		int ret = fscanf(fp, "%*d (%*[^)]) %*c %*d %*d %*d %*d %*d %*d %*d "
						 "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %Lu",
						 &rss);
		if (ret != 1)
		{
			llwarns << "couldn't parse contents of " << statPath << llendl;
			rss = 0;
		}
	}
	
	fclose(fp);

bail:
	return rss;
}

#elif LL_SOLARIS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define _STRUCTURED_PROC 1
#include <sys/procfs.h>

U64 LLMemory::getCurrentRSS()
{
	char path [LL_MAX_PATH];	/* Flawfinder: ignore */ 

	sprintf(path, "/proc/%d/psinfo", (int)getpid());
	int proc_fd = -1;
	if((proc_fd = open(path, O_RDONLY)) == -1){
		llwarns << "LLmemory::getCurrentRSS() unable to open " << path << ". Returning 0 RSS!" << llendl;
		return 0;
	}
	psinfo_t proc_psinfo;
	if(read(proc_fd, &proc_psinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)){
		llwarns << "LLmemory::getCurrentRSS() Unable to read from " << path << ". Returning 0 RSS!" << llendl;
		close(proc_fd);
		return 0;
	}

	close(proc_fd);

	return((U64)proc_psinfo.pr_rssize * 1024);
}
#else

U64 LLMemory::getCurrentRSS()
{
	return 0;
}

#endif
