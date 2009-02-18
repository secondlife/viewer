/** 
 * @file llspatialpartition.h
 * @brief LLSpatialGroup header file including definitions for supporting functions
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
#include "lldrawpool.h"
#include "llface.h"

#include <queue>

#define SG_STATE_INHERIT_MASK (OCCLUDED)
#define SG_INITIAL_STATE_MASK (DIRTY | GEOM_DIRTY)

class LLSpatialPartition;
class LLSpatialBridge;
class LLSpatialGroup;

S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &radius_squared);

// get index buffer for binary encoded axis vertex buffer given a box at center being viewed by given camera
U8* get_box_fan_indices(LLCamera* camera, const LLVector3& center);

class LLDrawInfo : public LLRefCount 
{
protected:
	~LLDrawInfo();	
	
public:
	LLDrawInfo(U16 start, U16 end, U32 count, U32 offset, 
				LLViewerImage* image, LLVertexBuffer* buffer, 
				BOOL fullbright = FALSE, U8 bump = 0, BOOL particle = FALSE, F32 part_size = 0);
	

	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLViewerImage> mTexture;
	LLColor4U mGlowColor;
	S32 mDebugColor;
	const LLMatrix4* mTextureMatrix;
	const LLMatrix4* mModelMatrix;
	U16 mStart;
	U16 mEnd;
	U32 mCount;
	U32 mOffset;
	BOOL mFullbright;
	U8 mBump;
	BOOL mParticle;
	F32 mPartSize;
	F32 mVSize;
	LLSpatialGroup* mGroup;
	LLFace* mFace; //associated face
	F32 mDistance;
	LLVector3 mExtents[2];

	struct CompareTexture
	{
		bool operator()(const LLDrawInfo& lhs, const LLDrawInfo& rhs)
		{
			return lhs.mTexture > rhs.mTexture;
		}
	};

	struct CompareTexturePtr
	{ //sort by texture
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs)	
		{
			// sort by pointer, sort NULL down to the end
			return lhs.get() != rhs.get() 
						&& (lhs.isNull() || (rhs.notNull() && lhs->mTexture.get() > rhs->mTexture.get()));
		}
	};

	struct CompareVertexBuffer
	{ //sort by texture
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs)	
		{
			// sort by pointer, sort NULL down to the end
			return lhs.get() != rhs.get() 
					&& (lhs.isNull() || (rhs.notNull() && lhs->mVertexBuffer.get() > rhs->mVertexBuffer.get()));
		}
	};

	struct CompareTexturePtrMatrix
	{
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs)	
		{
			return lhs.get() != rhs.get() 
						&& (lhs.isNull() || (rhs.notNull() && (lhs->mTexture.get() > rhs->mTexture.get() ||
															   (lhs->mTexture.get() == rhs->mTexture.get() && lhs->mModelMatrix > rhs->mModelMatrix))));
		}

	};

	struct CompareBump
	{
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs) 
		{
			// sort by mBump value, sort NULL down to the end
			return lhs.get() != rhs.get() 
						&& (lhs.isNull() || (rhs.notNull() && lhs->mBump > rhs->mBump));
		}
	};

	struct CompareDistanceGreater
	{
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs) 
		{
			// sort by mBump value, sort NULL down to the end
			return lhs.get() != rhs.get() 
						&& (lhs.isNull() || (rhs.notNull() && lhs->mDistance > rhs->mDistance));
		}
	};
};

class LLSpatialGroup : public LLOctreeListener<LLDrawable>
{
	friend class LLSpatialPartition;
public:
	static U32 sNodeCount;
	static BOOL sNoDelete; //deletion of spatial groups and draw info not allowed if TRUE

	typedef std::vector<LLPointer<LLSpatialGroup> > sg_vector_t;
	typedef std::set<LLPointer<LLSpatialGroup> > sg_set_t;
	typedef std::vector<LLPointer<LLSpatialBridge> > bridge_list_t;
	typedef std::vector<LLPointer<LLDrawInfo> > drawmap_elem_t; 
	typedef std::map<U32, drawmap_elem_t > draw_map_t;	
	typedef std::vector<LLPointer<LLVertexBuffer> > buffer_list_t;
	typedef std::map<LLPointer<LLViewerImage>, buffer_list_t> buffer_texture_map_t;
	typedef std::map<U32, buffer_texture_map_t> buffer_map_t;

	typedef LLOctreeListener<LLDrawable>	BaseType;
	typedef LLOctreeListener<LLDrawable>	OctreeListener;
	typedef LLTreeNode<LLDrawable>			TreeNode;
	typedef LLOctreeNode<LLDrawable>		OctreeNode;
	typedef LLOctreeRoot<LLDrawable>		OctreeRoot;
	typedef LLOctreeTraveler<LLDrawable>	OctreeTraveler;
	typedef LLOctreeNode<LLDrawable>::element_iter element_iter;
	typedef LLOctreeNode<LLDrawable>::element_list element_list;

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
		OCCLUDED				= 0x00000001,
		IN_QUEUE				= 0x00000002,
		QUERY_PENDING			= 0x00000004,
		ACTIVE_OCCLUSION		= 0x00000008,
		DISCARD_QUERY			= 0x00000010,
		DEAD					= 0x00000020,
		EARLY_FAIL				= 0x00000040,
		DIRTY					= 0x00000080,
		OBJECT_DIRTY			= 0x00000100,
		GEOM_DIRTY				= 0x00000200,
		ALPHA_DIRTY				= 0x00000800,
		SKIP_FRUSTUM_CHECK		= 0x00001000,
		IN_IMAGE_QUEUE			= 0x00002000,
		IMAGE_DIRTY				= 0x00004000,
		OCCLUSION_DIRTY			= 0x00008000,
		MESH_DIRTY				= 0x00010000,
	} eSpatialState;

	typedef enum
	{
		STATE_MODE_SINGLE = 0,		//set one node
		STATE_MODE_BRANCH,			//set entire branch
		STATE_MODE_DIFF				//set entire branch as long as current state is different
	} eSetStateMode;

	LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part);

	BOOL isDead()							{ return isState(DEAD); }
	BOOL isState(U32 state) const			{ return mState & state ? TRUE : FALSE; }
	U32 getState()							{ return mState; }
	void setState(U32 state);	
	void clearState(U32 state);	
	
	void clearDrawMap();
	void validate();
	void checkStates();
	void validateDrawMap();
	
	void setState(U32 state, S32 mode);

	LLSpatialGroup* getParent();

	void clearState(U32 state, S32 mode);
	BOOL addObject(LLDrawable *drawablep, BOOL add_all = FALSE, BOOL from_octree = FALSE);
	BOOL removeObject(LLDrawable *drawablep, BOOL from_octree = FALSE);
	BOOL updateInGroup(LLDrawable *drawablep, BOOL immediate = FALSE); // Update position if it's in the group
	BOOL isVisible() const;
	void setVisible();
	void shift(const LLVector3 &offset);
	BOOL boundObjects(BOOL empty, LLVector3& newMin, LLVector3& newMax);
	void unbound();
	BOOL rebound();
	void buildOcclusion(); //rebuild mOcclusionVerts
	void checkOcclusion(); //read back last occlusion query (if any)
	void doOcclusion(LLCamera* camera); //issue occlusion query
	void destroyGL();
	
	void updateDistance(LLCamera& camera);
	BOOL needsUpdate();
	BOOL changeLOD();
	void rebuildGeom();
	void rebuildMesh();

	void dirtyGeom() { setState(GEOM_DIRTY); }
	void dirtyMesh() { setState(MESH_DIRTY); }
	element_list& getData() { return mOctreeNode->getData(); }
	U32 getElementCount() const { return mOctreeNode->getElementCount(); }

	 //LISTENER FUNCTIONS
	virtual void handleInsertion(const TreeNode* node, LLDrawable* face);
	virtual void handleRemoval(const TreeNode* node, LLDrawable* face);
	virtual void handleDestruction(const TreeNode* node);
	virtual void handleStateChange(const TreeNode* node);
	virtual void handleChildAddition(const OctreeNode* parent, OctreeNode* child);
	virtual void handleChildRemoval(const OctreeNode* parent, const OctreeNode* child);

protected:
	virtual ~LLSpatialGroup();

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
	F32*					mOcclusionVerts;
	GLuint					mOcclusionQuery;

	U32 mBufferUsage;
	draw_map_t mDrawMap;
	
	S32 mVisible;
	F32 mDistance;
	F32 mDepth;
	F32 mLastUpdateDistance;
	F32 mLastUpdateTime;
			
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
	virtual void rebuildMesh(LLSpatialGroup* group) = 0;
	virtual void getGeometry(LLSpatialGroup* group) = 0;
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32 &index_count);
	
	virtual LLVertexBuffer* createVertexBuffer(U32 type_mask, U32 usage);
};

class LLSpatialPartition: public LLGeometryManager
{
public:
	static BOOL sFreezeState; //if true, no spatialgroup state updates will be made

	LLSpatialPartition(U32 data_mask, U32 mBufferUsage = GL_STATIC_DRAW_ARB);
	virtual ~LLSpatialPartition();

	LLSpatialGroup *put(LLDrawable *drawablep, BOOL was_visible = FALSE);
	BOOL remove(LLDrawable *drawablep, LLSpatialGroup *curp);
	
	LLDrawable* lineSegmentIntersect(const LLVector3& start, const LLVector3& end,
									 BOOL pick_transparent, 
									 S32* face_hit,                          // return the face hit
									 LLVector3* intersection = NULL,         // return the intersection point
									 LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
									 LLVector3* normal = NULL,               // return the surface normal at the intersection point
									 LLVector3* bi_normal = NULL             // return the surface bi-normal at the intersection point
		);
	
	
	// If the drawable moves, move it here.
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	virtual void shift(const LLVector3 &offset);

	virtual F32 calcDistance(LLSpatialGroup* group, LLCamera& camera);
	virtual F32 calcPixelArea(LLSpatialGroup* group, LLCamera& camera);

	virtual void rebuildGeom(LLSpatialGroup* group);
	virtual void rebuildMesh(LLSpatialGroup* group);

	BOOL visibleObjectsInFrustum(LLCamera& camera);
	S32 cull(LLCamera &camera, std::vector<LLDrawable *>* results = NULL, BOOL for_select = FALSE); // Cull on arbitrary frustum
	
	BOOL isVisible(const LLVector3& v);
	
	virtual LLSpatialBridge* asBridge() { return NULL; }
	virtual BOOL isBridge() { return asBridge() != NULL; }

	void renderDebug();
	void renderIntersectingBBoxes(LLCamera* camera);
	void restoreGL();
	void resetVertexBuffers();
	BOOL isOcclusionEnabled();
	BOOL getVisibleExtents(LLCamera& camera, LLVector3& visMin, LLVector3& visMax);

public:
	LLSpatialGroup::OctreeNode* mOctree;
	BOOL mOcclusionEnabled; // if TRUE, occlusion culling is performed
	BOOL mInfiniteFarClip; // if TRUE, frustum culling ignores far clip plane
	U32 mBufferUsage;
	BOOL mRenderByGroup;
	U32 mLODSeed;
	U32 mLODPeriod;	//number of frames between LOD updates for a given spatial group (staggered by mLODSeed)
	U32 mVertexDataMask;
	F32 mSlopRatio; //percentage distance must change before drawables receive LOD update (default is 0.25);
	BOOL mDepthMask; //if TRUE, objects in this partition will be written to depth during alpha rendering
	U32 mDrawableType;
	U32 mPartitionType;
};

// class for creating bridges between spatial partitions
class LLSpatialBridge : public LLDrawable, public LLSpatialPartition
{
protected:
	~LLSpatialBridge();

public:
	typedef std::vector<LLPointer<LLSpatialBridge> > bridge_vector_t;
	
	LLSpatialBridge(LLDrawable* root, U32 data_mask);
	
	virtual BOOL isSpatialBridge() const		{ return TRUE; }

	virtual void updateSpatialExtents();
	virtual void updateBinRadius();
	virtual void setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results = NULL, BOOL for_select = FALSE);
	virtual void updateDistance(LLCamera& camera_in);
	virtual void makeActive();
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	virtual BOOL updateMove();
	virtual void shiftPos(const LLVector3& vec);
	virtual void cleanupReferences();
	virtual LLSpatialPartition* asPartition()		{ return this; }
	virtual LLSpatialBridge* asBridge()				{ return this; }
	
	virtual LLCamera transformCamera(LLCamera& camera);
	
	LLDrawable* mDrawable;
};

class LLCullResult 
{
public:
	LLCullResult();

	typedef std::vector<LLSpatialGroup*> sg_list_t;
	typedef std::vector<LLDrawable*> drawable_list_t;
	typedef std::vector<LLSpatialBridge*> bridge_list_t;
	typedef std::vector<LLDrawInfo*> drawinfo_list_t;

	void clear();
	
	sg_list_t::iterator beginVisibleGroups();
	sg_list_t::iterator endVisibleGroups();

	sg_list_t::iterator beginAlphaGroups();
	sg_list_t::iterator endAlphaGroups();

	sg_list_t::iterator beginOcclusionGroups();
	sg_list_t::iterator endOcclusionGroups();

	sg_list_t::iterator beginDrawableGroups();
	sg_list_t::iterator endDrawableGroups();

	drawable_list_t::iterator beginVisibleList();
	drawable_list_t::iterator endVisibleList();

	bridge_list_t::iterator beginVisibleBridge();
	bridge_list_t::iterator endVisibleBridge();

	drawinfo_list_t::iterator beginRenderMap(U32 type);
	drawinfo_list_t::iterator endRenderMap(U32 type);

	void pushVisibleGroup(LLSpatialGroup* group);
	void pushAlphaGroup(LLSpatialGroup* group);
	void pushOcclusionGroup(LLSpatialGroup* group);
	void pushDrawableGroup(LLSpatialGroup* group);
	void pushDrawable(LLDrawable* drawable);
	void pushBridge(LLSpatialBridge* bridge);
	void pushDrawInfo(U32 type, LLDrawInfo* draw_info);
	
	U32 getVisibleGroupsSize()		{ return mVisibleGroupsSize; }
	U32	getAlphaGroupsSize()		{ return mAlphaGroupsSize; }
	U32	getDrawableGroupsSize()		{ return mDrawableGroupsSize; }
	U32	getVisibleListSize()		{ return mVisibleListSize; }
	U32	getVisibleBridgeSize()		{ return mVisibleBridgeSize; }
	U32	getRenderMapSize(U32 type)	{ return mRenderMapSize[type]; }

	void assertDrawMapsEmpty();

private:
	U32					mVisibleGroupsSize;
	U32					mAlphaGroupsSize;
	U32					mOcclusionGroupsSize;
	U32					mDrawableGroupsSize;
	U32					mVisibleListSize;
	U32					mVisibleBridgeSize;
	U32					mRenderMapSize[LLRenderPass::NUM_RENDER_TYPES];

	sg_list_t			mVisibleGroups;
	sg_list_t			mAlphaGroups;
	sg_list_t			mOcclusionGroups;
	sg_list_t			mDrawableGroups;
	drawable_list_t		mVisibleList;
	bridge_list_t		mVisibleBridge;
	drawinfo_list_t		mRenderMap[LLRenderPass::NUM_RENDER_TYPES];
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

class LLHUDParticlePartition : public LLParticlePartition
{
public:
	LLHUDParticlePartition();
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
	virtual void rebuildMesh(LLSpatialGroup* group);
	virtual void getGeometry(LLSpatialGroup* group);
	void genDrawInfo(LLSpatialGroup* group, U32 mask, std::vector<LLFace*>& faces, BOOL distance_sort = FALSE);
	void registerFace(LLSpatialGroup* group, LLFace* facep, U32 type);

};

//spatial partition that uses volume geometry manager (implemented in LLVOVolume.cpp)
class LLVolumePartition : public LLSpatialPartition, public LLVolumeGeometryManager
{
public:
	LLVolumePartition();
	virtual void rebuildGeom(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildGeom(group); }
	virtual void getGeometry(LLSpatialGroup* group) { LLVolumeGeometryManager::getGeometry(group); }
	virtual void rebuildMesh(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildMesh(group); }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { LLVolumeGeometryManager::addGeometryCount(group, vertex_count, index_count); }
};

//spatial bridge that uses volume geometry manager (implemented in LLVOVolume.cpp)
class LLVolumeBridge : public LLSpatialBridge, public LLVolumeGeometryManager
{
public:
	LLVolumeBridge(LLDrawable* drawable);
	virtual void rebuildGeom(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildGeom(group); }
	virtual void getGeometry(LLSpatialGroup* group) { LLVolumeGeometryManager::getGeometry(group); }
	virtual void rebuildMesh(LLSpatialGroup* group) { LLVolumeGeometryManager::rebuildMesh(group); }
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

