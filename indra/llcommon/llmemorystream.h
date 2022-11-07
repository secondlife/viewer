/** 
 * @file llmemorystream.h
 * @author Phoenix
 * @date 2005-06-03
 * @brief Implementation of a simple fixed memory stream
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

#ifndef LL_LLMEMORYSTREAM_H
#define LL_LLMEMORYSTREAM_H

/** 
 * This is a simple but effective optimization when you want to treat
 * a chunk of memory as an istream. I wrote this to avoid turing a
 * buffer into a string, and then throwing the string into an
 * iostringstream just to parse it into another datatype, eg, LLSD.
 */

#include <iostream>

/** 
 * @class LLMemoryStreamBuf
 * @brief This implements a wrapper around a piece of memory for istreams
 *
 * The memory passed in is NOT owned by an instance. The caller must
 * be careful to always pass in a valid memory location that exists
 * for at least as long as this streambuf.
 */
class LL_COMMON_API LLMemoryStreamBuf : public std::streambuf
{
public:
    LLMemoryStreamBuf(const U8* start, S32 length);
    ~LLMemoryStreamBuf();

    void reset(const U8* start, S32 length);

protected:
    int underflow();
    //std::streamsize xsgetn(char* dest, std::streamsize n);
};


/** 
 * @class LLMemoryStream
 * @brief This implements a wrapper around a piece of memory for istreams
 *
 * The memory passed in is NOT owned by an instance. The caller must
 * be careful to always pass in a valid memory location that exists
 * for at least as long as this streambuf.
 */
class LL_COMMON_API LLMemoryStream : public std::istream
{
public:
    LLMemoryStream(const U8* start, S32 length);
    ~LLMemoryStream();

protected:
    LLMemoryStreamBuf mStreamBuf;
};

#endif // LL_LLMEMORYSTREAM_H
