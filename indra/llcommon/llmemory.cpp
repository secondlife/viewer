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

#include "llthread.h"
#if defined(LL_WINDOWS)
//# include <windows.h>
# include <psapi.h>
#elif defined(LL_DARWIN)
# include <sys/types.h>
# include <mach/task.h>
# include <mach/mach_init.h>
#elif LL_LINUX || LL_SOLARIS
# include <unistd.h>
#endif

#include "llmemory.h"
#include "llsys.h"


//----------------------------------------------------------------------------

//static
char* LLMemory::reserveMem = 0;
U32 LLMemory::sAvailPhysicalMemInKB = U32_MAX ;
U32 LLMemory::sMaxPhysicalMemInKB = 0;
U32 LLMemory::sAllocatedMemInKB = 0;
U32 LLMemory::sAllocatedPageSizeInKB = 0 ;
U32 LLMemory::sMaxHeapSizeInKB = U32_MAX ;
BOOL LLMemory::sEnableMemoryFailurePrevention = FALSE;

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

//static 
void LLMemory::initMaxHeapSizeGB(F32 max_heap_size_gb, BOOL prevent_heap_failure)
{
	sMaxHeapSizeInKB = (U32)(max_heap_size_gb * 1024 * 1024) ;
	sEnableMemoryFailurePrevention = prevent_heap_failure ;
}

//static 
void LLMemory::updateMemoryInfo() 
{
#if LL_WINDOWS	
	HANDLE self = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS counters;
	
	if (!GetProcessMemoryInfo(self, &counters, sizeof(counters)))
	{
		llwarns << "GetProcessMemoryInfo failed" << llendl;
		return ;
	}

	sAllocatedMemInKB = (U32)(counters.WorkingSetSize / 1024) ;
	sAllocatedPageSizeInKB = (U32)(counters.PagefileUsage / 1024) ;
	sMaxPhysicalMemInKB = llmin(LLMemoryInfo::getAvailableMemoryKB() + sAllocatedMemInKB, sMaxHeapSizeInKB);

	if(sMaxPhysicalMemInKB > sAllocatedMemInKB)
	{
		sAvailPhysicalMemInKB = sMaxPhysicalMemInKB - sAllocatedMemInKB ;
	}
	else
	{
		sAvailPhysicalMemInKB = 0 ;
	}
#else
	//not valid for other systems for now.
	sAllocatedMemInKB = (U32)(LLMemory::getCurrentRSS() / 1024) ;
	sMaxPhysicalMemInKB = U32_MAX ;
	sAvailPhysicalMemInKB = U32_MAX ;
#endif

	return ;
}

//
//this function is to test if there is enough space with the size in the virtual address space.
//it does not do any real allocation
//if success, it returns the address where the memory chunk can fit in;
//otherwise it returns NULL.
//
//static 
void* LLMemory::tryToAlloc(void* address, U32 size)
{
#if LL_WINDOWS
	address = VirtualAlloc(address, size, MEM_RESERVE | MEM_TOP_DOWN, PAGE_NOACCESS) ;
	if(address)
	{
		if(!VirtualFree(address, 0, MEM_RELEASE))
		{
			llerrs << "error happens when free some memory reservation." << llendl ;
		}
	}
#else
#endif

	return address ;
}

//static 
void LLMemory::logMemoryInfo(BOOL update)
{
	if(update)
	{
		updateMemoryInfo() ;
	}

	llinfos << "Current allocated physical memory(KB): " << sAllocatedMemInKB << llendl ;
	llinfos << "Current allocated page size (KB): " << sAllocatedPageSizeInKB << llendl ;
	llinfos << "Current availabe physical memory(KB): " << sAvailPhysicalMemInKB << llendl ;
	llinfos << "Current max usable memory(KB): " << sMaxPhysicalMemInKB << llendl ;
}

//return 0: everything is normal;
//return 1: the memory pool is low, but not in danger;
//return -1: the memory pool is in danger, is about to crash.
//static 
S32 LLMemory::isMemoryPoolLow()
{
	static const U32 LOW_MEMEOY_POOL_THRESHOLD_KB = 64 * 1024 ; //64 MB for emergency use

	if(!sEnableMemoryFailurePrevention)
	{
		return 0 ; //no memory failure prevention.
	}

	if(sAvailPhysicalMemInKB < (LOW_MEMEOY_POOL_THRESHOLD_KB >> 2)) //out of physical memory
	{
		return -1 ;
	}

	if(sAllocatedPageSizeInKB + (LOW_MEMEOY_POOL_THRESHOLD_KB >> 2) > sMaxHeapSizeInKB) //out of virtual address space.
	{
		return -1 ;
	}

	return (S32)(sAvailPhysicalMemInKB < LOW_MEMEOY_POOL_THRESHOLD_KB || 
		sAllocatedPageSizeInKB + LOW_MEMEOY_POOL_THRESHOLD_KB > sMaxHeapSizeInKB) ;
}

//static 
U32 LLMemory::getAvailableMemKB() 
{
	return sAvailPhysicalMemInKB ;
}

//static 
U32 LLMemory::getMaxMemKB() 
{
	return sMaxPhysicalMemInKB ;
}

//static 
U32 LLMemory::getAllocatedMemKB() 
{
	return sAllocatedMemInKB ;
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

//-------------------------------------------------------------
//class LLPrivateMemoryPool::LLMemoryBlock
//-------------------------------------------------------------
//
//each memory block could fit for two page sizes: 0.75 * mSlotSize, which starts from the beginning of the memory chunk and grow towards the end of the
//the block; another is mSlotSize, which starts from the end of the block and grows towards the beginning of the block.
//
LLPrivateMemoryPool::LLMemoryBlock::LLMemoryBlock()
{
	//empty
}
		
LLPrivateMemoryPool::LLMemoryBlock::~LLMemoryBlock() 
{
	//empty
}

void LLPrivateMemoryPool::LLMemoryBlock::init(char* buffer, U32 buffer_size, U32 slot_size)
{
	mBuffer = buffer ;
	mBufferSize = buffer_size ;
	mSlotSize = slot_size ;
	mTotalSlots = buffer_size / mSlotSize ;	
	llassert_always(mTotalSlots < 256) ; //max number is 256
	mAllocatedSlots = 0 ;

	//mark free bits
	S32 usage_bit_len = (mTotalSlots + 31) / 32 ;
	mDummySize = usage_bit_len - 1 ;
	if(mDummySize > 0) //extra space to store mUsageBits
	{
		mTotalSlots -= (mDummySize * sizeof(mUsageBits) + mSlotSize - 1) / mSlotSize ;
		usage_bit_len = (mTotalSlots + 31) / 32 ;
		mDummySize = usage_bit_len - 1 ;

		if(mDummySize > 0)
		{
			mUsageBits = 0 ;
			for(S32 i = 0 ; i < mDummySize ; i++)
			{
				*((U32*)mBuffer + i) = 0 ;
			}
			if(mTotalSlots & 31)
			{
				*((U32*)mBuffer + mDummySize - 1) = (0xffffffff << (mTotalSlots & 31)) ;
			}
		}
	}
	
	if(mDummySize < 1)
	{
		mUsageBits = 0 ;
		if(mTotalSlots & 31)
		{
			mUsageBits = (0xffffffff << (mTotalSlots & 31)) ;
		}
	}

	mSelf = NULL ;
	mNext = NULL ;
}

void LLPrivateMemoryPool::LLMemoryBlock::setBuffer(char* buffer, U32 buffer_size)
{
	mBuffer = buffer ;
	mBufferSize = buffer_size ;
	mTotalSlots = 0 ; //set the block is free.
}

char* LLPrivateMemoryPool::LLMemoryBlock::allocate() 
{
	llassert_always(mAllocatedSlots < mTotalSlots) ;
	
	//find a free slot
	U32* bits = NULL ;
	U32  k = 0 ;
	if(mUsageBits != 0xffffffff)
	{
		bits = &mUsageBits ;
	}
	else if(mDummySize > 0)//go to extra space
	{		
		for(S32 i = 0 ; i < mDummySize; i++)
		{
			if(*((U32*)mBuffer + i) != 0xffffffff)
			{
				bits = (U32*)mBuffer + i ;
				k = i + 1 ;
				break ;
			}
		}
	}	
	S32 idx = 0 ;
	U32 tmp = *bits ;
	for(; tmp & 1 ; tmp >>= 1, idx++) ;

	//set the slot reserved
	if(!idx)
	{
		*bits |= 1 ;
	}
	else
	{
		*bits |= (1 << idx) ;
	}

	mAllocatedSlots++ ;

	return mBuffer + mDummySize * sizeof(U32) + (k * 32 + idx) * mSlotSize ;
}

void  LLPrivateMemoryPool::LLMemoryBlock::free(void* addr) 
{
	U32 idx = ((char*) addr - mBuffer - mDummySize * sizeof(U32)) / mSlotSize ;

	U32* bits = &mUsageBits ;
	if(idx > 32)
	{
		bits = (U32*)mBuffer + (idx - 32) / 32 ;
	}
	if(idx & 31)
	{
		*bits &= ~(1 << (idx & 31)) ;
	}
	else
	{
		*bits &= ~1 ;
	}

	mAllocatedSlots-- ;
}

//-------------------------------------------------------------------
//class LLMemoryChunk
//--------------------------------------------------------------------
LLPrivateMemoryPool::LLMemoryChunk::LLMemoryChunk()
{
	//empty
}

LLPrivateMemoryPool::LLMemoryChunk::~LLMemoryChunk()
{
	//empty
}

void LLPrivateMemoryPool::LLMemoryChunk::init(char* buffer, U32 buffer_size, U32 min_slot_size, U32 max_slot_size, U32 min_block_size, U32 max_block_size) 
{
	mBuffer = buffer ;
	mBufferSize = buffer_size ;

	mMetaBuffer = mBuffer + sizeof(LLMemoryChunk) ;

	mMinBlockSize = min_block_size;
	mMaxBlockSize = max_block_size;
	mMinSlotSize = min_slot_size;
	mBlockLevels = max_block_size / min_block_size ;
	mPartitionLevels = mMaxBlockSize / mMinBlockSize + 1 ;

	S32 max_num_blocks = (buffer_size - sizeof(LLMemoryChunk) - mBlockLevels * sizeof(LLMemoryBlock*) - mPartitionLevels * sizeof(LLMemoryBlock*)) / 
		                 (mMinBlockSize + sizeof(LLMemoryBlock)) ;
	//meta data space
	mBlocks = (LLMemoryBlock*)mMetaBuffer ;
	mAvailBlockList = (LLMemoryBlock**)((char*)mBlocks + sizeof(LLMemoryBlock) * max_num_blocks) ; 
	mFreeSpaceList = (LLMemoryBlock**)((char*)mAvailBlockList + sizeof(LLMemoryBlock*) * mBlockLevels) ; 
	
	//data buffer
	mDataBuffer = (char*)mFreeSpaceList + sizeof(LLMemoryBlock*) * mPartitionLevels ;

	//init
	for(U32 i = 0 ; i < mBlockLevels; i++)
	{
		mAvailBlockList[i] = NULL ;
	}
	for(U32 i = 0 ; i < mPartitionLevels ; i++)
	{
		mFreeSpaceList[i] = NULL ;
	}

	mBlocks[0].setBuffer(mDataBuffer, buffer_size - (mDataBuffer - mBuffer)) ;
	addToFreeSpace(&mBlocks[0]) ;

	mKey = (U32)mBuffer ;
	mNext = NULL ;
	mPrev = NULL ;
}

//static 
U32 LLPrivateMemoryPool::LLMemoryChunk::getMaxOverhead(U32 data_buffer_size, U32 min_page_size) 
{
	return 2048 +
		sizeof(LLMemoryBlock) * (data_buffer_size / min_page_size) ;
}

char* LLPrivateMemoryPool::LLMemoryChunk::allocate(U32 size)
{
	char* p = NULL ;
	U32 blk_idx = size / mMinSlotSize ;
	if(mMinSlotSize * blk_idx < size)
	{
		blk_idx++ ;
	}

	//check if there is free block available
	if(mAvailBlockList[blk_idx])
	{
		LLMemoryBlock* blk = mAvailBlockList[blk_idx] ;
		p = blk->allocate() ;

		if(blk->isFull())
		{
			//removeFromFreelist
			popAvailBlockList(blk_idx) ;
		}
	}

	//ask for a new block
	if(!p)
	{
		LLMemoryBlock* blk = addBlock(blk_idx) ;
		if(blk)
		{
			p = blk->allocate() ;

			if(blk->isFull())
			{
				//removeFromFreelist
				popAvailBlockList(blk_idx) ;
			}
		}
	}

	//ask for space from higher level blocks
	if(!p)
	{
		for(S32 i = blk_idx + 1 ; i < mBlockLevels; i++)
		{
			if(mAvailBlockList[i])
			{
				LLMemoryBlock* blk = mAvailBlockList[i] ;
				p = blk->allocate() ;

				if(blk->isFull())
				{
					//removeFromFreelist
					popAvailBlockList(i) ;
				}
				break ;
			}
		}
	}

	return p ;
}

void LLPrivateMemoryPool::LLMemoryChunk::free(void* addr)
{	
	LLMemoryBlock* blk = (LLMemoryBlock*)(mMetaBuffer + (((char*)addr - mDataBuffer) / mMinBlockSize) * sizeof(LLMemoryBlock)) ;
	blk = blk->mSelf ;

	bool was_full = blk->isFull() ;
	blk->free(addr) ;

	if(blk->empty())
	{
		removeBlock(blk) ;
	}
	else if(was_full)
	{
		addToAvailBlockList(blk) ;
	}
}

LLPrivateMemoryPool::LLMemoryBlock* LLPrivateMemoryPool::LLMemoryChunk::addBlock(U32 blk_idx)
{	
	U32 slot_size = mMinSlotSize * (blk_idx + 1) ;
	U32 preferred_block_size = llmax(mMinBlockSize, slot_size * 32) ;
	preferred_block_size = llmin(preferred_block_size, mMaxBlockSize) ;	

	U32 idx = preferred_block_size / mMinBlockSize ;	
	preferred_block_size = idx * mMinBlockSize ; //round to integer times of mMinBlockSize.

	LLMemoryBlock* blk = NULL ;
	
	if(mFreeSpaceList[idx])//if there is free slot for blk_idx
	{
		blk = createNewBlock(&mFreeSpaceList[idx], preferred_block_size, slot_size, blk_idx) ;
	}
	else if(mFreeSpaceList[mPartitionLevels - 1]) //search free pool
	{		
		blk = createNewBlock(&mFreeSpaceList[mPartitionLevels - 1], preferred_block_size, slot_size, blk_idx) ;
	}
	else //search for other non-preferred but enough space slot.
	{
		for(U32 i = idx - 1 ; i >= 0 ; i--) //search the small slots first
		{
			if(mFreeSpaceList[i])
			{
				//create a NEW BLOCK THERE.
				if(mFreeSpaceList[i]->getBufferSize() >= slot_size) //at least there is space for one slot.
				{
					blk = createNewBlock(&mFreeSpaceList[i], preferred_block_size, slot_size, blk_idx) ;
				}
				break ;
			} 
		}

		if(!blk)
		{
			for(U16 i = idx + 1 ; i < mPartitionLevels - 1; i++) //search the large slots 
			{
				if(mFreeSpaceList[i])
				{
					//create a NEW BLOCK THERE.
					blk = createNewBlock(&mFreeSpaceList[i], preferred_block_size, slot_size, blk_idx) ;
					break ;
				} 
			}
		}
	}

	return blk ;
}

LLPrivateMemoryPool::LLMemoryBlock* LLPrivateMemoryPool::LLMemoryChunk::createNewBlock(LLMemoryBlock** cur_idxp, U32 buffer_size, U32 slot_size, U32 blk_idx)
{
	LLMemoryBlock* blk = *cur_idxp ;
	
	buffer_size = llmin(buffer_size, blk->getBufferSize()) ;
	U32 new_free_blk_size = blk->getBufferSize() - buffer_size ;
	if(new_free_blk_size < mMinBlockSize) //can not partition the memory into size smaller than mMinBlockSize
	{
		buffer_size += new_free_blk_size ;
		new_free_blk_size = 0 ;
	}
	blk->init(blk->getBuffer(), buffer_size, slot_size) ;
		
	if(new_free_blk_size > 0) //cur_idx still has free space
	{
		LLMemoryBlock* next_blk = blk + (buffer_size / mMinBlockSize) ;
		next_blk->setBuffer(blk->getBuffer() + buffer_size, new_free_blk_size) ;
		
		if(new_free_blk_size > mMaxBlockSize) //stays in the free pool
		{
			next_blk->mPrev = NULL ;
			next_blk->mNext = blk->mNext ;
			if(next_blk->mNext)
			{
				next_blk->mNext->mPrev = next_blk ;
			}
			*cur_idxp = next_blk ;
		}
		else
		{
			*cur_idxp = blk->mNext ; //move to the next slot
			(*cur_idxp)->mPrev = NULL ;

			addToFreeSpace(next_blk) ;
		}		
	}
	else //move to the next block
	{
		*cur_idxp = blk->mNext ;
		(*cur_idxp)->mPrev = NULL ;
	}

	//insert to the available block list...
	blk->mNext = NULL ;
	blk->mPrev = NULL ;
	blk->mSelf = blk ;
	mAvailBlockList[blk_idx] = blk ;

	//mark the address map
	U32 end = (buffer_size / mMinBlockSize) ;
	for(U32 i = 1 ; i < end ; i++)
	{
		(blk + i)->mSelf = blk ;
	}

	return blk ;
}

void LLPrivateMemoryPool::LLMemoryChunk::removeBlock(LLMemoryBlock* blk)
{
	//remove from the available block list
	if(blk->mPrev)
	{
		blk->mPrev->mNext = blk->mNext ;
	}
	if(blk->mNext)
	{
		blk->mNext->mPrev = blk->mPrev ;
	}
	
	//mark it free
	blk->setBuffer(blk->getBuffer(), blk->getBufferSize()) ;

	//merge blk with neighbors if possible
	if(blk->getBuffer() > mDataBuffer) //has the left neighbor
	{
		if((blk - 1)->mSelf->isFree())
		{
			removeFromFreeSpace((blk - 1)->mSelf);
			(blk - 1)->mSelf->setBuffer((blk-1)->mSelf->getBuffer(), (blk-1)->mSelf->getBufferSize() + blk->getBufferSize()) ;
			blk = (blk - 1)->mSelf ;
		}
	}
	if(blk->getBuffer() + blk->getBufferSize() < mBuffer + mBufferSize) //has the right neighbor
	{
		U32 d = blk->getBufferSize() / mMinBlockSize ;
		if((blk + d)->isFree())
		{
			removeFromFreeSpace(blk + d) ;
			blk->setBuffer(blk->getBuffer(), blk->getBufferSize() + (blk + d)->getBufferSize()) ;
		}
	}

	addToFreeSpace(blk) ;

	return ;
}

//the top block in the list is full, pop it out of the list
void LLPrivateMemoryPool::LLMemoryChunk::popAvailBlockList(U32 blk_idx) 
{
	if(mAvailBlockList[blk_idx])
	{
		LLMemoryBlock* next = mAvailBlockList[blk_idx]->mNext ;
		next->mPrev = NULL ;
		mAvailBlockList[blk_idx]->mNext = NULL ;
		mAvailBlockList[blk_idx] = next ;
	}
}

void LLPrivateMemoryPool::LLMemoryChunk::addToFreeSpace(LLMemoryBlock* blk) 
{
	U16 free_idx = blk->getBufferSize() / mMinBlockSize ;
	(blk + free_idx)->mSelf = blk ; //mark the end pointing back to the head.
	free_idx = llmin(free_idx, (U16)(mPartitionLevels - 1)) ;

	blk->mNext = mFreeSpaceList[free_idx] ;
	if(mFreeSpaceList[free_idx])
	{
		mFreeSpaceList[free_idx]->mPrev = blk ;
	}
	mFreeSpaceList[free_idx] = blk ;
	blk->mPrev = NULL ;
	blk->mSelf = blk ;
	
	return ;
}

void LLPrivateMemoryPool::LLMemoryChunk::removeFromFreeSpace(LLMemoryBlock* blk) 
{
	U16 free_idx = blk->getBufferSize() / mMinBlockSize ;
	free_idx = llmin(free_idx, (U16)(mPartitionLevels - 1)) ;

	if(mFreeSpaceList[free_idx] == blk)
	{
		mFreeSpaceList[free_idx] = blk->mNext ;
	}
	if(blk->mPrev)
	{
		blk->mPrev->mNext = blk->mNext ;
	}
	if(blk->mNext)
	{
		blk->mNext->mPrev = blk->mPrev ;
	}
	
	return ;
}

void LLPrivateMemoryPool::LLMemoryChunk::addToAvailBlockList(LLMemoryBlock* blk) 
{
	U32 blk_idx = blk->getSlotSize() / mMinSlotSize ;

	blk->mNext = mAvailBlockList[blk_idx] ;
	if(blk->mNext)
	{
		blk->mNext->mPrev = blk ;
	}
	blk->mPrev = NULL ;
	
	return ;
}

//-------------------------------------------------------------------
//class LLPrivateMemoryPool
//--------------------------------------------------------------------
LLPrivateMemoryPool::LLPrivateMemoryPool(U32 max_size, bool threaded) :
	mMutexp(NULL),
	mMaxPoolSize(max_size),
	mReservedPoolSize(0)
{
	if(threaded)
	{
		mMutexp = new LLMutex(NULL) ;
	}

	for(S32 i = 0 ; i < SUPER_ALLOCATION ; i++)
	{
		mChunkList[i] = NULL ;
	}

	mChunkVectorCapacity = 128 ;
	mChunks.resize(mChunkVectorCapacity) ; //at most 128 chunks
	mNumOfChunks = 0 ;
}

LLPrivateMemoryPool::~LLPrivateMemoryPool()
{
	destroyPool();
	delete mMutexp ;
}

char* LLPrivateMemoryPool::allocate(U32 size)
{	
	const static U32 MAX_BLOCK_SIZE = 4 * 1024 * 1024 ; //4MB

	//if the asked size larger than MAX_BLOCK_SIZE, fetch from heap directly, the pool does not manage it
	if(size >= MAX_BLOCK_SIZE)
	{
		return new char[size] ;
	}

	char* p = NULL ;

	//find the appropriate chunk
	S32 chunk_idx = getChunkIndex(size) ;
	
	lock() ;

	LLMemoryChunk* chunk = mChunkList[chunk_idx];
	while(chunk)
	{
		if(p = chunk->allocate(size))
		{
			break ;
		}
		chunk = chunk->mNext ;
	}
	
	//fetch new memory chunk
	if(!p)
	{
		chunk = addChunk(chunk_idx) ;
		p = chunk->allocate(size) ;
	}

	unlock() ;

	return p ;
}

void LLPrivateMemoryPool::free(void* addr)
{
	lock() ;
	
	LLMemoryChunk* chunk = mChunks[findChunk((char*)addr)] ;
	if(!chunk)
	{
		delete[] (char*)addr ; //release from heap
	}
	else
	{
		chunk->free(addr) ;

		if(chunk->empty())
		{
			removeChunk(chunk) ;
		}
	}
	
	unlock() ;
}

void LLPrivateMemoryPool::dump()
{
}

void LLPrivateMemoryPool::lock()
{
	if(mMutexp)
	{
		mMutexp->lock() ;
	}
}

void LLPrivateMemoryPool::unlock()
{
	if(mMutexp)
	{
		mMutexp->unlock() ;
	}
}

S32  LLPrivateMemoryPool::getChunkIndex(U32 size) 
{
	if(size < 2048)
	{
		return 0 ;
	}
	else if(size  < (512 << 10))
	{
		return 1 ;
	}
	else 
	{
		return 2 ;
	}
}

//destroy the entire pool
void  LLPrivateMemoryPool::destroyPool()
{
	for(U16 i = 0 ; i < mNumOfChunks ; i++)
	{
		delete[] mChunks[i]->getBuffer() ;
	}
	mNumOfChunks = 0 ;
	for(S32 i = 0 ; i < SUPER_ALLOCATION ; i++)
	{
		mChunkList[i] = NULL ;
	}
}

LLPrivateMemoryPool::LLMemoryChunk* LLPrivateMemoryPool::addChunk(S32 chunk_index)
{
	static const U32 MIN_BLOCK_SIZES[SUPER_ALLOCATION] = {2 << 10, 32 << 10, 64 << 10} ;
	static const U32 MAX_BLOCK_SIZES[SUPER_ALLOCATION] = {64 << 10, 1 << 20, 4 << 20} ;
	static const U32 MIN_SLOT_SIZES[SUPER_ALLOCATION]  = {8, 2 << 10, 512 << 10};
	static const U32 MAX_SLOT_SIZES[SUPER_ALLOCATION]  = {(2 << 10) - 8, (512 - 2) << 10, 4 << 20};

	U32 preferred_size ;
	U32 overhead ;
	if(chunk_index < LARGE_ALLOCATION)
	{
		preferred_size = (4 << 20) ; //4MB
		overhead = LLMemoryChunk::getMaxOverhead(preferred_size, MIN_BLOCK_SIZES[chunk_index]) ;
	}
	else
	{
		preferred_size = (16 << 20) ; //16MB
		overhead = LLMemoryChunk::getMaxOverhead(preferred_size, MIN_BLOCK_SIZES[chunk_index]) ;
	}
	char* buffer = new(std::nothrow) char[preferred_size + overhead] ;
	if(!buffer)
	{
		return NULL ;
	}

	LLMemoryChunk* chunk = new (buffer) LLMemoryChunk() ;
	chunk->init(buffer, preferred_size + overhead, MIN_SLOT_SIZES[chunk_index],
		MAX_SLOT_SIZES[chunk_index], MIN_BLOCK_SIZES[chunk_index], MAX_BLOCK_SIZES[chunk_index]) ;

	//add to the head of the linked list
	chunk->mNext = mChunkList[chunk_index] ;
	if(mChunkList[chunk_index])
	{
		mChunkList[chunk_index]->mPrev = chunk ;
	}
	chunk->mPrev = NULL ;
	mChunkList[chunk_index] = chunk ;

	//insert into the array
	llassert_always(mNumOfChunks + 1 < mChunkVectorCapacity) ;
	if(!mNumOfChunks)
	{
		mChunks[0] = chunk ;
	}
	else
	{
		U16 k ;
		if(mChunks[0]->getBuffer() > chunk->getBuffer())
		{
			k = 0 ;
		}
		else
		{
			k = findChunk(chunk->getBuffer()) + 1 ;
		}
		for(U16 i = mNumOfChunks ; i > k ; i++)
		{
			mChunks[i] = mChunks[i-1] ;
		}
		mChunks[k] = chunk ;
	}
	mNumOfChunks++;

	return chunk ;
}

void LLPrivateMemoryPool::removeChunk(LLMemoryChunk* chunk) 
{
	//remove from the linked list
	if(chunk->mPrev)
	{
		chunk->mPrev->mNext = chunk->mNext ;
	}
	if(chunk->mNext)
	{
		chunk->mNext->mPrev = chunk->mPrev ;
	}

	//remove from the array
	U16 k = findChunk(chunk->getBuffer()) ;
	mNumOfChunks--;
	for(U16 i = k ; i < mNumOfChunks ; i++)
	{
		mChunks[i] = mChunks[i+1] ;
	}

	//release memory
	delete[] chunk->getBuffer() ;
}

U16 LLPrivateMemoryPool::findChunk(const char* addr) 
{
	llassert_always(mNumOfChunks > 0) ;
	
	U16 s = 0, e = mNumOfChunks;
	U16 k = (s + e) / 2 ;
	while(s < e)
	{
		if(mChunks[k]->mKey > (U32)addr)
		{
			e = k ;
		}
		else if(k < mNumOfChunks - 1 && mChunks[k+1]->mKey < (U32)addr)
		{
			s = k ;
		}
		else
		{
			break ;
		}

		k = (s + e) / 2 ;
	}

	return k ;
}

//--------------------------------------------------------------------
//class LLPrivateMemoryPoolTester
LLPrivateMemoryPoolTester* LLPrivateMemoryPoolTester::sInstance = NULL ;
LLPrivateMemoryPool* LLPrivateMemoryPoolTester::sPool = NULL ;
LLPrivateMemoryPoolTester::LLPrivateMemoryPoolTester()
{	
}
	
LLPrivateMemoryPoolTester::~LLPrivateMemoryPoolTester() 
{	
}

//static 
LLPrivateMemoryPoolTester* LLPrivateMemoryPoolTester::getInstance() 
{
	if(!sInstance)
	{
		sInstance = new LLPrivateMemoryPoolTester() ;
	}
	return sInstance ;
}

//static 
void LLPrivateMemoryPoolTester::destroy()
{
	if(sInstance)
	{
		delete sInstance ;
		sInstance = NULL ;
	}

	if(sPool)
	{
		delete sPool ;
		sPool = NULL ;
	}
}

void LLPrivateMemoryPoolTester::run() 
{
	const U32 max_pool_size = 16 << 20 ;
	const bool threaded = false ;
	if(!sPool)
	{
		sPool = new LLPrivateMemoryPool(max_pool_size, threaded) ;
	}

	//run the test
	correctnessTest() ;
	reliabilityTest() ;
	performanceTest() ;
	fragmentationtest() ;
}

void LLPrivateMemoryPoolTester::correctnessTest() 
{
	//try many different sized allocation, fill the memory fully to see if allocation is right.

}

void LLPrivateMemoryPoolTester::reliabilityTest() 
void LLPrivateMemoryPoolTester::performanceTest() 
void LLPrivateMemoryPoolTester::fragmentationtest() 

void* LLPrivateMemoryPoolTester::operator new(size_t size)
{
	return (void*)sPool->allocate(size) ;
}

void  LLPrivateMemoryPoolTester::operator delete(void* addr)
{
	sPool->free(addr) ;
}

//--------------------------------------------------------------------
