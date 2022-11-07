/** 
 * @file llpipeutil.cpp
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


#include "linden_common.h"
#include "llpipeutil.h"

#include <stdlib.h>
#include <sstream>

#include "llbufferstream.h"
#include "lldefs.h"
#include "llframetimer.h"
#include "llpumpio.h"
#include "llrand.h"
#include "lltimer.h"

F32 pump_loop(LLPumpIO* pump, F32 seconds)
{
    LLTimer timer;
    timer.setTimerExpirySec(seconds);
    while(!timer.hasExpired())
    {
        LLFrameTimer::updateFrameTime();            
        pump->pump();
        pump->callback();
    }
    return timer.getElapsedTimeF32();
}

//virtual 
LLIOPipe::EStatus LLPipeStringInjector::process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump)
{
    buffer->append(channels.out(), (U8*) mString.data(), mString.size());
    eos = true;
    return STATUS_DONE;
}


LLIOPipe::EStatus LLPipeStringExtractor::process_impl(
    const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    if(!eos) return STATUS_BREAK;
    if(!pump || !buffer) return STATUS_PRECONDITION_NOT_MET;

    LLBufferStream istr(channels, buffer.get());
    std::ostringstream ostr;
    while (istr.good())
    {
        char buf[1024];     /* Flawfinder: ignore */
        istr.read(buf, sizeof(buf));    /* Flawfinder: ignore */
        ostr.write(buf, istr.gcount());
    }
    mString = ostr.str();
    mDone = true;
    
    return STATUS_DONE;
}


// virtual
LLIOPipe::EStatus LLIOFuzz::process_impl(
    const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    while(mByteCount)
    {
        std::vector<U8> data;
        data.reserve(10000);
        int size = llmin(10000, mByteCount);
        std::generate_n(
            std::back_insert_iterator< std::vector<U8> >(data),
            size,
            rand);
        buffer->append(channels.out(), &data[0], size);
        mByteCount -= size;
    }
    return STATUS_OK;
}

struct random_ascii_generator
{
    random_ascii_generator() {}
    U8 operator()()
    {
        int rv = rand();
        rv %= (127 - 32);
        rv += 32;
        return rv;
    }
};

// virtual
LLIOPipe::EStatus LLIOASCIIFuzz::process_impl(
    const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    while(mByteCount)
    {
        std::vector<U8> data;
        data.reserve(10000);
        int size = llmin(10000, mByteCount);
        std::generate_n(
            std::back_insert_iterator< std::vector<U8> >(data),
            size,
            random_ascii_generator());
        buffer->append(channels.out(), &data[0], size);
        mByteCount -= size;
    }
    return STATUS_OK;
}

// virtual
LLIOPipe::EStatus LLIONull::process_impl(
    const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    return STATUS_OK;
}

// virtual
LLIOPipe::EStatus LLIOSleeper::process_impl(
    const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    if(!mRespond)
    {
        LL_DEBUGS() << "LLIOSleeper::process_impl() sleeping." << LL_ENDL;
        mRespond = true;
        static const F64 SLEEP_TIME = 2.0;
        pump->sleepChain(SLEEP_TIME);
        return STATUS_BREAK;
    }
    LL_DEBUGS() << "LLIOSleeper::process_impl() responding." << LL_ENDL;
    LLBufferStream ostr(channels, buffer.get());
    ostr << "huh? sorry, I was sleeping." << std::endl;
    return STATUS_DONE;
}
