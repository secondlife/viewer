/** 
 * @file llvlmanager.h
 * @brief LLVLManager class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVLMANAGER_H
#define LL_LLVLMANAGER_H

// This class manages the data coming in for viewer layers from the network.

#include "stdtypes.h"

class LLVLData;
class LLViewerRegion;

class LLVLManager
{
public:
	~LLVLManager();

	void addLayerData(LLVLData *vl_datap, const S32Bytes mesg_size);

	void unpackData(const S32 num_packets = 10);

	S32Bytes getTotalBytes() const;

	U32Bits getLandBits() const;
	U32Bits getWindBits() const;
	U32Bits getCloudBits() const;

	void resetBitCounts();

	void cleanupData(LLViewerRegion *regionp);
protected:

	std::vector<LLVLData *> mPacketData;
	U32Bits mLandBits;
	U32Bits mWindBits;
	U32Bits mCloudBits;
};

class LLVLData
{
public:
	LLVLData(LLViewerRegion *regionp,
			 const S8 type, U8 *data, const S32 size);
	~LLVLData();

	S8 mType;
	U8 *mData;
	S32 mSize;
	LLViewerRegion *mRegionp;
};

extern LLVLManager gVLManager;

#endif // LL_LLVIEWERLAYERMANAGER_H
