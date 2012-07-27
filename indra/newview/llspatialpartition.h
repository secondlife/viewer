/** 
 * @file llspatialpartition.h
 * @brief LLSpatialGroup header file including definitions for supporting functions
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

#ifndef LL_LLSPATIALPARTITION_H
#define LL_LLSPATIALPARTITION_H

#define SG_MIN_DIST_RATIO 0.00001f

#include "lldrawable.h"
#include "lloctree.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llvertexbuffer.h"
#include "llgltypes.h"
#include "llcubemap.h"
#include "lldrawpool.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llvector4a.h"
#include <queue>

#define SG_STATE_INHERIT_MASK (OCCLUDED)
#define SG_INITIAL_STATE_MASK (DIRTY | GEOM_DIRTY)

class LLSpatialPartition;
class LLSpatialBridge;
class LLSpatialGroup;
class LLTextureAtlas;
class LLTextureAtlasSlot;

S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &radius_squared);

S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad);
S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &radius_squared);
void pushVerts(LLFace* face, U32 mask);

// get index buffer for binary encoded axis vertex buffer given a box at center being viewed by given camera
U32 get_box_fan_indices(LLCamera* camera, const LLVector4a& center);
U8* get_box_fan_indices_ptr(LLCamera* camera, const LLVector4a& center);

class LLDrawInfo : public LLRefCount 
{
protected:
	~LLDrawInfo();	
	
public:

	LLDrawInfo(const LLDrawInfo& rhs)
	{
		*this = rhs;
	}

	const LLDrawInfo& operator=(const LLDrawInfo& rhs)
	{
		llerrs << "Illegal operation!" << llendl;
		return *this;
	}

	LLDrawInfo(U16 start, U16 end, U32 count, U32 offset, 
				LLViewerTexture* image, LLVertexBuffer* buffer, 
				BOOL fullbright = FALSE, U8 bump = 0, BOOL particle = FALSE, F32 part_size = 0);
	

	void validate();

	LLVector4a mExtents[2];
	
	LLPointer<LLVertexBuffer> mVertexBuffer;
	LLPointer<LLViewerTexture>     mTexture;
	std::vector<LLPointer<LLViewerTexture> > mTextureList;

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
	U32 mDrawMode;

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

	struct CompareMatrixTexturePtr
	{
		bool operator()(const LLPointer<LLDrawInfo>& lhs, const LLPointer<LLDrawInfo>& rhs)	
		{
			return lhs.get() != rhs.get() 
				&& (lhs.isNull() || (rhs.notNull() && (lhs->mModelMatrix > rhs->mModelMatrix ||
													   (lhs->mModelMatrix == rhs->mModelMatrix && lhs->mTexture.get() > rhs->mTexture.get()))));
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

LL_ALIGN_PREFIX(64)
class LLSpatialGroup : public LLOctreeListener<LLDrawable>
{
	friend class LLSpatialPartition;
	friend class LLOctreeStateCheck;
public:

	LLSpatialGroup(const LLSpatialGroup& rhs)
	{
		*this = rhs;
	}

	const LLSpatialGroup& operator=(const LLSpatialGroup& rhs)
	{
		llerrs << "Illegal operation!" << llendl;
		return *this;
	}

	static std::set<GLuint> sPendingQueries; //pending occlusion queries
	static U32 sNodeCount;
	static BOOL sNoDelete; //deletion of spatial groups and draw info not allowed if TRUE

	typedef std::vector<LLPointer<LLSpatialGroup> > sg_vector_t;
	typedef std::vector<LLPointer<LLSpatialBridge> > bridge_list_t;
	typedef std::vector<LLPointer<LLDrawInfo> > drawmap_elem_t; 
	typedef std::map<U32, drawmap_elem_t > draw_map_t;	
	typedef std::vector<LLPointer<LLVertexBuffer> > buffer_list_t;
	typedef std::map<LLFace*, buffer_list_t> buffer_texture_map_t;
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

	struct CompareUpdateUrgency
	{
		bool operator()(const LLPointer<LLSpatialGroup> lhs, const LLPointer<LLSpatialGroup> rhs)
		{
			return lhs->getUpdateUrgency() > rhs->getUpdateUrgency();
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
		OCCLUDED				= 0x00010000,
		QUERY_PENDING			= 0x00020000,
		ACTIVE_OCCLUSION		= 0x00040000,
		DISCARD_QUERY			= 0x00080000,
		EARLY_FAIL				= 0x00100000,
	} eOcclusionState;

	typedef enum
	{
		DEAD					= 0x00000001,
		DIRTY					= 0x00000002,
		OBJECT_DIRTY			= 0x00000004,
		GEOM_DIRTY				= 0x00000008,
		ALPHA_DIRTY				= 0x00000010,
		SKIP_FRUSTUM_CHECK		= 0x00000020,
		IN_IMAGE_QUEUE			= 0x00000040,
		IMAGE_DIRTY				= 0x00000080,
		MESH_DIRTY				= 0x00000100,
		NEW_DRAWINFO			= 0x00000200,
		IN_BUILD_Q1				= 0x00000400,
		IN_BUILD_Q2				= 0x00000800,
		STATE_MASK				= 0x0000FFFF,
	} eSpatialState;

	typedef enum
	{
		STATE_MODE_SINGLE = 0,		//set one node
		STATE_MODE_BRANCH,			//set entire branch
		STATE_MODE_DIFF,			//set entire branch as long as current state is different
		STATE_MODE_ALL_CAMERAS,		//used for occlusion state, set state for all cameras
	} eSetStateMode;

	LLSpatialGroup(OctreeNode* node, LLSpatialPartition* part);

	BOOL isHUDGroup() ;
	BOOL isDead()							{ return isState(DEAD); }
	BOOL isState(U32 state) const;	
	BOOL isOcclusionState(U32 state) const	{ return mOcclusionState[LLViewerCamera::sCurCameraID] & state ? TRUE : FALSE; }
	U32 getState()							{ return mState; }
	void setState(U32 state);	
	void clearState(U32 state);	
	
	void clearDrawMap();
	void validate();
	void checkStates();
	void validateDrawMap();
	
	void setState(U32 state, S32 mode);
	void clearState(U32 state, S32 mode);

	void setOcclusionState(U32 state, S32 mode = STATE_MODE_SINGLE);
	void clearOcclusionState(U32 state, S32 mode = STATE_MODE_SINGLE);

	LLSpatialGroup* getParent();

	
	BOOL addObject(LLDrawable *drawablep, BOOL add_all = FALSE, BOOL from_octree = FALSE);
	BOOL removeObject(LLDrawable *drawablep, BOOL from_octree = FALSE);
	BOOL updateInGroup(LLDrawable *drawablep, BOOL immediate = FALSE); // Update position if it's in the group
	BOOL isVisible() const;
	BOOL isRecentlyVisible() const;
	void setVisible();
	void shift(const LLVector4a &offset);
	BOOL boundObjects(BOOL empty, LLVector4a& newMin, LLVector4a& newMax);
	void unbound();
	BOOL rebound();
	void checkOcclusion(); //read back last occlusion query (if any)
	void doOcclusion(LLCamera* camera); //issue occlusion query
	void destroyGL(bool keep_occlusion = false);
	
	void updateDistance(LLCamera& camera);
	BOOL needsUpdate();
	F32 getUpdateUrgency() const;
	BOOL changeLOD();
	void rebuildGeom();
	void rebuildMesh();

	void dirtyGeom() { setState(GEOM_DIRTY); }
	void dirtyMesh() { setState(MESH_DIRTY); }
	element_list& getData() { return mOctreeNode->getData(); }
	U32 getElementCount() const { return mOctreeNode->getElementCount(); }

	void drawObjectBox(LLColor4 col);

	 //LISTENER FUNCTIONS
	virtual void handleInsertion(const TreeNode* node, LLDrawable* face);
	virtual void handleRemoval(const TreeNode* node, LLDrawable* face);
	virtual void handleDestruction(const TreeNode* node);
	virtual void handleStateChange(const TreeNode* node);
	virtual void handleChildAddition(const OctreeNode* parent, OctreeNode* child);
	virtual void handleChildRemoval(const OctreeNode* parent, const OctreeNode* child);

//-------------------
//for atlas use
//-------------------
	//atlas	
	void setCurUpdatingTime(U32 t) {mCurUpdatingTime = t ;}
	U32  getCurUpdatingTime() const { return mCurUpdatingTime ;}
	
	void setCurUpdatingSlot(LLTextureAtlasSlot* slotp) ;
	LLTextureAtlasSlot* getCurUpdatingSlot(LLViewerTexture* imagep, S8 recursive_level = 3) ;

	void setCurUpdatingTexture(LLViewerTexture* tex){ mCurUpdatingTexture = tex ;}
	LLViewerTexture* getCurUpdatingTexture() const { return mCurUpdatingTexture ;}
	
	BOOL hasAtlas(LLTextureAtlas* atlasp) ;
	LLTextureAtlas* getAtlas(S8 ncomponents, S8 to_be_reserved, S8 recursive_level = 3) ;
	void addAtlas(LLTextureAtlas* atlasp, S8 recursive_level = 3) ;
	void removeAtlas(LLTextureAtlas* atlasp, BOOL remove_group = TRUE, S8 recursive_level = 3) ;
	void clearAtlasList() ;

public:

	typedef enum
	{
		BOUNDS = 0,
		EXTENTS = 2,
		OBJECT_BOUNDS = 4,
		OBJECT_EXTENTS = 6,
		VIEW_ANGLE = 8,
		LAST_VIEW_ANGLE = 9,
		V4_COUNT = 10
	} eV4Index;

	LLVector4a mBounds[2]; // bounding box (center, size) of this node and all its children (tight fit to objects)
	LLVector4a mExtents[2]; // extents (min, max) of this node and all its children
	LLVector4a mObjectExtents[2]; // extents (min, max) of objects in this node
	LLVector4a mObjectBounds[2]; // bounding box (center, size) of objects in this node
	LLVector4a mViewAngle;
	LLVector4a mLastUpdateViewAngle;

	F32 mObjectBoxSize; //cached mObjectBounds[1].getLength3()
		
private:
	U32                     mCurUpdatingTime ;
	//do not make the below two to use LLPointer
	//because mCurUpdatingTime invalidates them automatically.
	LLTextureAtlasSlot* mCurUpdatingSlotp ;
	LLViewerTexture*          mCurUpdatingTexture ;

	std::vector< std::list<LLTextureAtlas*> > mAtlasList ; 
//-------------------
//end for atlas use
//-------------------

protected:
	virtual ~LLSpatialGroup();

	U32 mState;
	U32 mOcclusionState[LLViewerCamera::NUM_CAMERAS];
	U32 mOcclusionIssued[LLViewerCamera::NUM_CAMERAS];

	S32 mLODHash;
	static S32 sLODSeed;

public:
	bridge_list_t mBridgeList;
	buffer_map_t mBufferMap; //used by volume buffers to attempt to reuse vertex buffers

	U32 mGeometryBytes; //used by volumes to track how many bytes of geometry data are in this node
	F32 mSurfaceArea; //used by volumes to track estimated surface area of geometry in this node

	F32 mBuilt;
	OctreeNode* mOctreeNode;
	LLSpatialPartition* mSpatialPartition;
	
	LLPointer<LLVertexBuffer> mVertexBuffer;
	GLuint					mOcclusionQuery[LLViewerCamera::NUM_CAMERAS];

	U32 mBufferUsage;
	draw_map_t mDrawMap;
	
	S32 mVisible[LLViewerCamera::NUM_CAMERAS];
	F32 mDistance;
	F32 mDepth;
	F32 mLastUpdateDistance;
	F32 mLastUpdateTime;
	
	F32 mPixelArea;
	F32 mRadius;
} LL_ALIGN_POSTFIX(64);

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
	LLSpatialPartition(U32 data_mask,  BOOL render_by_group, U32 mBufferUsage);
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
	virtual void shift(const LLVector4a &offset);

	virtual F32 calcDistance(LLSpatialGroup* group, LLCamera& camera);
	virtual F32 calcPixelArea(LLSpatialGroup* group, LLCamera& camera);

	virtual void rebuildGeom(LLSpatialGroup* group);
	virtual void rebuildMesh(LLSpatialGroup* group);

	BOOL visibleObjectsInFrustum(LLCamera& camera);
	S32 cull(LLCamera &camera, std::vector<LLDrawable *>* results = NULL, BOOL for_select = FALSE); // Cull on arbitrary frustum
	
	BOOL isVisible(const LLVector3& v);
	bool isHUDPartition() ;
	
	LLSpatialBridge* asBridge() { return mBridge; }
	BOOL isBridge() { return asBridge() != NULL; }

	void renderPhysicsShapes();
	void renderDebug();
	void renderIntersectingBBoxes(LLCamera* camera);
	void restoreGL();
	void resetVertexBuffers();
	BOOL isOcclusionEnabled();
	BOOL getVisibleExtents(LLCamera& camera, LLVector3& visMin, LLVector3& visMax);

public:
	LLSpatialGroup::OctreeNode* mOctree;
	LLSpatialBridge* mBridge; // NULL for non-LLSpatialBridge instances, otherwise, mBridge == this
							// use a pointer instead of making "isBridge" and "asBridge" virtual so it's safe
							// to call asBridge() from the destructor
	BOOL mOcclusionEnabled; // if TRUE, occlusion culling is performed
	BOOL mInfiniteFarClip; // if TRUE, frustum culling ignores far clip plane
	U32 mBufferUsage;
	const BOOL mRenderByGroup;
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
	
	LLSpatialBridge(LLDrawable* root, BOOL render_by_group, U32 data_mask);
	
	void destroyTree();

	virtual BOOL isSpatialBridge() const		{ return TRUE; }
	virtual void updateSpatialExtents();
	virtual void updateBinRadius();
	virtual void setVisible(LLCamera& camera_in, std::vector<LLDrawable*>* results = NULL, BOOL for_select = FALSE);
	virtual void updateDistance(LLCamera& camera_in, bool force_update);
	virtual void makeActive();
	virtual void move(LLDrawable *drawablep, LLSpatialGroup *curp, BOOL immediate = FALSE);
	virtual BOOL updateMove();
	virtual void shiftPos(const LLVector4a& vec);
	virtual void cleanupReferences();
	virtual LLSpatialPartition* asPartition()		{ return this; }
		
	virtual LLCamera transformCamera(LLCamera& camera);
	
	LLDrawable* mDrawable;
	LLPointer<LLVOAvatar> mAvatar;

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

	bool hasOcclusionGroups() { return mOcclusionGroupsSize > 0; }
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
	sg_list_t::iterator mVisibleGroupsEnd;
	sg_list_t			mAlphaGroups;
	sg_list_t::iterator mAlphaGroupsEnd;
	sg_list_t			mOcclusionGroups;
	sg_list_t::iterator	mOcclusionGroupsEnd;
	sg_list_t			mDrawableGroups;
	sg_list_t::iterator mDrawableGroupsEnd;
	drawable_list_t		mVisibleList;
	drawable_list_t::iterator mVisibleListEnd;
	bridge_list_t		mVisibleBridge;
	bridge_list_t::iterator mVisibleBridgeEnd;
	drawinfo_list_t		mRenderMap[LLRenderPass::NUM_RENDER_TYPES];
	drawinfo_list_t::iterator mRenderMapEnd[LLRenderPass::NUM_RENDER_TYPES];
};


//spatial partition for water (implemented in LLVOWater.cpp)
class LLWaterPartition : public LLSpatialPartition
{
public:
	LLWaterPartition();
	virtual void getGeometry(LLSpatialGroup* group) {  }
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count) { }
};

//spatial partition for hole and edge water (implemented in LLVOWater.cpp)
class LLVoidWaterPartition : public LLWaterPartition
{
public:
	LLVoidWaterPartition();
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
	virtual void rebuildGeom(LLSpatialGroup* group);
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
class LLGrassPartition : public LLSpatialPartition
{
public:
	LLGrassPartition();
	virtual void getGeometry(LLSpatialGroup* group);
	virtual void addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32& index_count);
protected:
	U32 mRenderPass;
};

//class for wrangling geometry out of volumes (implemented in LLVOVolume.cpp)
class LLVolumeGeometryManager: public LLGeometryManager
{
 public:
	typedef enum
	{
		NONE = 0,
		BATCH_SORT,
		DISTANCE_SORT
	} eSortType;

	virtual ~LLVolumeGeometryManager() { }
	virtual void rebuildGeom(LLSpatialGroup* group);
	virtual void rebuildMesh(LLSpatialGroup* group);
	virtual void getGeometry(LLSpatialGroup* group);
	void genDrawInfo(LLSpatialGroup* group, U32 mask, std::vector<LLFace*>& faces, BOOL distance_sort = FALSE, BOOL batch_textures = FALSE);
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
	virtual void shiftPos(const LLVector4a& vec);
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
	virtual void shift(const LLVector4a &offset);
};

extern const F32 SG_BOX_SIDE;
extern const F32 SG_BOX_OFFSET;
extern const F32 SG_BOX_RAD;

extern const F32 SG_OBJ_SIDE;
extern const F32 SG_MAX_OBJ_RAD;


#endif //LL_LLSPATIALPARTITION_H

