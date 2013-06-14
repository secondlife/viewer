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
#include "pipeline.h"

LLTrace::MemStatHandle	LLVOCachePartition::sMemStat("LLVOCachePartition");

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
	mUpdateFlags(-1),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mState(INACTIVE),
	mMinFrameRange(64),
	mSceneContrib(0.f),
	mTouched(TRUE),
	mParentID(0)
{
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
	: LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
	mLocalID(0),
	mCRC(0),
	mUpdateFlags(-1),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mBuffer(NULL),
	mState(INACTIVE),
	mMinFrameRange(64),
	mSceneContrib(0.f),
	mTouched(TRUE),
	mParentID(0)
{
	mDP.assignBuffer(mBuffer, 0);
}

LLVOCacheEntry::LLVOCacheEntry(LLAPRFile* apr_file)
	: LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY), 
	mBuffer(NULL),
	mUpdateFlags(-1),
	mState(INACTIVE),
	mMinFrameRange(64),
	mSceneContrib(0.f),
	mTouched(FALSE),
	mParentID(0)
{
	S32 size = -1;
	BOOL success;

	mDP.assignBuffer(mBuffer, 0);
	
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
		mEntry = NULL;
		mState = 0;
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
		LLViewerObject::unpackUUID(&mDP, fullid, "ID");
		
		LLViewerObject* obj = gObjectList.findObject(fullid);
		if(obj && obj->mDrawable)
		{
			entry = obj->mDrawable->getEntry();
		}
	}

	LLViewerOctreeEntryData::setOctreeEntry(entry);
}

void LLVOCacheEntry::moveTo(LLVOCacheEntry* new_entry)
{
	//copy LLViewerOctreeEntry
	if(mEntry.notNull())
	{
		new_entry->setOctreeEntry(mEntry);
		mEntry = NULL;
	}

	//copy children
	S32 num_children = getNumOfChildren();
	for(S32 i = 0; i < num_children; i++)
	{
		new_entry->addChild(getChild(i));
	}
	mChildrenList.clear();
}

void LLVOCacheEntry::setState(U32 state)
{
	mState = state;

	if(getState() == ACTIVE)
	{
		const S32 MIN_REAVTIVE_INTERVAL = 32;
		U32 last_visible = getVisible();
		
		setVisible();

		if(getVisible() - last_visible < MIN_REAVTIVE_INTERVAL + mMinFrameRange)
		{
			mMinFrameRange = llmin(mMinFrameRange * 2, 2048);
		}
		else
		{
			mMinFrameRange = 64; //reset
		}
	}
}

//virtual 
S32  LLVOCacheEntry::getMinFrameRange()const
{
	return mMinFrameRange;
}

void LLVOCacheEntry::addChild(LLVOCacheEntry* entry)
{
	llassert(entry != NULL);

	mChildrenList.push_back(entry);
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
	setTouched();
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
		S32 size = mDP.getBufferSize();
		success = check_write(apr_file, (void*)&size, sizeof(S32));
	
		if(success)
		{
			success = check_write(apr_file, (void*)mBuffer, size);
		}
	}

	return success ;
}

void LLVOCacheEntry::calcSceneContribution(const LLVector3& camera_origin, bool needs_update, U32 last_update)
{
	if(!needs_update && getVisible() >= last_update)
	{
		return; //no need to update
	}

	const LLVector4a& center = getPositionGroup();
	
	LLVector4a origin;
	origin.load3(camera_origin.mV);

	LLVector4a lookAt;
	lookAt.setSub(center, origin);
	F32 squared_dist = lookAt.dot3(lookAt).getF32();

	F32 rad = getBinRadius();
	mSceneContrib = rad * rad / squared_dist;

	setVisible();
}

void LLVOCacheEntry::setBoundingInfo(const LLVector3& pos, const LLVector3& scale)
{
	LLVector4a center, newMin, newMax;
	center.load3(pos.mV);
	LLVector4a size;
	size.load3(scale.mV);
	newMin.setSub(center, size);
	newMax.setAdd(center, size);
	
	setPositionGroup(center);
	setSpatialExtents(newMin, newMax);
	setBinRadius(llmin(size.getLength3().getF32() * 4.f, 256.f));
}

//-------------------------------------------------------------------
//LLVOCachePartition
//-------------------------------------------------------------------
LLVOCachePartition::LLVOCachePartition(LLViewerRegion* regionp)
{
	mLODPeriod = 16;
	mRegionp = regionp;
	mPartitionType = LLViewerRegion::PARTITION_VO_CACHE;
	
	new LLOcclusionCullingGroup(mOctree, this);
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
	LLVOCacheOctreeCull(LLCamera* camera, LLViewerRegion* regionp, const LLVector3& shift, bool use_object_cache_occlusion) 
		: LLViewerOctreeCull(camera), 
		  mRegionp(regionp)
	{
		mLocalShift = shift;
		mUseObjectCacheOcclusion = (use_object_cache_occlusion && LLPipeline::sUseOcclusion);
	}

	virtual bool earlyFail(LLviewerOctreeGroup* base_group)
	{
		if( mUseObjectCacheOcclusion &&
			base_group->getOctreeNode()->getParent()) //never occlusion cull the root node
		{
			LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)base_group;
			if(group->needsUpdate())
			{
				return false; //needs to issue new occlusion culling check.
			}

			group->checkOcclusion();

			if (group->isOcclusionState(LLSpatialGroup::OCCLUDED))
			{
				return true;
			}
		}

		return false;
	}

	virtual S32 frustumCheck(const LLviewerOctreeGroup* group)
	{
#if 0
		S32 res = AABBInRegionFrustumGroupBounds(group);
#else	
		S32 res = AABBInRegionFrustumNoFarClipGroupBounds(group);
#endif
		if (res != 0)
		{
			res = llmin(res, AABBRegionSphereIntersectGroupExtents(group, mLocalShift));
		}
		return res;
	}

	virtual S32 frustumCheckObjects(const LLviewerOctreeGroup* group)
	{
#if 0
		S32 res = AABBInRegionFrustumObjectBounds(group);
#else
		S32 res = AABBInRegionFrustumNoFarClipObjectBounds(group);
#endif
		if (res != 0)
		{
			res = llmin(res, AABBRegionSphereIntersectObjectExtents(group, mLocalShift));
		}
		return res;
	}

	virtual void processGroup(LLviewerOctreeGroup* base_group)
	{
		if(mUseObjectCacheOcclusion && base_group->getOctreeNode()->getParent())
		{
			LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)base_group;
			if (group->needsUpdate() || group->mVisible[LLViewerCamera::sCurCameraID] < LLDrawable::getCurrentFrame() - 1)
			{
				((LLOcclusionCullingGroup*)group)->doOcclusion(mCamera);
				group->setVisible();
				return; //wait for occlusion culling results
			}
		}
		mRegionp->addVisibleGroup(base_group);
	}

private:
	LLViewerRegion* mRegionp;
	LLVector3       mLocalShift; //shift vector from agent space to local region space.
	bool            mUseObjectCacheOcclusion;
};

S32 LLVOCachePartition::cull(LLCamera &camera)
{
	static LLCachedControl<bool> use_object_cache_occlusion(gSavedSettings,"UseObjectCacheOcclusion");

	if(!LLViewerRegion::sVOCacheCullingEnabled)
	{
		return 0;
	}

	((LLviewerOctreeGroup*)mOctree->getListener(0))->rebound();

	//localize the camera
	LLVector3 region_agent = mRegionp->getOriginAgent();
	camera.calcRegionFrustumPlanes(region_agent);

	LLVOCacheOctreeCull culler(&camera, mRegionp, region_agent, use_object_cache_occlusion);
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


LLVOCache::LLVOCache():
	mInitialized(false),
	mReadOnly(true),
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
	mInitialized = true;

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
	
void LLVOCache::removeCache(ELLPath location, bool started) 
{
	if(started)
	{
		removeCache();
		return;
	}

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
	mInitialized = false;
}

void LLVOCache::removeCache() 
{
	if(!mInitialized)
	{
		//OK to remove cache even it is not initialized.
		llwarns << "Object cache is not initialized yet." << llendl;
	}

	if(mReadOnly)
	{
		llwarns << "Not clearing object cache: Cache is currently in read-only mode." << llendl;
		return ;
	}

	std::string mask = "*";
	llinfos << "Removing object cache at " << mObjectCacheDirName << llendl;
	gDirUtilp->deleteFilesInDir(mObjectCacheDirName, mask); 

	clearCacheInMemory() ;
	writeCacheHeader();
}

void LLVOCache::removeEntry(HeaderEntryInfo* entry) 
{
	llassert_always(mInitialized);
	if(mReadOnly)
	{
		return;
	}
	if(!entry)
	{
		return;
	}

	header_entry_queue_t::iterator iter = mHeaderEntryQueue.find(entry);
	if(iter != mHeaderEntryQueue.end())
	{		
		mHandleEntryMap.erase(entry->mHandle);		
		mHeaderEntryQueue.erase(iter);
		removeFromCache(entry);
		delete entry;

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

void LLVOCache::writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, BOOL dirty_cache, bool removal_enabled) 
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
				if(!removal_enabled || iter->second->isTouched())
				{
					success = iter->second->writeToFile(&apr_file) ;
				}
			}
		}
	}

	if(!success)
	{
		removeEntry(entry) ;

	}

	return ;
}
