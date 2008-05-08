/** 
 * @file llvocache.h
 * @brief Cache of objects on the viewer.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
