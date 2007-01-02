/** 
 * @file llvocache.cpp
 * @brief Cache of objects on the viewer.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvocache.h"

#include "llerror.h"

//---------------------------------------------------------------------------
// LLVOCacheEntry
//---------------------------------------------------------------------------

LLVOCacheEntry::LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp)
{
	mLocalID = local_id;
	mCRC = crc;
	mHitCount = 0;
	mDupeCount = 0;
	mCRCChangeCount = 0;
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
{
	mLocalID = 0;
	mCRC = 0;
	mHitCount = 0;
	mDupeCount = 0;
	mCRCChangeCount = 0;
	mBuffer = NULL;
	mDP.assignBuffer(mBuffer, 0);
}


LLVOCacheEntry::LLVOCacheEntry(FILE *fp)
{
	S32 size;
	fread(&mLocalID, 1, sizeof(U32), fp);
	fread(&mCRC, 1, sizeof(U32), fp);
	fread(&mHitCount, 1, sizeof(S32), fp);
	fread(&mDupeCount, 1, sizeof(S32), fp);
	fread(&mCRCChangeCount, 1, sizeof(S32), fp);

	fread(&size, 1, sizeof(S32), fp);

	// Corruption in the cache entries
	if ((size > 10000) || (size < 1))
	{
		// We've got a bogus size, skip reading it.
		// We won't bother seeking, because the rest of this file
		// is likely bogus, and will be tossed anyway.
		llwarns << "Bogus cache entry, size " << size << ", aborting!" << llendl;
		mLocalID = 0;
		mCRC = 0;
		mBuffer = NULL;
		return;
	}

	mBuffer = new U8[size];
	fread(mBuffer, 1, size, fp);
	mDP.assignBuffer(mBuffer, size);
}

LLVOCacheEntry::~LLVOCacheEntry()
{
	delete [] mBuffer;
}


// New CRC means the object has changed.
void LLVOCacheEntry::assignCRC(U32 crc, LLDataPackerBinaryBuffer &dp)
{
	if (  (mCRC != crc)
		||(mDP.getBufferSize() == 0))
	{
		mCRC = crc;
		mHitCount = 0;
		mCRCChangeCount++;

		mDP.freeBuffer();
		mBuffer = new U8[dp.getBufferSize()];
		mDP.assignBuffer(mBuffer, dp.getBufferSize());
		mDP = dp;
	}
}

LLDataPackerBinaryBuffer *LLVOCacheEntry::getDP(U32 crc)
{
	if (  (mCRC != crc)
		||(mDP.getBufferSize() == 0))
	{
		//llinfos << "Not getting cache entry, invalid!" << llendl;
		return NULL;
	}
	mHitCount++;
	return &mDP;
}


void LLVOCacheEntry::recordHit()
{
	mHitCount++;
}


void LLVOCacheEntry::dump() const
{
	llinfos << "local " << mLocalID
		<< " crc " << mCRC
		<< " hits " << mHitCount
		<< " dupes " << mDupeCount
		<< " change " << mCRCChangeCount
		<< llendl;
}

void LLVOCacheEntry::writeToFile(FILE *fp) const
{
	fwrite(&mLocalID, 1, sizeof(U32), fp);
	fwrite(&mCRC, 1, sizeof(U32), fp);
	fwrite(&mHitCount, 1, sizeof(S32), fp);
	fwrite(&mDupeCount, 1, sizeof(S32), fp);
	fwrite(&mCRCChangeCount, 1, sizeof(S32), fp);
	S32 size = mDP.getBufferSize();
	fwrite(&size, 1, sizeof(S32), fp);
	fwrite(mBuffer, 1, size, fp);
}
