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
#include "lldir.h"


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
			if(lhs->mTime == rhs->mTime) 
			{
				return lhs < rhs ;
			}
			
			return lhs->mTime < rhs->mTime ; // older entry in front of queue (set)
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
	void removeEntry(U64 handle) ;

	void setReadOnly(BOOL read_only) {mReadOnly = read_only;} 

private:
	void setDirNames(ELLPath location);	
	// determine the cache filename for the region from the region handle	
	void getObjectCacheFilename(U64 handle, std::string& filename);
	void removeFromCache(HeaderEntryInfo* entry);
	void readCacheHeader();
	void writeCacheHeader();
	void clearCacheInMemory();
	void removeCache() ;
	void removeEntry(HeaderEntryInfo* entry) ;
	void purgeEntries(U32 size);
	BOOL updateEntry(const HeaderEntryInfo* entry);
	
private:
	BOOL                 mEnabled;
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
