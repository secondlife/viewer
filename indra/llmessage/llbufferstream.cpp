/** 
 * @file llbufferstream.cpp
 * @author Phoenix
 * @date 2005-10-10
 * @brief Implementation of the buffer iostream classes
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llbufferstream.h"

#include "llbuffer.h"
#include "llthread.h"

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
}

LLBufferStreamBuf::~LLBufferStreamBuf()
{
	sync();
}

// virtual
int LLBufferStreamBuf::underflow()
{
	//lldebugs << "LLBufferStreamBuf::underflow()" << llendl;
	if(!mBuffer)
	{
		return EOF;
	}

	LLMutexLock lock(mBuffer->getMutex());
	LLBufferArray::segment_iterator_t iter;
	LLBufferArray::segment_iterator_t end = mBuffer->endSegment();
	U8* last_pos = (U8*)gptr();
	LLSegment segment;
	if(last_pos)
	{
		// Back up into a piece of memory we know that we have
		// allocated so that calls for the next segment based on
		// 'after' will succeed.
		--last_pos;
		iter = mBuffer->splitAfter(last_pos);
		if(iter != end)
		{
			// We need to clear the read segment just in case we have
			// an early exit in the function and never collect the
			// next segment. Calling eraseSegment() with the same
			// segment twice is just like double deleting -- nothing
			// good comes from it.
			mBuffer->eraseSegment(iter++);
			if(iter != end) segment = (*iter);
		}
		else
		{
			// This should never really happen, but somehow, the
			// istream is telling the buf that it just finished
			// reading memory that is not in the buf. I think this
			// would only happen if there were a bug in the c++ stream
			// class. Just bail.
			// *TODO: can we set the fail bit on the stream somehow?
			return EOF;
		}
	}
	else
	{
		// Get iterator to full segment containing last_pos
		// and construct sub-segment starting at last_pos.
		// Note: segment may != *it at this point
		iter = mBuffer->constructSegmentAfter(last_pos, segment);
	}
	if(iter == end)
	{
		return EOF;
	}

	// Iterate through segments to find a non-empty segment on input channel.
	while((!segment.isOnChannel(mChannels.in()) || (segment.size() == 0)))
	{
		++iter;
		if(iter == end)
		{
			return EOF;
		}

		segment = *(iter);
	}

	// set up the stream to read from the next segment.
	char* start = (char*)segment.data();
	setg(start, start, start + segment.size());
	return *gptr();
}

// virtual
int LLBufferStreamBuf::overflow(int c)
{
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
	LLMutexLock lock(mBuffer->getMutex());
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
	int return_value = -1;
	if(!mBuffer)
	{
		return return_value;
	}

	// This chunk of code is not necessary because typically, users of
	// the stream will read until EOF. Therefore, underflow was called
	// and the segment was discarded before the sync() was called in
	// the destructor. Theoretically, we could keep some more data
	// around and detect the rare case where an istream was deleted
	// before reading to the end, but that will only leave behind some
	// unavailable but still referenced memory. Also, if another
	// istream is constructed, it will re-read that segment, and then
	// discard it.
	//U8* last_pos = (U8*)gptr();
	//if(last_pos)
	//{
	//	// Looks like we read something. Discard what we have read.
	//	// gptr() actually returns the currrent position, but we call
	//	// it last_pos because of how it is used in the split call
	//	// below.
	//	--last_pos;
	//	LLBufferArray::segment_iterator_t iter;
	//	iter = mBuffer->splitAfter(last_pos);
	//	if(iter != mBuffer->endSegment())
	//	{
	//		// We need to clear the read segment just in case we have
	//		// an early exit in the function and never collect the
	//		// next segment. Calling eraseSegment() with the same
	//		// segment twice is just like double deleting -- nothing
	//		// good comes from it.
	//		mBuffer->eraseSegment(iter);
	//	}
	//}

	// set the put pointer so that we force an overflow on the next
	// write.
	U8* address = (U8*)pptr();
	setp(NULL, NULL);

	// *NOTE: I bet we could just --address if address is not NULL.
	// Need to think about that.
	LLMutexLock lock(mBuffer->getMutex());
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

		LLMutexLock lock(mBuffer->getMutex());
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

		LLMutexLock lock(mBuffer->getMutex());
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
}

LLBufferStream::~LLBufferStream()
{
}
