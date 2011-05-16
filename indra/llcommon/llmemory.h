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

#if LL_DEBUG
inline void* ll_aligned_malloc( size_t size, int align )
{
	void* mem = malloc( size + (align - 1) + sizeof(void*) );
	char* aligned = ((char*)mem) + sizeof(void*);
	aligned += align - ((uintptr_t)aligned & (align - 1));

	((void**)aligned)[-1] = mem;
	return aligned;
}

inline void ll_aligned_free( void* ptr )
{
	free( ((void**)ptr)[-1] );
}

inline void* ll_aligned_malloc_16(size_t size) // returned hunk MUST be freed with ll_aligned_free_16().
{
#if defined(LL_WINDOWS)
	return _mm_malloc(size, 16);
#elif defined(LL_DARWIN)
	return malloc(size); // default osx malloc is 16 byte aligned.
#else
	void *rtn;
	if (LL_LIKELY(0 == posix_memalign(&rtn, 16, size)))
		return rtn;
	else // bad alignment requested, or out of memory
		return NULL;
#endif
}

inline void ll_aligned_free_16(void *p)
{
#if defined(LL_WINDOWS)
	_mm_free(p);
#elif defined(LL_DARWIN)
	return free(p);
#else
	free(p); // posix_memalign() is compatible with heap deallocator
#endif
}

inline void* ll_aligned_malloc_32(size_t size) // returned hunk MUST be freed with ll_aligned_free_32().
{
#if defined(LL_WINDOWS)
	return _mm_malloc(size, 32);
#elif defined(LL_DARWIN)
	return ll_aligned_malloc( size, 32 );
#else
	void *rtn;
	if (LL_LIKELY(0 == posix_memalign(&rtn, 32, size)))
		return rtn;
	else // bad alignment requested, or out of memory
		return NULL;
#endif
}

inline void ll_aligned_free_32(void *p)
{
#if defined(LL_WINDOWS)
	_mm_free(p);
#elif defined(LL_DARWIN)
	ll_aligned_free( p );
#else
	free(p); // posix_memalign() is compatible with heap deallocator
#endif
}
#else // LL_DEBUG
// ll_aligned_foo are noops now that we use tcmalloc everywhere (tcmalloc aligns automatically at appropriate intervals)
#define ll_aligned_malloc( size, align ) malloc(size)
#define ll_aligned_free( ptr ) free(ptr)
#define ll_aligned_malloc_16 malloc
#define ll_aligned_free_16 free
#define ll_aligned_malloc_32 malloc
#define ll_aligned_free_32 free
#endif // LL_DEBUG

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
