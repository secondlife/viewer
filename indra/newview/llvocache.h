/** 
 * @file llvocache.h
 * @brief Cache of objects on the viewer.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

class LLVOCacheEntry
{
public:
	LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp);
	LLVOCacheEntry(LLAPRFile* apr_file);
	LLVOCacheEntry();
	~LLVOCacheEntry();

	U32 getLocalID() const			{ return mLocalID; }
	U32 getCRC() const				{ return mCRC; }
	S32 getHitCount() const			{ return mHitCount; }
	S32 getCRCChangeCount() const	{ return mCRCChangeCount; }

	void dump() const;
	BOOL writeToFile(LLAPRFile* apr_file) const;
	void assignCRC(U32 crc, LLDataPackerBinaryBuffer &dp);
	LLDataPackerBinaryBuffer *getDP(U32 crc);
	void recordHit();
	void recordDupe() { mDupeCount++; }

public:
	typedef std::map<U32, LLVOCacheEntry*>	vocache_entry_map_t;

protected:
	U32							mLocalID;
	U32							mCRC;
	S32							mHitCount;
	S32							mDupeCount;
	S32							mCRCChangeCount;
	LLDataPackerBinaryBuffer	mDP;
	U8							*mBuffer;
};

//
//Note: LLVOCache is not thread-safe
//
class LLVOCache
{
private:
	struct HeaderEntryInfo
	{
		HeaderEntryInfo() : mIndex(0), mHandle(0), mTime(0) {}
		S32 mIndex;
		U64 mHandle ;
		U32 mTime ;
	};

	struct HeaderMetaInfo
	{
		HeaderMetaInfo() : mVersion(0){}

		U32 mVersion;
	};

	struct header_entry_less
	{
		bool operator()(const HeaderEntryInfo* lhs, const HeaderEntryInfo* rhs) const
		{
			return lhs->mTime < rhs->mTime; // older entry in front of queue (set)
		}
	};
	typedef std::set<HeaderEntryInfo*, header_entry_less> header_entry_queue_t;
	typedef std::map<U64, HeaderEntryInfo*> handle_entry_map_t;
private:
	LLVOCache() ;

public:
	~LLVOCache() ;

	void initCache(ELLPath location, U32 size, U32 cache_version) ;
	void removeCache(ELLPath location) ;

	void readFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_entry_map_t& cache_entry_map) ;
	void writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, BOOL dirty_cache) ;

	void setReadOnly(BOOL read_only) {mReadOnly = read_only;} 

private:
	void setDirNames(ELLPath location);	
	// determine the cache filename for the region from the region handle	
	void getObjectCacheFilename(U64 handle, std::string& filename);
	void removeFromCache(U64 handle);
	void readCacheHeader();
	void writeCacheHeader();
	void clearCacheInMemory();
	void removeCache() ;
	void purgeEntries();
	BOOL updateEntry(const HeaderEntryInfo* entry);
	BOOL checkRead(LLAPRFile* apr_file, void* src, S32 n_bytes) ;
	BOOL checkWrite(LLAPRFile* apr_file, void* src, S32 n_bytes) ;
	
private:
	BOOL                 mInitialized ;
	BOOL                 mReadOnly ;
	HeaderMetaInfo       mMetaInfo;
	U32                  mCacheSize;
	U32                  mNumEntries;
	std::string          mHeaderFileName ;
	std::string          mObjectCacheDirName;
	LLVolatileAPRPool*   mLocalAPRFilePoolp ; 	
	header_entry_queue_t mHeaderEntryQueue;
	handle_entry_map_t   mHandleEntryMap;	

	static LLVOCache* sInstance ;
public:
	static LLVOCache* getInstance() ;
	static BOOL       hasInstance() ;
	static void       destroyClass() ;
};

#endif
