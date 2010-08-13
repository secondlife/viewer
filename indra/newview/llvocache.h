/** 
 * @file llvocache.h
 * @brief Cache of objects on the viewer.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
	LLVOCacheEntry(LLFILE *fp);
	LLVOCacheEntry();
	~LLVOCacheEntry();

	U32 getLocalID() const			{ return mLocalID; }
	U32 getCRC() const				{ return mCRC; }
	S32 getHitCount() const			{ return mHitCount; }
	S32 getCRCChangeCount() const	{ return mCRCChangeCount; }

	void dump() const;
	void writeToFile(LLFILE *fp) const;
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
