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
#include "llviewerobjectlist.h"
#include "lldrawable.h"
#include "llviewerregion.h"

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
	: LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
	mLocalID(local_id),
	mCRC(crc),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mState(INACTIVE),
	mRepeatedVisCounter(0),
	mVisFrameRange(64)
{
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
	: LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
	mLocalID(0),
	mCRC(0),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mBuffer(NULL),
	mState(INACTIVE),
	mRepeatedVisCounter(0),
	mVisFrameRange(64)
{
	mDP.assignBuffer(mBuffer, 0);
}

LLVOCacheEntry::LLVOCacheEntry(LLAPRFile* apr_file)
	: LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY), 
	mBuffer(NULL),
	mState(INACTIVE),
	mRepeatedVisCounter(0),
	mVisFrameRange(64)
{
	S32 size = -1;
	BOOL success;

	mDP.assignBuffer(mBuffer, 0);
	setOctreeEntry(NULL);

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
		LLVector4 pos;
		success = check_read(apr_file, (void*)pos.mV, sizeof(LLVector4));

		LLVector4a pos_;
		pos_.load4a(pos.mV);
		setPositionGroup(pos_);
	}
	if(success)
	{
		F32 rad;
		success = check_read(apr_file, &rad, sizeof(F32));
		setBinRadius(rad);
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
		mEntry = NULL;
	}
}

LLVOCacheEntry::~LLVOCacheEntry()
{
	mDP.freeBuffer();
	//llassert(mState == INACTIVE);
}

//virtual 
void LLVOCacheEntry::setOctreeEntry(LLViewerOctreeEntry* entry)
{
	if(!entry && mDP.getBufferSize() > 0)
	{
		LLUUID fullid;
		mDP.reset();
		mDP.unpackUUID(fullid, "ID");
		mDP.reset();

		LLViewerObject* obj = gObjectList.findObject(fullid);
		if(obj && obj->mDrawable)
		{
			entry = obj->mDrawable->getEntry();
		}
	}

	LLViewerOctreeEntryData::setOctreeEntry(entry);
}

void LLVOCacheEntry::setState(U32 state)
{
	mState = state;

	if(mState == ACTIVE)
	{
		const S32 MIN_REAVTIVE_INTERVAL = 20;
		U32 last_visible = getVisible();
		
		setVisible();

		if(getVisible() - last_visible < MIN_REAVTIVE_INTERVAL + mVisFrameRange)
		{
			mRepeatedVisCounter++;
		}
		else
		{
			mRepeatedVisCounter = 0;
			mVisFrameRange = 64;
		}

		if(mRepeatedVisCounter > 2) 
		{
			//if repeatedly becomes visible immediately after invisible, enlarge the visible frame range

			mRepeatedVisCounter = 0;
			mVisFrameRange *= 2;
		}
	}
}

//virtual 
S32  LLVOCacheEntry::getMinVisFrameRange()const
{
	return mVisFrameRange;
}

void LLVOCacheEntry::addChild(LLVOCacheEntry* entry)
{
	llassert(entry != NULL);

	mChildrenList.push_back(entry);
}
	
LLVOCacheEntry* LLVOCacheEntry::getNextChild()
{
	S32 size = mChildrenList.size();
	if(!size)
	{
		return NULL;
	}

	LLVOCacheEntry* entry = mChildrenList[size - 1];
	mChildrenList.pop_back(); //remove the entry;

	return entry;
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

LLDataPackerBinaryBuffer *LLVOCacheEntry::getDP()
{
	if (mDP.getBufferSize() == 0)
	{
		//llinfos << "Not getting cache entry, invalid!" << llendl;
		return NULL;
	}
	
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
	if(!mEntry)
	{
		return FALSE;
	}

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
		const LLVector4a pos_ = getPositionGroup() ;
		LLVector4 pos(pos_[0], pos_[1], pos_[2], pos_[3]);
		success = check_write(apr_file, pos.mV, sizeof(LLVector4));		
	}
	if(success)
	{
		F32 rad = getBinRadius();
		success = check_write(apr_file, (void*)&rad, sizeof(F32));
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
//LLVOCachePartition
//-------------------------------------------------------------------
LLVOCachePartition::LLVOCachePartition(LLViewerRegion* regionp)
{
	mRegionp = regionp;
	mPartitionType = LLViewerRegion::PARTITION_VO_CACHE;
	mVisitedTime = 0;

	new LLviewerOctreeGroup(mOctree);
}

void LLVOCachePartition::addEntry(LLViewerOctreeEntry* entry)
{
	llassert(entry->hasVOCacheEntry());

	mOctree->insert(entry);
}
	
void LLVOCachePartition::removeEntry(LLViewerOctreeEntry* entry)
{
	entry->getVOCacheEntry()->setGroup(NULL);

	llassert(!entry->getGroup());
}
	
class LLVOCacheOctreeCull : public LLViewerOctreeCull
{
public:
	LLVOCacheOctreeCull(LLCamera* camera, LLViewerRegion* regionp) : LLViewerOctreeCull(camera), mRegionp(regionp) {}

	virtual S32 frustumCheck(const LLviewerOctreeGroup* group)
	{
		S32 res = AABBInFrustumNoFarClipGroupBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBSphereIntersectGroupExtents(group));
		}
		return res;
	}

	virtual S32 frustumCheckObjects(const LLviewerOctreeGroup* group)
	{
		S32 res = AABBInFrustumNoFarClipObjectBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBSphereIntersectObjectExtents(group));
		}
		return res;
	}

	virtual void processGroup(LLviewerOctreeGroup* base_group)
	{
		mRegionp->addVisibleGroup(base_group);
	}

private:
	LLViewerRegion* mRegionp;
};

S32 LLVOCachePartition::cull(LLCamera &camera)
{
	if(mVisitedTime == LLViewerOctreeEntryData::getCurrentFrame())
	{
		return 0; //already visited.
	}
	mVisitedTime = LLViewerOctreeEntryData::getCurrentFrame();

	((LLviewerOctreeGroup*)mOctree->getListener(0))->rebound();
		
	LLVOCacheOctreeCull culler(&camera, mRegionp);
	culler.traverse(mOctree);

	return 0;
}

//-------------------------------------------------------------------
//LLVOCache
//-------------------------------------------------------------------
// Format string used to construct filename for the object cache
static const char OBJECT_CACHE_FILENAME[] = "objects_%d_%d.slc";

const U32 MAX_NUM_OBJECT_ENTRIES = 128 ;
const U32 MIN_ENTRIES_TO_PURGE = 16 ;
const U32 INVALID_TIME = 0 ;
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
	mInitialized = TRUE ;

	setDirNames(location);
	if (!mReadOnly)
	{
		LLFile::mkdir(mObjectCacheDirName);
	}
	mCacheSize = llclamp(size, MIN_ENTRIES_TO_PURGE, MAX_NUM_OBJECT_ENTRIES);
	mMetaInfo.mVersion = cache_version;
	readCacheHeader();	

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

	llinfos << "about to remove the object cache due to settings." << llendl ;

	std::string mask = "*";
	std::string cache_dir = gDirUtilp->getExpandedFilename(location, object_cache_dirname);
	llinfos << "Removing cache at " << cache_dir << llendl;
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

	llinfos << "about to remove the object cache due to some error." << llendl ;

	std::string mask = "*";
	llinfos << "Removing cache at " << mObjectCacheDirName << llendl;
	gDirUtilp->deleteFilesInDir(mObjectCacheDirName, mask); 

	clearCacheInMemory() ;
	writeCacheHeader();
}

void LLVOCache::removeEntry(HeaderEntryInfo* entry) 
{
	llassert_always(mInitialized) ;
	if(mReadOnly)
	{
		return ;
	}
	if(!entry)
	{
		return ;
	}

	header_entry_queue_t::iterator iter = mHeaderEntryQueue.find(entry) ;
	if(iter != mHeaderEntryQueue.end())
	{		
		mHandleEntryMap.erase(entry->mHandle) ;		
		mHeaderEntryQueue.erase(iter) ;
		removeFromCache(entry) ;
		delete entry ;

		mNumEntries = mHandleEntryMap.size() ;
	}
}

void LLVOCache::removeEntry(U64 handle) 
{
	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	if(iter == mHandleEntryMap.end()) //no cache
	{
		return ;
	}
	HeaderEntryInfo* entry = iter->second ;
	removeEntry(entry) ;
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

void LLVOCache::removeFromCache(HeaderEntryInfo* entry)
{
	if(mReadOnly)
	{
		llwarns << "Not removing cache for handle " << entry->mHandle << ": Cache is currently in read-only mode." << llendl;
		return ;
	}

	std::string filename;
	getObjectCacheFilename(entry->mHandle, filename);
	LLAPRFile::remove(filename, mLocalAPRFilePoolp);
	entry->mTime = INVALID_TIME ;
	updateEntry(entry) ; //update the head file.
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

	bool success = true ;
	if (LLAPRFile::isExist(mHeaderFileName, mLocalAPRFilePoolp))
	{
		LLAPRFile apr_file(mHeaderFileName, APR_READ|APR_BINARY, mLocalAPRFilePoolp);		
		
		//read the meta element
		success = check_read(&apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)) ;
		
		if(success)
		{
			HeaderEntryInfo* entry = NULL ;
			mNumEntries = 0 ;
			U32 num_read = 0 ;
			while(num_read++ < MAX_NUM_OBJECT_ENTRIES)
			{
				if(!entry)
				{
					entry = new HeaderEntryInfo() ;
				}
				success = check_read(&apr_file, entry, sizeof(HeaderEntryInfo));
								
				if(!success) //failed
				{
					llwarns << "Error reading cache header entry. (entry_index=" << mNumEntries << ")" << llendl;
					delete entry ;
					entry = NULL ;
					break ;
				}
				else if(entry->mTime == INVALID_TIME)
				{
					continue ; //an empty entry
				}

				entry->mIndex = mNumEntries++ ;
				mHeaderEntryQueue.insert(entry) ;
				mHandleEntryMap[entry->mHandle] = entry ;
				entry = NULL ;
			}
			if(entry)
			{
				delete entry ;
			}
		}

		//---------
		//debug code
		//----------
		//std::string name ;
		//for(header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ; success && iter != mHeaderEntryQueue.end(); ++iter)
		//{
		//	getObjectCacheFilename((*iter)->mHandle, name) ;
		//	llinfos << name << llendl ;
		//}
		//-----------
	}
	else
	{
		writeCacheHeader() ;
	}

	if(!success)
	{
		removeCache() ; //failed to read header, clear the cache
	}
	else if(mNumEntries >= mCacheSize)
	{
		purgeEntries(mCacheSize) ;
	}

	return ;
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

	bool success = true ;
	{
		LLAPRFile apr_file(mHeaderFileName, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);

		//write the meta element
		success = check_write(&apr_file, &mMetaInfo, sizeof(HeaderMetaInfo)) ;


		mNumEntries = 0 ;	
		for(header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ; success && iter != mHeaderEntryQueue.end(); ++iter)
		{
			(*iter)->mIndex = mNumEntries++ ;
			success = check_write(&apr_file, (void*)*iter, sizeof(HeaderEntryInfo));
		}
	
		mNumEntries = mHeaderEntryQueue.size() ;
		if(success && mNumEntries < MAX_NUM_OBJECT_ENTRIES)
		{
			HeaderEntryInfo* entry = new HeaderEntryInfo() ;
			entry->mTime = INVALID_TIME ;
			for(S32 i = mNumEntries ; success && i < MAX_NUM_OBJECT_ENTRIES ; i++)
			{
				//fill the cache with the default entry.
				success = check_write(&apr_file, entry, sizeof(HeaderEntryInfo)) ;			

			}
			delete entry ;
		}
	}

	if(!success)
	{
		clearCacheInMemory() ;
		mReadOnly = TRUE ; //disable the cache.
	}
	return ;
}

BOOL LLVOCache::updateEntry(const HeaderEntryInfo* entry)
{
	LLAPRFile apr_file(mHeaderFileName, APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
	apr_file.seek(APR_SET, entry->mIndex * sizeof(HeaderEntryInfo) + sizeof(HeaderMetaInfo)) ;

	return check_write(&apr_file, (void*)entry, sizeof(HeaderEntryInfo)) ;
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

	bool success = true ;
	{
		std::string filename;
		getObjectCacheFilename(handle, filename);
		LLAPRFile apr_file(filename, APR_READ|APR_BINARY, mLocalAPRFilePoolp);
	
		LLUUID cache_id ;
		success = check_read(&apr_file, cache_id.mData, UUID_BYTES) ;
	
		if(success)
		{		
			if(cache_id != id)
			{
				llinfos << "Cache ID doesn't match for this region, discarding"<< llendl;
				success = false ;
			}

			if(success)
			{
				S32 num_entries;
				success = check_read(&apr_file, &num_entries, sizeof(S32)) ;
	
				if(success)
				{
					for (S32 i = 0; i < num_entries; i++)
					{
						LLPointer<LLVOCacheEntry> entry = new LLVOCacheEntry(&apr_file);
						if (!entry->getLocalID())
						{
							llwarns << "Aborting cache file load for " << filename << ", cache file corruption!" << llendl;
							success = false ;
							break ;
						}
						cache_entry_map[entry->getLocalID()] = entry;
					}
				}
			}
		}		
	}
	
	if(!success)
	{
		if(cache_entry_map.empty())
		{
			removeEntry(iter->second) ;
		}
	}

	return ;
}
	
void LLVOCache::purgeEntries(U32 size)
{
	while(mHeaderEntryQueue.size() > size)
	{
		header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ;
		HeaderEntryInfo* entry = *iter ;			
		mHandleEntryMap.erase(entry->mHandle);
		mHeaderEntryQueue.erase(iter) ;
		removeFromCache(entry) ;
		delete entry;
	}
	mNumEntries = mHandleEntryMap.size() ;
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
	if(iter == mHandleEntryMap.end()) //new entry
	{				
		if(mNumEntries >= mCacheSize - 1)
		{
			purgeEntries(mCacheSize - 1) ;
		}

		entry = new HeaderEntryInfo();
		entry->mHandle = handle ;
		entry->mTime = time(NULL) ;
		entry->mIndex = mNumEntries++;
		mHeaderEntryQueue.insert(entry) ;
		mHandleEntryMap[handle] = entry ;
	}
	else
	{
		// Update access time.
		entry = iter->second ;		

		//resort
		mHeaderEntryQueue.erase(entry) ;
		
		entry->mTime = time(NULL) ;
		mHeaderEntryQueue.insert(entry) ;
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
	bool success = true ;
	{
		std::string filename;
		getObjectCacheFilename(handle, filename);
		LLAPRFile apr_file(filename, APR_CREATE|APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
	
		success = check_write(&apr_file, (void*)id.mData, UUID_BYTES) ;

	
		if(success)
		{
			S32 num_entries = cache_entry_map.size() ;
			success = check_write(&apr_file, &num_entries, sizeof(S32));
	
			for (LLVOCacheEntry::vocache_entry_map_t::const_iterator iter = cache_entry_map.begin(); success && iter != cache_entry_map.end(); ++iter)
			{
				success = iter->second->writeToFile(&apr_file) ;
			}
		}
	}

	if(!success)
	{
		removeEntry(entry) ;

	}

	return ;
}
