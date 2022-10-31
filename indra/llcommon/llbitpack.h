/** 
 * @file llbitpack.h
 * @brief Convert data to packed bit stream
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_BITPACK_H
#define LL_BITPACK_H

#include "llerror.h"

const U32 MAX_DATA_BITS = 8;


class LLBitPack
{
public:
    LLBitPack(U8 *buffer, U32 max_size) : mBuffer(buffer), mBufferSize(0), mLoad(0), mLoadSize(0), mTotalBits(0), mMaxSize(max_size)
    {
    }

    ~LLBitPack()
    {
    }

    void resetBitPacking()
    {
        mLoad = 0;
        mLoadSize = 0;
        mTotalBits = 0;
        mBufferSize = 0;
    }

    U32 bitPack(U8 *total_data, U32 total_dsize)
    {
        U32 dsize;
        U8 data;

        while (total_dsize > 0)
        {
            if (total_dsize > MAX_DATA_BITS)
            {
                dsize = MAX_DATA_BITS;
                total_dsize -= MAX_DATA_BITS;
            }
            else
            {
                dsize = total_dsize;
                total_dsize = 0;
            }

            data = *total_data++;

            data <<= (MAX_DATA_BITS - dsize);
            while (dsize > 0) 
            {
                if (mLoadSize == MAX_DATA_BITS) 
                {
                    *(mBuffer + mBufferSize++) = mLoad;
                    if (mBufferSize > mMaxSize)
                    {
                        LL_ERRS() << "mBufferSize exceeding mMaxSize!" << LL_ENDL;
                    }
                    mLoadSize = 0;
                    mLoad = 0x00;
                }
                mLoad <<= 1;            
                mLoad |= (data >> (MAX_DATA_BITS - 1));
                data <<= 1;
                mLoadSize++;
                mTotalBits++;
                dsize--;
            }
        }
        return mBufferSize;
    }

    U32 bitCopy(U8 *total_data, U32 total_dsize)
    {
        U32 dsize;
        U8  data;

        while (total_dsize > 0)
        {
            if (total_dsize > MAX_DATA_BITS)
            {
                dsize = MAX_DATA_BITS;
                total_dsize -= MAX_DATA_BITS;
            }
            else
            {
                dsize = total_dsize;
                total_dsize = 0;
            }

            data = *total_data++;

            while (dsize > 0) 
            {
                if (mLoadSize == MAX_DATA_BITS) 
                {
                    *(mBuffer + mBufferSize++) = mLoad;
                    if (mBufferSize > mMaxSize)
                    {
                        LL_ERRS() << "mBufferSize exceeding mMaxSize!" << LL_ENDL;
                    }
                    mLoadSize = 0;
                    mLoad = 0x00;
                }
                mLoad <<= 1;            
                mLoad |= (data >> (MAX_DATA_BITS - 1));
                data <<= 1;
                mLoadSize++;
                mTotalBits++;
                dsize--;
            }
        }
        return mBufferSize;
    }

    U32 bitUnpack(U8 *total_retval, U32 total_dsize)
    {
        U32 dsize;
        U8  *retval;

        while (total_dsize > 0)
        {
            if (total_dsize > MAX_DATA_BITS)
            {
                dsize = MAX_DATA_BITS;
                total_dsize -= MAX_DATA_BITS;
            }
            else
            {
                dsize = total_dsize;
                total_dsize = 0;
            }

            retval = total_retval++;
            *retval = 0x00;
            while (dsize > 0) 
            {
                if (mLoadSize == 0) 
                {
#ifdef _DEBUG
                    if (mBufferSize > mMaxSize)
                    {
                        LL_ERRS() << "mBufferSize exceeding mMaxSize" << LL_ENDL;
                        LL_ERRS() << mBufferSize << " > " << mMaxSize << LL_ENDL;
                    }
#endif
                    mLoad = *(mBuffer + mBufferSize++);
                    mLoadSize = MAX_DATA_BITS;
                }
                *retval <<= 1;
                *retval |= (mLoad >> (MAX_DATA_BITS - 1));
                mLoadSize--;
                mLoad <<= 1;
                dsize--;
            }
        }
        return mBufferSize;
    }

    U32 flushBitPack()
    {
        if (mLoadSize) 
        {
            mLoad <<= (MAX_DATA_BITS - mLoadSize);
            *(mBuffer + mBufferSize++) = mLoad;
            if (mBufferSize > mMaxSize)
            {
                LL_ERRS() << "mBufferSize exceeding mMaxSize!" << LL_ENDL;
            }
            mLoadSize = 0;
        }
        return mBufferSize;
    }

    U8      *mBuffer;
    U32     mBufferSize;
    U8      mLoad;
    U32     mLoadSize;
    U32     mTotalBits;
    U32     mMaxSize;
};

#endif
