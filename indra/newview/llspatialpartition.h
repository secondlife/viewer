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
#include "llvertexbuffer.h"
#include "llgltypes.h"
#include "llcubemap.h"

#include <queue>

#define SG_STATE_INHERIT_MASK (CULLED | OCCLUDED)
#define SG_INITIAL_STATE_MASK (OCCLUSION_DIRTY | DIRTY | GEOM_DIRTY)

class LLSpatialPartition;
class LLSpatialBridge;

class LLDrawInfo
{
public:
	LLDrawInfo(U32 start, U32 end, U32 count, U32 offset, 
				LLViewerImage* image, LLVertexBuffer* buffer, 
				BOOL fullbright = FALSE, U8 bump = 0, BOOL particle = FALSE, F32 part_size = 0);
	
	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLViewerImage> mTexture;
	LLPointer<LLCubeMap> mReflectionMap;
	LLColor4U mGlowColor;
	const LLMatrix4* mTextureMatrix;
	U32 mStart;
	U32 mEnd;
	U32 mCount;
	U32 mOffset;
	BOOL mFullbright;
	U8 mBump;
	BOOL mParticle;
	F32 mPartSize;
	F32 mVSize;
	
	struct CompareTexture
	{
		bool operator()(const LLDrawInfo& lhs, const LLDrawInfo& rhs)
		{
			return lhs.mTexture > rhs.mTexture;
		}
	};

	struct CompareTexturePtr
	{
		bool operator()(const LLDrawInfo* const& lhs, const LLDrawInfo* const& rhs)
		{
		
			return lhs == NULL || rhs == NULL || lhs->mTexture > rhs->mTexture;
		}
	};

	struct CompareBump
	{
		bool operator()(const LLDrawInfo* const& lhs, const LLDrawInfo* const& rhs)
		{
			return lhs == NULL || rhs == NULL || lhs->mBump > rhs->mBump;
		}
	};
};

class LLSpatialGroup : public LLOctreeListener<LLDrawable>
{
	friend class LLSpatialPartition;
public:

	typedef std::vector<LLPointer<LLSpatialGroup> > sg_vector_t;
	typedef std::set<LLPointer<LLSpatialGroup> > sg_set_t;
	typedef std::vector<LLPointer<LLSpatialBridge> > bridge_list_t;
	typedef std::map<U32, std::vector<LLDrawInfo*> > draw_map_t;
	typedef std::map<LLPointer<LLViewerImage>, LLPointer<LLVertexBuffer> > buffer_map_t;

	typedef LLOctreeListener<LLDrawable>	BaseType;
	typedef LLOctreeListener<LLDrawable>	OctreeListener;
	typedef LLTreeNode<LLDrawable>			TreeNode;
	typedef LLOctreeNode<LLDrawable>		OctreeNode;
	typedef LLOctreeRoot<LLDrawable>		OctreeRoot;
	typedef LLOctreeState<LLDrawable>		OctreeState;
	typedef LLOctreeTraveler<LLDrawable>	OctreeTraveler;
	typedef LLOctreeState<LLDrawable>::element_iter element_iter;
	typedef LLOctreeState<LLDrawable>::element_list element_list;

	struct CompareDistanceGreater
	{
		bool operator()(const LLSpatialGroup* const& lhs, const LLSpatialGroup* const& rhs)
		{
			return lhs->mDistance > rhs->mDistance;
		}
	};

	struct CompareDepthGreater
	{
		bool operator()(const LLSpatialGroup* const& lhs, const LLSpatialGroup* const& rhs)
		{
			return lhs->mDepth > rhs->mDepth;
		}
	};

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
		GEOM_DIRTY				= 0x00001000,
		MATRIX_DIRTY			= 0x00002000,
		ALPHA_DIRTY				= 0x00004000,
		DISCARD_QUERY			= 0x00008000,
		QUERY_OUT				= 0x00010000,
		OCCLUDING				= 0x00020000,
		SKIP_FRUSTUM_CHECK		= 0x00040000,
		OCCLUSION_DIRTY			= 0x00080000,
		BELOW_WATER				= 0x00100000,
		IN_IMAGE_QUEUE			= 0x00200000,
		IMAGE_DIRTY				= 0x00400000,
	} eSpatialState;

	typedef enum
	{
		STATE_MODE_SINGLE = 0,		//set one node
		STATE_MODE_BRANCH,			//set entire branch
		STATE_MODE_DIFF				//set entire branch as long as current state is different
	} eSetStateMode;

	LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part);
	virtual ~LLSpatialGroup();

	BOOL isDead()							{ return isState(DEAD); }
	BOOL isState(U32 state) const			{ return mState & state ? TRUE : FALSE; }
	U32 getState()							{ return mState; }
	void setState(U32 state)				{ mState |= state; }
	void clearState(U32 state)				{ mState &= ~state; }
	
	void clearDrawMap();
	void validate();
	void validateDrawMap();
	
	void setState(U32 state, S32 mode);

	LLSpatialGroup* getParent();

	void clearState(U32 state, S32 mode);
	BOOL addObject(LLDrawable *drawablep, BOOL add_all = FALSE, BOOL from_octree = FALSE);
	BOOL removeObject(LLDrawable *drawablep, BOOL from_octree = FALSE);
	BOOL updateInGroup(LLDrawable *drawablep, BOOL immediate = FALSE); // Update position if it's in the group
	BOOL isVisible();
	void shift(const LLVector3 &offset);
	BOOL boundObjects(BOOL empty, LLVector3& newMin, LLVector3& newMax);
	void unbound();
	BOOL rebound();
	void destroyGL();
	
	void updateDistance(LLCamera& camera);
	BOOL changeLOD();
	void rebuildGeom();
	void makeStatic();
	
	void dirtyGeom() { setState(GEOM_DIRTY); }
	element_list& getData() { return mOctreeNode->getOctState()->getData(); }

	 //LISTENER FUNCTIONS
	virtual void handleInsertion(const TreeNode* node, LLDrawable* face);
	virtual void handleRemoval(const TreeNode* node, LLDrawable* face);
	virtual void handleDestruction(const TreeNode* node);
	virtual void handleStateChange(const TreeNode* node);
	virtual void handleChildAddition(const OctreeNode* parent, OctreeNode* child);
	virtual void handleChildRemoval(const OctreeNode* parent, const OctreeNode* child);

protected:
	U32 mState;
	S32 mLODHash;
	static S32 sLODSeed;

public:
	bridge_list_t mBridgeList;
	buffer_map_t mBufferMap; //used by volume buffers to store unique buffers per texture

	F32 mBuilt;
	OctreeNode* mOctreeNode;
	LLSpatialPartition* mSpatialPartition;
	LLVector3 mBounds[2];
	LLVector3 mExtents[2];
	LLVector3 mObjectExtents[2];
	LLVector3 mObjectBounds[2];

	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLVertexBuffer> mOcclusionVerts;
	LLPointer<LLCubeMap>	mReflectionMap;

	U32 mBufferUsage;
	draw_map_t mDrawMap;
	
	U32 mVertexCount;
	U32 mIndexCount;
	F32 mDistance;
	F32 mDepth;
	F32 mLastUpdateDistance;
	F32 mLastUpdateTime;
	F32 mLastAddTime;
	F32 mLastRenderTime;
	
	LLVector3 mViewAngle;
	LLVector3 mLastUpdateViewAngle;
	
	F32 mPixelArea;
	F32 mRadius;
};

class LLGeometryManager
{
public:
	std::vector<LLFace*> mFaceList;
	virtual ~LLGeometryManager() { }
	virtual void rebuildGeom(LLSpatialGroup* group) = 0;
	virtual void getGeometry(LLSpatialGroup* group) = 0;
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32 &index_count);
	virtual LLVertexBuffer* createVertexBuffer(U32 type_mask, U32 usage);
};

class LLSpatialPartition: public LLGeometryManager
{
public:
	LLSpatialPartition(U32 data_mask, BOOL is_volatile = FALSE, U32 mBufferUsage = GL_STATIC_DRAW_ARB);
	virtual ~LLSpatialPartition();

	LLSpatialGroup *put(LLDrawable *drawablep, BOOL was_visible = FALSE);
	BOOL remove(LLDrawable *drawablep, LLSpatialGroup *curp);
	
	LLDrawable*	pickDrawable(const LLVector3& start, const LLVector3& end, LLVector3& collision);
	
	// If the drawable moves, move it here.
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	virtual void shift(const LLVector3 &offset);

	virtual F32 calcDistance(LLSpatialGroup* group, LLCamera& camera);
	virtual F32 calcPixelArea(LLSpatialGroup* group, LLCamera& camera);

	virtual void rebuildGeom(LLSpatialGroup* group);

	S32 cull(LLCamera &camera, std::vector<LLDrawable *>* results = NULL, BOOL for_select = FALSE); // Cull on arbitrary frustum
	BOOL checkOcclusion(LLSpatialGroup* group, LLCamera* camera);
	void markReimage(LLSpatialGroup* group);
	void processImagery(LLCamera* camera);
	void processOcclusion(LLCamera* camera);
	void buildOcclusion();
	void doOcclusion(LLCamera* camera);
	BOOL isVisible(const LLVector3& v);
	BOOL isVolatile() const { return mVolatile; }

	virtual LLSpatialBridge* asBridge() { return NULL; }
	virtual BOOL isBridge() { return asBridge() != NULL; }

	S32 getObjects(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results );
	S32 getLights(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results );
	
	void renderDebug();
	void restoreGL();
	void resetVertexBuffers();
	
protected:
	S32 getDrawables(const LLVector3& pos,  F32 rad,  LLDrawable::drawable_set_t &results, BOOL get_lights );
	
	typedef std::set<LLPointer<LLSpatialGroup> > spatial_group_set_t;
	spatial_group_set_t mSpatialGroups;

	//things that might be occluded
	typedef std::queue<LLPointer<LLSpatialGroup> > spatial_group_queue_t;
	spatial_group_queue_t mOcclusionQueue;

	//things that need an image update
	spatial_group_queue_t mImageQueue;

	//things awaiting query
	spatial_group_queue_t mQueryQueue;

	std::vector<LLGLuint> mOcclusionQueries;	

public:
	LLSpatialGroup::OctreeNode* mOctree;

	U32 mBufferUsage;
	BOOL mRenderByGroup;
	BOOL mImageEnabled;
	U32 mLODSeed;
	U32 mLODPeriod;
	U32 mVertexDataMask;
	F32 mSlopRatio; //percentage distance must change before drawables receive LOD update (default is 0.25);
	BOOL mVolatile; //if TRUE, occlusion queries will be discarded when nodes change size
	BOOL mDepthMask; //if TRUE, objects in this partition will be written to depth during alpha rendering
	U32 mDrawableType;
	U32 mPartitionType;

	//index buffer for occlusion verts
	LLPointer<LLVertexBuffer> mOcclusionIndices;

	//things that are occluded
	std::vector<LLPointer<LLSpatialGroup> > mOccludedList;
};

// class for creating bridges between spatial partitions
class LLSpatialBridge : public LLDrawable, public LLSpatialPartition
{
public:
	typedef std::vector<LLPointer<LLSpatialBridge> > bridge_vector_t;
	
	LLSpatialBridge(LLDrawable* root, U32 data_mask);
	virtual ~LLSpatialBridge();
	
	virtual BOOL isSpatialBridge() const		{ return TRUE; }

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
	virtual LLSpatialBridge* asBridge()				{ return this; }
	
	virtual LLCamera transformCamera(LLCamera& camera);
	
	LLDrawable* mDrawable;
};

//spatial partition for water (implemented in LLVOWater.cpp)
class LLWaterPartition : public LLSpatialPartition
{
public:
	LLWaterPartition();
	virtual void getGeometry(LLSpatialGroup* group) {  }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { }
};

//spatial partition for terrain (impelmented in LLVOSurfacePatch.cpp)
class LLTerrainPartition : public LLSpatialPartition
{
public:
	LLTerrainPartition();
	virtual void getGeometry(LLSpatialGroup* group);
	virtual LLVertexBuffer* createVertexBuffer(U32 type_mask, U32 usage);
};

//spatial partition for trees
class LLTreePartition : public LLSpatialPartition
{
public:
	LLTreePartition();
	virtual void getGeometry(LLSpatialGroup* group) { }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { }

};

//spatial partition for particles (implemented in LLVOPartGroup.cpp)
class LLParticlePartition : public LLSpatialPartition
{
public:
	LLParticlePartition();
	virtual void getGeometry(LLSpatialGroup* group);
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count);
	virtual F32 calcPixelArea(LLSpatialGroup* group, LLCamera& camera);
protected:
	U32 mRenderPass;
};

//spatial partition for grass (implemented in LLVOGrass.cpp)
class LLGrassPartition : public LLParticlePartition
{
public:
	LLGrassPartition();
};

//spatial partition for clouds (implemented in LLVOClouds.cpp)
class LLCloudPartition : public LLParticlePartition
{
public:
	LLCloudPartition();
};

//class for wrangling geometry out of volumes (implemented in LLVOVolume.cpp)
class LLVolumeGeometryManager: public LLGeometryManager
{
public:
	virtual ~LLVolumeGeometryManager() { }
	virtual void rebuildGeom(LLSpatialGroup* group);
	virtual void getGeometry(LLSpatialGroup* group);
	void registerFace(LLSpatialGroup* group, LLFace* facep, U32 type);
};

//spatial partition that uses volume geometry manager (implemented in LLVOVolume.cpp)
class LLVolumePartition : public LLSpatialPartition, public LLVolumeGeometryManager
{
public:
	LLVolumePartition();
	virtual void rebuildGeom(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildGeom(group); }
	virtual void getGeometry(LLSpatialGroup* group) { LLVolumeGeometryManager::getGeometry(group); }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { LLVolumeGeometryManager::addGeometryCount(group, vertex_count, index_count); }
};

//spatial bridge that uses volume geometry manager (implemented in LLVOVolume.cpp)
class LLVolumeBridge : public LLSpatialBridge, public LLVolumeGeometryManager
{
public:
	LLVolumeBridge(LLDrawable* drawable);
	virtual void rebuildGeom(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildGeom(group); }
	virtual void getGeometry(LLSpatialGroup* group) { LLVolumeGeometryManager::getGeometry(group); }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { LLVolumeGeometryManager::addGeometryCount(group, vertex_count, index_count); }
};

class LLHUDBridge : public LLVolumeBridge
{
public:
	LLHUDBridge(LLDrawable* drawablep);
	virtual void shiftPos(const LLVector3& vec);
	virtual F32 calcPixelArea(LLSpatialGroup* group, LLCamera& camera);
};

//spatial partition that holds nothing but spatial bridges
class LLBridgePartition : public LLSpatialPartition
{
public:
	LLBridgePartition();
	virtual void getGeometry(LLSpatialGroup* group) { }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) {  }
};

class LLHUDPartition : public LLBridgePartition
{
public:
	LLHUDPartition();
	virtual void shift(const LLVector3 &offset);
};

void validate_draw_info(LLDrawInfo& params);

extern const F32 SG_BOX_SIDE;
extern const F32 SG_BOX_OFFSET;
extern const F32 SG_BOX_RAD;

extern const F32 SG_OBJ_SIDE;
extern const F32 SG_MAX_OBJ_RAD;

#endif //LL_LLSPATIALPARTITION_H

