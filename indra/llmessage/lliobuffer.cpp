/** 
 * @file lliobuffer.cpp
 * @author Phoenix
 * @date 2005-05-04
 * @brief Definition of buffer based implementations of IO Pipes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
