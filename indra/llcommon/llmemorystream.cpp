/** 
 * @file llmemorystream.cpp
 * @author Phoenix
 * @date 2005-06-03
 * @brief Buffer and stream for a fixed linear memory segment.
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
#include "llmemorystream.h"

LLMemoryStreamBuf::LLMemoryStreamBuf(const U8* start, S32 length)
{
    reset(start, length);
}

LLMemoryStreamBuf::~LLMemoryStreamBuf()
{
}

void LLMemoryStreamBuf::reset(const U8* start, S32 length)
{
    setg((char*)start, (char*)start, (char*)start + length);
}

int LLMemoryStreamBuf::underflow()
{
    //LL_DEBUGS() << "LLMemoryStreamBuf::underflow()" << LL_ENDL;
    if(gptr() < egptr())
    {
        return *gptr();
    }
    return EOF;
}

/** 
 * @class LLMemoryStreamBuf
 */

LLMemoryStream::LLMemoryStream(const U8* start, S32 length) :
    std::istream(&mStreamBuf),
    mStreamBuf(start, length)
{
}

LLMemoryStream::~LLMemoryStream()
{
}


