/** 
 * @file llpipeutil.h
 * @date 2006-05-18
 * @brief Utility pipe fittings for injecting and extracting strings
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLPIPEUTIL_H
#define LL_LLPIPEUTIL_H

#include "lliopipe.h"


/**
 * @brief Simple function which pumps for the specified time.
 */
F32 pump_loop(LLPumpIO* pump, F32 seconds);

/**
 * @brief Simple class which writes a string and then marks the stream
 * as done.
 */
class LLPipeStringInjector : public LLIOPipe
{
public:
    LLPipeStringInjector(const std::string& string)
        : mString(string)
        { }
      
protected:
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

private:
    std::string mString;
};


class LLPipeStringExtractor : public LLIOPipe
{
public:
    LLPipeStringExtractor() : mDone(false) { }
    
    bool done() { return mDone; }
    std::string string() { return mString; }
    
protected:
    // LLIOPipe API implementation.
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        LLIOPipe::buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

private:
    bool mDone;
    std::string mString;
};

/**
 * @brief Generate a specified number of bytes of random data
 */
class LLIOFuzz : public LLIOPipe
{
public:
    LLIOFuzz(int byte_count) : mByteCount(byte_count) {}
      
protected:
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

private:
    int mByteCount;
};

/**
 * @brief Generate some ascii fuz
 */
class LLIOASCIIFuzz : public LLIOPipe
{
public:
    LLIOASCIIFuzz(int byte_count) : mByteCount(byte_count) {}
      
protected:
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

private:
    int mByteCount;
};


/**
 * @brief Pipe that does nothing except return STATUS_OK
 */
class LLIONull : public LLIOPipe
{
public:
    LLIONull() {}
      
protected:
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
};

/**
 * @brief Pipe that sleeps, and then responds later.
 */
class LLIOSleeper : public LLIOPipe
{
public:
    LLIOSleeper() : mRespond(false) {}
      
protected:
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
private:
    bool mRespond;

};

#endif // LL_LLPIPEUTIL_H
