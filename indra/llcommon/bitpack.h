/** 
 * @file bitpack.h
 * @brief Convert data to packed bit stream
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
						llerror("mBufferSize exceeding mMaxSize!", 0);
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
						llerror("mBufferSize exceeding mMaxSize!", 0);
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
		U8	*retval;

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
						llerrs << "mBufferSize exceeding mMaxSize" << llendl;
						llerrs << mBufferSize << " > " << mMaxSize << llendl;
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
				llerror("mBufferSize exceeding mMaxSize!", 0);
			}
			mLoadSize = 0;
		}
		return mBufferSize;
	}

	U8		*mBuffer;
	U32		mBufferSize;
	U8		mLoad;
	U32		mLoadSize;
	U32		mTotalBits;
	U32		mMaxSize;
};

#endif
