/**
 * @file bufferstream.cpp
 * @brief Implements the BufferStream adapter class
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

#include "bufferstream.h"

#include "bufferarray.h"


namespace LLCore
{

BufferArrayStreamBuf::BufferArrayStreamBuf(BufferArray * array)
	: mBufferArray(array),
	  mReadCurPos(0),
	  mReadCurBlock(-1),
	  mReadBegin(NULL),
	  mReadCur(NULL),
	  mReadEnd(NULL),
	  mWriteCurPos(0)
{
	if (array)
	{
		array->addRef();
		mWriteCurPos = array->mLen;
	}
}


BufferArrayStreamBuf::~BufferArrayStreamBuf()
{
	if (mBufferArray)
	{
		mBufferArray->release();
		mBufferArray = NULL;
	}
}

	
BufferArrayStreamBuf::int_type BufferArrayStreamBuf::underflow()
{
	if (! mBufferArray)
	{
		return traits_type::eof();
	}

	if (mReadCur == mReadEnd)
	{
		// Find the next block with actual data or leave
		// mCurBlock/mCur/mEnd unchanged if we're at the end
		// of any block chain.
		const char * new_begin(NULL), * new_end(NULL);
		int new_cur_block(mReadCurBlock + 1);

		while (mBufferArray->getBlockStartEnd(new_cur_block, &new_begin, &new_end))
		{
			if (new_begin != new_end)
			{
				break;
			}
			++new_cur_block;
		}
		if (new_begin == new_end)
		{
			return traits_type::eof();
		}

		mReadCurBlock = new_cur_block;
		mReadBegin = mReadCur = new_begin;
		mReadEnd = new_end;
	}

	return traits_type::to_int_type(*mReadCur);
}


BufferArrayStreamBuf::int_type BufferArrayStreamBuf::uflow()
{
	const int_type ret(underflow());

	if (traits_type::eof() != ret)
	{
		++mReadCur;
		++mReadCurPos;
	}
	return ret;
}


BufferArrayStreamBuf::int_type BufferArrayStreamBuf::pbackfail(int_type ch)
{
	if (! mBufferArray)
	{
		return traits_type::eof();
	}

	if (mReadCur == mReadBegin)
	{
		// Find the previous block with actual data or leave
		// mCurBlock/mBegin/mCur/mEnd unchanged if we're at the
		// beginning of any block chain.
		const char * new_begin(NULL), * new_end(NULL);
		int new_cur_block(mReadCurBlock - 1);

		while (mBufferArray->getBlockStartEnd(new_cur_block, &new_begin, &new_end))
		{
			if (new_begin != new_end)
			{
				break;
			}
			--new_cur_block;
		}
		if (new_begin == new_end)
		{
			return traits_type::eof();
		}

		mReadCurBlock = new_cur_block;
		mReadBegin = new_begin;
		mReadEnd = mReadCur = new_end;
	}

	if (traits_type::eof() != ch && mReadCur[-1] != ch)
	{
		return traits_type::eof();
	}
	--mReadCurPos;
	return traits_type::to_int_type(*--mReadCur);
}


std::streamsize BufferArrayStreamBuf::showmanyc()
{
	if (! mBufferArray)
	{
		return -1;
	}
	return mBufferArray->mLen - mReadCurPos;
}


BufferArrayStreamBuf::int_type BufferArrayStreamBuf::overflow(int c)
{
	if (! mBufferArray || mWriteCurPos > mBufferArray->mLen)
	{
		return traits_type::eof();
	}
	const size_t wrote(mBufferArray->write(mWriteCurPos, &c, 1));
	mWriteCurPos += wrote;
	return wrote ? c : traits_type::eof();
}


std::streamsize BufferArrayStreamBuf::xsputn(const char * src, std::streamsize count)
{
	if (! mBufferArray || mWriteCurPos > mBufferArray->mLen)
	{
		return 0;
	}
	const size_t wrote(mBufferArray->write(mWriteCurPos, src, count));
	mWriteCurPos += wrote;
	return wrote;
}


std::streampos BufferArrayStreamBuf::seekoff(std::streamoff off,
											 std::ios_base::seekdir way,
											 std::ios_base::openmode which)
{
	std::streampos ret(-1);

	if (! mBufferArray)
	{
		return ret;
	}
	
	if (std::ios_base::in == which)
	{
		size_t pos(0);

		switch (way)
		{
		case std::ios_base::beg:
			pos = off;
			break;
			
		case std::ios_base::cur:
			pos = mReadCurPos += off;
			break;
			
		case std::ios_base::end:
			pos = mBufferArray->mLen - off;
			break;

		default:
			return ret;
		}

		if (pos >= mBufferArray->size())
		{
			pos = (std::max)(size_t(0), mBufferArray->size() - 1);
		}
		size_t ba_offset(0);
		int block(mBufferArray->findBlock(pos, &ba_offset));
		if (block < 0)
			return ret;
		const char * start(NULL), * end(NULL);
		if (! mBufferArray->getBlockStartEnd(block, &start, &end))
			return ret;
		mReadCurBlock = block;
		mReadBegin = start;
		mReadCur = start + ba_offset;
		mReadEnd = end;
		ret = mReadCurPos = pos;
	}
	else if (std::ios_base::out == which)
	{
		size_t pos(0);

		switch (way)
		{
		case std::ios_base::beg:
			pos = off;
			break;
			
		case std::ios_base::cur:
			pos = mWriteCurPos += off;
			break;
			
		case std::ios_base::end:
			pos = mBufferArray->mLen - off;
			break;

		default:
			return ret;
		}

		if (pos < 0)
			return ret;
		if (pos > mBufferArray->size())
		{
			pos = mBufferArray->size();
		}
		ret = mWriteCurPos = pos;
	}

	return ret;
}


BufferArrayStream::BufferArrayStream(BufferArray * ba)
	: std::iostream(&mStreamBuf),
	  mStreamBuf(ba)
{}
	
			
BufferArrayStream::~BufferArrayStream()
{}
	

}  // end namespace LLCore


