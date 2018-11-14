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

#include "linden_common.h"
#include "llunits.h"
#include "stdtypes.h"
#if !LL_WINDOWS
#include <stdint.h>
#endif

class LLMutex ;

#if LL_WINDOWS && LL_DEBUG
#define LL_CHECK_MEMORY llassert(_CrtCheckMemory());
#else
#define LL_CHECK_MEMORY 
#endif


#if LL_WINDOWS
#define LL_ALIGN_OF __alignof
#else
#define LL_ALIGN_OF __align_of__
#endif

#if LL_WINDOWS
#define LL_DEFAULT_HEAP_ALIGN 8
#elif LL_DARWIN
#define LL_DEFAULT_HEAP_ALIGN 16
#elif LL_LINUX
#define LL_DEFAULT_HEAP_ALIGN 8
#endif


LL_COMMON_API void ll_assert_aligned_func(uintptr_t ptr,U32 alignment);

#ifdef SHOW_ASSERT
// This is incredibly expensive - in profiling Windows RWD builds, 30%
// of CPU time was in aligment checks.
//#define ASSERT_ALIGNMENT
#endif

#ifdef ASSERT_ALIGNMENT
#define ll_assert_aligned(ptr,alignment) ll_assert_aligned_func(uintptr_t(ptr),((U32)alignment))
#else
#define ll_assert_aligned(ptr,alignment)
#endif

#include <xmmintrin.h>

template <typename T> T* LL_NEXT_ALIGNED_ADDRESS(T* address) 
{ 
	return reinterpret_cast<T*>(
		(uintptr_t(address) + 0xF) & ~0xF);
}

template <typename T> T* LL_NEXT_ALIGNED_ADDRESS_64(T* address) 
{ 
	return reinterpret_cast<T*>(
		(uintptr_t(address) + 0x3F) & ~0x3F);
}

#if LL_LINUX || LL_DARWIN

#define			LL_ALIGN_PREFIX(x)
#define			LL_ALIGN_POSTFIX(x)		__attribute__((aligned(x)))

#elif LL_WINDOWS

#define			LL_ALIGN_PREFIX(x)		__declspec(align(x))
#define			LL_ALIGN_POSTFIX(x)

#else
#error "LL_ALIGN_PREFIX and LL_ALIGN_POSTFIX undefined"
#endif

#define LL_ALIGN_16(var) LL_ALIGN_PREFIX(16) var LL_ALIGN_POSTFIX(16)

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
	// for enable buffer overrun detection predefine LL_DEBUG_BUFFER_OVERRUN in current library
	// change preprocessor code to: #if 1 && defined(LL_WINDOWS)

#if 0 && defined(LL_WINDOWS)
	void* ll_aligned_malloc_fallback( size_t size, int align );
	void ll_aligned_free_fallback( void* ptr );
//------------------------------------------------------------------------------------------------
#else
	inline void* ll_aligned_malloc_fallback( size_t size, int align )
	{
	#if defined(LL_WINDOWS)
		return _aligned_malloc(size, align);
	#else
        char* aligned = NULL;
		void* mem = malloc( size + (align - 1) + sizeof(void*) );
        if (mem)
        {
            aligned = ((char*)mem) + sizeof(void*);
            aligned += align - ((uintptr_t)aligned & (align - 1));

            ((void**)aligned)[-1] = mem;
        }
		return aligned;
	#endif
	}

	inline void ll_aligned_free_fallback( void* ptr )
	{
	#if defined(LL_WINDOWS)
		_aligned_free(ptr);
	#else
		if (ptr)
		{
			free( ((void**)ptr)[-1] );
		}
	#endif
	}
#endif
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------

inline void* ll_aligned_malloc_16(size_t size) // returned hunk MUST be freed with ll_aligned_free_16().
{
#if defined(LL_WINDOWS)
	return _aligned_malloc(size, 16);
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
	_aligned_free(p);
#elif defined(LL_DARWIN)
	return free(p);
#else
	free(p); // posix_memalign() is compatible with heap deallocator
#endif
}

inline void* ll_aligned_realloc_16(void* ptr, size_t size, size_t old_size) // returned hunk MUST be freed with ll_aligned_free_16().
{
#if defined(LL_WINDOWS)
	return _aligned_realloc(ptr, size, 16);
#elif defined(LL_DARWIN)
	return realloc(ptr,size); // default osx malloc is 16 byte aligned.
#else
	//FIXME: memcpy is SLOW
	void* ret = ll_aligned_malloc_16(size);
	if (ptr)
	{
		if (ret)
		{
			// Only copy the size of the smallest memory block to avoid memory corruption.
			memcpy(ret, ptr, llmin(old_size, size));
		}
		ll_aligned_free_16(ptr);
	}
	return ret;
#endif
}

inline void* ll_aligned_malloc_32(size_t size) // returned hunk MUST be freed with ll_aligned_free_32().
{
#if defined(LL_WINDOWS)
	return _aligned_malloc(size, 32);
#elif defined(LL_DARWIN)
	return ll_aligned_malloc_fallback( size, 32 );
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
	_aligned_free(p);
#elif defined(LL_DARWIN)
	ll_aligned_free_fallback( p );
#else
	free(p); // posix_memalign() is compatible with heap deallocator
#endif
}

// general purpose dispatch functions that are forced inline so they can compile down to a single call
template<size_t ALIGNMENT>
LL_FORCE_INLINE void* ll_aligned_malloc(size_t size)
{
	if (LL_DEFAULT_HEAP_ALIGN % ALIGNMENT == 0)
	{
		return malloc(size);
	}
	else if (ALIGNMENT == 16)
	{
		return ll_aligned_malloc_16(size);
	}
	else if (ALIGNMENT == 32)
	{
		return ll_aligned_malloc_32(size);
	}
	else
	{
		return ll_aligned_malloc_fallback(size, ALIGNMENT);
	}
}

template<size_t ALIGNMENT>
LL_FORCE_INLINE void ll_aligned_free(void* ptr)
{
	if (ALIGNMENT == LL_DEFAULT_HEAP_ALIGN)
	{
		free(ptr);
	}
	else if (ALIGNMENT == 16)
	{
		ll_aligned_free_16(ptr);
	}
	else if (ALIGNMENT == 32)
	{
		return ll_aligned_free_32(ptr);
	}
	else
	{
		return ll_aligned_free_fallback(ptr);
	}
}

// Copy words 16-byte blocks from src to dst. Source and destination MUST NOT OVERLAP. 
// Source and dest must be 16-byte aligned and size must be multiple of 16.
//
inline void ll_memcpy_nonaliased_aligned_16(char* __restrict dst, const char* __restrict src, size_t bytes)
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(bytes > 0);
	assert((bytes % sizeof(F32))== 0); 
	ll_assert_aligned(src,16);
	ll_assert_aligned(dst,16);

	assert((src < dst) ? ((src + bytes) <= dst) : ((dst + bytes) <= src));
	assert(bytes%16==0);

	char* end = dst + bytes;

	if (bytes > 64)
	{

		// Find start of 64b aligned area within block
		//
		void* begin_64 = LL_NEXT_ALIGNED_ADDRESS_64(dst);
		
		//at least 64 bytes before the end of the destination, switch to 16 byte copies
		void* end_64 = end-64;
	
		// Prefetch the head of the 64b area now
		//
		_mm_prefetch((char*)begin_64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 128, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 192, _MM_HINT_NTA);
	
		// Copy 16b chunks until we're 64b aligned
		//
		while (dst < begin_64)
		{

			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			dst += 16;
			src += 16;
		}
	
		// Copy 64b chunks up to your tail
		//
		// might be good to shmoo the 512b prefetch offset
		// (characterize performance for various values)
		//
		while (dst < end_64)
		{
			_mm_prefetch((char*)src + 512, _MM_HINT_NTA);
			_mm_prefetch((char*)dst + 512, _MM_HINT_NTA);
			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			_mm_store_ps((F32*)(dst + 16), _mm_load_ps((F32*)(src + 16)));
			_mm_store_ps((F32*)(dst + 32), _mm_load_ps((F32*)(src + 32)));
			_mm_store_ps((F32*)(dst + 48), _mm_load_ps((F32*)(src + 48)));
			dst += 64;
			src += 64;
		}
	}

	// Copy remainder 16b tail chunks (or ALL 16b chunks for sub-64b copies)
	//
	while (dst < end)
	{
		_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
		dst += 16;
		src += 16;
	}
}

#ifndef __DEBUG_PRIVATE_MEM__
#define __DEBUG_PRIVATE_MEM__  0
#endif

class LL_COMMON_API LLMemory
{
public:
	// Return the resident set size of the current process, in bytes.
	// Return value is zero if not known.
	static U64 getCurrentRSS();
	static void* tryToAlloc(void* address, U32 size);
	static void initMaxHeapSizeGB(F32Gigabytes max_heap_size, BOOL prevent_heap_failure);
	static void updateMemoryInfo() ;
	static void logMemoryInfo(BOOL update = FALSE);
	static bool isMemoryPoolLow();

	static U32Kilobytes getAvailableMemKB() ;
	static U32Kilobytes getMaxMemKB() ;
	static U32Kilobytes getAllocatedMemKB() ;
private:
	static U32Kilobytes sAvailPhysicalMemInKB ;
	static U32Kilobytes sMaxPhysicalMemInKB ;
	static U32Kilobytes sAllocatedMemInKB;
	static U32Kilobytes sAllocatedPageSizeInKB ;

	static U32Kilobytes sMaxHeapSizeInKB;
	static BOOL sEnableMemoryFailurePrevention;
};

// LLRefCount moved to llrefcount.h

// LLPointer moved to llpointer.h

// LLSafeHandle moved to llsafehandle.h

// LLSingleton moved to llsingleton.h




#endif
