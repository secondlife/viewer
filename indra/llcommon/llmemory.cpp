/** 
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
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

#include "llmemory.h"

#if MEM_TRACK_MEM
#include "llthread.h"
#endif

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

//static 
U32 LLMemory::getWorkingSetSize()
{
    PROCESS_MEMORY_COUNTERS pmc ;
	U32 ret = 0 ;

    if (GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc)) )
	{
		ret = pmc.WorkingSetSize ;
	}

	return ret ;
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

U32 LLMemory::getWorkingSetSize()
{
	return 0 ;
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

U32 LLMemory::getWorkingSetSize()
{
	return 0 ;
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

U32 LLMemory::getWorkingSetSize()
{
	return 0 ;
}

#else

U64 LLMemory::getCurrentRSS()
{
	return 0;
}

U32 LLMemory::getWorkingSetSize()
{
	return 0 ;
}

#endif

//--------------------------------------------------------------------------------------------------
#if MEM_TRACK_MEM
#include "llframetimer.h"

//static 
LLMemTracker* LLMemTracker::sInstance = NULL ;

LLMemTracker::LLMemTracker()
{
	mLastAllocatedMem = LLMemory::getWorkingSetSize() ;
	mCapacity = 128 ;	
	mCurIndex = 0 ;
	mCounter = 0 ;
	mDrawnIndex = 0 ;
	mPaused = FALSE ;

	mMutexp = new LLMutex(NULL) ;
	mStringBuffer = new char*[128] ;
	mStringBuffer[0] = new char[mCapacity * 128] ;
	for(S32 i = 1 ; i < mCapacity ; i++)
	{
		mStringBuffer[i] = mStringBuffer[i-1] + 128 ;
	}
}

LLMemTracker::~LLMemTracker()
{
	delete[] mStringBuffer[0] ;
	delete[] mStringBuffer;
	delete mMutexp ;
}

//static 
LLMemTracker* LLMemTracker::getInstance()
{
	if(!sInstance)
	{
		sInstance = new LLMemTracker() ;
	}
	return sInstance ;
}

//static 
void LLMemTracker::release() 
{
	if(sInstance)
	{
		delete sInstance ;
		sInstance = NULL ;
	}
}

//static
void LLMemTracker::track(const char* function, const int line)
{
	static const S32 MIN_ALLOCATION = 0 ; //1KB

	if(mPaused)
	{
		return ;
	}

	U32 allocated_mem = LLMemory::getWorkingSetSize() ;

	LLMutexLock lock(mMutexp) ;

	S32 delta_mem = allocated_mem - mLastAllocatedMem ;
	mLastAllocatedMem = allocated_mem ;

	if(delta_mem <= 0)
	{
		return ; //occupied memory does not grow
	}

	if(delta_mem < MIN_ALLOCATION)
	{
		return ;
	}
		
	char* buffer = mStringBuffer[mCurIndex++] ;
	F32 time = (F32)LLFrameTimer::getElapsedSeconds() ;
	S32 hours = (S32)(time / (60*60));
	S32 mins = (S32)((time - hours*(60*60)) / 60);
	S32 secs = (S32)((time - hours*(60*60) - mins*60));
	strcpy(buffer, function) ;
	sprintf(buffer + strlen(function), " line: %d DeltaMem: %d (bytes) Time: %d:%02d:%02d", line, delta_mem, hours,mins,secs) ;

	if(mCounter < mCapacity)
	{
		mCounter++ ;
	}
	if(mCurIndex >= mCapacity)
	{
		mCurIndex = 0 ;		
	}
}


//static 
void LLMemTracker::preDraw(BOOL pause) 
{
	mMutexp->lock() ;

	mPaused = pause ;
	mDrawnIndex = mCurIndex - 1;
	mNumOfDrawn = 0 ;
}
	
//static 
void LLMemTracker::postDraw() 
{
	mMutexp->unlock() ;
}

//static 
const char* LLMemTracker::getNextLine() 
{
	if(mNumOfDrawn >= mCounter)
	{
		return NULL ;
	}
	mNumOfDrawn++;

	if(mDrawnIndex < 0)
	{
		mDrawnIndex = mCapacity - 1 ;
	}

	return mStringBuffer[mDrawnIndex--] ;
}

#endif //MEM_TRACK_MEM
//--------------------------------------------------------------------------------------------------

