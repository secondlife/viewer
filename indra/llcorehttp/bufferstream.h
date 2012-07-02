/**
 * @file bufferstream.h
 * @brief Public-facing declaration for the BufferStream adapter class
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

#ifndef	_LLCORE_BUFFER_STREAM_H_
#define	_LLCORE_BUFFER_STREAM_H_


#include <sstream>
#include <cstdlib>

#include "bufferarray.h"


/// @file bufferstream.h
///
/// std::streambuf and std::iostream adapters for BufferArray
/// objects.
///
/// BufferArrayStreamBuf inherits std::streambuf and implements
/// an unbuffered interface for streambuf.  This may or may not
/// be the most time efficient implementation and it is a little
/// challenging.
///
/// BufferArrayStream inherits std::iostream and will be the
/// adapter object most callers will be interested in (though
/// it uses BufferArrayStreamBuf internally).  Instances allow
/// for the usual streaming operators ('<<', '>>') and serialization
/// methods.
///
/// Example of LLSD serialization to a BufferArray:
///
///   BufferArray * ba = new BufferArray;
///   BufferArrayStream bas(ba);
///   LLSDSerialize::toXML(llsd, bas);
///   operationOnBufferArray(ba);
///   ba->release();
///   ba = NULL;
///   // operationOnBufferArray and bas are each holding
///   // references to the ba instance at this point.
///

namespace LLCore
{


// =====================================================
// BufferArrayStreamBuf
// =====================================================

/// Adapter class to put a std::streambuf interface on a BufferArray
///
/// Application developers will rarely be interested in anything
/// other than the constructor and even that will rarely be used
/// except indirectly via the @BufferArrayStream class.  The
/// choice of interfaces implemented yields a bufferless adapter
/// that doesn't used either the input or output pointer triplets
/// of the more common buffered implementations.  This may or may
/// not be faster and that question could stand to be looked at
/// sometime.
///

class BufferArrayStreamBuf : public std::streambuf
{
public:
	/// Constructor increments the reference count on the
	/// BufferArray argument and calls release() on destruction.
	BufferArrayStreamBuf(BufferArray * array);
	virtual ~BufferArrayStreamBuf();

private:
	BufferArrayStreamBuf(const BufferArrayStreamBuf &);	// Not defined
	void operator=(const BufferArrayStreamBuf &);		// Not defined

public:
	// Input interfaces from std::streambuf
	int_type underflow();
	int_type uflow();
	int_type pbackfail(int_type ch);
	std::streamsize showmanyc();

	// Output interfaces from std::streambuf
	int_type overflow(int c);
	std::streamsize xsputn(const char * src, std::streamsize count);

	// Common/misc interfaces from std::streambuf
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
	
protected:
	BufferArray *		mBufferArray;			// Ref counted
	size_t				mReadCurPos;
	int					mReadCurBlock;
	const char *		mReadBegin;
	const char *		mReadCur;
	const char *		mReadEnd;
	size_t				mWriteCurPos;
	
}; // end class BufferArrayStreamBuf


// =====================================================
// BufferArrayStream
// =====================================================

/// Adapter class that supplies streaming operators to BufferArray
///
/// Provides a streaming adapter to an existing BufferArray
/// instance so that the convenient '<<' and '>>' conversions
/// can be applied to a BufferArray.  Very convenient for LLSD
/// serialization and parsing as well.

class BufferArrayStream : public std::iostream
{
public:
	/// Constructor increments the reference count on the
	/// BufferArray argument and calls release() on destruction.
	BufferArrayStream(BufferArray * ba);
	~BufferArrayStream();

protected:
	BufferArrayStream(const BufferArrayStream &);
	void operator=(const BufferArrayStream &);

protected:
	BufferArrayStreamBuf		mStreamBuf;
}; // end class BufferArrayStream


}  // end namespace LLCore

#endif	// _LLCORE_BUFFER_STREAM_H_
