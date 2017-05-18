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

//
//class LLPrivateMemoryPool defines a private memory pool for an application to use, so the application does not
//need to access the heap directly fro each memory allocation. Throught this, the allocation speed is faster, 
//and reduces virtaul address space gragmentation problem.
//Note: this class is thread-safe by passing true to the constructor function. However, you do not need to do this unless
//you are sure the memory allocation and de-allocation will happen in different threads. To make the pool thread safe
//increases allocation and deallocation cost.
//
class LL_COMMON_API LLPrivateMemoryPool
{
	friend class LLPrivateMemoryPoolManager ;

public:
	class LL_COMMON_API LLMemoryBlock //each block is devided into slots uniformly
	{
	public: 
		LLMemoryBlock() ;
		~LLMemoryBlock() ;

		void init(char* buffer, U32 buffer_size, U32 slot_size) ;
		void setBuffer(char* buffer, U32 buffer_size) ;

		char* allocate() ;
		void  freeMem(void* addr) ;

		bool empty() {return !mAllocatedSlots;}
		bool isFull() {return mAllocatedSlots == mTotalSlots;}
		bool isFree() {return !mTotalSlots;}

		U32  getSlotSize()const {return mSlotSize;}
		U32  getTotalSlots()const {return mTotalSlots;}
		U32  getBufferSize()const {return mBufferSize;}
		char* getBuffer() const {return mBuffer;}

		//debug use
		void resetBitMap() ;
	private:
		char* mBuffer;
		U32   mSlotSize ; //when the block is not initialized, it is the buffer size.
		U32   mBufferSize ;
		U32   mUsageBits ;
		U8    mTotalSlots ;
		U8    mAllocatedSlots ;
		U8    mDummySize ; //size of extra bytes reserved for mUsageBits.

	public:
		LLMemoryBlock* mPrev ;
		LLMemoryBlock* mNext ;
		LLMemoryBlock* mSelf ;

		struct CompareAddress
		{
			bool operator()(const LLMemoryBlock* const& lhs, const LLMemoryBlock* const& rhs)
			{
				return (uintptr_t)lhs->getBuffer() < (uintptr_t)rhs->getBuffer();
			}
		};
	};

	class LL_COMMON_API LLMemoryChunk //is divided into memory blocks.
	{
	public:
		LLMemoryChunk() ;
		~LLMemoryChunk() ;

		void init(char* buffer, U32 buffer_size, U32 min_slot_size, U32 max_slot_size, U32 min_block_size, U32 max_block_size) ;
		void setBuffer(char* buffer, U32 buffer_size) ;

		bool empty() ;
		
		char* allocate(U32 size) ;
		void  freeMem(void* addr) ;

		char* getBuffer() const {return mBuffer;}
		U32 getBufferSize() const {return mBufferSize;}
		U32 getAllocatedSize() const {return mAlloatedSize;}

		bool containsAddress(const char* addr) const;

		static U32 getMaxOverhead(U32 data_buffer_size, U32 min_slot_size, 
													   U32 max_slot_size, U32 min_block_size, U32 max_block_size) ;
	
		void dump() ;

	private:
		U32 getPageIndex(uintptr_t addr) ;
		U32 getBlockLevel(U32 size) ;
		U16 getPageLevel(U32 size) ;
		LLMemoryBlock* addBlock(U32 blk_idx) ;
		void popAvailBlockList(U32 blk_idx) ;
		void addToFreeSpace(LLMemoryBlock* blk) ;
		void removeFromFreeSpace(LLMemoryBlock* blk) ;
		void removeBlock(LLMemoryBlock* blk) ;
		void addToAvailBlockList(LLMemoryBlock* blk) ;
		U32  calcBlockSize(U32 slot_size);
		LLMemoryBlock* createNewBlock(LLMemoryBlock* blk, U32 buffer_size, U32 slot_size, U32 blk_idx) ;

	private:
		LLMemoryBlock** mAvailBlockList ;//256 by mMinSlotSize
		LLMemoryBlock** mFreeSpaceList;
		LLMemoryBlock*  mBlocks ; //index of blocks by address.
		
		char* mBuffer ;
		U32   mBufferSize ;
		char* mDataBuffer ;
		char* mMetaBuffer ;
		U32   mMinBlockSize ;
		U32   mMinSlotSize ;
		U32   mMaxSlotSize ;
		U32   mAlloatedSize ;
		U16   mBlockLevels;
		U16   mPartitionLevels;

	public:
		//form a linked list
		LLMemoryChunk* mNext ;
		LLMemoryChunk* mPrev ;
	} ;

private:
	LLPrivateMemoryPool(S32 type, U32 max_pool_size) ;
	~LLPrivateMemoryPool() ;

	char *allocate(U32 size) ;
	void  freeMem(void* addr) ;
	
	void  dump() ;
	U32   getTotalAllocatedSize() ;
	U32   getTotalReservedSize() {return mReservedPoolSize;}
	S32   getType() const {return mType; }
	bool  isEmpty() const {return !mNumOfChunks; }

private:
	void lock() ;
	void unlock() ;	
	S32 getChunkIndex(U32 size) ;
	LLMemoryChunk*  addChunk(S32 chunk_index) ;
	bool checkSize(U32 asked_size) ;
	void removeChunk(LLMemoryChunk* chunk) ;
	U16  findHashKey(const char* addr);
	void addToHashTable(LLMemoryChunk* chunk) ;
	void removeFromHashTable(LLMemoryChunk* chunk) ;
	void rehash() ;
	bool fillHashTable(U16 start, U16 end, LLMemoryChunk* chunk) ;
	LLMemoryChunk* findChunk(const char* addr) ;

	void destroyPool() ;

public:
	enum
	{
		SMALL_ALLOCATION = 0, //from 8 bytes to 2KB(exclusive), page size 2KB, max chunk size is 4MB.
		MEDIUM_ALLOCATION,    //from 2KB to 512KB(exclusive), page size 32KB, max chunk size 4MB
		LARGE_ALLOCATION,     //from 512KB to 4MB(inclusive), page size 64KB, max chunk size 16MB
		SUPER_ALLOCATION      //allocation larger than 4MB.
	};

	enum
	{
		STATIC = 0 ,       //static pool(each alllocation stays for a long time) without threading support
		VOLATILE,          //Volatile pool(each allocation stays for a very short time) without threading support
		STATIC_THREADED,   //static pool with threading support
		VOLATILE_THREADED, //volatile pool with threading support
		MAX_TYPES
	}; //pool types

private:
	LLMutex* mMutexp ;
	U32  mMaxPoolSize;
	U32  mReservedPoolSize ;	

	LLMemoryChunk* mChunkList[SUPER_ALLOCATION] ; //all memory chunks reserved by this pool, sorted by address	
	U16 mNumOfChunks ;
	U16 mHashFactor ;

	S32 mType ;

	class LLChunkHashElement
	{
	public:
		LLChunkHashElement() {mFirst = NULL ; mSecond = NULL ;}

		bool add(LLMemoryChunk* chunk) ;
		void remove(LLMemoryChunk* chunk) ;
		LLMemoryChunk* findChunk(const char* addr) ;

		bool empty() {return !mFirst && !mSecond; }
		bool full()  {return mFirst && mSecond; }
		bool hasElement(LLMemoryChunk* chunk) {return mFirst == chunk || mSecond == chunk;}

	private:
		LLMemoryChunk* mFirst ;
		LLMemoryChunk* mSecond ;
	};
	std::vector<LLChunkHashElement> mChunkHashList ;
};

class LL_COMMON_API LLPrivateMemoryPoolManager
{
private:
	LLPrivateMemoryPoolManager(BOOL enabled, U32 max_pool_size) ;
	~LLPrivateMemoryPoolManager() ;

public:	
	static LLPrivateMemoryPoolManager* getInstance() ;
	static void initClass(BOOL enabled, U32 pool_size) ;
	static void destroyClass() ;

	LLPrivateMemoryPool* newPool(S32 type) ;
	void deletePool(LLPrivateMemoryPool* pool) ;

private:	
	std::vector<LLPrivateMemoryPool*> mPoolList ;	
	U32  mMaxPrivatePoolSize;

	static LLPrivateMemoryPoolManager* sInstance ;
	static BOOL sPrivatePoolEnabled;
	static std::vector<LLPrivateMemoryPool*> sDanglingPoolList ;
public:
	//debug and statistics info.
	void updateStatistics() ;

	U32 mTotalReservedSize ;
	U32 mTotalAllocatedSize ;

public:
#if __DEBUG_PRIVATE_MEM__
	static char* allocate(LLPrivateMemoryPool* poolp, U32 size, const char* function, const int line) ;	
	
	typedef std::map<char*, std::string> mem_allocation_info_t ;
	static mem_allocation_info_t sMemAllocationTracker;
#else
	static char* allocate(LLPrivateMemoryPool* poolp, U32 size) ;	
#endif
	static void  freeMem(LLPrivateMemoryPool* poolp, void* addr) ;
};

//-------------------------------------------------------------------------------------
#if __DEBUG_PRIVATE_MEM__
#define ALLOCATE_MEM(poolp, size) LLPrivateMemoryPoolManager::allocate((poolp), (size), __FUNCTION__, __LINE__)
#else
#define ALLOCATE_MEM(poolp, size) LLPrivateMemoryPoolManager::allocate((poolp), (size))
#endif
#define FREE_MEM(poolp, addr) LLPrivateMemoryPoolManager::freeMem((poolp), (addr))
//-------------------------------------------------------------------------------------

//
//the below singleton is used to test the private memory pool.
//
#if 0
class LL_COMMON_API LLPrivateMemoryPoolTester
{
private:
	LLPrivateMemoryPoolTester() ;
	~LLPrivateMemoryPoolTester() ;

public:
	static LLPrivateMemoryPoolTester* getInstance() ;
	static void destroy() ;

	void run(S32 type) ;	

private:
	void correctnessTest() ;
	void performanceTest() ;
	void fragmentationtest() ;

	void test(U32 min_size, U32 max_size, U32 stride, U32 times, bool random_deletion, bool output_statistics) ;
	void testAndTime(U32 size, U32 times) ;

#if 0
public:
	void* operator new(size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete(void* addr)
	{
		sPool->freeMem(addr) ;
	}
	void* operator new[](size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete[](void* addr)
	{
		sPool->freeMem(addr) ;
	}
#endif

private:
	static LLPrivateMemoryPoolTester* sInstance;
	static LLPrivateMemoryPool* sPool ;
	static LLPrivateMemoryPool* sThreadedPool ;
};
#if 0
//static
void* LLPrivateMemoryPoolTester::operator new(size_t size)
{
	return (void*)sPool->allocate(size) ;
}

//static
void  LLPrivateMemoryPoolTester::operator delete(void* addr)
{
	sPool->free(addr) ;
}

//static
void* LLPrivateMemoryPoolTester::operator new[](size_t size)
{
	return (void*)sPool->allocate(size) ;
}

//static
void  LLPrivateMemoryPoolTester::operator delete[](void* addr)
{
	sPool->free(addr) ;
}
#endif
#endif
// LLRefCount moved to llrefcount.h

// LLPointer moved to llpointer.h

// LLSafeHandle moved to llsafehandle.h

// LLSingleton moved to llsingleton.h




#endif
