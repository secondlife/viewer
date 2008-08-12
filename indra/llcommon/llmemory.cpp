/** 
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
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

#if defined(LL_WINDOWS)
# include <windows.h>
# include <psapi.h>
#elif defined(LL_DARWIN)
# include <sys/types.h>
# include <mach/task.h>
# include <mach/mach_init.h>
#elif defined(LL_LINUX)
# include <unistd.h>
#endif

#include "llmemory.h"
#include "llmemtype.h"

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


//----------------------------------------------------------------------------

//static
#if MEM_TRACK_TYPE
S32 LLMemType::sCurDepth = 0;
S32 LLMemType::sCurType = LLMemType::MTYPE_INIT;
S32 LLMemType::sType[LLMemType::MTYPE_MAX_DEPTH];
S32 LLMemType::sMemCount[LLMemType::MTYPE_NUM_TYPES] = { 0 };
S32 LLMemType::sMaxMemCount[LLMemType::MTYPE_NUM_TYPES] = { 0 };
S32 LLMemType::sNewCount[LLMemType::MTYPE_NUM_TYPES] = { 0 };
S32 LLMemType::sOverheadMem = 0;

const char* LLMemType::sTypeDesc[LLMemType::MTYPE_NUM_TYPES] =
{
	"INIT",
	"STARTUP",
	"MAIN",
	
	"IMAGEBASE",
	"IMAGERAW",
	"IMAGEFORMATTED",
	
	"APPFMTIMAGE",
	"APPRAWIMAGE",
	"APPAUXRAWIMAGE",
	
	"DRAWABLE",
	"OBJECT",
	"PIPELINE",
	"AVATAR",
	"PARTICLES",
	"REGIONS",
	"INVENTORY",
	"ANIMATION",
	"NETWORK",
	"PHYSICS",
	"INTERESTLIST",
	
	"SCRIPT",
	"SCRIPT_RUN",
	"SCRIPT_BYTECODE",
	
	"IO_PUMP",
	"IO_TCP",
	"IO_BUFFER",
	"IO_HTTP_SERVER"
	"IO_SD_SERVER",
	"IO_SD_CLIENT",
	"IO_URL_REQUEST",

	"TEMP1",
	"TEMP2",
	"TEMP3",
	"TEMP4",
	"TEMP5",
	"TEMP6",
	"TEMP7",
	"TEMP8",
	"TEMP9"
};

#endif
S32 LLMemType::sTotalMem = 0;
S32 LLMemType::sMaxTotalMem = 0;

//static
void LLMemType::printMem()
{
	S32 misc_mem = sTotalMem;
#if MEM_TRACK_TYPE
	for (S32 i=0; i<MTYPE_NUM_TYPES; i++)
	{
		if (sMemCount[i])
		{
			llinfos << llformat("MEM: % 20s %03d MB (%03d MB) in %06d News",sTypeDesc[i],sMemCount[i]>>20,sMaxMemCount[i]>>20, sNewCount[i]) << llendl;
		}
		misc_mem -= sMemCount[i];
	}
#endif
	llinfos << llformat("MEM: % 20s %03d MB","MISC",misc_mem>>20) << llendl;
	llinfos << llformat("MEM: % 20s %03d MB (Max=%d MB)","TOTAL",sTotalMem>>20,sMaxTotalMem>>20) << llendl;
}

#if MEM_TRACK_MEM

void* ll_allocate (size_t size)
{
	if (size == 0)
	{
		llwarns << "Null allocation" << llendl;
	}
	
	size = (size+3)&~3;
	S32 alloc_size = size + 4;
#if MEM_TRACK_TYPE
	alloc_size += 4;
#endif
	char* p = (char*)malloc(alloc_size);
	if (p == NULL)
	{
		LLMemory::freeReserve();
		llerrs << "Out of memory Error" << llendl;
	}
	LLMemType::sTotalMem += size;
	LLMemType::sMaxTotalMem = llmax(LLMemType::sTotalMem, LLMemType::sMaxTotalMem);
	LLMemType::sOverheadMem += 4;
	*(size_t*)p = size;
	p += 4;
#if MEM_TRACK_TYPE
	if (LLMemType::sCurType < 0 || LLMemType::sCurType >= LLMemType::MTYPE_NUM_TYPES)
	{
		llerrs << "Memory Type Error: new" << llendl;
	}
	LLMemType::sOverheadMem += 4;
	*(S32*)p = LLMemType::sCurType;
	p += 4;
	LLMemType::sMemCount[LLMemType::sCurType] += size;
	if (LLMemType::sMemCount[LLMemType::sCurType] > LLMemType::sMaxMemCount[LLMemType::sCurType])
	{
		LLMemType::sMaxMemCount[LLMemType::sCurType] = LLMemType::sMemCount[LLMemType::sCurType];
	}
	LLMemType::sNewCount[LLMemType::sCurType]++;
#endif
	return (void*)p;
}

void ll_release (void *pin)
{
	if (!pin)
	{
		return;
	}
	char* p = (char*)pin;
#if MEM_TRACK_TYPE
	p -= 4;
	S32 type = *(S32*)p;
	if (type < 0 || type >= LLMemType::MTYPE_NUM_TYPES)
	{
		llerrs << "Memory Type Error: delete" << llendl;
	}
#endif
	p -= 4;
	S32 size = *(size_t*)p;
	LLMemType::sOverheadMem -= 4;
#if MEM_TRACK_TYPE
	LLMemType::sMemCount[type] -= size;
	LLMemType::sOverheadMem -= 4;
	LLMemType::sNewCount[type]--;
#endif
	LLMemType::sTotalMem -= size;
	free(p);
}

#else

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

#endif

#if MEM_TRACK_MEM

void* operator new (size_t size)
{
	return ll_allocate(size);
}

void* operator new[] (size_t size)
{
	return ll_allocate(size);
}

void operator delete (void *p)
{
	ll_release(p);
}

void operator delete[] (void *p)
{
	ll_release(p);
}

#endif

//----------------------------------------------------------------------------

LLRefCount::LLRefCount() :
	mRef(0)
{
}

LLRefCount::~LLRefCount()
{ 
	if (mRef != 0)
	{
		llerrs << "deleting non-zero reference" << llendl;
	}
}
	
//----------------------------------------------------------------------------

#if defined(LL_WINDOWS)

U64 getCurrentRSS()
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

U64 getCurrentRSS()
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

U64 getCurrentRSS()
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

#else

U64 getCurrentRSS()
{
	return 0;
}

#endif
