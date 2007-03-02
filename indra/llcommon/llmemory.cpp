/** 
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llmemory.h"
#include "llmemtype.h"

// not defining nullfunc will currently crash when trying to use a LLHandle
template< typename _Ty >
  const typename LLHandle< _Ty >::NullFunc
    LLHandle< _Ty >::sNullFunc = LLHandle< _Ty >::defaultNullFunc;

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
			llinfos << llformat("MEM: % 20s %03d MB (%03d MB)",sTypeDesc[i],sMemCount[i]>>20,sMaxMemCount[i]>>20) << llendl;
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

