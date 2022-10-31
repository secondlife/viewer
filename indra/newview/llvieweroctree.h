/** 
 * @file llvieweroctree.h
 * @brief LLViewerObjectOctree.cpp header file, defining all supporting classes.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_VIEWEROCTREE_H
#define LL_VIEWEROCTREE_H

#include <vector>
#include <map>

#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llvector4a.h"
#include "llquaternion.h"
#include "lloctree.h"
#include "llviewercamera.h"

class LLViewerRegion;
class LLViewerOctreeEntryData;
class LLViewerOctreeGroup;
class LLViewerOctreeEntry;
class LLViewerOctreePartition;

typedef LLOctreeListener<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry>> OctreeListener;
typedef LLTreeNode<LLViewerOctreeEntry> TreeNode;
typedef LLOctreeNode<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry>> OctreeNode;
typedef LLOctreeRoot<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry>> OctreeRoot;
typedef LLOctreeTraveler<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry>> OctreeTraveler;

#if LL_OCTREE_PARANOIA_CHECK
#define assert_octree_valid(x) x->validate()
#define assert_states_valid(x) ((LLViewerOctreeGroup*) x->mSpatialPartition->mOctree->getListener(0))->checkStates()
#else
#define assert_octree_valid(x)
#define assert_states_valid(x)
#endif

// get index buffer for binary encoded axis vertex buffer given a box at center being viewed by given camera
U32 get_box_fan_indices(LLCamera* camera, const LLVector4a& center);
U8* get_box_fan_indices_ptr(LLCamera* camera, const LLVector4a& center);

S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &radius_squared);

S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &radius_squared);

//defines data needed for octree of an entry
//LL_ALIGN_PREFIX(16)
class LLViewerOctreeEntry : public LLRefCount
{
    LL_ALIGN_NEW
    friend class LLViewerOctreeEntryData;

public:
    typedef enum
    {
        LLDRAWABLE = 0,
        LLVOCACHEENTRY,
        NUM_DATA_TYPE
    }eEntryDataType_t;

protected:
    virtual ~LLViewerOctreeEntry();

public:
    LLViewerOctreeEntry();
    
    void nullGroup(); //called by group handleDestruction() only
    void setGroup(LLViewerOctreeGroup* group);
    void removeData(LLViewerOctreeEntryData* data);

    LLViewerOctreeEntryData* getDrawable() const {return mData[LLDRAWABLE];}
    bool                     hasDrawable() const {return mData[LLDRAWABLE] != NULL;}
    LLViewerOctreeEntryData* getVOCacheEntry() const {return mData[LLVOCACHEENTRY];}
    bool                     hasVOCacheEntry() const {return mData[LLVOCACHEENTRY] != NULL;}

    const LLVector4a* getSpatialExtents() const {return mExtents;} 
    const LLVector4a& getPositionGroup() const  {return mPositionGroup;}    
    LLViewerOctreeGroup* getGroup()const        {return mGroup;}
    
    F32  getBinRadius() const                   {return mBinRadius;}
    S32  getBinIndex() const                    {return mBinIndex; }
    void setBinIndex(S32 index) const           {mBinIndex = index; }

private:
    void addData(LLViewerOctreeEntryData* data);            

private:
    LLViewerOctreeEntryData*    mData[NUM_DATA_TYPE]; //do not use LLPointer here.
    LLViewerOctreeGroup*        mGroup;

    //aligned members
    LL_ALIGN_16(LLVector4a      mExtents[2]);
    LL_ALIGN_16(LLVector4a      mPositionGroup);
    F32                         mBinRadius;
    mutable S32                 mBinIndex;
    mutable U32                 mVisible;   

} ;//LL_ALIGN_POSTFIX(16);

//defines an abstract class for entry data
//LL_ALIGN_PREFIX(16)
class LLViewerOctreeEntryData : public LLRefCount
{
protected:
    virtual ~LLViewerOctreeEntryData();

public:
    LLViewerOctreeEntryData(const LLViewerOctreeEntryData& rhs)
    {
        *this = rhs;
    }
    LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t data_type);
    
    LLViewerOctreeEntry::eEntryDataType_t getDataType() const {return mDataType;}
    LLViewerOctreeEntry* getEntry() {return mEntry;}
    
    virtual void setOctreeEntry(LLViewerOctreeEntry* entry);
    void         removeOctreeEntry();

    F32                  getBinRadius() const   {return mEntry->getBinRadius();}
    const LLVector4a*    getSpatialExtents() const;
    LLViewerOctreeGroup* getGroup()const;
    const LLVector4a&    getPositionGroup() const;
    
    void setBinRadius(F32 rad)  {mEntry->mBinRadius = rad;}
    void setSpatialExtents(const LLVector3& min, const LLVector3& max);
    void setSpatialExtents(const LLVector4a& min, const LLVector4a& max);
    void setPositionGroup(const LLVector4a& pos);
    
    virtual void setGroup(LLViewerOctreeGroup* group);
    void         shift(const LLVector4a &shift_vector);

    U32          getVisible() const {return mEntry ? mEntry->mVisible : 0;}
    void         setVisible() const;
    void         resetVisible() const;
    virtual bool isVisible() const;
    virtual bool isRecentlyVisible() const; 
    
    static S32 getCurrentFrame() { return sCurVisible; }

protected:
    LLVector4a& getGroupPosition()  {return mEntry->mPositionGroup;}
    void        initVisible(U32 visible) {mEntry->mVisible = visible;}

    static void incrementVisible() {sCurVisible++;}
protected:
    LLPointer<LLViewerOctreeEntry>        mEntry;
    LLViewerOctreeEntry::eEntryDataType_t mDataType;
    static  U32                           sCurVisible; // Counter for what value of mVisible means currently visible
};//LL_ALIGN_POSTFIX(16);


//defines an octree group for an octree node, which contains multiple entries.
//LL_ALIGN_PREFIX(16)
class LLViewerOctreeGroup
:   public OctreeListener
{
    LL_ALIGN_NEW
    friend class LLViewerOctreeCull;
protected:
    virtual ~LLViewerOctreeGroup();

public: 
    enum
    {
        CLEAN              = 0x00000000,
        DIRTY              = 0x00000001,
        OBJECT_DIRTY       = 0x00000002,
        SKIP_FRUSTUM_CHECK = 0x00000004,
        DEAD               = 0x00000008,
        INVALID_STATE      = 0x00000010,
    };

public:
    typedef OctreeNode::element_iter element_iter;
    typedef OctreeNode::element_list element_list;

    LLViewerOctreeGroup(OctreeNode* node);
    LLViewerOctreeGroup(const LLViewerOctreeGroup& rhs)
    {
        *this = rhs;
    }

    bool removeFromGroup(LLViewerOctreeEntryData* data);
    bool removeFromGroup(LLViewerOctreeEntry* entry);

    virtual void unbound();
    virtual void rebound();
    
    BOOL isDead()                           { return hasState(DEAD); }  

    void setVisible();
    BOOL isVisible() const;
    virtual BOOL isRecentlyVisible() const;
    S32  getVisible(LLViewerCamera::eCameraID id) const {return mVisible[id];}
    S32  getAnyVisible() const {return mAnyVisible;}
    bool isEmpty() const { return mOctreeNode->isEmpty(); }

    U32  getState()                {return mState; }
    bool isDirty() const           {return mState & DIRTY;}
    bool hasState(U32 state) const {return mState & state;}
    void setState(U32 state)       {mState |= state;}
    void clearState(U32 state)     {mState &= ~state;}  

    //LISTENER FUNCTIONS
    virtual void handleInsertion(const TreeNode* node, LLViewerOctreeEntry* obj);
    virtual void handleRemoval(const TreeNode* node, LLViewerOctreeEntry* obj);
    virtual void handleDestruction(const TreeNode* node);
    virtual void handleStateChange(const TreeNode* node);
    virtual void handleChildAddition(const OctreeNode* parent, OctreeNode* child);
    virtual void handleChildRemoval(const OctreeNode* parent, const OctreeNode* child);

    OctreeNode*          getOctreeNode() {return mOctreeNode;}
    LLViewerOctreeGroup* getParent();

    const LLVector4a* getBounds() const        {return mBounds;}
    const LLVector4a* getExtents() const       {return mExtents;}
    const LLVector4a* getObjectBounds() const  {return mObjectBounds;}
    const LLVector4a* getObjectExtents() const {return mObjectExtents;}

    //octree wrappers to make code more readable
    element_iter getDataBegin() { return mOctreeNode->getDataBegin(); }
    element_iter getDataEnd() { return mOctreeNode->getDataEnd(); }
    U32 getElementCount() const { return mOctreeNode->getElementCount(); }
    bool hasElement(LLViewerOctreeEntryData* data);
    
protected:
    void checkStates();
private:
    virtual bool boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut);          

protected:
    U32         mState;
    OctreeNode* mOctreeNode;    

    LL_ALIGN_16(LLVector4a mBounds[2]);        // bounding box (center, size) of this node and all its children (tight fit to objects)
    LL_ALIGN_16(LLVector4a mObjectBounds[2]);  // bounding box (center, size) of objects in this node
    LL_ALIGN_16(LLVector4a mExtents[2]);       // extents (min, max) of this node and all its children
    LL_ALIGN_16(LLVector4a mObjectExtents[2]); // extents (min, max) of objects in this node    

    S32         mAnyVisible; //latest visible to any camera
    S32         mVisible[LLViewerCamera::NUM_CAMERAS];  

};//LL_ALIGN_POSTFIX(16);

//octree group which has capability to support occlusion culling
//LL_ALIGN_PREFIX(16)
class LLOcclusionCullingGroup : public LLViewerOctreeGroup
{
public:
    typedef enum
    {
        OCCLUDED                = 0x00010000,
        QUERY_PENDING           = 0x00020000,
        ACTIVE_OCCLUSION        = 0x00040000,
        DISCARD_QUERY           = 0x00080000,
        EARLY_FAIL              = 0x00100000,
    } eOcclusionState;

    typedef enum
    {
        STATE_MODE_SINGLE = 0,      //set one node
        STATE_MODE_BRANCH,          //set entire branch
        STATE_MODE_DIFF,            //set entire branch as long as current state is different
        STATE_MODE_ALL_CAMERAS,     //used for occlusion state, set state for all cameras
    } eSetStateMode;

protected:
    virtual ~LLOcclusionCullingGroup();

public:
    LLOcclusionCullingGroup(OctreeNode* node, LLViewerOctreePartition* part);
    LLOcclusionCullingGroup(const LLOcclusionCullingGroup& rhs) : LLViewerOctreeGroup(rhs)
    {
        *this = rhs;
    }   

    void setOcclusionState(U32 state, S32 mode = STATE_MODE_SINGLE);
    void clearOcclusionState(U32 state, S32 mode = STATE_MODE_SINGLE);
    void checkOcclusion(); //read back last occlusion query (if any)
    void doOcclusion(LLCamera* camera, const LLVector4a* shift = NULL); //issue occlusion query
    BOOL isOcclusionState(U32 state) const  { return mOcclusionState[LLViewerCamera::sCurCameraID] & state ? TRUE : FALSE; }
    U32  getOcclusionState() const  { return mOcclusionState[LLViewerCamera::sCurCameraID];}

    BOOL needsUpdate();
    U32  getLastOcclusionIssuedTime();

    //virtual 
    void handleChildAddition(const OctreeNode* parent, OctreeNode* child);

    //virtual
    BOOL isRecentlyVisible() const;
    LLViewerOctreePartition* getSpatialPartition()const {return mSpatialPartition;}
    BOOL isAnyRecentlyVisible() const;

    static U32 getNewOcclusionQueryObjectName();
    static void releaseOcclusionQueryObjectName(U32 name);

protected:
    void releaseOcclusionQueryObjectNames();

private:    
    BOOL earlyFail(LLCamera* camera, const LLVector4a* bounds);

protected:
    U32         mOcclusionState[LLViewerCamera::NUM_CAMERAS];
    U32         mOcclusionIssued[LLViewerCamera::NUM_CAMERAS];

    S32         mLODHash;

    LLViewerOctreePartition* mSpatialPartition;
    U32                      mOcclusionQuery[LLViewerCamera::NUM_CAMERAS];

public:     
    static std::set<U32> sPendingQueries;
};//LL_ALIGN_POSTFIX(16);

class LLViewerOctreePartition
{
public:
    LLViewerOctreePartition();
    virtual ~LLViewerOctreePartition();

    // Cull on arbitrary frustum
    virtual S32 cull(LLCamera &camera, bool do_occlusion) = 0;
    BOOL isOcclusionEnabled();

protected:
    // MUST call from destructor of any derived classes (SL-17276)
    void cleanup();

public: 
    U32              mPartitionType;
    U32              mDrawableType;
    OctreeNode*      mOctree;
    LLViewerRegion*  mRegionp; // the region this partition belongs to.
    BOOL             mOcclusionEnabled; // if TRUE, occlusion culling is performed
    U32              mLODSeed;
    U32              mLODPeriod;    //number of frames between LOD updates for a given spatial group (staggered by mLODSeed)
};

class LLViewerOctreeCull : public OctreeTraveler
{
public:
    LLViewerOctreeCull(LLCamera* camera)
        : mCamera(camera), mRes(0) { }
    
    virtual void traverse(const OctreeNode* n);

protected:
    virtual bool earlyFail(LLViewerOctreeGroup* group); 
    
    //agent space group cull
    S32 AABBInFrustumNoFarClipGroupBounds(const LLViewerOctreeGroup* group);    
    S32 AABBSphereIntersectGroupExtents(const LLViewerOctreeGroup* group);
    S32 AABBInFrustumGroupBounds(const LLViewerOctreeGroup* group);

    //agent space object set cull
    S32 AABBInFrustumNoFarClipObjectBounds(const LLViewerOctreeGroup* group);
    S32 AABBSphereIntersectObjectExtents(const LLViewerOctreeGroup* group); 
    S32 AABBInFrustumObjectBounds(const LLViewerOctreeGroup* group);
    
    //local region space group cull
    S32 AABBInRegionFrustumNoFarClipGroupBounds(const LLViewerOctreeGroup* group);
    S32 AABBInRegionFrustumGroupBounds(const LLViewerOctreeGroup* group);
    S32 AABBRegionSphereIntersectGroupExtents(const LLViewerOctreeGroup* group, const LLVector3& shift);

    //local region space object set cull
    S32 AABBInRegionFrustumNoFarClipObjectBounds(const LLViewerOctreeGroup* group);
    S32 AABBInRegionFrustumObjectBounds(const LLViewerOctreeGroup* group);
    S32 AABBRegionSphereIntersectObjectExtents(const LLViewerOctreeGroup* group, const LLVector3& shift);   
    
    virtual S32 frustumCheck(const LLViewerOctreeGroup* group) = 0;
    virtual S32 frustumCheckObjects(const LLViewerOctreeGroup* group) = 0;

    bool checkProjectionArea(const LLVector4a& center, const LLVector4a& size, const LLVector3& shift, F32 pixel_threshold, F32 near_radius);
    virtual bool checkObjects(const OctreeNode* branch, const LLViewerOctreeGroup* group);
    virtual void preprocess(LLViewerOctreeGroup* group);
    virtual void processGroup(LLViewerOctreeGroup* group);
    virtual void visit(const OctreeNode* branch);
    
protected:
    LLCamera *mCamera;
    S32 mRes;
};

//scan the octree, output the info of each node for debug use.
class LLViewerOctreeDebug : public OctreeTraveler
{
public:
    virtual void processGroup(LLViewerOctreeGroup* group);
    virtual void visit(const OctreeNode* branch);

public:
    static BOOL sInDebug;
};

#endif
