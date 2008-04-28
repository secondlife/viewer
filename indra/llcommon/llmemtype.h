/** 
 * @file llmemtype.h
 * @brief Runtime memory usage debugging utilities.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#ifndef LL_MEMTYPE_H
#define LL_MEMTYPE_H

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class LLMemType;

extern void* ll_allocate (size_t size);
extern void ll_release (void *p);

#define MEM_TRACK_MEM 0
#define MEM_TRACK_TYPE (1 && MEM_TRACK_MEM)

#if MEM_TRACK_TYPE
#define MEM_DUMP_DATA 1
#define MEM_TYPE_NEW(T) \
static void* operator new(size_t s) { LLMemType mt(T); return ll_allocate(s); } \
static void  operator delete(void* p) { ll_release(p); }

#else
#define MEM_TYPE_NEW(T)
#endif // MEM_TRACK_TYPE


//----------------------------------------------------------------------------

class LLMemType
{
public:
	// Also update sTypeDesc in llmemory.cpp
	enum EMemType
	{
		MTYPE_INIT,
		MTYPE_STARTUP,
		MTYPE_MAIN,

		MTYPE_IMAGEBASE,
		MTYPE_IMAGERAW,
		MTYPE_IMAGEFORMATTED,
		
		MTYPE_APPFMTIMAGE,
		MTYPE_APPRAWIMAGE,
		MTYPE_APPAUXRAWIMAGE,
		
		MTYPE_DRAWABLE,
		MTYPE_OBJECT,
		MTYPE_VERTEX_DATA,
		MTYPE_SPACE_PARTITION,
		MTYPE_PIPELINE,
		MTYPE_AVATAR,
		MTYPE_PARTICLES,
		MTYPE_REGIONS,
		MTYPE_INVENTORY,
		MTYPE_ANIMATION,
		MTYPE_VOLUME,
		MTYPE_PRIMITIVE,

		MTYPE_NETWORK,
		MTYPE_PHYSICS,
		MTYPE_INTERESTLIST,
		
		MTYPE_SCRIPT,
		MTYPE_SCRIPT_RUN,
		MTYPE_SCRIPT_BYTECODE,
		
		MTYPE_IO_PUMP,
		MTYPE_IO_TCP,
		MTYPE_IO_BUFFER,
		MTYPE_IO_HTTP_SERVER,
		MTYPE_IO_SD_SERVER,
		MTYPE_IO_SD_CLIENT,
		MTYPE_IO_URL_REQUEST,

		MTYPE_TEMP1,
		MTYPE_TEMP2,
		MTYPE_TEMP3,
		MTYPE_TEMP4,
		MTYPE_TEMP5,
		MTYPE_TEMP6,
		MTYPE_TEMP7,
		MTYPE_TEMP8,
		MTYPE_TEMP9,

		MTYPE_OTHER, // Special, used by display code
		
		MTYPE_NUM_TYPES
	};
	enum { MTYPE_MAX_DEPTH = 64 };
	
public:
	LLMemType(EMemType type)
	{
#if MEM_TRACK_TYPE
		if (type < 0 || type >= MTYPE_NUM_TYPES)
			llerrs << "LLMemType error" << llendl;
		if (sCurDepth < 0 || sCurDepth >= MTYPE_MAX_DEPTH)
			llerrs << "LLMemType error" << llendl;
		sType[sCurDepth] = sCurType;
		sCurDepth++;
		sCurType = type;
#endif
	}
	~LLMemType()
	{
#if MEM_TRACK_TYPE
		sCurDepth--;
		sCurType = sType[sCurDepth];
#endif
	}

	static void reset();
	static void printMem();
	
public:
#if MEM_TRACK_TYPE
	static S32 sCurDepth;
	static S32 sCurType;
	static S32 sType[MTYPE_MAX_DEPTH];
	static S32 sMemCount[MTYPE_NUM_TYPES];
	static S32 sMaxMemCount[MTYPE_NUM_TYPES];
	static S32 sNewCount[MTYPE_NUM_TYPES];
	static S32 sOverheadMem;
	static const char* sTypeDesc[MTYPE_NUM_TYPES];
#endif
	static S32 sTotalMem;
	static S32 sMaxTotalMem;
};

//----------------------------------------------------------------------------

#endif

