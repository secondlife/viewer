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

	static void* tryToAlloc(void* address, U32 size);
	static void initMaxHeapSizeGB(F32 max_heap_size_gb, BOOL prevent_heap_failure);
	static void updateMemoryInfo() ;
	static void logMemoryInfo(BOOL update = FALSE);
	static S32  isMemoryPoolLow();

	static U32 getAvailableMemKB() ;
	static U32 getMaxMemKB() ;
	static U32 getAllocatedMemKB() ;
private:
	static char* reserveMem;
	static U32 sAvailPhysicalMemInKB ;
	static U32 sMaxPhysicalMemInKB ;
	static U32 sAllocatedMemInKB;
	static U32 sAllocatedPageSizeInKB ;

	static U32 sMaxHeapSizeInKB;
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
public:
	class LL_COMMON_API LLMemoryBlock //each block is devided into slots uniformly
	{
	public: 
		LLMemoryBlock() ;
		~LLMemoryBlock() ;

		void init(char* buffer, U32 buffer_size, U32 slot_size) ;
		void setBuffer(char* buffer, U32 buffer_size) ;

		char* allocate() ;
		void  free(void* addr) ;

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
		U8    mDummySize ; //size of extra U32 reserved for mUsageBits.

	public:
		LLMemoryBlock* mPrev ;
		LLMemoryBlock* mNext ;
		LLMemoryBlock* mSelf ;

		struct CompareAddress
		{
			bool operator()(const LLMemoryBlock* const& lhs, const LLMemoryBlock* const& rhs)
			{
				return (U32)lhs->getBuffer() < (U32)rhs->getBuffer();
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
		void  free(void* addr) ;

		const char* getBuffer() const {return mBuffer;}
		U32 getBufferSize() const {return mBufferSize;}
		U32 getAllocatedSize() const {return mAlloatedSize;}

		bool containsAddress(const char* addr) const;

		static U32 getMaxOverhead(U32 data_buffer_size, U32 min_page_size) ;
	
		void dump() ;

	private:
		U32 getPageIndex(U32 addr) ;
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

		LLMemoryChunk* mHashNext ;
	} ;

public:
	LLPrivateMemoryPool(U32 max_size, bool threaded) ;
	~LLPrivateMemoryPool() ;

	char *allocate(U32 size) ;
	void  free(void* addr) ;
	
	void  dump() ;
	U32   getTotalAllocatedSize() ;

private:
	void lock() ;
	void unlock() ;	
	S32 getChunkIndex(U32 size) ;
	LLMemoryChunk*  addChunk(S32 chunk_index) ;
	void checkSize(U32 asked_size) ;
	void removeChunk(LLMemoryChunk* chunk) ;
	U16  findHashKey(const char* addr);
	void addToHashTable(LLMemoryChunk* chunk) ;
	void removeFromHashTable(LLMemoryChunk* chunk) ;
	void rehash() ;
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

private:
	LLMutex* mMutexp ;
	U32  mMaxPoolSize;
	U32  mReservedPoolSize ;	

	LLMemoryChunk* mChunkList[SUPER_ALLOCATION] ; //all memory chunks reserved by this pool, sorted by address
	std::vector<LLMemoryChunk*> mChunkHashList ;
	U16 mNumOfChunks ;
	U16 mHashFactor ;
};

//
//the below singleton is used to test the private memory pool.
//
class LL_COMMON_API LLPrivateMemoryPoolTester
{
private:
	LLPrivateMemoryPoolTester() ;
	~LLPrivateMemoryPoolTester() ;

public:
	static LLPrivateMemoryPoolTester* getInstance() ;
	static void destroy() ;

	void run(bool threaded) ;	

private:
	void correctnessTest() ;
	void performanceTest() ;
	void fragmentationtest() ;

	void test(U32 min_size, U32 max_size, U32 stride, U32 times, bool random_deletion, bool output_statistics) ;
	void testAndTime(U32 size, U32 times) ;

public:
	void* operator new(size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete(void* addr)
	{
		sPool->free(addr) ;
	}
	void* operator new[](size_t size)
	{
		return (void*)sPool->allocate(size) ;
	}
    void  operator delete[](void* addr)
	{
		sPool->free(addr) ;
	}

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
// LLRefCount moved to llrefcount.h

// LLPointer moved to llpointer.h

// LLSafeHandle moved to llsafehandle.h

// LLSingleton moved to llsingleton.h

#endif
