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


static inline void checkedRead(FILE *fp, void *data, size_t nbytes)
{
	if (fread(data, 1, nbytes, fp) != nbytes)
	{
		llwarns << "Short read" << llendl;
		memset(data, 0, nbytes);
	}
}

LLVOCacheEntry::LLVOCacheEntry(FILE *fp)
{
	S32 size;
	checkedRead(fp, &mLocalID, sizeof(U32));
	checkedRead(fp, &mCRC, sizeof(U32));
	checkedRead(fp, &mHitCount, sizeof(S32));
	checkedRead(fp, &mDupeCount, sizeof(S32));
	checkedRead(fp, &mCRCChangeCount, sizeof(S32));

	checkedRead(fp, &size, sizeof(S32));

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
	checkedRead(fp, mBuffer, size);
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

static inline void checkedWrite(FILE *fp, const void *data, size_t nbytes)
{
	if (fwrite(data, 1, nbytes, fp) != nbytes)
	{
		llwarns << "Short write" << llendl;
	}
}

void LLVOCacheEntry::writeToFile(FILE *fp) const
{
	checkedWrite(fp, &mLocalID, sizeof(U32));
	checkedWrite(fp, &mCRC, sizeof(U32));
	checkedWrite(fp, &mHitCount, sizeof(S32));
	checkedWrite(fp, &mDupeCount, sizeof(S32));
	checkedWrite(fp, &mCRCChangeCount, sizeof(S32));
	S32 size = mDP.getBufferSize();
	checkedWrite(fp, &size, sizeof(S32));
	checkedWrite(fp, mBuffer, size);
}
