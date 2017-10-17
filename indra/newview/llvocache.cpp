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
#include "llagentcamera.h"
#include "llmemory.h"

//static variables
U32 LLVOCacheEntry::sMinFrameRange = 0;
F32 LLVOCacheEntry::sNearRadius = 1.0f;
F32 LLVOCacheEntry::sRearFarRadius = 1.0f;
F32 LLVOCacheEntry::sFrontPixelThreshold = 1.0f;
F32 LLVOCacheEntry::sRearPixelThreshold = 1.0f;
BOOL LLVOCachePartition::sNeedsOcclusionCheck = FALSE;

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
:	LLTrace::MemTrackable<LLVOCacheEntry, 16>("LLVOCacheEntry"),
	LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
	mLocalID(local_id),
	mCRC(crc),
	mUpdateFlags(-1),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mState(INACTIVE),
	mSceneContrib(0.f),
	mValid(TRUE),
	mParentID(0),
	mBSphereRadius(-1.0f)
{
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
:	LLTrace::MemTrackable<LLVOCacheEntry, 16>("LLVOCacheEntry"),
	LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
	mLocalID(0),
	mCRC(0),
	mUpdateFlags(-1),
	mHitCount(0),
	mDupeCount(0),
	mCRCChangeCount(0),
	mBuffer(NULL),
	mState(INACTIVE),
	mSceneContrib(0.f),
	mValid(TRUE),
	mParentID(0),
	mBSphereRadius(-1.0f)
{
	mDP.assignBuffer(mBuffer, 0);
}

LLVOCacheEntry::LLVOCacheEntry(LLAPRFile* apr_file)
:	LLTrace::MemTrackable<LLVOCacheEntry, 16>("LLVOCacheEntry"),
	LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY), 
	mBuffer(NULL),
	mUpdateFlags(-1),
	mState(INACTIVE),
	mSceneContrib(0.f),
	mValid(FALSE),
	mParentID(0),
	mBSphereRadius(-1.0f)
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
			LL_WARNS() << "Bogus cache entry, size " << size << ", aborting!" << LL_ENDL;
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
		mState = INACTIVE;
	}
}

LLVOCacheEntry::~LLVOCacheEntry()
{
	mDP.freeBuffer();
}

void LLVOCacheEntry::updateEntry(U32 crc, LLDataPackerBinaryBuffer &dp)
{
	if(mCRC != crc)
	{
		mCRC = crc;
		mCRCChangeCount++;
	}

	mDP.freeBuffer();

	llassert_always(dp.getBufferSize() > 0);
	mBuffer = new U8[dp.getBufferSize()];
	mDP.assignBuffer(mBuffer, dp.getBufferSize());
	mDP = dp;
}

void LLVOCacheEntry::setParentID(U32 id) 
{
	if(mParentID != id)
	{
		removeAllChildren();
		mParentID = id;
	}
}

void LLVOCacheEntry::removeAllChildren()
{
	if(mChildrenList.empty())
	{
		return;
	}

	for(vocache_entry_set_t::iterator iter = mChildrenList.begin(); iter != mChildrenList.end(); ++iter)
 	{
		(*iter)->setParentID(0);
 	}
	mChildrenList.clear();

	return;
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

void LLVOCacheEntry::setState(U32 state)
{
	if(state > LOW_BITS) //special states
	{
		mState |= (HIGH_BITS & state);
		return;
	}

	//
	//otherwise LOW_BITS states
	//
	clearState(LOW_BITS);
	mState |= (LOW_BITS & state);

	if(getState() == ACTIVE)
	{
		const S32 MIN_INTERVAL = 64 + sMinFrameRange;
		U32 last_visible = getVisible();
		
		setVisible();

		U32 cur_visible = getVisible();
		if(cur_visible - last_visible > MIN_INTERVAL ||
			cur_visible < MIN_INTERVAL)
		{
			mLastCameraUpdated = 0; //reset
		}
		else
		{
			mLastCameraUpdated = LLViewerRegion::sLastCameraUpdated;
		}
	}
}

void LLVOCacheEntry::addChild(LLVOCacheEntry* entry)
{
	llassert(entry != NULL);
	llassert(entry->getParentID() == mLocalID);
	llassert(entry->getEntry() != NULL);

	if(!entry || !entry->getEntry() || entry->getParentID() != mLocalID)
	{
		return;
	}
	
	mChildrenList.insert(entry);

	//update parent bbox
	if(getEntry() != NULL && isState(INACTIVE))
	{
		updateParentBoundingInfo(entry);
		resetVisible();
	}
}
	
void LLVOCacheEntry::removeChild(LLVOCacheEntry* entry)
{
	entry->setParentID(0);

	vocache_entry_set_t::iterator iter = mChildrenList.find(entry);
	if(iter != mChildrenList.end())
	{
		mChildrenList.erase(iter);
	}
}

//remove the first child, and return it.
LLVOCacheEntry* LLVOCacheEntry::getChild()
{
	LLVOCacheEntry* child = NULL;
	vocache_entry_set_t::iterator iter = mChildrenList.begin();
	if(iter != mChildrenList.end())
	{
		child = *iter;
		mChildrenList.erase(iter);
	}

	return child;
}

LLDataPackerBinaryBuffer *LLVOCacheEntry::getDP()
{
	if (mDP.getBufferSize() == 0)
	{
		//LL_INFOS() << "Not getting cache entry, invalid!" << LL_ENDL;
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
	LL_INFOS() << "local " << mLocalID
		<< " crc " << mCRC
		<< " hits " << mHitCount
		<< " dupes " << mDupeCount
		<< " change " << mCRCChangeCount
		<< LL_ENDL;
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

//static 
void LLVOCacheEntry::updateDebugSettings()
{
	static LLFrameTimer timer;
	if(timer.getElapsedTimeF32() < 1.0f) //update frequency once per second.
	{
		return;
	}
	timer.reset();

	//the number of frames invisible objects stay in memory
	static LLCachedControl<U32> inv_obj_time(gSavedSettings,"NonvisibleObjectsInMemoryTime");
	sMinFrameRange = inv_obj_time - 1; //make 0 to be the maximum 

	//min radius: all objects within this radius remain loaded in memory
	static LLCachedControl<F32> min_radius(gSavedSettings,"SceneLoadMinRadius");
	sNearRadius = llmin((F32)min_radius, gAgentCamera.mDrawDistance); //can not exceed the draw distance
	sNearRadius = llmax(sNearRadius, 1.f); //minimum value is 1.0m

	//objects within the view frustum whose visible area is greater than this threshold will be loaded
	static LLCachedControl<F32> front_pixel_threshold(gSavedSettings,"SceneLoadFrontPixelThreshold");
	sFrontPixelThreshold = front_pixel_threshold;

	//objects out of the view frustum whose visible area is greater than this threshold will remain loaded
	static LLCachedControl<F32> rear_pixel_threshold(gSavedSettings,"SceneLoadRearPixelThreshold");
	sRearPixelThreshold = rear_pixel_threshold;
	sRearPixelThreshold = llmax(sRearPixelThreshold, sFrontPixelThreshold); //can not be smaller than sFrontPixelThreshold.

	// a percentage of draw distance beyond which all objects outside of view frustum will be unloaded, regardless of pixel threshold
	static LLCachedControl<F32> rear_max_radius_frac(gSavedSettings,"SceneLoadRearMaxRadiusFraction");
	sRearFarRadius = llmax(rear_max_radius_frac * gAgentCamera.mDrawDistance / 100.f, 1.0f); //minimum value is 1.0m
	sRearFarRadius = llmax(sRearFarRadius, (F32)min_radius); //can not be less than "SceneLoadMinRadius".
	sRearFarRadius = llmin(sRearFarRadius, gAgentCamera.mDrawDistance); //can not be more than the draw distance.

	//make the above parameters adaptive to memory usage
	//starts to put restrictions from low_mem_bound_MB, apply tightest restrictions when hits high_mem_bound_MB
	static LLCachedControl<U32> low_mem_bound_MB(gSavedSettings,"SceneLoadLowMemoryBound");
	static LLCachedControl<U32> high_mem_bound_MB(gSavedSettings,"SceneLoadHighMemoryBound");
	
	LLMemory::updateMemoryInfo() ;
	U32 allocated_mem = LLMemory::getAllocatedMemKB().value();
	allocated_mem /= 1024; //convert to MB.
	if(allocated_mem < low_mem_bound_MB)
	{
		return; 
	}
	F32 adjust_factor = llmax(0.f, (F32)(high_mem_bound_MB - allocated_mem) / (high_mem_bound_MB - low_mem_bound_MB));

	sRearFarRadius = llmin(adjust_factor * sRearFarRadius, 96.f);  //[0.f, 96.f]
	sMinFrameRange = (U32)llclamp(adjust_factor * sMinFrameRange, 10.f, 64.f);  //[10, 64]
	sNearRadius    = llmax(adjust_factor * sNearRadius, 1.0f);
}

//static 
F32 LLVOCacheEntry::getSquaredPixelThreshold(bool is_front)
{
	F32 threshold;
	if(is_front)
	{
		threshold = sFrontPixelThreshold;
	}
	else
	{
		threshold = sRearPixelThreshold;
	}

	//object projected area threshold
	F32 pixel_meter_ratio = LLViewerCamera::getInstance()->getPixelMeterRatio();
	F32 projection_threshold = pixel_meter_ratio > 0.f ? threshold / pixel_meter_ratio : 0.f;
	projection_threshold *= projection_threshold;

	return projection_threshold;
}

bool LLVOCacheEntry::isAnyVisible(const LLVector4a& camera_origin, const LLVector4a& local_camera_origin, F32 dist_threshold)
{
	LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)getGroup();
	if(!group)
	{
		return false;
	}

	//any visible
	bool vis = group->isAnyRecentlyVisible();

	//not ready to remove
	if(!vis)
	{
		S32 cur_vis = llmax(group->getAnyVisible(), (S32)getVisible());
		vis = (cur_vis + sMinFrameRange > LLViewerOctreeEntryData::getCurrentFrame());
	}

	//within the back sphere
	if(!vis && !mParentID && !group->isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
	{
		LLVector4a lookAt;

		if(mBSphereRadius > 0.f)
		{
			lookAt.setSub(mBSphereCenter, local_camera_origin);		
			dist_threshold += mBSphereRadius;
		}
		else
		{
			lookAt.setSub(getPositionGroup(), camera_origin);
			dist_threshold += getBinRadius();
		}

		vis = (lookAt.dot3(lookAt).getF32() < dist_threshold * dist_threshold);
	}

	return vis;
}

void LLVOCacheEntry::calcSceneContribution(const LLVector4a& camera_origin, bool needs_update, U32 last_update, F32 max_dist)
{
	if(!needs_update && getVisible() >= last_update)
	{
		return; //no need to update
	}

	LLVector4a lookAt;
	lookAt.setSub(getPositionGroup(), camera_origin);
	F32 distance = lookAt.getLength3().getF32();
	distance -= sNearRadius;

	if(distance <= 0.f)
	{
		//nearby objects, set a large number
		const F32 LARGE_SCENE_CONTRIBUTION = 1000.f; //a large number to force to load the object.
		mSceneContrib = LARGE_SCENE_CONTRIBUTION;
	}
	else
	{
		F32 rad = getBinRadius();
		max_dist += rad;

		if(distance + sNearRadius < max_dist)
		{
			mSceneContrib = (rad * rad) / distance;		
		}
		else
		{
			mSceneContrib = 0.f; //out of draw distance, not to load
		}
	}

	setVisible();
}

void LLVOCacheEntry::saveBoundingSphere()
{
	mBSphereCenter = getPositionGroup();
	mBSphereRadius = getBinRadius();
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

	if(getNumOfChildren() > 0) //has children
	{
		updateParentBoundingInfo();
	}
	else
	{
		setBinRadius(llmin(size.getLength3().getF32() * 4.f, 256.f));
	}
}

//make the parent bounding box to include all children
void LLVOCacheEntry::updateParentBoundingInfo()
{
	if(mChildrenList.empty())
	{
		return;
	}

	for(vocache_entry_set_t::iterator iter = mChildrenList.begin(); iter != mChildrenList.end(); ++iter)
	{
		updateParentBoundingInfo(*iter);
	}
	resetVisible();
}

//make the parent bounding box to include this child
void LLVOCacheEntry::updateParentBoundingInfo(const LLVOCacheEntry* child)
{
	const LLVector4a* child_exts = child->getSpatialExtents();
	LLVector4a newMin, newMax;
	newMin = child_exts[0];
	newMax = child_exts[1];
	
	//move to regional space.
	{
		const LLVector4a& parent_pos = getPositionGroup();
		newMin.add(parent_pos);
		newMax.add(parent_pos);
	}

	//update parent's bbox(min, max)
	const LLVector4a* parent_exts = getSpatialExtents();
	update_min_max(newMin, newMax, parent_exts[0]);
	update_min_max(newMin, newMax, parent_exts[1]);
	for(S32 i = 0; i < 4; i++)
	{
		llclamp(newMin[i], 0.f, 256.f);
		llclamp(newMax[i], 0.f, 256.f);
	}
	setSpatialExtents(newMin, newMax);

	//update parent's bbox center
	LLVector4a center;
	center.setAdd(newMin, newMax);
	center.mul(0.5f);
	setPositionGroup(center);	

	//update parent's bbox size vector
	LLVector4a size;
	size.setSub(newMax, newMin);
	size.mul(0.5f);
	setBinRadius(llmin(size.getLength3().getF32() * 4.f, 256.f));
}
//-------------------------------------------------------------------
//LLVOCachePartition
//-------------------------------------------------------------------
LLVOCacheGroup::~LLVOCacheGroup()
{
	if(mOcclusionState[LLViewerCamera::CAMERA_WORLD] & ACTIVE_OCCLUSION)
	{
		((LLVOCachePartition*)mSpatialPartition)->removeOccluder(this);
	}
}

//virtual
void LLVOCacheGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	if (child->getListenerCount() == 0)
	{
		new LLVOCacheGroup(child, mSpatialPartition);
	}
	else
	{
		OCT_ERRS << "LLVOCacheGroup redundancy detected." << LL_ENDL;
	}

	unbound();
	
	((LLViewerOctreeGroup*)child->getListener(0))->unbound();
}

LLVOCachePartition::LLVOCachePartition(LLViewerRegion* regionp)
:	LLTrace::MemTrackable<LLVOCachePartition>("LLVOCachePartition")
{
	mLODPeriod = 16;
	mRegionp = regionp;
	mPartitionType = LLViewerRegion::PARTITION_VO_CACHE;
	mBackSlectionEnabled = -1;
	mIdleHash = 0;
	
	for(S32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
	{
		mCulledTime[i] = 0;	
	}
	mCullHistory = -1;

	new LLVOCacheGroup(mOctree, this);
}

bool LLVOCachePartition::addEntry(LLViewerOctreeEntry* entry)
{
	llassert(entry->hasVOCacheEntry());

	if(!llfinite(entry->getBinRadius()) || !entry->getPositionGroup().isFinite3())
	{
		return false; //data corrupted
	}

	mOctree->insert(entry);

	return true;
}
	
void LLVOCachePartition::removeEntry(LLViewerOctreeEntry* entry)
{
	entry->getVOCacheEntry()->setGroup(NULL);

	llassert(!entry->getGroup());
}
	
class LLVOCacheOctreeCull : public LLViewerOctreeCull
{
public:
	LLVOCacheOctreeCull(LLCamera* camera, LLViewerRegion* regionp, 
		const LLVector3& shift, bool use_object_cache_occlusion, F32 pixel_threshold, LLVOCachePartition* part) 
		: LLViewerOctreeCull(camera), 
		  mRegionp(regionp),
		  mPartition(part),
		  mPixelThreshold(pixel_threshold)
	{
		mLocalShift = shift;
		mUseObjectCacheOcclusion = use_object_cache_occlusion;
		mNearRadius = LLVOCacheEntry::sNearRadius;
	}

	virtual bool earlyFail(LLViewerOctreeGroup* base_group)
	{
		if( mUseObjectCacheOcclusion &&
			base_group->getOctreeNode()->getParent()) //never occlusion cull the root node
		{
			LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)base_group;
			if(group->needsUpdate())
			{
				//needs to issue new occlusion culling check, perform view culling check first.
				return false;
			}

			group->checkOcclusion();

			if (group->isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
			{
				return true;
			}
		}

		return false;
	}

	virtual S32 frustumCheck(const LLViewerOctreeGroup* group)
	{
#if 0
		S32 res = AABBInRegionFrustumGroupBounds(group);
#else	
		S32 res = AABBInRegionFrustumNoFarClipGroupBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBRegionSphereIntersectGroupExtents(group, mLocalShift));
		}
#endif

		return res;
	}

	virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group)
	{
#if 0
		S32 res = AABBInRegionFrustumObjectBounds(group);
#else
		S32 res = AABBInRegionFrustumNoFarClipObjectBounds(group);
		if (res != 0)
		{
			res = llmin(res, AABBRegionSphereIntersectObjectExtents(group, mLocalShift));
		}
#endif

		if(res != 0)
		{
			//check if the objects projection large enough
			const LLVector4a* exts = group->getObjectExtents();
			res = checkProjectionArea(exts[0], exts[1], mLocalShift, mPixelThreshold, mNearRadius);
		}

		return res;
	}

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		if( !mUseObjectCacheOcclusion ||
			!base_group->getOctreeNode()->getParent())
		{ 
			//no occlusion check
			if(mRegionp->addVisibleGroup(base_group))
			{
				base_group->setVisible();
			}
			return;
		}

		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)base_group;
		if(group->needsUpdate() || !group->isRecentlyVisible())//needs to issue new occlusion culling check.
		{
			mPartition->addOccluders(group);
			group->setVisible();
			return ; //wait for occlusion culling result
		}

		if(group->isOcclusionState(LLOcclusionCullingGroup::QUERY_PENDING) || 
			group->isOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION))
		{
			//keep waiting
			group->setVisible();
		}
		else
		{
			if(mRegionp->addVisibleGroup(base_group))
			{
				base_group->setVisible();
			}
		}
	}

private:
	LLVOCachePartition* mPartition;
	LLViewerRegion*     mRegionp;
	LLVector3           mLocalShift; //shift vector from agent space to local region space.
	F32                 mPixelThreshold;
	F32                 mNearRadius;
	bool                mUseObjectCacheOcclusion;
};

//select objects behind camera
class LLVOCacheOctreeBackCull : public LLViewerOctreeCull
{
public:
	LLVOCacheOctreeBackCull(LLCamera* camera, const LLVector3& shift, LLViewerRegion* regionp, F32 pixel_threshold, bool use_occlusion) 
		: LLViewerOctreeCull(camera), mRegionp(regionp), mPixelThreshold(pixel_threshold), mUseObjectCacheOcclusion(use_occlusion)
	{
		mLocalShift = shift;
		mSphereRadius = LLVOCacheEntry::sRearFarRadius;
	}
	
	virtual bool earlyFail(LLViewerOctreeGroup* base_group)
	{
		if( mUseObjectCacheOcclusion &&
			base_group->getOctreeNode()->getParent()) //never occlusion cull the root node
		{
			LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*)base_group;

			if (group->getOcclusionState() > 0) //occlusion state is not clear.
			{
				return true;
			}
		}

		return false;
	}

	virtual S32 frustumCheck(const LLViewerOctreeGroup* group)
	{			
		const LLVector4a* exts = group->getExtents();
		return backSphereCheck(exts[0], exts[1]);
	}

	virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group)
	{
		const LLVector4a* exts = group->getObjectExtents();
		if(backSphereCheck(exts[0], exts[1]))
		{
			//check if the objects projection large enough
			const LLVector4a* exts = group->getObjectExtents();
			return checkProjectionArea(exts[0], exts[1], mLocalShift, mPixelThreshold, mSphereRadius);
		}
		return false;
	}

	virtual void processGroup(LLViewerOctreeGroup* base_group)
	{
		mRegionp->addVisibleGroup(base_group);
		return;
	}

private:
	//a sphere around the camera origin, including objects behind camera.
	S32 backSphereCheck(const LLVector4a& min, const LLVector4a& max)
	{
		return AABBSphereIntersect(min, max, mCamera->getOrigin() - mLocalShift, mSphereRadius);
	}

private:
	F32              mSphereRadius;
	LLViewerRegion*  mRegionp;
	LLVector3        mLocalShift; //shift vector from agent space to local region space.
	F32              mPixelThreshold;
	bool             mUseObjectCacheOcclusion;
};

void LLVOCachePartition::selectBackObjects(LLCamera &camera, F32 pixel_threshold, bool use_occlusion)
{
	if(LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		return;
	}

	if(mBackSlectionEnabled < 0)
	{
		mBackSlectionEnabled = LLVOCacheEntry::sMinFrameRange - 1;
		mBackSlectionEnabled = llmax(mBackSlectionEnabled, (S32)1);
	}

	if(!mBackSlectionEnabled)
	{
		return;
	}

	//localize the camera
	LLVector3 region_agent = mRegionp->getOriginAgent();
	
	LLVOCacheOctreeBackCull culler(&camera, region_agent, mRegionp, pixel_threshold, use_occlusion);
	culler.traverse(mOctree);

	mBackSlectionEnabled--;
	if(!mRegionp->getNumOfVisibleGroups())
	{
		mBackSlectionEnabled = 0;
	}

	return;
}

S32 LLVOCachePartition::cull(LLCamera &camera, bool do_occlusion)
{
	static LLCachedControl<bool> use_object_cache_occlusion(gSavedSettings,"UseObjectCacheOcclusion");
	
	if(!LLViewerRegion::sVOCacheCullingEnabled)
	{
		return 0;
	}
	if(mRegionp->isPaused())
	{
		return 0;
	}

	((LLViewerOctreeGroup*)mOctree->getListener(0))->rebound();

	if(LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		return 0; //no need for those cameras.
	}

	if(mCulledTime[LLViewerCamera::sCurCameraID] == LLViewerOctreeEntryData::getCurrentFrame())
	{
		return 0; //already culled
	}
	mCulledTime[LLViewerCamera::sCurCameraID] = LLViewerOctreeEntryData::getCurrentFrame();

	if(!mCullHistory && LLViewerRegion::isViewerCameraStatic())
	{
		U32 seed = llmax(mLODPeriod >> 1, (U32)4);
		if(LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
		{
			if(!(LLViewerOctreeEntryData::getCurrentFrame() % seed))
			{
				mIdleHash = (mIdleHash + 1) % seed;
			}
		}
		if(LLViewerOctreeEntryData::getCurrentFrame() % seed != mIdleHash)
		{
			mFrontCull = FALSE;

			//process back objects selection
			selectBackObjects(camera, LLVOCacheEntry::getSquaredPixelThreshold(mFrontCull), 
				do_occlusion && use_object_cache_occlusion);
			return 0; //nothing changed, reduce frequency of culling
		}
	}
	else
	{
		mBackSlectionEnabled = -1; //reset it.
	}

	//localize the camera
	LLVector3 region_agent = mRegionp->getOriginAgent();
	camera.calcRegionFrustumPlanes(region_agent, gAgentCamera.mDrawDistance);

	mFrontCull = TRUE;
	LLVOCacheOctreeCull culler(&camera, mRegionp, region_agent, do_occlusion && use_object_cache_occlusion, 
		LLVOCacheEntry::getSquaredPixelThreshold(mFrontCull), this);
	culler.traverse(mOctree);	

	if(!sNeedsOcclusionCheck)
	{
		sNeedsOcclusionCheck = !mOccludedGroups.empty();
	}
	return 1;
}

void LLVOCachePartition::setCullHistory(BOOL has_new_object)
{
	mCullHistory <<= 1;
	mCullHistory |= has_new_object;
}

void LLVOCachePartition::addOccluders(LLViewerOctreeGroup* gp)
{
	LLVOCacheGroup* group = (LLVOCacheGroup*)gp;

	if(!group->isOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION))
	{
		group->setOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION);
		mOccludedGroups.insert(group);
	}
}

void LLVOCachePartition::processOccluders(LLCamera* camera)
{
	if(mOccludedGroups.empty())
	{
		return;
	}
	if(LLViewerCamera::sCurCameraID != LLViewerCamera::CAMERA_WORLD)
	{
		return; //no need for those cameras.
	}

	LLVector3 region_agent = mRegionp->getOriginAgent();
	LLVector4a shift(region_agent[0], region_agent[1], region_agent[2]);
	for(std::set<LLVOCacheGroup*>::iterator iter = mOccludedGroups.begin(); iter != mOccludedGroups.end(); ++iter)
	{
		LLVOCacheGroup* group = *iter;
		if(group->isOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION))
		{
			group->doOcclusion(camera, &shift);
			group->clearOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION);
		}
	}

	//safe to clear mOccludedGroups here because only the world camera accesses it.
	mOccludedGroups.clear();
	sNeedsOcclusionCheck = FALSE;
}

void LLVOCachePartition::resetOccluders()
{
	if(mOccludedGroups.empty())
	{
		return;
	}

	for(std::set<LLVOCacheGroup*>::iterator iter = mOccludedGroups.begin(); iter != mOccludedGroups.end(); ++iter)
	{
		LLVOCacheGroup* group = *iter;
		group->clearOcclusionState(LLOcclusionCullingGroup::ACTIVE_OCCLUSION);
	}	
	mOccludedGroups.clear();
	sNeedsOcclusionCheck = FALSE;
}

void LLVOCachePartition::removeOccluder(LLVOCacheGroup* group)
{
	if(mOccludedGroups.empty())
	{
		return;
	}
	mOccludedGroups.erase(group);
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
		LL_WARNS() << "Not initializing cache: Cache is currently disabled." << LL_ENDL;
		return ;
	}

	if(mInitialized)
	{
		LL_WARNS() << "Cache already initialized." << LL_ENDL;
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

#if defined(ADDRESS_SIZE)
	U32 expected_address = ADDRESS_SIZE;
#else
	U32 expected_address = 32;
#endif
	mMetaInfo.mAddressSize = expected_address;

	readCacheHeader();	

	if( mMetaInfo.mVersion != cache_version
		|| mMetaInfo.mAddressSize != expected_address) 
	{
		mMetaInfo.mVersion = cache_version ;
		mMetaInfo.mAddressSize = expected_address;
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
		LL_WARNS() << "Not removing cache at " << location << ": Cache is currently in read-only mode." << LL_ENDL;
		return ;
	}	

	LL_INFOS() << "about to remove the object cache due to settings." << LL_ENDL ;

	std::string mask = "*";
	std::string cache_dir = gDirUtilp->getExpandedFilename(location, object_cache_dirname);
	LL_INFOS() << "Removing cache at " << cache_dir << LL_ENDL;
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
		LL_WARNS() << "Object cache is not initialized yet." << LL_ENDL;
	}

	if(mReadOnly)
	{
		LL_WARNS() << "Not clearing object cache: Cache is currently in read-only mode." << LL_ENDL;
		return ;
	}

	std::string mask = "*";
	LL_INFOS() << "Removing object cache at " << mObjectCacheDirName << LL_ENDL;
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
		LL_WARNS() << "Not removing cache for handle " << entry->mHandle << ": Cache is currently in read-only mode." << LL_ENDL;
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
		LL_WARNS() << "Not reading cache header: Cache is currently disabled." << LL_ENDL;
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
					LL_WARNS() << "Error reading cache header entry. (entry_index=" << mNumEntries << ")" << LL_ENDL;
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
		//	LL_INFOS() << name << LL_ENDL ;
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
		LL_WARNS() << "Not writing cache header: Cache is currently disabled." << LL_ENDL;
		return;
	}

	if(mReadOnly)
	{
		LL_WARNS() << "Not writing cache header: Cache is currently in read-only mode." << LL_ENDL;
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
		LL_WARNS() << "Not reading cache for handle " << handle << "): Cache is currently disabled." << LL_ENDL;
		return ;
	}
	llassert_always(mInitialized);

	handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
	if(iter == mHandleEntryMap.end()) //no cache
	{
		LL_WARNS() << "No handle map entry for " << handle << LL_ENDL;
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
				LL_INFOS() << "Cache ID doesn't match for this region, discarding"<< LL_ENDL;
				success = false ;
			}

			if(success)
			{
				S32 num_entries;
				success = check_read(&apr_file, &num_entries, sizeof(S32)) ;
	
				if(success)
				{
					for (S32 i = 0; i < num_entries && apr_file.eof() != APR_EOF; i++)
					{
						LLPointer<LLVOCacheEntry> entry = new LLVOCacheEntry(&apr_file);
						if (!entry->getLocalID())
						{
							LL_WARNS() << "Aborting cache file load for " << filename << ", cache file corruption!" << LL_ENDL;
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
		LL_WARNS() << "Not writing cache for handle " << handle << "): Cache is currently disabled." << LL_ENDL;
		return ;
	}
	llassert_always(mInitialized);

	if(mReadOnly)
	{
		LL_WARNS() << "Not writing cache for handle " << handle << "): Cache is currently in read-only mode." << LL_ENDL;
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
		LL_WARNS() << "Failed to update cache header index " << entry->mIndex << ". handle = " << handle << LL_ENDL;
		return ; //update failed.
	}

	if(!dirty_cache)
	{
		LL_WARNS() << "Skipping write to cache for handle " << handle << ": cache not dirty" << LL_ENDL;
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
				if(!removal_enabled || iter->second->isValid())
				{
					success = iter->second->writeToFile(&apr_file) ;
					if(!success)
					{
						break;
					}
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
