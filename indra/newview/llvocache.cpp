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
#include "llregionhandle.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "lldrawable.h"
#include "llviewerregion.h"
#include "llagentcamera.h"
#include "llsdserialize.h"
#include "llworld.h" // For LLWorld::getInstance()
//static variables
U32 LLVOCacheEntry::sMinFrameRange = 0;
F32 LLVOCacheEntry::sNearRadius = 1.0f;
F32 LLVOCacheEntry::sRearFarRadius = 1.0f;
F32 LLVOCacheEntry::sFrontPixelThreshold = 1.0f;
F32 LLVOCacheEntry::sRearPixelThreshold = 1.0f;
bool LLVOCachePartition::sNeedsOcclusionCheck = false;

const S32 ENTRY_HEADER_SIZE = 6 * sizeof(S32);
const S32 MAX_ENTRY_BODY_SIZE = 10000;

bool check_read(LLAPRFile* apr_file, void* src, S32 n_bytes)
{
    return apr_file->read(src, n_bytes) == n_bytes ;
}

bool check_write(LLAPRFile* apr_file, void* src, S32 n_bytes)
{
    return apr_file->write(src, n_bytes) == n_bytes ;
}

// Material Override Cache needs a version label, so we can upgrade this later.
const std::string LLGLTFOverrideCacheEntry::VERSION_LABEL = {"GLTFCacheVer"};
const int LLGLTFOverrideCacheEntry::VERSION = 1;

bool LLGLTFOverrideCacheEntry::fromLLSD(const LLSD& data)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK;

    llassert(data.has("local_id"));
    llassert(data.has("object_id"));
    llassert(data.has("region_handle_x") && data.has("region_handle_y"));

    if (!data.has("local_id"))
    {
        return false;
    }

    if (data.has("region_handle_x") && data.has("region_handle_y"))
    {
        // TODO start requiring this once server sends this for all messages
        U32 region_handle_y = data["region_handle_y"].asInteger();
        U32 region_handle_x = data["region_handle_x"].asInteger();
        mRegionHandle = to_region_handle(region_handle_x, region_handle_y);
    }
    else
    {
        return false;
    }

    mLocalId = data["local_id"].asInteger();
    mObjectId = data["object_id"];

    // message should be interpreted thusly:
    ///  sides is a list of face indices
    //   gltf_llsd is a list of corresponding GLTF override LLSD
    //   any side not represented in "sides" has no override
    if (data.has("sides") && data.has("gltf_llsd"))
    {
        LLSD const& sides = data.get("sides");
        LLSD const& gltf_llsd = data.get("gltf_llsd");

        if (sides.isArray() && gltf_llsd.isArray() &&
            sides.size() != 0 &&
            sides.size() == gltf_llsd.size())
        {
            for (int i = 0; i < sides.size(); ++i)
            {
                S32 side_idx = sides[i].asInteger();
                mSides[side_idx] = gltf_llsd[i];
                LLGLTFMaterial* override_mat = new LLGLTFMaterial();
                override_mat->applyOverrideLLSD(gltf_llsd[i]);
                mGLTFMaterial[side_idx] = override_mat;
            }
        }
        else
        {
            LL_WARNS_IF(sides.size() != 0, "GLTF") << "broken override cache entry" << LL_ENDL;
        }
    }

    llassert(mSides.size() == mGLTFMaterial.size());
#ifdef SHOW_ASSERT
    for (auto const & side : mSides)
    {
        // check that mSides and mGLTFMaterial have exactly the same keys present
        llassert(mGLTFMaterial.count(side.first) == 1);
    }
#endif

    return true;
}

LLSD LLGLTFOverrideCacheEntry::toLLSD() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK;
    LLSD data;
    U32 region_handle_x, region_handle_y;
    from_region_handle(mRegionHandle, &region_handle_x, &region_handle_y);
    data["region_handle_y"] = LLSD::Integer(region_handle_y);
    data["region_handle_x"] = LLSD::Integer(region_handle_x);

    data["object_id"] = mObjectId;
    data["local_id"] = (LLSD::Integer) mLocalId;

    llassert(mSides.size() == mGLTFMaterial.size());
    for (auto const & side : mSides)
    {
        // check that mSides and mGLTFMaterial have exactly the same keys present
        llassert(mGLTFMaterial.count(side.first) == 1);
        data["sides"].append(LLSD::Integer(side.first));
        data["gltf_llsd"].append(side.second);
    }

    return data;
}

//---------------------------------------------------------------------------
// LLVOCacheEntry
//---------------------------------------------------------------------------

LLVOCacheEntry::LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp)
:   LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
    mLocalID(local_id),
    mCRC(crc),
    mUpdateFlags(-1),
    mHitCount(0),
    mDupeCount(0),
    mCRCChangeCount(0),
    mState(INACTIVE),
    mSceneContrib(0.f),
    mValid(true),
    mParentID(0),
    mBSphereRadius(-1.0f)
{
    mBuffer = new U8[dp.getBufferSize()];
    mDP.assignBuffer(mBuffer, dp.getBufferSize());
    mDP = dp;
}

LLVOCacheEntry::LLVOCacheEntry()
:   LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
    mLocalID(0),
    mCRC(0),
    mUpdateFlags(-1),
    mHitCount(0),
    mDupeCount(0),
    mCRCChangeCount(0),
    mBuffer(NULL),
    mState(INACTIVE),
    mSceneContrib(0.f),
    mValid(true),
    mParentID(0),
    mBSphereRadius(-1.0f)
{
    mDP.assignBuffer(mBuffer, 0);
}

LLVOCacheEntry::LLVOCacheEntry(LLAPRFile* apr_file)
:   LLViewerOctreeEntryData(LLViewerOctreeEntry::LLVOCACHEENTRY),
    mBuffer(NULL),
    mUpdateFlags(-1),
    mState(INACTIVE),
    mSceneContrib(0.f),
    mValid(false),
    mParentID(0),
    mBSphereRadius(-1.0f)
{
    S32 size = -1;
    bool success;
    static U8 data_buffer[ENTRY_HEADER_SIZE];

    mDP.assignBuffer(mBuffer, 0);

    success = check_read(apr_file, (void *)data_buffer, ENTRY_HEADER_SIZE);
    if (success)
    {
        memcpy(&mLocalID, data_buffer, sizeof(U32));
        memcpy(&mCRC, data_buffer + sizeof(U32), sizeof(U32));
        memcpy(&mHitCount, data_buffer + (2 * sizeof(U32)), sizeof(S32));
        memcpy(&mDupeCount, data_buffer + (3 * sizeof(U32)), sizeof(S32));
        memcpy(&mCRCChangeCount, data_buffer + (4 * sizeof(U32)), sizeof(S32));
        memcpy(&size, data_buffer + (5 * sizeof(U32)), sizeof(S32));

        // Corruption in the cache entries
        if ((size > MAX_ENTRY_BODY_SIZE) || (size < 1))
        {
            // We've got a bogus size, skip reading it.
            // We won't bother seeking, because the rest of this file
            // is likely bogus, and will be tossed anyway.
            LL_WARNS() << "Bogus cache entry, size " << size << ", aborting!" << LL_ENDL;
            success = false;
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
            // Improve logging around vocache
            LL_WARNS() << "Error loading cache entry for " << mLocalID << ", size " << size << " aborting!" << LL_ENDL;
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
        const U32 MIN_INTERVAL = 64U + sMinFrameRange;
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

S32 LLVOCacheEntry::writeToBuffer(U8 *data_buffer) const
{
    S32 size = mDP.getBufferSize();

    if (size > MAX_ENTRY_BODY_SIZE)
    {
        LL_WARNS() << "Failed to write entry with size above allowed limit: " << size << LL_ENDL;
        return 0;
    }

    memcpy(data_buffer, &mLocalID, sizeof(U32));
    memcpy(data_buffer + sizeof(U32), &mCRC, sizeof(U32));
    memcpy(data_buffer + (2 * sizeof(U32)), &mHitCount, sizeof(S32));
    memcpy(data_buffer + (3 * sizeof(U32)), &mDupeCount, sizeof(S32));
    memcpy(data_buffer + (4 * sizeof(U32)), &mCRCChangeCount, sizeof(S32));
    memcpy(data_buffer + (5 * sizeof(U32)), &size, sizeof(S32));
    memcpy(data_buffer + ENTRY_HEADER_SIZE, (void*)mBuffer, size);

    return ENTRY_HEADER_SIZE + size;
}

#ifndef LL_TEST
//static
void LLVOCacheEntry::updateDebugSettings()
{
    static LLFrameTimer timer;
    if(timer.getElapsedTimeF32() < 1.0f) //update frequency once per second.
    {
        return;
    }
    timer.reset();

    //objects within the view frustum whose visible area is greater than this threshold will be loaded
    static LLCachedControl<F32> front_pixel_threshold(gSavedSettings,"SceneLoadFrontPixelThreshold");
    sFrontPixelThreshold = front_pixel_threshold;

    //objects out of the view frustum whose visible area is greater than this threshold will remain loaded
    static LLCachedControl<F32> rear_pixel_threshold(gSavedSettings,"SceneLoadRearPixelThreshold");
    sRearPixelThreshold = rear_pixel_threshold;
    sRearPixelThreshold = llmax(sRearPixelThreshold, sFrontPixelThreshold); //can not be smaller than sFrontPixelThreshold.

    //make parameters adaptive to memory usage
    //starts to put restrictions from low_mem_bound_MB, apply tightest restrictions when hits high_mem_bound_MB
    static LLCachedControl<U32> low_mem_bound_MB(gSavedSettings,"SceneLoadLowMemoryBound");
    static LLCachedControl<U32> high_mem_bound_MB(gSavedSettings,"SceneLoadHighMemoryBound");

    LLMemory::updateMemoryInfo() ;
    U32 allocated_mem = LLMemory::getAllocatedMemKB().value();
    static const F32 KB_to_MB = 1.f / 1024.f;
    U32 clamped_memory = (U32)llclamp(allocated_mem * KB_to_MB, (F32) low_mem_bound_MB, (F32) high_mem_bound_MB);
    const F32 adjust_range = (F32)(high_mem_bound_MB - low_mem_bound_MB);
    const F32 adjust_factor = (high_mem_bound_MB - clamped_memory) / adjust_range; // [0, 1]

    //min radius: all objects within this radius remain loaded in memory
    static LLCachedControl<F32> min_radius(gSavedSettings,"SceneLoadMinRadius");
    static const F32 MIN_RADIUS = 1.0f;

    F32 draw_radius = gAgentCamera.mDrawDistance;
    if (LLViewerTexture::sDesiredDiscardBias > 2.f && LLViewerTexture::isSystemMemoryLow())
    {
        // Discard's bias maximum is 4 so we need to check 2 to 4 range
        // Factor is intended to go from 1.0 to 2.0
        F32 factor = 1.f + (LLViewerTexture::sDesiredDiscardBias - 2.f) / 2.f;
        // For safety cap reduction at 50%, we don't want to go below half of draw distance
        draw_radius = llmax(draw_radius / factor, draw_radius / 2.f);
    }
    const F32 clamped_min_radius = llclamp((F32) min_radius, MIN_RADIUS, draw_radius); // [1, mDrawDistance]
    sNearRadius = MIN_RADIUS + ((clamped_min_radius - MIN_RADIUS) * adjust_factor);

    // a percentage of draw distance beyond which all objects outside of view frustum will be unloaded, regardless of pixel threshold
    static LLCachedControl<F32> rear_max_radius_frac(gSavedSettings,"SceneLoadRearMaxRadiusFraction");
    const F32 min_radius_plus_one = sNearRadius + 1.f;
    const F32 max_radius = rear_max_radius_frac * draw_radius;
    const F32 clamped_max_radius = llclamp(max_radius, min_radius_plus_one, draw_radius); // [sNearRadius, mDrawDistance]
    sRearFarRadius = min_radius_plus_one + ((clamped_max_radius - min_radius_plus_one) * adjust_factor);

    //the number of frames invisible objects stay in memory
    static LLCachedControl<U32> inv_obj_time(gSavedSettings,"NonvisibleObjectsInMemoryTime");
    static const U32 MIN_FRAMES = 10;
    static const U32 MAX_FRAMES = 64;
    const U32 clamped_frames = inv_obj_time ? llclamp((U32) inv_obj_time, MIN_FRAMES, MAX_FRAMES) : MAX_FRAMES; // [10, 64], with zero => 64
    sMinFrameRange = MIN_FRAMES + (U32)((clamped_frames - MIN_FRAMES) * adjust_factor);
}
#endif // LL_TEST

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

extern bool gCubeSnapshot;

bool LLVOCacheEntry::isAnyVisible(const LLVector4a& camera_origin, const LLVector4a& local_camera_origin, F32 dist_threshold)
{
#if 0
    // this is ill-conceived and should be removed pending QA
    // In the name of saving memory, we evict objects that are still within view distance from memory
    // This results in constant paging of objects in and out of memory, leading to poor performance
    // and many unacceptable visual glitches when rotating the camera

    // Honestly, the entire VOCache partition system needs to be removed since it doubles the overhead of
    // the spatial partition system and is redundant to the object cache, but this is a start
    //  - davep 2024.06.07

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
        vis = (cur_vis + (S32)sMinFrameRange > LLViewerOctreeEntryData::getCurrentFrame());
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
#else
    return true;
#endif
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

LLVOCachePartition::~LLVOCachePartition()
{
    // SL-17276 make sure to do base class cleanup while this instance
    // can still be treated as an LLVOCachePartition
    cleanup();
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

#ifndef LL_TEST
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
            mFrontCull = false;

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

    mFrontCull = true;
    LLVOCacheOctreeCull culler(&camera, mRegionp, region_agent, do_occlusion && use_object_cache_occlusion,
        LLVOCacheEntry::getSquaredPixelThreshold(mFrontCull), this);
    culler.traverse(mOctree);

    if(!sNeedsOcclusionCheck)
    {
        sNeedsOcclusionCheck = !mOccludedGroups.empty();
    }
    return 1;
}
#endif // LL_TEST

void LLVOCachePartition::setCullHistory(bool has_new_object)
{
    mCullHistory <<= 1;
    mCullHistory |= static_cast<U32>(has_new_object);
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
    sNeedsOcclusionCheck = false;
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
    sNeedsOcclusionCheck = false;
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
// Format strings used to construct filename for the object cache
static const char OBJECT_CACHE_FILENAME[] = "objects_%d_%d.slc";
static const char OBJECT_CACHE_EXTRAS_FILENAME[] = "objects_%d_%d_extras.slec";

const U32 MAX_NUM_OBJECT_ENTRIES = 128 ;
const U32 MIN_ENTRIES_TO_PURGE = 16 ;
const U32 INVALID_TIME = 0 ;
const char* object_cache_dirname = "objectcache";
const char* header_filename = "object.cache";


LLVOCache::LLVOCache(bool read_only) :
    mInitialized(false),
    mReadOnly(read_only),
    mNumEntries(0),
    mCacheSize(1),
    mEnabled(true)
{
#ifndef LL_TEST
    mEnabled = gSavedSettings.getBOOL("ObjectCacheEnabled");
#endif
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

    LL_INFOS() << "Viewer Object Cache Versions - expected: " << cache_version << " found: " << mMetaInfo.mVersion <<  LL_ENDL;

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
            LL_INFOS() << "Viewer Object Cache Versions unmatched.  clearing cache." <<  LL_ENDL;
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
    // Bit more tracking of cache creation/destruction.
    std::string filename;
    getObjectCacheFilename(entry->mHandle, filename);
    LL_INFOS() << "Removing entry for region with filename" << filename << LL_ENDL;

    // make sure corresponding LLViewerRegion also clears its in-memory cache
    LLViewerRegion* regionp = LLWorld::instance().getRegionFromHandle(entry->mHandle);
    if (regionp)
    {
        regionp->clearVOCacheFromMemory();
    }

    header_entry_queue_t::iterator iter = mHeaderEntryQueue.find(entry);
    if(iter != mHeaderEntryQueue.end())
    {
        mHandleEntryMap.erase(entry->mHandle);
        mHeaderEntryQueue.erase(iter);
        removeFromCache(entry);
        delete entry;

        mNumEntries = static_cast<U32>(mHandleEntryMap.size());
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

std::string LLVOCache::getObjectCacheExtrasFilename(U64 handle)
{
    U32 region_x, region_y;

    grid_from_region_handle(handle, &region_x, &region_y);
    return gDirUtilp->getExpandedFilename(LL_PATH_CACHE, object_cache_dirname,
               llformat(OBJECT_CACHE_EXTRAS_FILENAME, region_x, region_y));
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
    LL_WARNS("GLTF", "VOCache") << "Removing object cache for handle " << entry->mHandle << "Filename: " << filename << LL_ENDL;
    LLAPRFile::remove(filename, mLocalAPRFilePoolp);

    // Note: `removeFromCache` should take responsibility for cleaning up all cache artefacts specfic to the handle/entry.
    // as such this now includes the generic extras
    filename = getObjectCacheExtrasFilename(entry->mHandle);
    LL_WARNS("GLTF", "VOCache") << "Removing generic extras for handle " << entry->mHandle << "Filename: " << filename << LL_ENDL;
    LLFile::remove(filename);

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
        //  getObjectCacheFilename((*iter)->mHandle, name) ;
        //  LL_INFOS() << name << LL_ENDL ;
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

        mNumEntries = static_cast<U32>(mHeaderEntryQueue.size());
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
        mReadOnly = true ; //disable the cache.
    }
    return ;
}

bool LLVOCache::updateEntry(const HeaderEntryInfo* entry)
{
    LLAPRFile apr_file(mHeaderFileName, APR_WRITE|APR_BINARY, mLocalAPRFilePoolp);
    apr_file.seek(APR_SET, entry->mIndex * sizeof(HeaderEntryInfo) + sizeof(HeaderMetaInfo)) ;

    return check_write(&apr_file, (void*)entry, sizeof(HeaderEntryInfo)) ;
}

// we now return bool to trigger dirty cache
// this in turn forces a rewrite after a partial read due to corruption.
bool LLVOCache::readFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_entry_map_t& cache_entry_map)
{
    if(!mEnabled)
    {
        LL_WARNS() << "Not reading cache for handle " << handle << "): Cache is currently disabled." << LL_ENDL;
        return true; // no problem we're just read only
    }
    llassert_always(mInitialized);

    handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle) ;
    if(iter == mHandleEntryMap.end()) //no cache
    {
        LL_WARNS() << "No handle map entry for " << handle << LL_ENDL;
        return false; // arguably no a problem, but we'll mark this as dirty anyway.
    }

    bool success = true ;
    S32 num_entries = 0 ; // lifted out of inner loop.
    std::string filename; // lifted out of loop
    {
        LLUUID cache_id;
        getObjectCacheFilename(handle, filename);
        LLAPRFile apr_file(filename, APR_READ|APR_BINARY, mLocalAPRFilePoolp);

        success = check_read(&apr_file, cache_id.mData, UUID_BYTES);

        if(success)
        {
            if(cache_id != id)
            {
                LL_INFOS() << "Cache ID doesn't match for this region, discarding"<< LL_ENDL;
                success = false ;
            }

            if(success)
            {
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

    LL_DEBUGS("GLTF", "VOCache") << "Read " << cache_entry_map.size() << " entries from object cache " << filename << ", expected " << num_entries << ", success=" << (success?"True":"False") << LL_ENDL;
    return success;
}

// We now pass in the cache entry map, so that we can remove entries from extras that are no longer in the primary cache.
void LLVOCache::readGenericExtrasFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_gltf_overrides_map_t& cache_extras_entry_map, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map)
{
    int loaded= 0;
    int discarded = 0;
    // get ViewerRegion pointer from handle
    LLViewerRegion* pRegion = LLWorld::getInstance()->getRegionFromHandle(handle);
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
        return;
    }

    std::string filename(getObjectCacheExtrasFilename(handle));
    llifstream in(filename, std::ios::in | std::ios::binary);

    std::string line;
    std::getline(in, line);
    if(!in.good())
    {
        LL_WARNS() << "Failed reading extras cache for handle " << handle << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }
    // file formats need versions, let's add one. legacy cache files will be considered version 0
    // This will make it easier to upgrade/revise later.
    int versionNumber=0;
    if (line.compare(0, LLGLTFOverrideCacheEntry::VERSION_LABEL.length(), LLGLTFOverrideCacheEntry::VERSION_LABEL) == 0)
    {
        std::string versionStr = line.substr(LLGLTFOverrideCacheEntry::VERSION_LABEL.length()+1); // skip the version label and ':'
        versionNumber = std::stol(versionStr);
    }
    // For future versions we may call a legacy handler here, but realistically we'll just consider this cache out of date.
    // The important thing is to make sure it gets removed.
    if(versionNumber != LLGLTFOverrideCacheEntry::VERSION)
    {
        LL_WARNS() << "Unexpected version number " << versionNumber << " for extras cache for handle " << handle << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }

    LL_DEBUGS("VOCache") << "Reading extras cache for handle " << handle << ", version " << versionNumber << LL_ENDL;
    std::getline(in, line);
    if(!LLUUID::validate(line))
    {
        LL_WARNS() << "Failed reading extras cache for handle" << handle << ". invalid uuid line: '" << line << "'" << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }

    LLUUID cache_id(line);
    if(cache_id != id)
    {
        // if the cache id doesn't match the expected region we should just kill the file.
        LL_WARNS() << "Cache ID doesn't match for this region, deleting it" << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }

    U32 num_entries;  // if removal was enabled during write num_entries might be wrong
    std::getline(in, line);
    if(!in.good())
    {
        LL_WARNS() << "Failed reading extras cache for handle " << handle << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }
    try
    {
        num_entries = std::stol(line);
    }
    catch(std::logic_error&)  // either invalid_argument or out_of_range
    {
        LL_WARNS() << "Failed reading extras cache for handle " << handle << ". unreadable num_entries" << LL_ENDL;
        in.close();
        removeGenericExtrasForHandle(handle);
        return;
    }

    LL_DEBUGS("GLTF") << "Beginning reading extras cache for handle " << handle << " from " << getObjectCacheExtrasFilename(handle) << LL_ENDL;

    LLSD entry_llsd;
    for (U32 i = 0; i < num_entries && !in.eof(); i++)
    {
        static const U32 max_size = 4096;
        bool success = LLSDSerialize::deserialize(entry_llsd, in, max_size);
        // check bool(in) this time since eof is not a failure condition here
        if(!success || !in)
        {
            LL_WARNS() << "Failed reading extras cache for handle " << handle << ", entry number " << i << " cache patrtial load only." << LL_ENDL;
            in.close();
            removeGenericExtrasForHandle(handle);
            break;
        }

        LLGLTFOverrideCacheEntry entry;
        entry.fromLLSD(entry_llsd);
        U32 local_id = entry_llsd["local_id"].asInteger();
        // only add entries that exist in the primary cache
        // this is a self-healing test that avoids us polluting the cache with entries that are no longer valid based on the main cache.
        if(cache_entry_map.find(local_id)!= cache_entry_map.end())
        {
            // attempt to backfill a null objectId, though these shouldn't be in the persisted cache really
            if(entry.mObjectId.isNull() && pRegion)
            {
                gObjectList.getUUIDFromLocal( entry.mObjectId, local_id, pRegion->getHost().getAddress(), pRegion->getHost().getPort() );
            }
            cache_extras_entry_map[local_id] = entry;
            loaded++;
        }
        else
        {
            discarded++;
        }
    }
    LL_DEBUGS("GLTF") << "Completed reading extras cache for handle " << handle << ", " << loaded << " loaded, " << discarded << " discarded" << LL_ENDL;
}

void LLVOCache::purgeEntries(U32 size)
{
    LL_DEBUGS("VOCache","GLTF") << "Purging " << size << " entries from cache" << LL_ENDL;
    while(mHeaderEntryQueue.size() > size)
    {
        header_entry_queue_t::iterator iter = mHeaderEntryQueue.begin() ;
        HeaderEntryInfo* entry = *iter ;
        mHandleEntryMap.erase(entry->mHandle) ;
        mHeaderEntryQueue.erase(iter) ;
        removeFromCache(entry) ; // This now handles removing extras cache where appropriate.
        delete entry;
    }
    mNumEntries = static_cast<U32>(mHandleEntryMap.size());
}

void LLVOCache::writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, bool dirty_cache, bool removal_enabled)
{
    std::string filename;
    getObjectCacheFilename(handle, filename);
    if(!mEnabled)
    {
        LL_WARNS() << "Not writing cache for " << filename << " (handle:" << handle << "): Cache is currently disabled." << LL_ENDL;
        return ;
    }
    llassert_always(mInitialized);

    if(mReadOnly)
    {
        LL_WARNS() << "Not writing cache for " << filename << " (handle:" << handle << "): Cache is currently in read-only mode." << LL_ENDL;
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
        entry->mTime = (U32)time(NULL) ;
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

        entry->mTime = (U32)time(NULL) ;
        mHeaderEntryQueue.insert(entry) ;
    }

    //update cache header
    if(!updateEntry(entry))
    {
        LL_WARNS() << "Failed to update cache header index " << entry->mIndex << ". " << filename << " handle = " << handle << LL_ENDL;
        return ; //update failed.
    }

    if(!dirty_cache)
    {
        LL_WARNS() << "Skipping write to cache for " << filename << " (handle:" << handle << "): cache not dirty" << LL_ENDL;
        return ; //nothing changed, no need to update.
    }

    //write to cache file
    bool success = true ;
    {
        std::string filename;
        getObjectCacheFilename(handle, filename);
        LLAPRFile apr_file(filename, APR_CREATE|APR_WRITE|APR_BINARY|APR_TRUNCATE, mLocalAPRFilePoolp);

        success = check_write(&apr_file, (void*)id.mData, UUID_BYTES);

        if(success)
        {
            S32 num_entries = static_cast<S32>(cache_entry_map.size()); // if removal is enabled num_entries might be wrong
            success = check_write(&apr_file, &num_entries, sizeof(S32));
            if (success)
            {
                const S32 buffer_size = 32768; //should be large enough for couple MAX_ENTRY_BODY_SIZE
                U8 data_buffer[buffer_size]; // generaly entries are fairly small, so collect them and drop onto disk in one go
                S32 size_in_buffer = 0;

                // This can have a lot of entries, so might be better to dump them into buffer first and write in one go.
                for (LLVOCacheEntry::vocache_entry_map_t::const_iterator iter = cache_entry_map.begin(); success && iter != cache_entry_map.end(); ++iter)
                {
                    if (!removal_enabled || iter->second->isValid())
                    {
                        S32 size = iter->second->writeToBuffer(data_buffer + size_in_buffer);

                        if (size > ENTRY_HEADER_SIZE) // body is minimum of 1
                        {
                            size_in_buffer += size;
                        }
                        else
                        {
                            LL_WARNS() << "Failed to write cache entry to buffer for " << filename << ", entry number " << iter->second->getLocalID() << LL_ENDL;
                            success = false;
                            break;
                        }

                        // Make sure we have space in buffer for next element
                        if (buffer_size - size_in_buffer < MAX_ENTRY_BODY_SIZE + ENTRY_HEADER_SIZE)
                        {
                            success = check_write(&apr_file, (void*)data_buffer, size_in_buffer);
                            size_in_buffer = 0;
                            if (!success)
                            {
                                LL_WARNS() << "Failed to write cache to disk " << filename << LL_ENDL;
                                break;
                            }
                        }
                    }
                }

                if (success && size_in_buffer > 0)
                {
                    // final write
                    success = check_write(&apr_file, (void*)data_buffer, size_in_buffer);
                    if(!success)
                    {
                        LL_WARNS() << "Failed to write cache entry to disk " << filename << LL_ENDL;
                    }
                    size_in_buffer = 0;
                }
                LL_DEBUGS("VOCache") << "Wrote " << num_entries << " entries to the primary VOCache file " << filename << ". success = " << (success ? "True":"False") << LL_ENDL;
            }
        }
    }

    if(!success)
    {
        removeEntry(entry) ;
    }

    return ;
}

void LLVOCache::removeGenericExtrasForHandle(U64 handle)
{
    if(mReadOnly)
    {
        LL_WARNS() << "Not removing cache for handle " << handle << ": Cache is currently in read-only mode." << LL_ENDL;
        return ;
    }

    // NOTE: when removing the extras, we must also remove the objects so the simulator will send us a full upddate with the valid overrides
    handle_entry_map_t::iterator iter = mHandleEntryMap.find(handle);
    if (iter != mHandleEntryMap.end())
    {
        LL_WARNS("GLTF", "VOCache") << "Removing generic extras for handle " << handle << "Filename: " << getObjectCacheExtrasFilename(handle) << LL_ENDL;
        removeEntry(iter->second);
    }
    else
    {
        //shouldn't happen, but if it does, we should remove the extras file since it's orphaned
        LLFile::remove(getObjectCacheExtrasFilename(handle));
    }
}

void LLVOCache::writeGenericExtrasToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_gltf_overrides_map_t& cache_extras_entry_map, bool dirty_cache, bool removal_enabled)
{
    if(!mEnabled)
    {
        LL_WARNS() << "Not writing extras cache for handle " << handle << "): Cache is currently disabled." << LL_ENDL;
        return;
    }
    llassert_always(mInitialized);

    if(mReadOnly)
    {
        LL_WARNS() << "Not writing extras cache for handle " << handle << "): Cache is currently in read-only mode." << LL_ENDL;
        return;
    }

    std::string filename = getObjectCacheExtrasFilename(handle);
    llofstream out(filename, std::ios::out | std::ios::binary);
    if(!out.good())
    {
        LL_WARNS() << "Failed writing extras cache for handle " << handle << LL_ENDL;
        removeGenericExtrasForHandle(handle);
        return;
    }
    // It is good practice to version file formats so let's add one.
    // legacy versions will be treated as version 0.
    out << LLGLTFOverrideCacheEntry::VERSION_LABEL << ":" << LLGLTFOverrideCacheEntry::VERSION << '\n';

    out << id << '\n';
    if(!out.good())
    {
        LL_WARNS() << "Failed writing extras cache for handle " << handle << LL_ENDL;
        removeGenericExtrasForHandle(handle);
        return;
    }
    // Because we don't write out all the entries we need to record a placeholder and rewrite this later
    auto num_entries_placeholder = out.tellp();
    out << std::setw(10) << std::setfill('0') << 0 << '\n';
    if(!out.good())
    {
        LL_WARNS() << "Failed writing extras cache for handle " << handle << LL_ENDL;
        removeGenericExtrasForHandle(handle);
        return;
    }

    // get ViewerRegion pointer from handle
    LLViewerRegion* pRegion = LLWorld::getInstance()->getRegionFromHandle(handle);

    U32 num_entries = 0;
    U32 skipped = 0;
    size_t inmem_entries = cache_extras_entry_map.size();
    for (auto [local_id, entry] : cache_extras_entry_map)
    {
        // Only write out GLTFOverrides that we can actually apply again on import.
        // worst case we have an extra cache miss.
        // Note: A null mObjectId is valid when in memory as we might have a data race between GLTF of the object itself.
        // This remains a valid state to persist as it is consistent with the localid checks on import with the main cache.
        // the mObjectId will be updated if/when the local object is updated from the gObject list (due to full update)
        if(entry.mObjectId.isNull() && pRegion)
        {
            gObjectList.getUUIDFromLocal( entry.mObjectId, local_id, pRegion->getHost().getAddress(), pRegion->getHost().getPort() );
        }

        if( entry.mSides.size() > 0 &&
            entry.mSides.size() == entry.mGLTFMaterial.size()
          )
        {
            LLSD entry_llsd = entry.toLLSD();
            entry_llsd["local_id"] = (S32)local_id;
            LLSDSerialize::serialize(entry_llsd, out, LLSDSerialize::LLSD_XML);
            out << '\n';
            if(!out.good())
            {
                // We're not in a good place when this happens so we might as well nuke the file.
                LL_WARNS() << "Failed writing extras cache for handle " << handle << ". Corrupted cache file " << filename << " removed." << LL_ENDL;
                removeGenericExtrasForHandle(handle);
                return;
            }
            num_entries++;
        }
        else
        {
            skipped++;
        }
    }
    // Rewrite the placeholder
    out.seekp(num_entries_placeholder);
    out << std::setw(10) << std::setfill('0') << num_entries << '\n';
    if(!out.good())
    {
        LL_WARNS() << "Failed writing extras cache for handle " << handle << LL_ENDL;
        removeGenericExtrasForHandle(handle);
        return;
    }
    LL_DEBUGS("GLTF") << "Completed writing extras cache for handle " << handle << ", " << num_entries << " entries. Total in RAM: " << inmem_entries << " skipped (no persist): " << skipped << LL_ENDL;
}
