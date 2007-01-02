/** 
 * @file llvlmanager.h
 * @brief LLVLManager class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVLMANAGER_H
#define LL_LLVLMANAGER_H

// This class manages the data coming in for viewer layers from the network.

#include "stdtypes.h"
#include "lldarray.h"

class LLVLData;
class LLViewerRegion;

class LLVLManager
{
public:
	~LLVLManager();

	void addLayerData(LLVLData *vl_datap, const S32 mesg_size);

	void unpackData(const S32 num_packets = 10);

	S32 getTotalBytes() const;

	S32 getLandBits() const;
	S32 getWindBits() const;
	S32 getCloudBits() const;

	void resetBitCounts();

	void cleanupData(LLViewerRegion *regionp);
protected:

	LLDynamicArray<LLVLData *> mPacketData;
	U32 mLandBits;
	U32 mWindBits;
	U32 mCloudBits;
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
