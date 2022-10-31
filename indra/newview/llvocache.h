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
#include "lldir.h"
#include "llvieweroctree.h"
#include "llapr.h"

//---------------------------------------------------------------------------
// Cache entries
class LLCamera;

class LLVOCacheEntry 
:   public LLViewerOctreeEntryData
{
    LL_ALIGN_NEW
public:
    enum 
    {
        //low 16-bit state
        INACTIVE = 0x00000000,     //not visible
        IN_QUEUE = 0x00000001,     //in visible queue, object to be created
        WAITING  = 0x00000002,     //object creation request sent
        ACTIVE   = 0x00000004,      //object created, and in rendering pipeline.

        //high 16-bit state
        IN_VO_TREE = 0x00010000,    //the entry is in the object cache tree.

        LOW_BITS  = 0x0000ffff,
        HIGH_BITS = 0xffff0000
    };

    struct CompareVOCacheEntry
    {
        bool operator()(const LLVOCacheEntry* const& lhs, const LLVOCacheEntry* const& rhs) const
        {
            F32 lpa = lhs->getSceneContribution();
            F32 rpa = rhs->getSceneContribution();

            //larger pixel area first
            if(lpa > rpa)       
            {
                return true;
            }
            else if(lpa < rpa)
            {
                return false;
            }
            else
            {
                return lhs < rhs;
            }           
        }
    };
protected:
    ~LLVOCacheEntry();
public:
    LLVOCacheEntry(U32 local_id, U32 crc, LLDataPackerBinaryBuffer &dp);
    LLVOCacheEntry(LLAPRFile* apr_file);
    LLVOCacheEntry();   

    void updateEntry(U32 crc, LLDataPackerBinaryBuffer &dp);

    void clearState(U32 state) {mState &= ~state;}
    bool hasState(U32 state)   {return mState & state;} 
    void setState(U32 state);   
    bool isState(U32 state)    {return (mState & LOW_BITS) == state;}   
    U32  getState() const      {return mState & LOW_BITS;}
    
    bool isAnyVisible(const LLVector4a& camera_origin, const LLVector4a& local_camera_origin, F32 dist_threshold);

    U32 getLocalID() const          { return mLocalID; }
    U32 getCRC() const              { return mCRC; }
    S32 getHitCount() const         { return mHitCount; }
    S32 getCRCChangeCount() const   { return mCRCChangeCount; }
    
    void calcSceneContribution(const LLVector4a& camera_origin, bool needs_update, U32 last_update, F32 dist_threshold);
    void setSceneContribution(F32 scene_contrib) {mSceneContrib = scene_contrib;}
    F32 getSceneContribution() const             { return mSceneContrib;}

    void dump() const;
    S32 writeToBuffer(U8 *data_buffer) const;
    LLDataPackerBinaryBuffer *getDP();
    void recordHit();
    void recordDupe() { mDupeCount++; }
    
    /*virtual*/ void setOctreeEntry(LLViewerOctreeEntry* entry);

    void setParentID(U32 id);
    U32  getParentID() const {return mParentID;}
    bool isChild() const {return mParentID > 0;}

    void addChild(LLVOCacheEntry* entry);
    void removeChild(LLVOCacheEntry* entry);
    void removeAllChildren();
    LLVOCacheEntry* getChild(); //remove the first child, and return it.
    S32  getNumOfChildren() const  {return mChildrenList.size();}
    
    void setBoundingInfo(const LLVector3& pos, const LLVector3& scale); //called from processing object update message  
    void updateParentBoundingInfo();
    void saveBoundingSphere();

    void setValid(BOOL valid = TRUE) {mValid = valid;}
    BOOL isValid() const {return mValid;}

    void setUpdateFlags(U32 flags) {mUpdateFlags = flags;}
    U32  getUpdateFlags() const    {return mUpdateFlags;}

    static void updateDebugSettings();
    static F32  getSquaredPixelThreshold(bool is_front);

private:
    void updateParentBoundingInfo(const LLVOCacheEntry* child); 

public:
    typedef std::map<U32, LLPointer<LLVOCacheEntry> >      vocache_entry_map_t;
    typedef std::set<LLVOCacheEntry*>                      vocache_entry_set_t;
    typedef std::set<LLVOCacheEntry*, CompareVOCacheEntry> vocache_entry_priority_list_t;   

    S32                         mLastCameraUpdated;
protected:
    U32                         mLocalID;
    U32                         mParentID;
    U32                         mCRC;
    U32                         mUpdateFlags; //receive from sim
    S32                         mHitCount;
    S32                         mDupeCount;
    S32                         mCRCChangeCount;
    LLDataPackerBinaryBuffer    mDP;
    U8                          *mBuffer;

    F32                         mSceneContrib; //projected scene contributuion of this object.
    U32                         mState; //high 16 bits reserved for special use.
    vocache_entry_set_t         mChildrenList; //children entries in a linked set.

    BOOL                        mValid; //if set, this entry is valid, otherwise it is invalid and will be removed.

    LLVector4a                  mBSphereCenter; //bounding sphere center
    F32                         mBSphereRadius; //bounding sphere radius

public:
    static U32                  sMinFrameRange;
    static F32                  sNearRadius;
    static F32                  sRearFarRadius;
    static F32                  sFrontPixelThreshold;
    static F32                  sRearPixelThreshold;
};

class LLVOCacheGroup : public LLOcclusionCullingGroup
{
public:
    LLVOCacheGroup(OctreeNode* node, LLViewerOctreePartition* part) : LLOcclusionCullingGroup(node, part){} 

    //virtual
    void handleChildAddition(const OctreeNode* parent, OctreeNode* child);

protected:
    virtual ~LLVOCacheGroup();
};

class LLVOCachePartition : public LLViewerOctreePartition
{
public:
    LLVOCachePartition(LLViewerRegion* regionp);
    virtual ~LLVOCachePartition();

    bool addEntry(LLViewerOctreeEntry* entry);
    void removeEntry(LLViewerOctreeEntry* entry);
    /*virtual*/ S32 cull(LLCamera &camera, bool do_occlusion);
    void addOccluders(LLViewerOctreeGroup* gp);
    void resetOccluders();
    void processOccluders(LLCamera* camera);
    void removeOccluder(LLVOCacheGroup* group);

    void setCullHistory(BOOL has_new_object);

    bool isFrontCull() const {return mFrontCull;}

private:
    void selectBackObjects(LLCamera &camera, F32 projection_area_cutoff, bool use_occlusion); //select objects behind camera.

public:
    static BOOL sNeedsOcclusionCheck;

private:
    BOOL  mFrontCull; //the view frustum cull if set, otherwise is back sphere cull.
    U32   mCullHistory;
    U32   mCulledTime[LLViewerCamera::NUM_CAMERAS];
    std::set<LLVOCacheGroup*> mOccludedGroups;

    S32   mBackSlectionEnabled; //enable to select back objects if > 0.
    U32   mIdleHash;
};

//
//Note: LLVOCache is not thread-safe
//
class LLVOCache : public LLParamSingleton<LLVOCache>
{
    LLSINGLETON(LLVOCache, bool read_only);
    ~LLVOCache() ;

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
        HeaderMetaInfo() : mVersion(0), mAddressSize(0) {}

        U32 mVersion;
        U32 mAddressSize;
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

public:
    // We need this init to be separate from constructor, since we might construct cache, purge it, then init.
    void initCache(ELLPath location, U32 size, U32 cache_version);
    void removeCache(ELLPath location, bool started = false) ;

    void readFromCache(U64 handle, const LLUUID& id, LLVOCacheEntry::vocache_entry_map_t& cache_entry_map) ;
    void writeToCache(U64 handle, const LLUUID& id, const LLVOCacheEntry::vocache_entry_map_t& cache_entry_map, BOOL dirty_cache, bool removal_enabled);
    void removeEntry(U64 handle) ;

    U32 getCacheEntries() { return mNumEntries; }
    U32 getCacheEntriesMax() { return mCacheSize; }

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
    bool                 mEnabled;
    bool                 mInitialized ;
    bool                 mReadOnly ;
    HeaderMetaInfo       mMetaInfo;
    U32                  mCacheSize;
    U32                  mNumEntries;
    std::string          mHeaderFileName ;
    std::string          mObjectCacheDirName;
    LLVolatileAPRPool*   mLocalAPRFilePoolp ;   
    header_entry_queue_t mHeaderEntryQueue;
    handle_entry_map_t   mHandleEntryMap;   
};

#endif
