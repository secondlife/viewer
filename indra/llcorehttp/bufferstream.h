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


namespace LLCore
{


class BufferArrayStreamBuf : public std::streambuf
{
public:
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


class BufferArrayStream : public std::iostream
{
public:
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
