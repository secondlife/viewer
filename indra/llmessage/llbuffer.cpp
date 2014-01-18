/** 
 * @file llbuffer.cpp
 * @author Phoenix
 * @date 2005-09-20
 * @brief Implementation of the segments, buffers, and buffer arrays.
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
#include "llbuffer.h"

#include "llmath.h"
#include "llstl.h"
#include "llthread.h"

#define ASSERT_LLBUFFERARRAY_MUTEX_LOCKED llassert(!mMutexp || mMutexp->isSelfLocked());

/** 
 * LLSegment
 */
LLSegment::LLSegment() :
	mChannel(0),
	mData(NULL),
	mSize(0)
{
}

LLSegment::LLSegment(S32 channel, U8* data, S32 data_len) :
	mChannel(channel),
	mData(data),
	mSize(data_len)
{
}

LLSegment::~LLSegment()
{
}

bool LLSegment::isOnChannel(S32 channel) const
{
	return (mChannel == channel);
}

S32 LLSegment::getChannel() const
{
	return mChannel;
}

void LLSegment::setChannel(S32 channel)
{
	mChannel = channel;
}


U8* LLSegment::data() const
{
	return mData;
}

S32 LLSegment::size() const
{
	return mSize;
}

bool LLSegment::operator==(const LLSegment& rhs) const
{
	if((mData != rhs.mData)||(mSize != rhs.mSize)||(mChannel != rhs.mChannel))
	{
		return false;
	}
	return true;
}

/** 
 * LLHeapBuffer
 */
LLHeapBuffer::LLHeapBuffer() :
	mBuffer(NULL),
	mSize(0),
	mNextFree(NULL),
	mReclaimedBytes(0)
{
	const S32 DEFAULT_HEAP_BUFFER_SIZE = 16384;
	allocate(DEFAULT_HEAP_BUFFER_SIZE);
}

LLHeapBuffer::LLHeapBuffer(S32 size) :
	mBuffer(NULL),
	mSize(0),
	mNextFree(NULL),
	mReclaimedBytes(0)
{
	allocate(size);
}

LLHeapBuffer::LLHeapBuffer(const U8* src, S32 len) :
	mBuffer(NULL),
	mSize(0),
	mNextFree(NULL),
	mReclaimedBytes(0)
{
	if((len > 0) && src)
	{
		allocate(len);
		if(mBuffer)
		{
			memcpy(mBuffer, src, len);	/*Flawfinder: ignore*/
		}
	}
}

// virtual
LLHeapBuffer::~LLHeapBuffer()
{
	delete[] mBuffer;
	mBuffer = NULL;
	mSize = 0;
	mNextFree = NULL;
}

S32 LLHeapBuffer::bytesLeft() const
{
	return (mSize - (mNextFree - mBuffer));
}

// virtual
bool LLHeapBuffer::createSegment(
	S32 channel,
	S32 size,
	LLSegment& segment)
{
	// get actual size of the segment.
	S32 actual_size = llmin(size, (mSize - S32(mNextFree - mBuffer)));

	// bail if we cannot build a valid segment
	if(actual_size <= 0)
	{
		return false;
	}

	// Yay, we're done.
	segment = LLSegment(channel, mNextFree, actual_size);
	mNextFree += actual_size;
	return true;
}

// virtual
bool LLHeapBuffer::reclaimSegment(const LLSegment& segment)
{
	if(containsSegment(segment))
	{
		mReclaimedBytes += segment.size();
		if(mReclaimedBytes == mSize)
		{
			// We have reclaimed all of the memory from this
			// buffer. Therefore, we can reset the mNextFree to the
			// start of the buffer, and reset the reclaimed bytes.
			mReclaimedBytes = 0;
			mNextFree = mBuffer;
		}
		else if(mReclaimedBytes > mSize)
		{
			llwarns << "LLHeapBuffer reclaimed more memory than allocated."
				<< " This is probably programmer error." << llendl;
		}
		return true;
	}
	return false;
}

// virtual
bool LLHeapBuffer::containsSegment(const LLSegment& segment) const
{
	// *NOTE: this check is fairly simple because heap buffers are
	// simple contiguous chunks of heap memory.
	if((mBuffer > segment.data())
	   || ((mBuffer + mSize) < (segment.data() + segment.size())))
	{
		return false;
	}
	return true;
}

void LLHeapBuffer::allocate(S32 size)
{
	mReclaimedBytes = 0;	
	mBuffer = new U8[size];
	if(mBuffer)
	{
		mSize = size;
		mNextFree = mBuffer;
	}
}


/** 
 * LLBufferArray
 */
LLBufferArray::LLBufferArray() :
	mNextBaseChannel(0),
	mMutexp(NULL)
{
}

LLBufferArray::~LLBufferArray()
{
	std::for_each(mBuffers.begin(), mBuffers.end(), DeletePointer());

	delete mMutexp;
}

// static
LLChannelDescriptors LLBufferArray::makeChannelConsumer(
	const LLChannelDescriptors& channels)
{
	LLChannelDescriptors rv(channels.out());
	return rv;
}

void LLBufferArray::lock()
{
	if(mMutexp)
	{
		mMutexp->lock() ;
	}
}

void LLBufferArray::unlock()
{
	if(mMutexp)
	{
		mMutexp->unlock() ;
	}
}

LLMutex* LLBufferArray::getMutex()
{
	return mMutexp ;
}

void LLBufferArray::setThreaded(bool threaded)
{
	if(threaded)
	{
		if(!mMutexp)
		{
			mMutexp = new LLMutex(NULL);
		}
	}
	else
	{
		if(mMutexp)
		{
			delete mMutexp ;
			mMutexp = NULL ;
		}
	}
}

LLChannelDescriptors LLBufferArray::nextChannel()
{
	LLChannelDescriptors rv(mNextBaseChannel++);
	return rv;
}

//mMutexp should be locked before calling this.
S32 LLBufferArray::capacity() const
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED

	S32 total = 0;
	const_buffer_iterator_t iter = mBuffers.begin();
	const_buffer_iterator_t end = mBuffers.end();
	for(; iter != end; ++iter)
	{
		total += (*iter)->capacity();
	}
	return total;
}

bool LLBufferArray::append(S32 channel, const U8* src, S32 len)
{
	LLMutexLock lock(mMutexp) ;

	std::vector<LLSegment> segments;
	if(copyIntoBuffers(channel, src, len, segments))
	{
		mSegments.insert(mSegments.end(), segments.begin(), segments.end());
		return true;
	}
	return false;
}

//mMutexp should be locked before calling this.
bool LLBufferArray::prepend(S32 channel, const U8* src, S32 len)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED

	std::vector<LLSegment> segments;
	if(copyIntoBuffers(channel, src, len, segments))
	{
		mSegments.insert(mSegments.begin(), segments.begin(), segments.end());
		return true;
	}
	return false;
}

bool LLBufferArray::insertAfter(
	segment_iterator_t segment,
	S32 channel,
	const U8* src,
	S32 len)
{
	std::vector<LLSegment> segments;

	LLMutexLock lock(mMutexp) ;
	if(mSegments.end() != segment)
	{
		++segment;
	}
	if(copyIntoBuffers(channel, src, len, segments))
	{
		mSegments.insert(segment, segments.begin(), segments.end());
		return true;
	}
	return false;
}

//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::splitAfter(U8* address)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED

	segment_iterator_t end = mSegments.end();
	segment_iterator_t it = getSegment(address);
	if(it == end)
	{
		return end;
	}

	// We have the location and the segment.
	U8* base = (*it).data();
	S32 size = (*it).size();
	if(address == (base + size))
	{
		// No need to split, since this is the last byte of the
		// segment. We do not want to have zero length segments, since
		// that will only incur processing overhead with no advantage.
		return it;
	}
	S32 channel = (*it).getChannel();
	LLSegment segment1(channel, base, (address - base) + 1);
	*it = segment1;
	segment_iterator_t rv = it;
	++it;
	LLSegment segment2(channel, address + 1, size - (address - base) - 1);
	mSegments.insert(it, segment2);
	return rv;
}
							   
//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::beginSegment()
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	return mSegments.begin();
}

//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::endSegment()
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	return mSegments.end();
}

//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::constructSegmentAfter(
	U8* address,
	LLSegment& segment)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	segment_iterator_t rv = mSegments.begin();
	segment_iterator_t end = mSegments.end();
	if(!address)
	{
		if(rv != end)
		{
			segment = (*rv);
		}
	}
	else
	{
		// we have an address - find the segment it is in.
		for( ; rv != end; ++rv)
		{
			if((address >= (*rv).data())
			   && (address < ((*rv).data() + (*rv).size())))
			{
				if((++address) < ((*rv).data() + (*rv).size()))
				{
					// it's in this segment - construct an appropriate
					// sub-segment.
					segment = LLSegment(
						(*rv).getChannel(),
						address,
						(*rv).size() - (address - (*rv).data()));
				}
				else
				{
					++rv;
					if(rv != end)
					{
						segment = (*rv);
					}
				}
				break;
			}
		}
	}
	if(rv == end)
	{
		segment = LLSegment();
	}
	return rv;
}

//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::getSegment(U8* address)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	segment_iterator_t end = mSegments.end();
	if(!address)
	{
		return end;
	}
	segment_iterator_t it = mSegments.begin();
	for( ; it != end; ++it)
	{
		if((address >= (*it).data())&&(address < (*it).data() + (*it).size()))
		{
			// found it.
			return it;
		}
	}
	return end;
}

//mMutexp should be locked before calling this.
LLBufferArray::const_segment_iterator_t LLBufferArray::getSegment(
	U8* address) const
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	const_segment_iterator_t end = mSegments.end();
	if(!address)
	{
		return end;
	}
	const_segment_iterator_t it = mSegments.begin();
	for( ; it != end; ++it)
	{
		if((address >= (*it).data())
		   && (address < (*it).data() + (*it).size()))
		{
			// found it.
			return it;
		}
	}
	return end;
}

/*
U8* LLBufferArray::getAddressAfter(U8* address) 
{
	U8* rv = NULL;
	segment_iterator_t it = getSegment(address);
	segment_iterator_t end = mSegments.end();
	if(it != end)
	{
		if(++address < ((*it).data() + (*it).size()))
		{
			// it's in the same segment
			rv = address;
		}
		else
		{
			// it's in the next segment
			if(++it != end)
			{
				rv = (*it).data();
			}
		}
	}
	return rv;
}
*/

S32 LLBufferArray::countAfter(S32 channel, U8* start) const
{
	S32 count = 0;
	S32 offset = 0;
	const_segment_iterator_t it;

	LLMutexLock lock(mMutexp) ;
	const_segment_iterator_t end = mSegments.end();
	if(start)
	{
		it = getSegment(start);
		if(it == end)
		{
			return count;
		}
		if(++start < ((*it).data() + (*it).size()))
		{
			// it's in the same segment
			offset = start - (*it).data();
		}
		else if(++it == end)
		{
			// it's in the next segment
			return count;
		}
	}
	else
	{
		it = mSegments.begin();
	}
	while(it != end)
	{
		if((*it).isOnChannel(channel))
		{
			count += (*it).size() - offset;
		}
		offset = 0;
		++it;
	}
	return count;
}

U8* LLBufferArray::readAfter(
	S32 channel,
	U8* start,
	U8* dest,
	S32& len) const
{
	U8* rv = start;
	if(!dest || len <= 0)
	{
		return rv;
	}
	S32 bytes_left = len;
	len = 0;
	S32 bytes_to_copy = 0;
	const_segment_iterator_t it;

	LLMutexLock lock(mMutexp) ;
	const_segment_iterator_t end = mSegments.end();
	if(start)
	{
		it = getSegment(start);
		if(it == end)
		{
			return rv;
		}
		if((++start < ((*it).data() + (*it).size()))
		   && (*it).isOnChannel(channel))
		{
			// copy the data out of this segment
			S32 bytes_in_segment = (*it).size() - (start - (*it).data());
			bytes_to_copy = llmin(bytes_left, bytes_in_segment);
			memcpy(dest, start, bytes_to_copy); /*Flawfinder: ignore*/
			len += bytes_to_copy;
			bytes_left -= bytes_to_copy;
			rv = start + bytes_to_copy - 1;
			++it;
		}
		else
		{
			++it;
		}
	}
	else
	{
		it = mSegments.begin();
	}
	while(bytes_left && (it != end))
	{
		if(!((*it).isOnChannel(channel)))
		{
			++it;
			continue;
		}
		bytes_to_copy = llmin(bytes_left, (*it).size());
		memcpy(dest + len, (*it).data(), bytes_to_copy); /*Flawfinder: ignore*/
		len += bytes_to_copy;
		bytes_left -= bytes_to_copy;
		rv = (*it).data() + bytes_to_copy - 1;
		++it;
	}
	return rv;
}

U8* LLBufferArray::seek(
	S32 channel,
	U8* start,
	S32 delta) const
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	const_segment_iterator_t it;
	const_segment_iterator_t end = mSegments.end();
	U8* rv = start;
	if(0 == delta)
	{
		if((U8*)npos == start)
		{
			// someone is looking for end of data. 
			segment_list_t::const_reverse_iterator rit = mSegments.rbegin();
			segment_list_t::const_reverse_iterator rend = mSegments.rend();
			while(rit != rend)
			{
				if(!((*rit).isOnChannel(channel)))
				{
					++rit;
					continue;
				}
				rv = (*rit).data() + (*rit).size();
				break;
			}
		}
		else if(start)
		{
			// This is sort of a weird case - check if zero bytes away
			// from current position is on channel and return start if
			// that is true. Otherwise, return NULL.
			it = getSegment(start);
			if((it == end) || !(*it).isOnChannel(channel))
			{
				rv = NULL;
			}
		}
		else
		{
			// Start is NULL, so return the very first byte on the
			// channel, or NULL.
			it = mSegments.begin();
			while((it != end) && !(*it).isOnChannel(channel))
			{
				++it;
			}
			if(it != end)
			{
				rv = (*it).data();
			}
		}
		return rv;
	}
	if(start)
	{
		it = getSegment(start);
		if((it != end) && (*it).isOnChannel(channel))
		{
			if(delta > 0)
			{
				S32 bytes_in_segment = (*it).size() - (start - (*it).data());
				S32 local_delta = llmin(delta, bytes_in_segment);
				rv += local_delta;
				delta -= local_delta;
				++it;
			}
			else
			{
				S32 bytes_in_segment = start - (*it).data();
				S32 local_delta = llmin(llabs(delta), bytes_in_segment);
				rv -= local_delta;
				delta += local_delta;
			}
		}
	}
	else if(delta < 0)
	{
		// start is NULL, and delta indicates seeking backwards -
		// return NULL.
		return NULL;
	}
	else
	{
		// start is NULL and delta > 0
		it = mSegments.begin();
	}
	if(delta > 0)
	{
		// At this point, we have an iterator into the segments, and
		// are seeking forward until delta is zero or we run out
		while(delta && (it != end))
		{
			if(!((*it).isOnChannel(channel)))
			{
				++it;
				continue;
			}
			if(delta <= (*it).size())
			{
				// it's in this segment
				rv = (*it).data() + delta;
			}
			delta -= (*it).size();
			++it;
		}
		if(delta && (it == end))
		{
			// Whoops - sought past end.
			rv = NULL;
		}
	}
	else //if(delta < 0)
	{
		// We are at the beginning of a segment, and need to search
		// backwards.
		segment_list_t::const_reverse_iterator rit(it);
		segment_list_t::const_reverse_iterator rend = mSegments.rend();
		while(delta && (rit != rend))
		{
			if(!((*rit).isOnChannel(channel)))
			{
				++rit;
				continue;
			}
			if(llabs(delta) <= (*rit).size())
			{
				// it's in this segment.
				rv = (*rit).data() + (*rit).size() + delta;
				delta = 0;
			}
			else
			{
				delta += (*rit).size();
			}
			++rit;
		}
		if(delta && (rit == rend))
		{
			// sought past the beginning.
			rv = NULL;
		}
	}
	return rv;
}

//test use only
bool LLBufferArray::takeContents(LLBufferArray& source)
{
	LLMutexLock lock(mMutexp);
	source.lock();

	std::copy(
		source.mBuffers.begin(),
		source.mBuffers.end(),
		std::back_insert_iterator<buffer_list_t>(mBuffers));
	source.mBuffers.clear();
	std::copy(
		source.mSegments.begin(),
		source.mSegments.end(),
		std::back_insert_iterator<segment_list_t>(mSegments));
	source.mSegments.clear();
	source.mNextBaseChannel = 0;
	source.unlock();

	return true;
}

//mMutexp should be locked before calling this.
LLBufferArray::segment_iterator_t LLBufferArray::makeSegment(
	S32 channel,
	S32 len)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	// start at the end of the buffers, because it is the most likely
	// to have free space.
	LLSegment segment;
	buffer_list_t::reverse_iterator it = mBuffers.rbegin();
	buffer_list_t::reverse_iterator end = mBuffers.rend();
	bool made_segment = false;
	for(; it != end; ++it)
	{
		if((*it)->createSegment(channel, len, segment))
		{
			made_segment = true;
			break;
		}
	}
	segment_iterator_t send = mSegments.end();
	if(!made_segment)
	{
		LLBuffer* buf = new LLHeapBuffer;
		mBuffers.push_back(buf);
		if(!buf->createSegment(channel, len, segment))
		{
			// failed. this should never happen.
			return send;
		}
	}

	// store and return the newly made segment
	mSegments.insert(send, segment);
	std::list<LLSegment>::reverse_iterator rv = mSegments.rbegin();
	++rv;
	send = rv.base();
	return send;
}

//mMutexp should be locked before calling this.
bool LLBufferArray::eraseSegment(const segment_iterator_t& erase_iter)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED

	// Find out which buffer contains the segment, and if it is found,
	// ask it to reclaim the memory.
	bool rv = false;
	LLSegment segment(*erase_iter);
	buffer_iterator_t iter = mBuffers.begin();
	buffer_iterator_t end = mBuffers.end();
	for(; iter != end; ++iter)
	{
		// We can safely call reclaimSegment on every buffer, and once
		// it returns true, the segment was found.
		if((*iter)->reclaimSegment(segment))
		{
			rv = true;
			break;
		}
	}

	// No need to get the return value since we are not interested in
	// the interator retured by the call.
	(void)mSegments.erase(erase_iter);
	return rv;
}

//mMutexp should be locked before calling this.
bool LLBufferArray::copyIntoBuffers(
	S32 channel,
	const U8* src,
	S32 len,
	std::vector<LLSegment>& segments)
{
	ASSERT_LLBUFFERARRAY_MUTEX_LOCKED
	if(!src || !len) return false;
	S32 copied = 0;
	LLSegment segment;
	buffer_iterator_t it = mBuffers.begin();
	buffer_iterator_t end = mBuffers.end();
	for(; it != end;)
	{
		if(!(*it)->createSegment(channel, len, segment))
		{
			++it;
			continue;
		}
		segments.push_back(segment);
		S32 bytes = llmin(segment.size(), len);
		memcpy(segment.data(), src + copied, bytes);  /* Flawfinder: Ignore */
		copied += bytes;
		len -= bytes;
		if(0 == len)
		{
			break;
		}
	}
	while(len)
	{
		LLBuffer* buf = new LLHeapBuffer;
		mBuffers.push_back(buf);
		if(!buf->createSegment(channel, len, segment))
		{
			// this totally failed - bail. This is the weird corner
			// case were we 'leak' memory. No worries about an actual
			// leak - we will still reclaim the memory later, but this
			// particular buffer array is hosed for some reason.
			// This should never happen.
			return false;
		}
		segments.push_back(segment);
		memcpy(segment.data(), src + copied, segment.size());	/*Flawfinder: ignore*/
		copied += segment.size();
		len -= segment.size();
	}
	return true;
}
