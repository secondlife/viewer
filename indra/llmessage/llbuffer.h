/** 
 * @file llbuffer.h
 * @author Phoenix
 * @date 2005-09-20
 * @brief Declaration of buffer and buffer arrays primarily used in I/O.
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

#ifndef LL_LLBUFFER_H
#define LL_LLBUFFER_H

/**
 * Declaration of classes used for minimizing calls to new[],
 * memcpy(), and delete[]. Typically, you would create an LLBufferArray,
 * feed it data, modify and add segments as you process it, and feed
 * it to a sink.
 */

#include <list>
#include <vector>

class LLMutex;
/** 
 * @class LLChannelDescriptors
 * @brief A way simple interface to accesss channels inside a buffer
 */
class LLChannelDescriptors
{
public:
	// enumeration for segmenting the channel information
	enum { E_CHANNEL_COUNT = 3 };
	LLChannelDescriptors() : mBaseChannel(0) {}
	explicit LLChannelDescriptors(S32 base) : mBaseChannel(base) {}
	S32 in() const { return mBaseChannel; }
	S32 out() const { return mBaseChannel + 1; }
	//S32 err() const { return mBaseChannel + 2; }
protected:
	S32 mBaseChannel;
};


/** 
 * @class LLSegment
 * @brief A segment is a single, contiguous chunk of memory in a buffer
 *
 * Each segment represents a contiguous addressable piece of memory
 * which is located inside a buffer. The segment is not responsible
 * for allocation or deallcoation of the data. Each segment is a light
 * weight object, and simple enough to copy around, use, and generate
 * as necessary.
 * This is the preferred interface for working with memory blocks,
 * since it is the only way to safely, inexpensively, and directly
 * access linear blocks of memory. 
 */
class LLSegment
{
public:
	LLSegment();
	LLSegment(S32 channel, U8* data, S32 data_len);
	~LLSegment();

	/**
	 * @brief Check if this segment is on the given channel.
	 *
	 */
	bool isOnChannel(S32 channel) const;

	/**
	 * @brief Get the channel
	 */
	S32 getChannel() const;

	/**
	 * @brief Set the channel
	 */
	void setChannel(S32 channel);

	/** 
	 * @brief Return a raw pointer to the current data set.
	 *
	 * The pointer returned can be used for reading or even adjustment
	 * if you are a bit crazy up to size() bytes into memory.
	 * @return A potentially NULL pointer to the raw buffer data
	 */
	U8* data() const;

	/** 
	 * @brief Return the size of the segment
	 */
	S32 size() const;

	/** 
	 * @brief Check if two segments are the same.
	 *
	 * Two segments are considered equal if they are on the same
	 * channel and cover the exact same address range.
	 * @param rhs the segment to compare with this segment.
	 * @return Returns true if they are equal.
	 */
	bool operator==(const LLSegment& rhs) const;

protected:
	S32 mChannel;
	U8* mData;
	S32 mSize;
};

/** 
 * @class LLBuffer
 * @brief Abstract base class for buffers
 *
 * This class declares the interface necessary for buffer arrays. A
 * buffer is not necessarily a single contiguous memory chunk, so
 * please do not circumvent the segment API.
 */
class LLBuffer
{
public:
	/** 
	 * @brief The buffer base class should have no responsibilities
	 * other than an interface.
	 */ 
	virtual ~LLBuffer() {}

	/** 
	 * @brief Generate a segment for this buffer.
	 *
	 * The segment returned is always contiguous memory. This call can
	 * fail if no contiguous memory is available, eg, offset is past
	 * the end. The segment returned may be smaller than the requested
	 * size. The segment will never be larger than the requested size.
	 * @param channel The channel for the segment.
	 * @param offset The offset from zero in the buffer.
	 * @param size The requested size of the segment.
	 * @param segment[out] The out-value from the operation
	 * @return Returns true if a segment was found.
	 */
	virtual bool createSegment(S32 channel, S32 size, LLSegment& segment) = 0;

	/** 
	 * @brief Reclaim a segment from this buffer. 
	 *
	 * This method is called on a buffer object when a caller is done
	 * with a contiguous segment of memory inside this buffer. Since
	 * segments can be cut arbitrarily outside of the control of the
	 * buffer, this segment may not match any segment returned from
	 * <code>createSegment()</code>.  
	 * @param segment The contiguous buffer segment to reclaim.
	 * @return Returns true if the call was successful.
	 */
	virtual bool reclaimSegment(const LLSegment& segment) = 0;

	/** 
	 * @brief Test if a segment is inside this buffer.
	 *
	 * @param segment The contiguous buffer segment to test.
	 * @return Returns true if the segment is in the bufffer.
	 */
	virtual bool containsSegment(const LLSegment& segment) const = 0;

	/** 
	 * @brief Return the current number of bytes allocated.
	 *
	 * This was implemented as a debugging tool, and it is not
	 * necessarily a good idea to use it for anything else.
	 */
	virtual S32 capacity() const = 0;
};

/** 
 * @class LLHeapBuffer
 * @brief A large contiguous buffer allocated on the heap with new[].
 *
 * This class is a simple buffer implementation which allocates chunks
 * off the heap. Once a buffer is constructed, it's buffer has a fixed
 * length.
 */
class LLHeapBuffer : public LLBuffer
{
public:
	/** 
	 * @brief Construct a heap buffer with a reasonable default size.
	 */
	LLHeapBuffer();

	/** 
	 * @brief Construct a heap buffer with a specified size.
	 *
	 * @param size The minimum size of the buffer.
	 */
	explicit LLHeapBuffer(S32 size);

	/** 
	 * @brief Construct a heap buffer of minimum size len, and copy from src.
	 *
	 * @param src The source of the data to be copied.
	 * @param len The minimum size of the buffer.
	 */
	LLHeapBuffer(const U8* src, S32 len);

	/** 
	 * @brief Simple destruction.
	 */
	virtual ~LLHeapBuffer();

	/** 
	 * @brief Get the number of bytes left in the buffer.
	 *
	 * Note that this is not a virtual function, and only available in
	 * the LLHeapBuffer as a debugging aid.
	 * @return Returns the number of bytes left.
	 */
	S32 bytesLeft() const;

	/** 
	 * @brief Generate a segment for this buffer.
	 *
	 * The segment returned is always contiguous memory. This call can
	 * fail if no contiguous memory is available, eg, offset is past
	 * the end. The segment returned may be smaller than the requested
	 * size. It is up to the caller to delete the segment returned.
	 * @param channel The channel for the segment.
	 * @param offset The offset from zero in the buffer
	 * @param size The requested size of the segment
	 * @param segment[out] The out-value from the operation
	 * @return Returns true if a segment was found.
	 */
	virtual bool createSegment(S32 channel, S32 size, LLSegment& segment);

	/** 
	 * @brief reclaim a segment from this buffer. 
	 *
	 * This method is called on a buffer object when a caller is done
	 * with a contiguous segment of memory inside this buffer. Since
	 * segments can be cut arbitrarily outside of the control of the
	 * buffer, this segment may not match any segment returned from
	 * <code>createSegment()</code>.  
	 * This call will fail if the segment passed in is note completely
	 * inside the buffer, eg, if the segment starts before this buffer
	 * in memory or ends after it.
	 * @param segment The contiguous buffer segment to reclaim.
	 * @return Returns true if the call was successful.
	 */
	virtual bool reclaimSegment(const LLSegment& segment);

	/** 
	 * @brief Test if a segment is inside this buffer.
	 *
	 * @param segment The contiguous buffer segment to test.
	 * @return Returns true if the segment is in the bufffer.
	 */
	virtual bool containsSegment(const LLSegment& segment) const;

	/** 
	 * @brief Return the current number of bytes allocated.
	 */
	virtual S32 capacity() const { return mSize; }

protected:
	U8* mBuffer;
	S32 mSize;
	U8* mNextFree;
	S32 mReclaimedBytes;

private:
	/** 
	 * @brief Helper method to allocate a buffer and correctly set
	 * intertnal state of this buffer.
	 */ 
	void allocate(S32 size);
};

/** 
 * @class LLBufferArray
 * @brief Class to represent scattered memory buffers and in-order segments
 * of that buffered data.
 *
 * *NOTE: This class needs to have an iovec interface
 */
class LLBufferArray
{
public:
	typedef std::vector<LLBuffer*> buffer_list_t;
	typedef buffer_list_t::iterator buffer_iterator_t;
	typedef buffer_list_t::const_iterator const_buffer_iterator_t;
	typedef std::list<LLSegment> segment_list_t;
	typedef segment_list_t::const_iterator const_segment_iterator_t;
	typedef segment_list_t::iterator segment_iterator_t;
	enum { npos = 0xffffffff };

	LLBufferArray();
	~LLBufferArray();

	/* @name Channel methods
	 */
	//@{
	/** 
	 * @brief Generate the a channel descriptor which consumes the
	 * output for the channel passed in.
	 */
	static LLChannelDescriptors makeChannelConsumer(
		const LLChannelDescriptors& channels);

	/** 
	 * @brief Generate the next channel descriptor for this buffer array.
	 * 
	 * The channel descriptor interface is how the buffer array
	 * clients can know where to read and write data. Use this
	 * interface to get the 'next' channel set for usage. This is a
	 * bit of a simple hack until it's utility indicates it should be
	 * extended.
	 * @return Returns a valid channel descriptor set for input and output.
	 */
	LLChannelDescriptors nextChannel();
	//@}

	/* @name Data methods
	 */
	//@{

	/** 
	 * @brief Return the sum of all allocated bytes.
	 */
	S32 capacity() const;

	// These methods will be useful once there is any kind of buffer
	// besides a heap buffer.
	//bool append(EBufferChannel channel, LLBuffer* data);
	//bool prepend(EBufferChannel channel, LLBuffer* data);
	//bool insertAfter(
	//	segment_iterator_t segment,
	//	EBufferChannel channel,
	//	LLBuffer* data);

	/** 
	 * @brief Put data on a channel at the end of this buffer array.
	 *
	 * The data is copied from src into the buffer array. At least one
	 * new segment is created and put on the end of the array. This
	 * object will internally allocate new buffers if necessary.
	 * @param channel The channel for this data
	 * @param src The start of memory for the data to be copied
	 * @param len The number of bytes of data to copy
	 * @return Returns true if the method worked.
	 */
	bool append(S32 channel, const U8* src, S32 len);

	/** 
	 * @brief Put data on a channel at the front of this buffer array.
	 *
	 * The data is copied from src into the buffer array. At least one
	 * new segment is created and put in the front of the array. This
	 * object will internally allocate new buffers if necessary.
	 * @param channel The channel for this data
	 * @param src The start of memory for the data to be copied
	 * @param len The number of bytes of data to copy
	 * @return Returns true if the method worked.
	 */
	bool prepend(S32 channel, const U8* src, S32 len);

	/** 
	 * @brief Insert data into a buffer array after a particular segment.
	 *
	 * The data is copied from src into the buffer array. At least one
	 * new segment is created and put in the array. This object will
	 * internally allocate new buffers if necessary.
	 * @param segment The segment in front of the new segments location
	 * @param channel The channel for this data
	 * @param src The start of memory for the data to be copied
	 * @param len The number of bytes of data to copy
	 * @return Returns true if the method worked.
	 */
	bool insertAfter(
		segment_iterator_t segment,
		S32 channel,
		const U8* src,
		S32 len);

	/** 
	 * @brief Count bytes in the buffer array on the specified channel
	 *
	 * @param channel The channel to count.
	 * @param start The start address in the array for counting. You
	 * can specify NULL to start at the beginning.
	 * @return Returns the number of bytes in the channel after start
	 */
	S32 countAfter(S32 channel, U8* start) const;

	/** 
	 * @brief Count all bytes on channel.
	 *
	 * Helper method which just calls countAfter().
	 * @param channel The channel to count.
	 * @return Returns the number of bytes in the channel.
	 */
	S32 count(S32 channel) const
	{
		return countAfter(channel, NULL);
	}

	/** 
	 * @brief Read bytes in the buffer array on the specified channel
	 *
	 * You should prefer iterating over segments is possible since
	 * this method requires you to allocate large buffers - precisely
	 * what this class is trying to prevent.  This method will skip
	 * any segments which are not on the given channel, so this method
	 * would usually be used to read a channel and copy that to a log
	 * or a socket buffer or something.
	 * @param channel The channel to read.
	 * @param start The start address in the array for reading. You
	 * can specify NULL to start at the beginning.
	 * @param dest The destination of the data read. This must be at
	 * least len bytes long.
	 * @param len[in,out] <b>in</b> How many bytes to read. <b>out</b> How 
	 * many bytes were read.
	 * @return Returns the address of the last read byte.
	 */
	U8* readAfter(S32 channel, U8* start, U8* dest, S32& len) const;
 
	/** 
	 * @brief Find an address in a buffer array
	 *
	 * @param channel The channel to seek in.
	 * @param start The start address in the array for the seek
	 * operation. You can specify NULL to start the seek at the
	 * beginning, or pass in npos to start at the end.
	 * @param delta How many bytes to seek through the array.
	 * @return Returns the address of the last read byte.
	 */
	U8* seek(S32 channel, U8* start, S32 delta) const;
	//@}

	/* @name Buffer interaction
	 */
	//@{
	/** 
	 * @brief Take the contents of another buffer array
	 *
	 * This method simply strips the contents out of the source
	 * buffery array - segments, buffers, etc, and appends them to
	 * this instance. After this operation, the source is empty and
	 * ready for reuse.
	 * @param source The source buffer
	 * @return Returns true if the operation succeeded.
	 */
	bool takeContents(LLBufferArray& source);
	//@}

	/* @name Segment methods
	 */
	//@{
	/** 
	 * @brief Split a segments so that address is the last address of
	 * one segment, and the rest of the original segment becomes
	 * another segment on the same channel.
	 *
	 * After this method call,
	 * <code>getLastSegmentAddress(*getSegment(address)) ==
	 * address</code> should be true. This call will only create a new
	 * segment if the statement above is false before the call. Since
	 * you usually call splitAfter() to change a segment property, use
	 * getSegment() to perform those operations.
	 * @param address The address which will become the last address
	 * of the segment it is in.
	 * @return Returns an iterator to the segment which contains
	 * <code>address</code> which is <code>endSegment()</code> on
	 * failure.
	 */
	segment_iterator_t splitAfter(U8* address);

	/** 
	 * @brief Get the first segment in the buffer array.
	 *
	 * @return Returns the segment if there is one.
	 */
	segment_iterator_t beginSegment();

	/** 
	 * @brief Get the one-past-the-end segment in the buffer array
	 *
	 * @return Returns the iterator for an invalid segment location.
	 */
	segment_iterator_t endSegment();

	/** 
	 * @brief Get the segment which holds the given address.
	 *
	 * As opposed to some methods, passing a NULL will result in
	 * returning the end segment.
	 * @param address An address in the middle of the sought segment.
	 * @return Returns the iterator for the segment or endSegment() on
	 * failure.
	 */
	const_segment_iterator_t getSegment(U8* address) const;

	/** 
	 * @brief Get the segment which holds the given address.
	 *
	 * As opposed to some methods, passing a NULL will result in
	 * returning the end segment.
	 * @param address An address in the middle of the sought segment.
	 * @return Returns the iterator for the segment or endSegment() on
	 * failure.
	 */
	segment_iterator_t getSegment(U8* address);

	/** 
	 * @brief Get a segment iterator after address, and a constructed
	 * segment to represent the next linear block of memory.
	 *
	 * This method is a helper by giving you the largest segment
	 * possible in the out-value param after the address provided. The
	 * iterator will be useful for iteration, while the segment can be
	 * used for direct access to memory after address if the return
	 * values isnot end. Passing in NULL will return beginSegment()
	 * which may be endSegment(). The segment returned will only be
	 * zero length if the return value equals end.
	 * This is really just a helper method, since all the information
	 * returned could be constructed through other methods.
	 * @param address An address in the middle of the sought segment.
	 * @param segment[out] segment to be used for reading or writing
	 * @return Returns an iterator which contains at least segment or
	 * endSegment() on failure.
	 */
	segment_iterator_t constructSegmentAfter(U8* address, LLSegment& segment);

	/** 
	 * @brief Make a new segment at the end of buffer array
	 *
	 * This method will attempt to create a new and empty segment of
	 * the specified length. The segment created may be shorter than
	 * requested.
	 * @param channel[in] The channel for the newly created segment.
	 * @param length[in] The requested length of the segment.
	 * @return Returns an iterator which contains at least segment or
	 * endSegment() on failure.
	 */
	segment_iterator_t makeSegment(S32 channel, S32 length);

	/** 
	 * @brief Erase the segment if it is in the buffer array.
	 *
	 * @param iter An iterator referring to the segment to erase.
	 * @return Returns true on success.
	 */
	bool eraseSegment(const segment_iterator_t& iter);

	/**
	* @brief Lock the mutex if it exists
	* This method locks mMutexp to make accessing LLBufferArray thread-safe
	*/
	void lock();

	/**
	* @brief Unlock the mutex if it exists
	*/
	void unlock();

	/**
	* @brief Return mMutexp
	*/
	LLMutex* getMutex();

	/**
	* @brief Set LLBufferArray to be shared across threads or not
	* This method is to create mMutexp if is threaded.
	* @param threaded Indicates this LLBufferArray instance is shared across threads if true.
	*/
	void setThreaded(bool threaded);
	//@}

protected:
	/** 
	 * @brief Optimally put data in buffers, and reutrn segments.
	 *
	 * This is an internal function used to create buffers as
	 * necessary, and sequence the segments appropriately for the
	 * various ways to copy data from src into this. 
	 * If this method fails, it may actually leak some space inside
	 * buffers, but I am not too worried about the slim possibility
	 * that we may have some 'dead' space which will be recovered when
	 * the buffer (which we will not lose) is deleted. Addressing this
	 * weakness will make the buffers almost as complex as a general
	 * memory management system.
	 * @param channel The channel for this data
	 * @param src The start of memory for the data to be copied
	 * @param len The number of bytes of data to copy
	 * @param segments Out-value for the segments created.
	 * @return Returns true if the method worked.
	 */
	bool copyIntoBuffers(
		S32 channel,
		const U8* src,
		S32 len,
		std::vector<LLSegment>& segments);

protected:
	S32 mNextBaseChannel;
	buffer_list_t mBuffers;
	segment_list_t mSegments;
	LLMutex* mMutexp;
};

#endif // LL_LLBUFFER_H
