/** 
 * @file llspatialpartition.h
 * @brief LLSpatialGroup header file including definitions for supporting functions
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSPATIALPARTITION_H
#define LL_LLSPATIALPARTITION_H

#define SG_MIN_DIST_RATIO 0.00001f

#include "llmemory.h"
#include "lldrawable.h"
#include "lloctree.h"
#include "llgltypes.h"

#include <queue>

class LLCullInfo
{
public:
	LLVector3 mPos;
	F32 mRadius;
	LLPointer<LLDrawable> mDrawablep;
};

#define SG_STATE_INHERIT_MASK (CULLED | OCCLUDED)
class LLSpatialPartition;

class LLSpatialGroup : public LLOctreeListener<LLDrawable>
{
	friend class LLSpatialPartition;
public:

	typedef LLOctreeListener<LLDrawable>	BaseType;
	typedef LLOctreeListener<LLDrawable>	OctreeListener;
	typedef LLTreeNode<LLDrawable>			TreeNode;
	typedef LLOctreeNode<LLDrawable>		OctreeNode;
	typedef LLOctreeRoot<LLDrawable>		OctreeRoot;
	typedef LLOctreeState<LLDrawable>		OctreeState;
	typedef LLOctreeTraveler<LLDrawable>	OctreeTraveler;

	typedef enum
	{
		IN_QUEUE				= 0x00000001,
		QUERY_PENDING			= 0x00000002,
		CULLED					= 0x00000004,
		OCCLUDED				= 0x00000008,
		DEAD					= 0x00000010,
		ACTIVE_OCCLUSION		= 0x00000020,
		EARLY_FAIL				= 0x00000040,
		DEACTIVATE_OCCLUSION	= 0x00000080,
		RESHADOW				= 0x00000100,
		RESHADOW_QUEUE			= 0x00000200,
		DIRTY					= 0x00000400,
		OBJECT_DIRTY			= 0x00000800,
		DISCARD_QUERY			= 0x00001000,
		QUERY_OUT				= 0x00002000,
		OCCLUDING				= 0x00004000,
		SKIP_FRUSTUM_CHECK		= 0x00008000,
	} eSpatialState;

	typedef enum
	{
		STATE_MODE_SINGLE = 0,		//set one node
		STATE_MODE_BRANCH,			//set entire branch
		STATE_MODE_DIFF				//set entire branch as long as current state is different
	} eSetStateMode;

	LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part);
	BOOL safeToDelete();
	virtual ~LLSpatialGroup();

	S32 getCount() const					{ return mObjects.size(); }
	BOOL isDead()							{ return isState(DEAD); }
	BOOL isState(U32 state) const			{ return mState & state ? TRUE : FALSE; }
	U32 getState()							{ return mState; }
	void setState(U32 state)				{ mState |= state; }
	void clearState(U32 state)				{ mState &= ~state; }
	
	void validate();

	void setState(U32 state, S32 mode);


	void clearState(U32 state, S32 mode);
	BOOL addObject(LLDrawable *drawablep, BOOL add_all = FALSE, BOOL from_octree = FALSE);
	BOOL removeObject(LLDrawable *drawablep, BOOL from_octree = FALSE);
	BOOL updateInGroup(LLDrawable *drawablep, BOOL immediate = FALSE); // Update position if it's in the group
	BOOL isVisible();
	void shift(const LLVector3 &offset);
	BOOL boundObjects(BOOL empty, LLVector3& newMin, LLVector3& newMax);
	void unbound();
	BOOL rebound();
	BOOL changeLOD();
		
	 //LISTENER FUNCTIONS
	virtual void handleInsertion(const TreeNode* node, LLDrawable* face);
	virtual void handleRemoval(const TreeNode* node, LLDrawable* face);
	virtual void handleDestruction(const TreeNode* node);
	virtual void handleStateChange(const TreeNode* node);
	virtual void handleChildAddition(const OctreeNode* parent, OctreeNode* child);
	virtual void handleChildRemoval(const OctreeNode* parent, const OctreeNode* child);

protected:
	std::vector<LLCullInfo> mObjects;
	U32 mState;
	S32 mLODHash;
	static S32 sLODSeed;

public:
	OctreeNode* mOctreeNode;
	LLSpatialPartition* mSpatialPartition;
	LLVector3 mBounds[2];
	LLVector3 mExtents[2];
	LLVector3 mObjectExtents[2];
	LLVector3 mObjectBounds[2];

};

class LLSpatialPartition
{
public:
	LLSpatialPartition();
	virtual ~LLSpatialPartition();

	LLSpatialGroup *put(LLDrawable *drawablep);
	BOOL remove(LLDrawable *drawablep, LLSpatialGroup *curp);
	
	LLDrawable*	pickDrawable(const LLVector3& start, const LLVector3& end, LLVector3& collision);
	
	// If the drawable moves, move it here.
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	void shift(const LLVector3 &offset);

	S32 cull(LLCamera &camera, std::vector<LLDrawable *>* results = NULL, BOOL for_select = FALSE); // Cull on arbitrary frustum
	BOOL checkOcclusion(LLSpatialGroup* group, LLCamera* camera);
	void processOcclusion(LLCamera* camera);
	void doOcclusion(LLCamera* camera);
	BOOL isVisible(const LLVector3& v);
	
	S32 getObjects(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results );
	S32 getLights(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results );
	
	void renderDebug();
	void restoreGL();
	
protected:
	S32 getDrawables(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results, BOOL get_lights );
	
	LLSpatialGroup *mLastAddedGroupp;

	typedef std::set<LLSpatialGroup*> spatial_group_set_t;
	spatial_group_set_t mSpatialGroups;

	//things that might be occluded
	std::queue<LLSpatialGroup*> mOcclusionQueue;

	//things awaiting query
	std::queue<LLSpatialGroup*> mQueryQueue;

	std::vector<LLGLuint> mOcclusionQueries;	

public:
	LLSpatialGroup::OctreeNode* mOctree;

	//things that are occluded
	std::vector<LLSpatialGroup*> mOccludedList;

	std::queue<LLSpatialGroup*> mReshadowQueue;

};

// class for creating bridges between spatial partitions
class LLSpatialBridge : public LLDrawable, public LLSpatialPartition
{
public:
	LLSpatialBridge(LLDrawable* root);
	virtual ~LLSpatialBridge();
	
	virtual BOOL		isSpatialBridge() const		{ return TRUE; }

	virtual void updateSpatialExtents();
	virtual void updateBinRadius();
	virtual void setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results = NULL, BOOL for_select = FALSE);
	virtual void updateDistance(LLCamera& camera_in);
	virtual void makeActive();
	virtual void makeStatic();
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	virtual BOOL updateMove();
	virtual void shiftPos(const LLVector3& vec);
	virtual void cleanupReferences();
	virtual LLSpatialPartition* asPartition()		{ return this; }
	LLCamera transformCamera(LLCamera& camera);
	
	LLDrawable* mDrawable;
};

extern const F32 SG_BOX_SIDE;
extern const F32 SG_BOX_OFFSET;
extern const F32 SG_BOX_RAD;

extern const F32 SG_OBJ_SIDE;
extern const F32 SG_MAX_OBJ_RAD;

#endif //LL_LLSPATIALPARTITION_H

