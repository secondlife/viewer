/** 
 * @file llioutil.h
 * @author Phoenix
 * @date 2005-10-05
 * @brief Helper classes for dealing with IOPipes
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

#ifndef LL_LLIOUTIL_H
#define LL_LLIOUTIL_H

#include "llbuffer.h"
#include "lliopipe.h"
#include "llpumpio.h"

/** 
 * @class LLIOFlush
 * @brief This class is used as a mini chain head which drains the buffer.
 * @see LLIOPipe
 *
 * An instance of this class acts as a useful chain head when all data
 * is known, and you simply want to get the chain moving.
 */
class LLIOFlush : public LLIOPipe
{
public:
    LLIOFlush() {}
    virtual ~LLIOFlush() {}

protected:
    /* @name LLIOPipe virtual implementations
     */
    //@{
    /** 
     * @brief Process the data in buffer
     */
    EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
    //@}
protected:
};

/** 
 * @class LLIOSleep
 * @brief This is a simple helper class which will hold a chain and
 * process it later using pump mechanisms
 * @see LLIOPipe
 */
class LLIOSleep : public LLIOPipe
{
public:
    LLIOSleep(F64 sleep_seconds) : mSeconds(sleep_seconds) {}
    virtual ~LLIOSleep() {}

protected:
    /* @name LLIOPipe virtual implementations
     */
    //@{
    /** 
     * @brief Process the data in buffer
     */
    EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
    //@}
protected:
    F64 mSeconds;
};

/** 
 * @class LLIOAddChain
 * @brief Simple pipe that just adds a chain to a pump.
 * @see LLIOPipe
 */
class LLIOAddChain : public LLIOPipe
{
public:
    LLIOAddChain(const LLPumpIO::chain_t& chain, F32 timeout) :
        mChain(chain),
        mTimeout(timeout)
    {}
    virtual ~LLIOAddChain() {}

protected:
    /* @name LLIOPipe virtual implementations
     */
    //@{
    /** 
     * @brief Process the data in buffer
     */
    EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
    //@}

protected:
    LLPumpIO::chain_t mChain;
    F32 mTimeout;
};

/** 
 * @class LLChangeChannel
 * @brief This class changes the channel of segments in the buffer
 * @see LLBufferArray
 *
 * This class is useful for iterating over the segments in a buffer
 * array and changing each channel that matches to a different
 * channel.
 * Example:
 * <code>
 * set_in_to_out(LLChannelDescriptors channels, LLBufferArray* buf)
 * {
 *   std::for_each(
 *     buf->beginSegment(),
 *     buf->endSegment(),
 *     LLChangeChannel(channels.in(), channels.out()));
 * }
 * </code>
 */
class LLChangeChannel //: public unary_function<T, void>
{
public:
    /** 
     * @brief Constructor for iterating over a segment range to change channel.
     *
     * @param is The channel to match when looking at a segment.
     * @param becomes The channel to set the segment when a match is found.
     */
    LLChangeChannel(S32 is, S32 becomes);

    /** 
     * @brief Do the work of changing the channel
     */
    void operator()(LLSegment& segment);

protected:
    S32 mIs;
    S32 mBecomes;
};


#endif // LL_LLIOUTIL_H
