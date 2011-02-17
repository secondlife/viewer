/** 
 * @file llmemory.h
 * @brief Memory allocation/deallocation header-stuff goes here.
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
#ifndef LLMEMORY_H
#define LLMEMORY_H

#include "llmemtype.h"

extern S32 gTotalDAlloc;
extern S32 gTotalDAUse;
extern S32 gDACount;

extern void* ll_allocate (size_t size);
extern void ll_release (void *p);

class LL_COMMON_API LLMemory
{
public:
	static void initClass();
	static void cleanupClass();
	static void freeReserve();
	// Return the resident set size of the current process, in bytes.
	// Return value is zero if not known.
	static U64 getCurrentRSS();
	static U32 getWorkingSetSize();
private:
	static char* reserveMem;
};

//----------------------------------------------------------------------------
#if MEM_TRACK_MEM
class LLMutex ;
class LL_COMMON_API LLMemTracker
{
private:
	LLMemTracker() ;
	~LLMemTracker() ;

public:
	static void release() ;
	static LLMemTracker* getInstance() ;

	void track(const char* function, const int line) ;
	void preDraw(BOOL pause) ;
	void postDraw() ;
	const char* getNextLine() ;

private:
	static LLMemTracker* sInstance ;
	
	char**     mStringBuffer ;
	S32        mCapacity ;
	U32        mLastAllocatedMem ;
	S32        mCurIndex ;
	S32        mCounter;
	S32        mDrawnIndex;
	S32        mNumOfDrawn;
	BOOL       mPaused;
	LLMutex*   mMutexp ;
};

#define MEM_TRACK_RELEASE LLMemTracker::release() ;
#define MEM_TRACK         LLMemTracker::getInstance()->track(__FUNCTION__, __LINE__) ;

#else // MEM_TRACK_MEM

#define MEM_TRACK_RELEASE
#define MEM_TRACK

#endif // MEM_TRACK_MEM

//----------------------------------------------------------------------------

// LLRefCount moved to llrefcount.h

// LLPointer moved to llpointer.h

// LLSafeHandle moved to llsafehandle.h

// LLSingleton moved to llsingleton.h

#endif
