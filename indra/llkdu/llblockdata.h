/** 
 * @file llblockdata.h
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

#ifndef LL_LLBLOCKDATA_H
#define LL_LLBLOCKDATA_H

#include "stdtypes.h"

//////////////////////////////////////////////////
//
//  This class stores all of the information about
//  a single channel of raw data, either integer
//	or floating point.
//
class LLBlockData
{
protected:
	U32 mType;
	U32 mWidth;
	U32 mHeight;
	U32 mRowStride;
	U8 *mData;
public:
	enum
	{
		BLOCK_TYPE_U32 = 1,
		BLOCK_TYPE_F32 = 2
	};

	LLBlockData(const U32 type);
	virtual ~LLBlockData() {}

	void setData(U8 *data, const U32 width, const U32 height, const U32 row_stride = 0);

	U32 getType() const;
	U8 *getData() const;
	virtual U32 getSize() const;
	U32 getWidth() const;
	U32 getHeight() const;
	U32 getRowStride() const;
};

class LLBlockDataU32 : public LLBlockData
{
protected:
	U32 mPrecision;
public:
	LLBlockDataU32();

	void setData(U32 *data, const U32 width, const U32 height, const U32 row_stride = 0);
	void setPrecision(const U32 bits);

	/*virtual*/ U32 getSize() const;
	U32 getPrecision() const;
};

class LLBlockDataF32 : public LLBlockData
{
protected:
	U32 mPrecision;
	F32 mMin;
	F32 mMax;
public:
	LLBlockDataF32()
		: LLBlockData(LLBlockData::BLOCK_TYPE_F32),
		  mPrecision(0),
		  mMin(0.f),
		  mMax(0.f)
	{};
	
	void setData(F32 *data, const U32 width, const U32 height, const U32 row_stride = 0);

	void setPrecision(const U32 bits);
	void setMin(const F32 min);
	void setMax(const F32 max);

	void calcMinMax();

	U32 getPrecision() const;
	F32 getMin() const;
	F32 getMax() const;
};

#endif // LL_LLBLOCKDATA_H
