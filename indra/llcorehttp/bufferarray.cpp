/**
 * @file bufferarray.cpp
 * @brief Implements the BufferArray scatter/gather buffer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "bufferarray.h"


// BufferArray is a list of chunks, each a BufferArray::Block, of contiguous
// data presented as a single array.  Chunks are at least BufferArray::BLOCK_ALLOC_SIZE
// in length and can be larger.  Any chunk may be partially filled or even
// empty.
//
// The BufferArray itself is sharable as a RefCounted entity.  As shared
// reads don't work with the concept of a current position/seek value,
// none is kept with the object.  Instead, the read and write operations
// all take position arguments.  Single write/shared read isn't supported
// directly and any such attempts have to be serialized outside of this
// implementation.

namespace LLCore
{


// ==================================
// BufferArray::Block Declaration
// ==================================

class BufferArray::Block
{
public:
	~Block();

	void operator delete(void *);
	void operator delete(void *, size_t len);

protected:
	Block(size_t len);

	Block(const Block &);						// Not defined
	void operator=(const Block &);				// Not defined

	// Allocate the block with the additional space for the
	// buffered data at the end of the object.
	void * operator new(size_t len, size_t addl_len);
	
public:
	// Only public entry to get a block.
	static Block * alloc(size_t len);

public:
	size_t mUsed;
	size_t mAlloced;

	// *NOTE:  Must be last member of the object.  We'll
	// overallocate as requested via operator new and index
	// into the array at will.
	char mData[1];		
};


// ==================================
// BufferArray Definitions
// ==================================


#if	! LL_WINDOWS
const size_t BufferArray::BLOCK_ALLOC_SIZE;
#endif	// ! LL_WINDOWS

BufferArray::BufferArray()
	: LLCoreInt::RefCounted(true),
	  mLen(0)
{}


BufferArray::~BufferArray()
{
	for (container_t::iterator it(mBlocks.begin());
		 it != mBlocks.end();
		 ++it)
	{
		delete *it;
		*it = NULL;
	}
	mBlocks.clear();
}


size_t BufferArray::append(const void * src, size_t len)
{
	const size_t ret(len);
	const char * c_src(static_cast<const char *>(src));
					  
	// First, try to copy into the last block
	if (len && ! mBlocks.empty())
	{
		Block & last(*mBlocks.back());
		if (last.mUsed < last.mAlloced)
		{
			// Some will fit...
			const size_t copy_len((std::min)(len, (last.mAlloced - last.mUsed)));

			memcpy(&last.mData[last.mUsed], c_src, copy_len);
			last.mUsed += copy_len;
			llassert_always(last.mUsed <= last.mAlloced);
			mLen += copy_len;
			c_src += copy_len;
			len -= copy_len;
		}
	}

	// Then get new blocks as needed
	while (len)
	{
		const size_t copy_len((std::min)(len, BLOCK_ALLOC_SIZE));
		
		if (mBlocks.size() >= mBlocks.capacity())
		{
			mBlocks.reserve(mBlocks.size() + 5);
		}
		Block * block = Block::alloc(BLOCK_ALLOC_SIZE);
		memcpy(block->mData, c_src, copy_len);
		block->mUsed = copy_len;
		llassert_always(block->mUsed <= block->mAlloced);
		mBlocks.push_back(block);
		mLen += copy_len;
		c_src += copy_len;
		len -= copy_len;
	}
	return ret;
}


void * BufferArray::appendBufferAlloc(size_t len)
{
	// If someone asks for zero-length, we give them a valid pointer.
	if (mBlocks.size() >= mBlocks.capacity())
	{
		mBlocks.reserve(mBlocks.size() + 5);
	}
	Block * block = Block::alloc((std::max)(BLOCK_ALLOC_SIZE, len));
	block->mUsed = len;
	mBlocks.push_back(block);
	mLen += len;
	return block->mData;
}


size_t BufferArray::read(size_t pos, void * dst, size_t len)
{
	char * c_dst(static_cast<char *>(dst));
	
	if (pos >= mLen)
		return 0;
	size_t len_limit(mLen - pos);
	len = (std::min)(len, len_limit);
	if (0 == len)
		return 0;
	
	size_t result(0), offset(0);
	const int block_limit(mBlocks.size());
	int block_start(findBlock(pos, &offset));
	if (block_start < 0)
		return 0;

	do
	{
		Block & block(*mBlocks[block_start]);
		size_t block_limit(block.mUsed - offset);
		size_t block_len((std::min)(block_limit, len));
		
		memcpy(c_dst, &block.mData[offset], block_len);
		result += block_len;
		len -= block_len;
		c_dst += block_len;
		offset = 0;
		++block_start;
	}
	while (len && block_start < block_limit);

	return result;
}


size_t BufferArray::write(size_t pos, const void * src, size_t len)
{
	const char * c_src(static_cast<const char *>(src));
	
	if (pos > mLen || 0 == len)
		return 0;
	
	size_t result(0), offset(0);
	const int block_limit(mBlocks.size());
	int block_start(findBlock(pos, &offset));

	if (block_start >= 0)
	{
		// Some or all of the write will be on top of
		// existing data.
		do
		{
			Block & block(*mBlocks[block_start]);
			size_t block_limit(block.mUsed - offset);
			size_t block_len((std::min)(block_limit, len));
		
			memcpy(&block.mData[offset], c_src, block_len);
			result += block_len;
			c_src += block_len;
			len -= block_len;
			offset = 0;
			++block_start;
		}
		while (len && block_start < block_limit);
	}

	// Something left, see if it will fit in the free
	// space of the last block.
	if (len && ! mBlocks.empty())
	{
		Block & last(*mBlocks.back());
		if (last.mUsed < last.mAlloced)
		{
			// Some will fit...
			const size_t copy_len((std::min)(len, (last.mAlloced - last.mUsed)));

			memcpy(&last.mData[last.mUsed], c_src, copy_len);
			last.mUsed += copy_len;
			result += copy_len;
			llassert_always(last.mUsed <= last.mAlloced);
			mLen += copy_len;
			c_src += copy_len;
			len -= copy_len;
		}
	}
	
	if (len)
	{
		// Some or all of the remaining write data will
		// be an append.
		result += append(c_src, len);
	}

	return result;
}
		

int BufferArray::findBlock(size_t pos, size_t * ret_offset)
{
	*ret_offset = 0;
	if (pos >= mLen)
		return -1;		// Doesn't exist

	const int block_limit(mBlocks.size());
	for (int i(0); i < block_limit; ++i)
	{
		if (pos < mBlocks[i]->mUsed)
		{
			*ret_offset = pos;
			return i;
		}
		pos -= mBlocks[i]->mUsed;
	}

	// Shouldn't get here but...
	return -1;
}


bool BufferArray::getBlockStartEnd(int block, const char ** start, const char ** end)
{
	if (block < 0 || block >= mBlocks.size())
	{
		return false;
	}

	const Block & b(*mBlocks[block]);
	*start = &b.mData[0];
	*end = &b.mData[b.mUsed];
	return true;
}


// ==================================
// BufferArray::Block Definitions
// ==================================


BufferArray::Block::Block(size_t len)
	: mUsed(0),
	  mAlloced(len)
{
	memset(mData, 0, len);
}
			

BufferArray::Block::~Block()
{
	mUsed = 0;
	mAlloced = 0;
}


void * BufferArray::Block::operator new(size_t len, size_t addl_len)
{
	void * mem = new char[len + addl_len + sizeof(void *)];
	return mem;
}


void BufferArray::Block::operator delete(void * mem)
{
	char * cmem = static_cast<char *>(mem);
	delete [] cmem;
}


void BufferArray::Block::operator delete(void * mem, size_t)
{
	operator delete(mem);
}


BufferArray::Block * BufferArray::Block::alloc(size_t len)
{
	Block * block = new (len) Block(len);
	return block;
}
	

}  // end namespace LLCore
