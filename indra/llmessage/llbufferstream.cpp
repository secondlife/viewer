/** 
 * @file llbufferstream.cpp
 * @author Phoenix
 * @date 2005-10-10
 * @brief Implementation of the buffer iostream classes
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llbufferstream.h"

#include "llbuffer.h"
#include "llmemtype.h"

static const S32 DEFAULT_OUTPUT_SEGMENT_SIZE = 1024 * 4;

/*
 * LLBufferStreamBuf
 */
LLBufferStreamBuf::LLBufferStreamBuf(
	const LLChannelDescriptors& channels,
	LLBufferArray* buffer) :
	mChannels(channels),
	mBuffer(buffer)
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
}

LLBufferStreamBuf::~LLBufferStreamBuf()
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
	sync();
}

// virtual
int LLBufferStreamBuf::underflow()
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
	//lldebugs << "LLBufferStreamBuf::underflow()" << llendl;
	if(!mBuffer)
	{
		return EOF;
	}
	LLSegment segment;
	LLBufferArray::segment_iterator_t it;
	U8* last_pos = (U8*)gptr();
	if(last_pos) --last_pos;
	
	LLBufferArray::segment_iterator_t end = mBuffer->endSegment();

	// Get iterator to full segment containing last_pos
	// and construct sub-segment starting at last_pos.
	// Note: segment may != *it at this point
	it = mBuffer->constructSegmentAfter(last_pos, segment);
	if(it == end)
	{
		return EOF;
	}
	
	// Iterate through segments to find a non-empty segment on input channel.
	while((!segment.isOnChannel(mChannels.in()) || (segment.size() == 0)))
	{
		++it;
		if(it == end)
		{
			return EOF;
		}

		segment = *it;
	}
	
	char* start = (char*)segment.data();
	setg(start, start, start + segment.size());
	return *gptr();
}

// virtual
int LLBufferStreamBuf::overflow(int c)
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
	if(!mBuffer)
	{
		return EOF;
	}
	if(EOF == c)
	{
		// if someone puts an EOF, I suppose we should sync and return
		// success.
		if(0 == sync())
		{
			return 1;
		}
		else
		{
			return EOF;
		}
	}

	// since we got here, we have a buffer, and we have a character to
	// put on it.
	LLBufferArray::segment_iterator_t it;
	it = mBuffer->makeSegment(mChannels.out(), DEFAULT_OUTPUT_SEGMENT_SIZE);
	if(it != mBuffer->endSegment())
	{
		char* start = (char*)(*it).data();
		(*start) = (char)(c);
		setp(start + 1, start + (*it).size());
		return c;
	}
	else
	{
		return EOF;
	}
}

// virtual
int LLBufferStreamBuf::sync()
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
	int return_value = -1;
	if(!mBuffer)
	{
		return return_value;
	}

	// set the put pointer so that we force an overflow on the next
	// write.
	U8* address = (U8*)pptr();
	setp(NULL, NULL);

	// *NOTE: I bet we could just --address. Need to think about that.
	address = mBuffer->seek(mChannels.out(), address, -1);
	if(address)
	{
		LLBufferArray::segment_iterator_t it;
		it = mBuffer->splitAfter(address);
		LLBufferArray::segment_iterator_t end = mBuffer->endSegment();
		if(it != end)
		{
			++it;
			if(it != end)
			{
				mBuffer->eraseSegment(it);
			}
			return_value = 0;
		}
	}
	else
	{
		// nothing was put on the buffer, so the sync() is a no-op.
		return_value = 0;
	}
	return return_value;
}

// virtual
#if( LL_WINDOWS || __GNUC__ > 2)
LLBufferStreamBuf::pos_type LLBufferStreamBuf::seekoff(
	LLBufferStreamBuf::off_type off,
	std::ios::seekdir way,
	std::ios::openmode which)
#else
streampos LLBufferStreamBuf::seekoff(
	streamoff off,
	std::ios::seekdir way,
	std::ios::openmode which)
#endif
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
	if(!mBuffer
	   || ((way == std::ios::beg) && (off < 0))
	   || ((way == std::ios::end) && (off > 0)))
	{
		return -1;
	}
	U8* address = NULL;
	if(which & std::ios::in)
	{
		U8* base_addr = NULL;
		switch(way)
		{
		case std::ios::end:
			base_addr = (U8*)LLBufferArray::npos;
			break;
		case std::ios::cur:
			// get the current get pointer and adjust it for buffer
			// array semantics.
			base_addr = (U8*)gptr();
			break;
		case std::ios::beg:
		default:
			// NULL is fine
			break;
		}
		address = mBuffer->seek(mChannels.in(), base_addr, off);
		if(address)
		{
			LLBufferArray::segment_iterator_t iter;
			iter = mBuffer->getSegment(address);
			char* start = (char*)(*iter).data();
			setg(start, (char*)address, start + (*iter).size());
		}
		else
		{
			address = (U8*)(-1);
		}
	}
	if(which & std::ios::out)
	{
		U8* base_addr = NULL;
		switch(way)
		{
		case std::ios::end:
			base_addr = (U8*)LLBufferArray::npos;
			break;
		case std::ios::cur:
			// get the current put pointer and adjust it for buffer
			// array semantics.
			base_addr = (U8*)pptr();
			break;
		case std::ios::beg:
		default:
			// NULL is fine
			break;
		}
		address = mBuffer->seek(mChannels.out(), base_addr, off);
		if(address)
		{
			LLBufferArray::segment_iterator_t iter;
			iter = mBuffer->getSegment(address);
			setp((char*)address, (char*)(*iter).data() + (*iter).size());
		}
		else
		{
			address = (U8*)(-1);
		}
	}

#if( LL_WINDOWS || __GNUC__ > 2 )
	S32 rv = (S32)(intptr_t)address;
	return (pos_type)rv;
#else
	return (streampos)address;
#endif
}


/*
 * LLBufferStream
 */
LLBufferStream::LLBufferStream(
	const LLChannelDescriptors& channels,
	LLBufferArray* buffer) :
	std::iostream(&mStreamBuf),
	mStreamBuf(channels, buffer)
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
}

LLBufferStream::~LLBufferStream()
{
	LLMemType m1(LLMemType::MTYPE_IO_BUFFER);
}
