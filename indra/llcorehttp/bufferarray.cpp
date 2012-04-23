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
	size_t mLen;

	// *NOTE:  Must be last member of the object.  We'll
	// overallocate as requested via operator new and index
	// into the array at will.
	char mData[1];		
};


// ==================================
// BufferArray Definitions
// ==================================


BufferArray::BufferArray()
	: LLCoreInt::RefCounted(true),
	  mPos(0),
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


size_t BufferArray::append(const char * src, size_t len)
{
	if (len)
	{
		if (mBlocks.size() >= mBlocks.capacity())
		{
			mBlocks.reserve(mBlocks.size() + 5);
		}
		Block * block = Block::alloc(len);
		memcpy(block->mData, src, len);
		mBlocks.push_back(block);
		mLen += len;
		mPos = mLen;
	}
	return len;
}


char * BufferArray::appendBufferAlloc(size_t len)
{
	// If someone asks for zero-length, we give them a valid pointer.
	if (mBlocks.size() >= mBlocks.capacity())
	{
		mBlocks.reserve(mBlocks.size() + 5);
	}
	Block * block = Block::alloc(len);
	mBlocks.push_back(block);
	mLen += len;
	mPos = mLen;
	return block->mData;
}


size_t BufferArray::seek(size_t pos)
{
	if (pos > mLen)
		pos = mLen;
	mPos = pos;
	return mPos;
}

	
size_t BufferArray::read(char * dst, size_t len)
{
	size_t result(0), offset(0);
	size_t len_limit(mLen - mPos);
	len = std::min(len, len_limit);

	if (mPos >= mLen || 0 == len)
		return 0;
	
	const int block_limit(mBlocks.size());
	int block_start(findBlock(mPos, &offset));
	if (block_start < 0)
		return 0;

	do
	{
		Block & block(*mBlocks[block_start]);
		size_t block_limit(block.mLen - offset);
		size_t block_len(std::min(block_limit, len));
		
		memcpy(dst, &block.mData[offset], block_len);
		result += block_len;
		len -= block_len;
		dst += block_len;
		offset = 0;
		++block_start;
	}
	while (len && block_start < block_limit);

	mPos += result;
	return result;
}


size_t BufferArray::write(const char * src, size_t len)
{
	size_t result(0), offset(0);
	if (mPos > mLen || 0 == len)
		return 0;
	
	const int block_limit(mBlocks.size());
	int block_start(findBlock(mPos, &offset));

	if (block_start >= 0)
	{
		// Some or all of the write will be on top of
		// existing data.
		do
		{
			Block & block(*mBlocks[block_start]);
			size_t block_limit(block.mLen - offset);
			size_t block_len(std::min(block_limit, len));
		
			memcpy(&block.mData[offset], src, block_len);
			result += block_len;
			len -= block_len;
			src += block_len;
			offset = 0;
			++block_start;
		}
		while (len && block_start < block_limit);
	}
	mPos += result;

	if (len)
	{
		// Some or all of the remaining write data will
		// be an append.
		result += append(src, len);
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
		if (pos < mBlocks[i]->mLen)
		{
			*ret_offset = pos;
			return i;
		}
		pos -= mBlocks[i]->mLen;
	}

	// Shouldn't get here but...
	return -1;
}


// ==================================
// BufferArray::Block Definitions
// ==================================


BufferArray::Block::Block(size_t len)
	: mLen(len)
{
	memset(mData, 0, len);
}
			

BufferArray::Block::~Block()
{
	mLen = 0;
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


