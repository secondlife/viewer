/** 
 * @file lliobuffer.cpp
 * @author Phoenix
 * @date 2005-05-04
 * @brief Definition of buffer based implementations of IO Pipes.
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
           || ((delta < 0) && ((mReadHead + delta) >= mBuffer)))
        {
            mReadHead += delta;
            status = STATUS_OK;
        }
        break;
    case WRITE:
        if(((delta >= 0) && ((mWriteHead + delta) < (mBuffer + mBufferSize)))
           || ((delta < 0) && ((mWriteHead + delta) > mReadHead)))
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
    LL_WARNS() << "You are using an LLIOBuffer which is deprecated." << LL_ENDL;
    return STATUS_OK;
}
