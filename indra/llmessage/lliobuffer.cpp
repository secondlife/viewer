/** 
 * @file lliobuffer.cpp
 * @author Phoenix
 * @date 2005-05-04
 * @brief Definition of buffer based implementations of IO Pipes.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lliobuffer.h"

//
// LLIOBuffer
//
LLIOBuffer::LLIOBuffer() :
	mBuffer(NULL),
	mBufferSize(0L),
	mReadHead(NULL),
	mWriteHead(NULL)
{
}

LLIOBuffer::~LLIOBuffer()
{
	if(mBuffer)
	{
		delete[] mBuffer;
	}
}

U8* LLIOBuffer::data() const
{
	return mBuffer;
}

S64 LLIOBuffer::size() const
{
	return mBufferSize;
}

U8* LLIOBuffer::current() const
{
	return mReadHead;
}

S64 LLIOBuffer::bytesLeft() const
{
	return mWriteHead - mReadHead;
}

void LLIOBuffer::clear()
{
	mReadHead = mBuffer;
	mWriteHead = mBuffer;
}

LLIOPipe::EStatus LLIOBuffer::seek(LLIOBuffer::EHead head, S64 delta)
{
	LLIOPipe::EStatus status = STATUS_ERROR;
	switch(head)
	{
	case READ:
		if(((delta >= 0) && ((mReadHead + delta) <= mWriteHead))
		   || (delta < 0) && ((mReadHead + delta) >= mBuffer))
		{
			mReadHead += delta;
			status = STATUS_OK;
		}
		break;
	case WRITE:
		if(((delta >= 0) && ((mWriteHead + delta) < (mBuffer + mBufferSize)))
		   || (delta < 0) && ((mWriteHead + delta) > mReadHead))
		{
			mWriteHead += delta;
			status = STATUS_OK;
		}
	default:
		break;
	}
	return status;
}

// virtual
LLIOPipe::EStatus LLIOBuffer::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	// no-op (I think)
	llwarns << "You are using an LLIOBuffer which is deprecated." << llendl;
	return STATUS_OK;
}
