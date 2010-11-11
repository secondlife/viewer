/** 
 * @file llblockdata.cpp
 * @brief Image block structure
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llblockdata.h"
#include "llmath.h"

LLBlockData::LLBlockData(const U32 type)
{
	mType = type;
	mWidth = 0;
	mHeight = 0;
	mRowStride = 0;
	mData = NULL;
}

void LLBlockData::setData(U8 *data, const U32 width, const U32 height, const U32 row_stride)
{
	mData = data;
	mWidth = width;
	mHeight = height;
	if (row_stride)
	{
		mRowStride = row_stride;
	}
	else
	{
		mRowStride = width * 4;
	}
}

U32 LLBlockData::getType() const
{
	return mType;
}


U8 *LLBlockData::getData() const
{
	return mData;
}

U32 LLBlockData::getSize() const
{
	return mWidth*mHeight;
}

U32 LLBlockData::getWidth() const
{
	return mWidth;
}
U32 LLBlockData::getHeight() const
{
	return mHeight;
}

U32 LLBlockData::getRowStride() const
{
	return mRowStride;
}

LLBlockDataU32::LLBlockDataU32() : LLBlockData(BLOCK_TYPE_U32)
{
	mPrecision = 32;
}

void LLBlockDataU32::setData(U32 *data, const U32 width, const U32 height, const U32 row_stride)
{
	LLBlockData::setData((U8 *)data, width, height, row_stride);
}

U32 LLBlockDataU32::getSize() const
{
	return mWidth*mHeight*4;
}

void LLBlockDataU32::setPrecision(const U32 bits)
{
	mPrecision = bits;
}

U32 LLBlockDataU32::getPrecision() const
{
	return mPrecision;
}

void LLBlockDataF32::setPrecision(const U32 bits)
{
	mPrecision = bits;
}

U32 LLBlockDataF32::getPrecision() const
{
	return mPrecision;
}

void LLBlockDataF32::setData(F32 *data, const U32 width, const U32 height, const U32 row_stride)
{
	LLBlockData::setData((U8 *)data, width, height, row_stride);
}

void LLBlockDataF32::setMin(const F32 min)
{
	mMin = min;
}

void LLBlockDataF32::setMax(const F32 max)
{
	mMax = max;
}

void LLBlockDataF32::calcMinMax()
{
	U32 x, y;

	mMin = *(F32*)mData;
	mMax = mMin;

	for (y = 0; y < mHeight; y++)
	{
		for (x = 0; x < mWidth; x++)
		{
			F32 data = *(F32*)(mData + y*mRowStride + x*4);
			mMin = llmin(data, mMin);
			mMax = llmax(data, mMax);
		}
	}
}

F32 LLBlockDataF32::getMin() const
{
	return mMin;
}

F32 LLBlockDataF32::getMax() const
{
	return mMax;
}
