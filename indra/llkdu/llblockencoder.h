/** 
 * @file llblockencoder.h
 * @brief Image block compression
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

#ifndef LL_LLBLOCKENCODER_H
#define LL_LLBLOCKENCODER_H

#include "stdtypes.h"

class LLBlockData;
class LLBlockDataU32;
class LLBlockDataF32;

class LLBlockEncoder
{
	F32 mBPP; // bits per point
public:
	LLBlockEncoder();
	U8 *encode(const LLBlockData &block_data, U32 &output_size) const;
	U8 *encodeU32(const LLBlockDataU32 &block_data, U32 &output_size) const;
	U8 *encodeF32(const LLBlockDataF32 &block_data, U32 &output_size) const;

	void setBPP(const F32 bpp);
};

#endif // LL_LLBLOCKENCODER_H

