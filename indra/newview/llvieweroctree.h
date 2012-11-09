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
class LLviewerOctreeGroup;
class LLViewerOctreeEntry;

typedef LLOctreeListener<LLViewerOctreeEntry>	OctreeListener;
typedef LLTreeNode<LLViewerOctreeEntry>			TreeNode;
typedef LLOctreeNode<LLViewerOctreeEntry>		OctreeNode;
typedef LLOctreeRoot<LLViewerOctreeEntry>		OctreeRoot;
typedef LLOctreeTraveler<LLViewerOctreeEntry>	OctreeTraveler;

S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &radius_squared);

S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &radius_squared);

//defines data needed for octree of an entry
LL_ALIGN_PREFIX(16)
class LLViewerOctreeEntry : public LLRefCount
{
	friend class LLViewerOctreeEntryData;

public:
	typedef enum
	{
		LLDRAWABLE = 0,
		LLVOCACHEENTRY,
		NUM_DATA_TYPE
	}eEntryDataType_t;

	~LLViewerOctreeEntry();
public:
	LLViewerOctreeEntry();
	
	void nullGroup(); //called by group handleDestruction() only
	void setGroup(LLviewerOctreeGroup* group);
	void removeData(LLViewerOctreeEntryData* data);

	LLViewerOctreeEntryData* getData(U32 data_type)const {return mData[data_type];}
	bool                     hasData(U32 data_type)const {return mData[data_type] != NULL;}

	LLViewerOctreeEntryData* getDrawable() const {return mData[LLDRAWABLE];}
	bool                     hasDrawable() const {return mData[LLDRAWABLE] != NULL;}
	LLViewerOctreeEntryData* getVOCacheEntry() const {return mData[LLVOCACHEENTRY];}
	bool                     hasVOCacheEntry() const {return mData[LLVOCACHEENTRY] != NULL;}

	const LLVector4a* getSpatialExtents() const {return mExtents;} 
	const LLVector4a& getPositionGroup() const  {return mPositionGroup;}	
	LLviewerOctreeGroup* getGroup()const        {return mGroup;}
	
	F32  getBinRadius() const                   {return mBinRadius;}
	S32	 getBinIndex() const			        {return mBinIndex; }
	void setBinIndex(S32 index) const	        {mBinIndex = index; }

private:
	void addData(LLViewerOctreeEntryData* data);			

private:
	LLViewerOctreeEntryData*    mData[NUM_DATA_TYPE]; //do not use LLPointer here.
	LLviewerOctreeGroup*        mGroup;

	//aligned members
	LL_ALIGN_16(LLVector4a		mExtents[2]);
	LL_ALIGN_16(LLVector4a		mPositionGroup);
	F32				            mBinRadius;
	mutable S32		            mBinIndex;
	mutable U32		            mVisible;	

} LL_ALIGN_POSTFIX(16);

//defines an abstract class for entry data
LL_ALIGN_PREFIX(16)
class LLViewerOctreeEntryData : public LLRefCount
{
protected:
	~LLViewerOctreeEntryData();

public:
	LLViewerOctreeEntryData(const LLViewerOctreeEntryData& rhs)
	{
		*this = rhs;
	}
	LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t data_type);
	
	LLViewerOctreeEntry::eEntryDataType_t getDataType() const {return mDataType;}
	LLViewerOctreeEntry* getEntry() {return mEntry;}
	
	virtual void setOctreeEntry(LLViewerOctreeEntry* entry);

	virtual S32  getMinVisFrameRange()const = 0;

	F32                  getBinRadius() const   {return mEntry->getBinRadius();}
	const LLVector4a*    getSpatialExtents() const;
	LLviewerOctreeGroup* getGroup()const;
	const LLVector4a&    getPositionGroup() const;
	
	void setBinRadius(F32 rad)  {mEntry->mBinRadius = rad;}
	void setSpatialExtents(const LLVector3& min, const LLVector3& max);
	void setSpatialExtents(const LLVector4a& min, const LLVector4a& max);
	void setPositionGroup(const LLVector4a& pos);
	
	virtual void setGroup(LLviewerOctreeGroup* group);
	void         shift(const LLVector4a &shift_vector);

	U32          getVisible() const {return mEntry ? mEntry->mVisible : 0;}
	void         setVisible() const;
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
}LL_ALIGN_POSTFIX(16);


//defines an octree group for an octree node, which contains multiple entries.
LL_ALIGN_PREFIX(16)
class LLviewerOctreeGroup : public LLOctreeListener<LLViewerOctreeEntry>
{
	friend class LLViewerOctreeCull;
protected:
	~LLviewerOctreeGroup();

public:
	enum
	{
		CLEAN              = 0x00000000,
		DIRTY              = 0x00000001,
		OBJECT_DIRTY       = 0x00000002,
		SKIP_FRUSTUM_CHECK = 0x00000004,
		INVALID_STATE      = 0x00000008,
	};

public:
	typedef LLOctreeNode<LLViewerOctreeEntry>::element_iter element_iter;
	typedef LLOctreeNode<LLViewerOctreeEntry>::element_list element_list;

	LLviewerOctreeGroup(OctreeNode* node);
	LLviewerOctreeGroup(const LLviewerOctreeGroup& rhs)
	{
		*this = rhs;
	}

	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	bool removeFromGroup(LLViewerOctreeEntryData* data);
	bool removeFromGroup(LLViewerOctreeEntry* entry);

	virtual void unbound();
	virtual void rebound();

	void setVisible();
	BOOL isVisible() const;
	virtual BOOL isRecentlyVisible() const;
	bool isEmpty() const { return mOctreeNode->isEmpty(); }

	U32  getState()				   {return mState; }
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
	LLviewerOctreeGroup* getParent();

	const LLVector4a* getBounds() const        {return mBounds;}
	const LLVector4a* getExtents() const       {return mExtents;}
	const LLVector4a* getObjectBounds() const  {return mObjectBounds;}
	const LLVector4a* getObjectExtents() const {return mObjectExtents;}

	//octree wrappers to make code more readable
	element_list& getData() { return mOctreeNode->getData(); }
	element_iter getDataBegin() { return mOctreeNode->getDataBegin(); }
	element_iter getDataEnd() { return mOctreeNode->getDataEnd(); }
	U32 getElementCount() const { return mOctreeNode->getElementCount(); }	

private:
	virtual bool boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut);	
	
protected:
	U32         mState;
	OctreeNode* mOctreeNode;	

	LL_ALIGN_16(LLVector4a mBounds[2]);        // bounding box (center, size) of this node and all its children (tight fit to objects)
	LL_ALIGN_16(LLVector4a mObjectBounds[2]);  // bounding box (center, size) of objects in this node
	LL_ALIGN_16(LLVector4a mExtents[2]);       // extents (min, max) of this node and all its children
	LL_ALIGN_16(LLVector4a mObjectExtents[2]); // extents (min, max) of objects in this node

public:
	S32         mVisible[LLViewerCamera::NUM_CAMERAS];
}LL_ALIGN_POSTFIX(16);

class LLViewerOctreePartition
{
public:
	LLViewerOctreePartition();
	virtual ~LLViewerOctreePartition();

	// Cull on arbitrary frustum
	virtual S32 cull(LLCamera &camera) = 0;

public:	
	U32              mPartitionType;
	OctreeNode*      mOctree;
	LLViewerRegion*  mRegionp; // the region this partition belongs to.
};

class LLViewerOctreeCull : public OctreeTraveler
{
public:
	LLViewerOctreeCull(LLCamera* camera)
		: mCamera(camera), mRes(0) { }

	virtual bool earlyFail(LLviewerOctreeGroup* group);
	virtual void traverse(const OctreeNode* n);
	
	S32 AABBInFrustumNoFarClipGroupBounds(const LLviewerOctreeGroup* group);
	S32 AABBSphereIntersectGroupExtents(const LLviewerOctreeGroup* group);
	S32 AABBInFrustumNoFarClipObjectBounds(const LLviewerOctreeGroup* group);
	S32 AABBSphereIntersectObjectExtents(const LLviewerOctreeGroup* group);
	S32 AABBInFrustumGroupBounds(const LLviewerOctreeGroup* group);
	S32 AABBInFrustumObjectBounds(const LLviewerOctreeGroup* group);
	
	virtual S32 frustumCheck(const LLviewerOctreeGroup* group) = 0;
	virtual S32 frustumCheckObjects(const LLviewerOctreeGroup* group) = 0;

	virtual bool checkObjects(const OctreeNode* branch, const LLviewerOctreeGroup* group);
	virtual void preprocess(LLviewerOctreeGroup* group);
	virtual void processGroup(LLviewerOctreeGroup* group);
	virtual void visit(const OctreeNode* branch);
	
protected:
	LLCamera *mCamera;
	S32 mRes;
};

#endif