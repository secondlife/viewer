/**
 * @file bufferarray.h
 * @brief Public-facing declaration for the BufferArray scatter/gather class
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

#ifndef	_LLCORE_BUFFER_ARRAY_H_
#define	_LLCORE_BUFFER_ARRAY_H_


#include <cstdlib>
#include <vector>

#include "_refcounted.h"


namespace LLCore
{

class BufferArrayStreamBuf;

/// A very simple scatter/gather type map for bulk data.  The motivation
/// for this class is the writedata callback used by libcurl.  Response
/// bodies are delivered to the caller in a sequence of sequential write
/// operations and this class captures them without having to reallocate
/// and move data.
///
/// The interface looks a little like a unix file descriptor but only
/// just.  There is a notion of a current position, starting from 0,
/// which is used as the position in the data when performing read and
/// write operations.  The position also moves after various operations:
/// - seek(...)
/// - read(...)
/// - write(...)
/// - append(...)
/// - appendBufferAlloc(...)
/// The object also keeps a total length value which is updated after
/// write and append operations and beyond which the current position
/// cannot be set.
///
/// Threading:  not thread-safe
///
/// Allocation:  Refcounted, heap only.  Caller of the constructor
/// is given a single refcount.
///
class BufferArray : public LLCoreInt::RefCounted
{
public:
	// BufferArrayStreamBuf has intimate knowledge of this
	// implementation to implement a buffer-free adapter.
	// Changes here will likely need to be reflected there.
	friend class BufferArrayStreamBuf;
	
	BufferArray();

protected:
	virtual ~BufferArray();						// Use release()

private:
	BufferArray(const BufferArray &);			// Not defined
	void operator=(const BufferArray &);		// Not defined

public:
	// Internal magic number, may be used by unit tests.
	static const size_t BLOCK_ALLOC_SIZE = 65540;
	
	/// Appends the indicated data to the BufferArray
	/// modifying current position and total size.  New
	/// position is one beyond the final byte of the buffer.
	///
	/// @return			Count of bytes copied to BufferArray
	size_t append(const void * src, size_t len);

	/// Similar to @see append(), this call guarantees a
	/// contiguous block of memory of requested size placed
	/// at the current end of the BufferArray.  On return,
	/// the data in the memory is considered valid whether
	/// the caller writes to it or not.
	///
	/// @return			Pointer to contiguous region at end
	///					of BufferArray of 'len' size.
	void * appendBufferAlloc(size_t len);

	/// Current count of bytes in BufferArray instance.
	size_t size() const
		{
			return mLen;
		}

	/// Copies data from the given position in the instance
	/// to the caller's buffer.  Will return a short count of
	/// bytes copied if the 'len' extends beyond the data.
	size_t read(size_t pos, void * dst, size_t len);

	/// Copies data from the caller's buffer to the instance
	/// at the current position.  May overwrite existing data,
	/// append data when current position is equal to the
	/// size of the instance or do a mix of both.
	size_t write(size_t pos, const void * src, size_t len);
	
protected:
	int findBlock(size_t pos, size_t * ret_offset);

	bool getBlockStartEnd(int block, const char ** start, const char ** end);
	
protected:
	class Block;
	typedef std::vector<Block *> container_t;

	container_t			mBlocks;
	size_t				mLen;
};  // end class BufferArray


}  // end namespace LLCore

#endif	// _LLCORE_BUFFER_ARRAY_H_
