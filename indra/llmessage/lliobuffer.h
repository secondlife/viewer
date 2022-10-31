/** 
 * @file lliobuffer.h
 * @author Phoenix
 * @date 2005-05-04
 * @brief Declaration of buffers for use in IO Pipes.
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

#ifndef LL_LLIOBUFFER_H
#define LL_LLIOBUFFER_H

#include "lliopipe.h"

/** 
 * @class LLIOBuffer
 * @brief This class is an io class that represents an automtically
 * resizing io buffer.
 * @see LLIOPipe
 *
 * This class is currently impelemented quick and dirty, but should be
 * correct. This class should be extended to have a more flexible
 * (and capped) memory allocation and usage scheme. Eventually, I
 * would like to have the ability to share this buffer between
 * different objects.
 */
class LLIOBuffer : public LLIOPipe
{
public:
    LLIOBuffer();
    virtual ~LLIOBuffer();

    /** 
     * @brief Return a raw pointer to the current data set.
     *
     * The pointer returned can be used for reading or even adjustment
     * if you are a bit crazy up to size() bytes into memory.
     * @return A potentially NULL pointer to the raw buffer data
     */
    U8* data() const;

    /** 
     * @brief Return the size of the buffer
     */
    S64 size() const;

    /** 
     * @brief Return a raw pointer to the current read position in the data.
     *
     * The pointer returned can be used for reading or even adjustment
     * if you are a bit crazy up to bytesLeft() bytes into memory.
     * @return A potentially NULL pointer to the buffer data starting
     * at the read point
     */
    U8* current() const;

    /** 
     * @brief Return the number of unprocessed bytes in buffer.
     */
    S64 bytesLeft() const;

    /** 
     * @brief Move the buffer offsets back to the beginning.
     *
     * This method effectively clears what has been stored here,
     * without mucking around with memory allocation.
     */
    void clear();

    /** 
     * @brief Enumeration passed into the seek function
     *
     * The READ head is used for where to start processing data for
     * the next link in the chain, while the WRITE head specifies
     * where new data processed from the previous link in the chain
     * will be written.
     */
    enum EHead
    {
        READ,
        WRITE
    };

    /** 
     * @brief Seek to a place in the buffer
     *
     * @param head The READ or WRITE head.
     * @param delta The offset from the current position to seek.
     * @return The status of the operation. status >= if head moved.
     */
    EStatus seek(EHead head, S64 delta);

public:
    /* @name LLIOPipe virtual implementations
     */
    //@{
protected:
    /** 
     * @brief Process the data in buffer
     */
    virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);
    //@}

protected:
    U8* mBuffer;
    S64 mBufferSize;
    U8* mReadHead;
    U8* mWriteHead;
};

#endif // LL_LLIOBUFFER_H
