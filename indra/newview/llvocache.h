/** 
 * @file llvocache.h
 * @brief Cache of objects on the viewer.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOCACHE_H
#define LL_LLVOCACHE_H

#include "lluuid.h"
#include "lldatapacker.h"
#include "lldlinked.h"


//---------------------------------------------------------------------------
// Cache entries
class LLVOCacheEntry;

class LLVOCacheEntry : public LLDLinked<LLVOCacheEntry>
{
public:
	LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp);
	LLVOCacheEntry(FILE *fp);
	LLVOCacheEntry();
	~LLVOCacheEntry();

	U32 getLocalID() const			{ return mLocalID; }
	U32 getCRC() const				{ return mCRC; }
	S32 getHitCount() const			{ return mHitCount; }
	S32 getCRCChangeCount() const	{ return mCRCChangeCount; }

	void dump() const;
	void writeToFile(FILE *fp) const;
	void assignCRC(U32 crc, LLDataPackerBinaryBuffer &dp);
	LLDataPackerBinaryBuffer *getDP(U32 crc);
	void recordHit();
	void recordDupe() { mDupeCount++; }

protected:
	U32							mLocalID;
	U32							mCRC;
	S32							mHitCount;
	S32							mDupeCount;
	S32							mCRCChangeCount;
	LLDataPackerBinaryBuffer	mDP;
	U8							*mBuffer;
};

#endif
