/** 
 * @file llvocache.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llvocache.h"
#include "llerror.h"
#include "llregionhandle.h"
#include "llviewercontrol.h"

BOOL check_read(LLAPRFile* apr_file, void* src, S32 n_bytes) 
{
	return apr_file->read(src, n_bytes) == n_bytes ;
}

BOOL check_write(LLAPRFile* apr_file, void* src, S32 n_bytes) 
{
	return apr_file->write(src, n_bytes) == n_bytes ;
}

//---------------------------------------------------------------------------
// LLVOCacheEntry
//---------------------------------------------------------------------------

LLVOCacheEntry::LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp)
	:
	mLocalID(local_id),
	mCRC(crc),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0)
{
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
	:
	mLocalID(0),
	mCRC(0),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mBuffer(NULL)
{
	mDP.assignBuffer(mBuffer, 0);
}

LLVOCacheEntry::LLVOCacheEntry(LLAPRFile* apr_file)
{
	S32 size = -1;
	BOOL success;

	success = check_read(apr_file, &mLocalID, sizeof(U32));
	if(success)
	{
		success = check_read(apr_file, &mCRC, sizeof(U32));
	}
	if(success)
	{
		success = check_read(apr_file, &mHitCount, sizeof(S32));
	}
	if(success)
	{
		success = check_read(apr_file, &mDupeCount, sizeof(S32));
	}
	if(success)
	{
		success = check_read(apr_file, &mCRCChangeCount, sizeof(S32));
	}
	if(success)
	{
		success = check_read(apr_file, &size, sizeof(S32));

		// Corruption in the cache entries
		if ((size > 10000) || (size < 1))
		{
			// We've got a bogus size, skip reading it.
			// We won't bother seeking, because the rest of this file
			// is likely bogus, and will be tossed anyway.
			llwarns << "Bogus cache entry, size " << size << ", aborting!" << llendl;
			success = FALSE;
		}
	}
	if(success && size > 0)
	{
		mBuffer = new U8[size];
		success = check_read(apr_file, mBuffer, size);

		if(success)
		{
			mDP.assignBuffer(mBuffer, size);
		}
		else
		{
			delete[] mBuffer ;
			mBuffer = NULL ;
		}
	}

	if(!success)
	{
		mLocalID = 0;
		mCRC = 0;
		mHitCount = 0;
		mDupeCount = 0;
		mCRCChangeCount = 0;
		mBuffer = NULL;
	}
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

BOOL LLVOCacheEntry::writeToFile(LLAPRFile* apr_file) const
{
	BOOL success;
	success = check_write(apr_file, (void*)&mLocalID, sizeof(U32));
	if(success)
	{
		success = check_write(apr_file, (void*)&mCRC, sizeof(U32));
	}
	if(success)
	{
		success = check_write(apr_file, (void*)&mHitCount, sizeof(S32));
	}
	if(success)
	{
		success = check_write(apr_file, (void*)&mDupeCount, sizeof(S32));
	}
	if(success)
	{
		success = check_write(apr_file, (void*)&mCRCChangeCount, sizeof(S32));
	}
	if(success)
	{
		S32 size = mDP.getBufferSize();
		success = check_write(apr_file, (void*)&size, sizeof(S32));
	
		if(success)
		{
			success = check_write(apr_file, (void*)mBuffer, size);
	}
}

	return success ;
}

//-------------------------------------------------------------------
//LLVOCache
//-------------------------------------------------------------------
// Format string used to construct filename for the object cache
static const char OBJECT_CACHE_FILENAME[] = "objects_%d_%d.slc";

const U32 MAX_NUM_OBJECT_ENTRIES = 128 ;
const U32 NUM_ENTRIES_TO_PURGE = 16 ;
const char* object_cache_dirname = "objectcache";
const char* header_filename = "object.cache";

LLVOCache* LLVOCache::sInstance = NULL;

//static 
LLVOCache* LLVOCache::getInstance() 
{	
	if(!sInstance)
	{
		sInstance = new LLVOCache() ;
	}
	return sInstance ;
}

//static 
BOOL LLVOCache::hasInstance() 
{
	return sInstance != NULL ;
}

//static 
void LLVOCache::destroyClass() 
{
	if(sInstance)
	{
		delete sInstance ;
		sInstance = NULL ;
	}
}

LLVOCache::LLVOCache():
	mInitialized(FALSE),
	mReadOnly(TRUE),
	mNumEntries(0),
	mCacheSize(1)
{
	mEnabled = gSavedSettings.getBOOL("ObjectCacheEnabled");
	mLocalAPRFilePoolp = new LLVolatileAPRPool() ;
}

LLVOCache::~LLVOCache()
{
	if(mEnabled)
	{
		writeCacheHeader();
		clearCacheInMemory();
	}
	delete mLocalAPRFilePoolp;
}

void LLVOCache::setDirNames(ELLPath location)
{
	std::string delem = gDirUtilp->getDirDelimiter();

	mHeaderFileName = gDirUtilp->getExpandedFilename(location, object_cache_dirname, header_filename);
	mObjectCacheDirName = gDirUtilp->getExpandedFilename(location, object_cache_dirname);
}

void LLVOCache::initCache(ELLPath location, U32 size, U32 cache_version)
{
	if(mInitialized || !mEnabled)
	{
		return ;
	}

	setDirNames(location);
	if (!mReadOnly)
	{
		LLFile::mkdir(mObjectCacheDirName);
	}	
	mCacheSize = llclamp(size,
			     MAX_NUM_OBJECT_ENTRIES, NUM_ENTRIES_TO_PURGE);

	mMetaInfo.mVersion = cache_version;
	readCacheHeader();
	mInitialized = TRUE ;

	if(mMetaInfo.mVersion != cache_version) 
	{
		mMetaInfo.mVersion = cache_version ;
		if(mReadOnly) //disable cache
		{
			clearCacheInMemory();
		}
		else //delete the current cache if the format does not match.
		{			
			removeCache();
		}
	}	
}
	
void LLVOCache::removeCache(ELLPath location) 
{
	if(mReadOnly)
	{
		return ;
	}

	std::string delem = gDirUtilp->getDirDelimiter();
	std::string mask = delem + "*";
	std::string cache_dir = gDirUtilp->getExpandedFilename(location, object_cache_dirname);
	gDirUtilp->deleteFilesInDir(cache_dir, mask); //delete all files
	LLFile::rmdir(cache_dir);

	clearCacheInMemory();
	mInitialized = FALSE ;
}

void LLVOCache::removeCache() 
{
	llassert_always(mInitialized) ;
	if(mReadOnly)
	{
		return ;
	}

	std::string delem = gDirUtilp->getDirDelimiter();
	std::string mask = delem + "*";
	gDirUtilp->deleteFilesInDir(mObjectCacheDirName, mask); 

	clearCacheInMemory() ;
	writeCacheHeader();
}

void LLVOCache::clearCacheInMemory()
{
	if(!mHeaderEntryQueue.empty()) 
	{
		for(header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin(); iter != mHeaderEntryQueue.end(); ++iter)
		{
			delete *iter ;
		}
		mHeaderEntryQueue.clear();
		mHandleEntryMap.clear();
		mNumEntries = 0 ;
	}
}

void LLVOCache::getObjectCacheFilename(U64 handle, std::string& filename) 
{
	U32 region_x, region_y;

	grid_from_region_handle(handle, &region_x, &region_y);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, object_cache_dirname,
			   llformat(OBJECT_CACHE_FILENAME, region_x, region_y));

	return ;
}

void LLVOCache::removeFromCache(U64 handle)
{
	if(mReadOnly)
	{
		return ;
	}

	std::string filename;
	getObjectCacheFilename(handle, filename);
	LLAPRFile::remove(filename, mLocalAPRFilePoolp);	
}

BOOL LLVOCache::checkRead(LLAPRFile* apr_file, void* src, S32 n_bytes) 
{
	if(!check_read(apr_file, src, n_bytes))
	{
		delete apr_file ;
		removeCache() ;
		return FALSE ;
	}

	return TRUE ;
}

BOOL LLVOCache::checkWrite(LLAPRFile* apr_file, void* src, S32 n_bytes) 
{
	if(!check_write(apr_file, src, n_bytes))
	{
		delete apr_file ;
		removeCache() ;
		return FALSE ;
	}

	return TRUE ;
}

void LLVOCache::readCacheHeader()
{
	if(!mEnabled)
	{
		return ;
	}

	//clear stale info.
	clearCacheInMemory();	

	if (LLAPRFile::isExist(mHeaderFileName, mLocalAPRFilePoolp))
	{
		LLAPRFile* apr_file = new LLAPRFile(mHeaderFileName, APR_READ|APR_BINARY, mLocalAPRFilePoolp);		
		
		//read the meta element
		if(!checkRead(apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)))
		{
			return ;
		}

		HeaderEntryInfo* entry ;
		mNumEntries = 0 ;
		while(mNumEntries < MAX_NUM_OBJECT_ENTRIES)
		{
			entry = new HeaderEntryInfo() ;
			if(!checkRead(apr_file, entry, sizeof(HeaderEntryInfo)))
			{
				delete entry ;			
				return ;
			}
			else if(!entry->mTime) //end of the cache.
			{
				delete entry ;
				return ;
			}

			entry->mIndex = mNumEntries++ ;
			mHeaderEntryQueue.insert(entry) ;
			mHandleEntryMap[entry->mHandle] = entry ;
		}

		delete apr_file ;
	}
	else
	{
		writeCacheHeader() ;
	}
}

void LLVOCache::writeCacheHeader()
{
	if(mReadOnly || !mEnabled)
	{
		return ;
	}	

	LLAPRFile* apr_file = new LLAPRFile(mHeaderFileName, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);

	//write the meta element
	if(!checkWrite(apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)))
	{
		return ;
	}

	mNumEntries = 0 ;
	for(header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ; iter != mHeaderEntryQueue.end(); ++iter)
	{
		(*iter)->mIndex = mNumEntries++ ;
		if(!checkWrite(apr_file, (void*)*iter, sizeof(HeaderEntryInfo)))
		{
			return ;
		}
	}

	mNumEntries = mHeaderEntryQueue.size() ;
	if(mNumEntries < MAX_NUM_OBJECT_ENTRIES)
	{
		HeaderEntryInfo* entry = new HeaderEntryInfo() ;
		for(S32 i = mNumEntries ; i < MAX_NUM_OBJECT_ENTRIES ; i++)
		{
			//fill the cache with the default entry.
			if(!checkWrite(apr_file, entry, sizeof(HeaderEntryInfo)))
			{
				mReadOnly = TRUE ; //disable the cache.
				return ;
			}
		}
		delete entry ;
	}
	delete apr_file ;
}

BOOL LLVOCache::updateEntry(const HeaderEntryInfo* entry)
{
	LLAPRFile* apr_file = new LLAPRFile(mHeaderFileName, APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
	apr_file->seek(APR_SET, entry->mIndex * sizeof(HeaderEntryInfo) + sizeof(HeaderMetaInfo)) ;

	return checkWrite(apr_file, (void*)entry, sizeof(HeaderEntryInfo)) ;
}

void LLVOCache::readFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_entry_map_t& cache_entry_map) 
{
	if(!mEnabled)
	{
		return ;
	}
	llassert_always(mInitialized);

	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	if(iter == mHandleEntryMap.end()) //no cache
	{
		return ;
	}

	std::string filename;
	getObjectCacheFilename(handle, filename);
	LLAPRFile* apr_file = new LLAPRFile(filename, APR_READ|APR_BINARY, mLocalAPRFilePoolp);

	LLUUID cache_id ;
	if(!checkRead(apr_file, cache_id.mData, UUID_BYTES))
	{
		return ;
	}
	if(cache_id != id)
	{
		llinfos << "Cache ID doesn't match for this region, discarding"<< llendl;

		delete apr_file ;
		return ;
	}

	S32 num_entries;
	if(!checkRead(apr_file, &num_entries, sizeof(S32)))
	{
		return ;
	}
	
	for (S32 i = 0; i < num_entries; i++)
	{
		LLVOCacheEntry* entry = new LLVOCacheEntry(apr_file);
		if (!entry->getLocalID())
		{
			llwarns << "Aborting cache file load for " << filename << ", cache file corruption!" << llendl;
			delete entry ;
			break;
		}
		cache_entry_map[entry->getLocalID()] = entry;
	}
	num_entries = cache_entry_map.size() ;

	delete apr_file ;
	return ;
}
	
void LLVOCache::purgeEntries()
{
	U32 limit = mCacheSize - NUM_ENTRIES_TO_PURGE ;
	while(mHeaderEntryQueue.size() > limit)
	{
		header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ;
		HeaderEntryInfo* entry = *iter ;
		
		removeFromCache(entry->mHandle) ;
		mHandleEntryMap.erase(entry->mHandle) ;		
		mHeaderEntryQueue.erase(iter) ;
		delete entry ;
	}

	writeCacheHeader() ;
	readCacheHeader() ;
	mNumEntries = mHandleEntryMap.size() ;
}

void LLVOCache::writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, BOOL dirty_cache) 
{
	if(!mEnabled)
	{
		return ;
	}
	llassert_always(mInitialized);

	if(mReadOnly)
	{
		return ;
	}

	HeaderEntryInfo* entry;
	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	if(iter == mHandleEntryMap.end()) //new entry
	{		
		if(mNumEntries >= mCacheSize)
		{
			purgeEntries() ;
		}
		
		entry = new HeaderEntryInfo();
		entry->mHandle = handle ;
		entry->mTime = time(NULL) ;
		entry->mIndex = mNumEntries++ ;
		mHeaderEntryQueue.insert(entry) ;
		mHandleEntryMap[handle] = entry ;
	}
	else
	{
		entry = iter->second ;
		entry->mTime = time(NULL) ;

		//resort
		mHeaderEntryQueue.erase(entry) ;
		mHeaderEntryQueue.insert(entry) ;
	}

	//update cache header
	if(!updateEntry(entry))
	{
		return ; //update failed.
	}

	if(!dirty_cache)
	{
		return ; //nothing changed, no need to update.
	}

	//write to cache file
	std::string filename;
	getObjectCacheFilename(handle, filename);
	LLAPRFile* apr_file = new LLAPRFile(filename, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
	
	if(!checkWrite(apr_file, (void*)id.mData, UUID_BYTES))
	{
		return ;
	}

	S32 num_entries = cache_entry_map.size() ;
	if(!checkWrite(apr_file, &num_entries, sizeof(S32)))
	{
		return ;
	}

	for (LLVOCacheEntry::vocache_entry_map_t::const_iterator iter = cache_entry_map.begin(); iter != cache_entry_map.end(); ++iter)
	{
		if(!iter->second->writeToFile(apr_file))
		{
			//failed
			delete apr_file ;
			removeCache() ;
			return ;
		}
	}

	delete apr_file ;
	return ;
}

