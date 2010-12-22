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

// Throw out 1/20 (5%) of our cache entries if we run out of room.
const U32 ENTRIES_PURGE_FACTOR = 20;
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
	if(!mEnabled)
	{
		llwarns << "Not initializing cache: Cache is currently disabled." << llendl;
		return ;
	}

	if(mInitialized)
	{
		llwarns << "Cache already initialized." << llendl;
		return ;
	}

	setDirNames(location);
	if (!mReadOnly)
	{
		LLFile::mkdir(mObjectCacheDirName);
	}

	mCacheSize = size;

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
		llwarns << "Not removing cache at " << location << ": Cache is currently in read-only mode." << llendl;
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
		llwarns << "Not clearing object cache: Cache is currently in read-only mode." << llendl;
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
	std::for_each(mHandleEntryMap.begin(), mHandleEntryMap.end(), DeletePairedPointer());
	mHandleEntryMap.clear();
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
		llwarns << "Not removing cache for handle " << handle << ": Cache is currently in read-only mode." << llendl;
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
		removeCache() ;
		return FALSE ;
	}

	return TRUE ;
}

BOOL LLVOCache::checkWrite(LLAPRFile* apr_file, void* src, S32 n_bytes) 
{
	if(!check_write(apr_file, src, n_bytes))
	{
		removeCache() ;
		return FALSE ;
	}

	return TRUE ;
}

void LLVOCache::readCacheHeader()
{
	if(!mEnabled)
	{
		llwarns << "Not reading cache header: Cache is currently disabled." << llendl;
		return;
	}

	//clear stale info.
	clearCacheInMemory();	

	if (LLAPRFile::isExist(mHeaderFileName, mLocalAPRFilePoolp))
	{
		LLAPRFile* apr_file = new LLAPRFile(mHeaderFileName, APR_READ|APR_BINARY, mLocalAPRFilePoolp);		
		
		//read the meta element
		if(!checkRead(apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)))
		{
			llwarns << "Error reading meta information from cache header." << llendl;
			delete apr_file;
			return;
		}

		HeaderEntryInfo* entry ;
		for(U32 entry_index = 0; entry_index < mCacheSize; ++entry_index)
		{
			entry = new HeaderEntryInfo() ;
			if(!checkRead(apr_file, entry, sizeof(HeaderEntryInfo)))
			{
				llwarns << "Error reading cache header entry. (entry_index=" << entry_index << ")" << llendl;
				delete entry ;			
				break;
			}
			else if(!entry->mTime) //end of the cache.
			{
				delete entry ;
				break;
			}

			entry->mIndex = entry_index;
			mHandleEntryMap[entry->mHandle] = entry;
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
	if (!mEnabled)
	{
		llwarns << "Not writing cache header: Cache is currently disabled." << llendl;
		return;
	}

	if(mReadOnly)
	{
		llwarns << "Not writing cache header: Cache is currently in read-only mode." << llendl;
		return;
	}

	LLAPRFile* apr_file = new LLAPRFile(mHeaderFileName, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);

	//write the meta element
	if(!checkWrite(apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)))
	{
		llwarns << "Error writing meta information to cache header." << llendl;
		delete apr_file;
		return;
	}

	U32 entry_index = 0;
	handle_entry_map_t::iterator iter_end = mHandleEntryMap.end();
	for(handle_entry_map_t::iterator iter = mHandleEntryMap.begin();
		iter != iter_end;
		++iter)
	{
		HeaderEntryInfo* entry = iter->second;
		entry->mIndex = entry_index++;
		if(!checkWrite(apr_file, (void*)entry, sizeof(HeaderEntryInfo)))
		{
			llwarns << "Failed to write cache header for entry " << entry->mHandle << " (entry_index = " << entry_index << ")" << llendl;
			delete apr_file;
			return;
		}
	}

	// Why do we need to fill the cache header with default entries?  DK 2010-12-14
	// It looks like we currently rely on the file being pre-allocated so we can seek during updateEntry().
	if(entry_index < mCacheSize)
	{
		HeaderEntryInfo* entry = new HeaderEntryInfo() ;
		for(; entry_index < mCacheSize; ++entry_index)
		{
			//fill the cache with the default entry.
			if(!checkWrite(apr_file, entry, sizeof(HeaderEntryInfo)))
			{
				llwarns << "Failed to fill cache header with default entries (entry_index = " << entry_index << ").  Switching to read-only mode." << llendl;
				mReadOnly = TRUE ; //disable the cache.
				break;
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

	BOOL result = checkWrite(apr_file, (void*)entry, sizeof(HeaderEntryInfo)) ;
	delete apr_file;
	return result;
}

void LLVOCache::readFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_entry_map_t& cache_entry_map) 
{
	if(!mEnabled)
	{
		llwarns << "Not reading cache for handle " << handle << "): Cache is currently disabled." << llendl;
		return ;
	}
	llassert_always(mInitialized);

	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	if(iter == mHandleEntryMap.end()) //no cache
	{
		llwarns << "No handle map entry for " << handle << llendl;
		return ;
	}

	std::string filename;
	getObjectCacheFilename(handle, filename);
	LLAPRFile* apr_file = new LLAPRFile(filename, APR_READ|APR_BINARY, mLocalAPRFilePoolp);

	LLUUID cache_id ;
	if(!checkRead(apr_file, cache_id.mData, UUID_BYTES))
	{
		llwarns << "Error reading cache_id from " << filename << llendl;
		delete apr_file;
		return ;
	}
	if(cache_id != id)
	{
		llwarns << "Cache ID (" << cache_id << ") doesn't match id for this region (" << id << "), discarding.  handle = " << handle << llendl;
		delete apr_file ;
		return ;
	}

	S32 num_entries;
	if(!checkRead(apr_file, &num_entries, sizeof(S32)))
	{
		llwarns << "Error reading num_entries from " << filename << llendl;
		delete apr_file;
		return ;
	}
	
	for (S32 i = 0; i < num_entries; i++)
	{
		LLVOCacheEntry* entry = new LLVOCacheEntry(apr_file);
		if (!entry->getLocalID())
		{
			llwarns << "Aborting cache file load for " << filename << ", cache file corruption! (entry number = " << i << ")" << llendl;
			delete entry ;
			break;
		}
		cache_entry_map[entry->getLocalID()] = entry;
	}

	delete apr_file ;
	return ;
}
	
void LLVOCache::purgeEntries()
{
	U32 limit = mCacheSize - (mCacheSize / ENTRIES_PURGE_FACTOR);
	limit = llclamp(limit, (U32)1, mCacheSize);
	// Construct a vector of entries out of the map so we can sort by time.
	std::vector<HeaderEntryInfo*> header_vector;
	handle_entry_map_t::iterator iter_end = mHandleEntryMap.end();
	for (handle_entry_map_t::iterator iter = mHandleEntryMap.begin();
		iter != iter_end;
		++iter)
	{
		header_vector.push_back(iter->second);
	}
	// Sort by time, oldest first.
	std::sort(header_vector.begin(), header_vector.end(), header_entry_less());
	while(header_vector.size() > limit)
	{
		HeaderEntryInfo* entry = header_vector.front();
		
		removeFromCache(entry->mHandle);
		mHandleEntryMap.erase(entry->mHandle);
		header_vector.erase(header_vector.begin());
		delete entry;
	}

	writeCacheHeader() ;
	// *TODO: Verify that we can avoid re-reading the cache header.  DK 2010-12-14
	readCacheHeader() ;
}

void LLVOCache::writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, BOOL dirty_cache) 
{
	if(!mEnabled)
	{
		llwarns << "Not writing cache for handle " << handle << "): Cache is currently disabled." << llendl;
		return ;
	}
	llassert_always(mInitialized);

	if(mReadOnly)
	{
		llwarns << "Not writing cache for handle " << handle << "): Cache is currently in read-only mode." << llendl;
		return ;
	}

	HeaderEntryInfo* entry;
	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	U32 num_handle_entries = mHandleEntryMap.size();
	if(iter == mHandleEntryMap.end()) //new entry
	{
		if(num_handle_entries >= mCacheSize)
		{
			purgeEntries() ;
		}
		
		entry = new HeaderEntryInfo();
		entry->mHandle = handle ;
		entry->mTime = time(NULL) ;
		entry->mIndex = num_handle_entries++;
		mHandleEntryMap[handle] = entry ;
	}
	else
	{
		// Update access time.
		entry = iter->second ;
		entry->mTime = time(NULL) ;
	}

	//update cache header
	if(!updateEntry(entry))
	{
		llwarns << "Failed to update cache header index " << entry->mIndex << ". handle = " << handle << llendl;
		return ; //update failed.
	}

	if(!dirty_cache)
	{
		llwarns << "Skipping write to cache for handle " << handle << ": cache not dirty" << llendl;
		return ; //nothing changed, no need to update.
	}

	//write to cache file
	std::string filename;
	getObjectCacheFilename(handle, filename);
	LLAPRFile* apr_file = new LLAPRFile(filename, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
	
	if(!checkWrite(apr_file, (void*)id.mData, UUID_BYTES))
	{
		llwarns << "Error writing id to " << filename << llendl;
		delete apr_file;
		return ;
	}

	S32 num_entries = cache_entry_map.size() ;
	if(!checkWrite(apr_file, &num_entries, sizeof(S32)))
	{
		llwarns << "Error writing num_entries to " << filename << llendl;
		delete apr_file;
		return ;
	}

	for (LLVOCacheEntry::vocache_entry_map_t::const_iterator iter = cache_entry_map.begin(); iter != cache_entry_map.end(); ++iter)
	{
		if(!iter->second->writeToFile(apr_file))
		{
			llwarns << "Aborting cache file write for " << filename << ", error writing to file!" << llendl;
			//failed
			removeCache() ;
			break;
		}
	}

	delete apr_file ;
	return ;
}

