/** 
 * @file pipeline.h
 * @brief Rendering pipeline definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_PIPELINE_H
#define LL_PIPELINE_H

#include "llerror.h"
#include "lldarrayptr.h"
#include "lldqueueptr.h"
#include "llstat.h"
#include "lldrawpool.h"
#include "llspatialpartition.h"
#include "m4math.h"
#include "llmemory.h"
#include "lldrawpool.h"
#include "llgl.h"
#include "lldrawable.h"
#include "llrendertarget.h"

class LLViewerImage;
class LLEdge;
class LLFace;
class LLViewerObject;
class LLAgent;
class LLDisplayPrimitive;
class LLTextureEntry;
class LLRenderFunc;
class LLCubeMap;
class LLCullResult;
class LLVOAvatar;

typedef enum e_avatar_skinning_method
{
	SKIN_METHOD_SOFTWARE,
	SKIN_METHOD_VERTEX_PROGRAM
} EAvatarSkinningMethod;

BOOL compute_min_max(LLMatrix4& box, LLVector2& min, LLVector2& max); // Shouldn't be defined here!
bool LLRayAABB(const LLVector3 &center, const LLVector3 &size, const LLVector3& origin, const LLVector3& dir, LLVector3 &coord, F32 epsilon = 0);
BOOL LLLineSegmentAABB(const LLVector3& start, const LLVector3& end, const LLVector3& center, const LLVector3& size);
BOOL setup_hud_matrices(BOOL for_select);
glh::matrix4f glh_get_current_modelview();
void glh_set_current_modelview(glh::matrix4f& mat);
glh::matrix4f glh_get_current_projection();
void glh_set_current_projection(glh::matrix4f& mat);
glh::matrix4f gl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar);
glh::matrix4f gl_perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);

class LLPipeline
{
public:
	LLPipeline();
	~LLPipeline();

	void destroyGL();
	void restoreGL();
	void resetVertexBuffers();
	void resizeScreenTexture();
	void releaseGLBuffers();
	void createGLBuffers();

	void resetVertexBuffers(LLDrawable* drawable);
	void setUseVBO(BOOL use_vbo);
	void generateImpostor(LLVOAvatar* avatar);
	void generateReflectionMap(LLCubeMap* cube_map, LLCamera& camera);
	void blurReflectionMap(LLCubeMap* cube_in, LLCubeMap* cube_out);
	void bindScreenToTexture();
	void renderBloom(BOOL for_snapshot);

	LLCubeMap* findReflectionMap(const LLVector3& location);

	void init();
	void cleanup();
	BOOL isInit() { return mInitialized; };

	/// @brief Get a draw pool from pool type (POOL_SIMPLE, POOL_MEDIA) and texture.
	/// @return Draw pool, or NULL if not found.
	LLDrawPool *findPool(const U32 pool_type, LLViewerImage *tex0 = NULL);

	/// @brief Get a draw pool for faces of the appropriate type and texture.  Create if necessary.
	/// @return Always returns a draw pool.
	LLDrawPool *getPool(const U32 pool_type, LLViewerImage *tex0 = NULL);

	/// @brief Figures out draw pool type from texture entry. Creates pool if necessary.
	static LLDrawPool* getPoolFromTE(const LLTextureEntry* te, LLViewerImage* te_image);
	static U32 getPoolTypeFromTE(const LLTextureEntry* te, LLViewerImage* imagep);

	void		 addPool(LLDrawPool *poolp);	// Only to be used by LLDrawPool classes for splitting pools!
	void		 removePool( LLDrawPool* poolp );

	void		 allocDrawable(LLViewerObject *obj);

	void		 unlinkDrawable(LLDrawable*);

	// Object related methods
	void        markVisible(LLDrawable *drawablep, LLCamera& camera);
	void		markOccluder(LLSpatialGroup* group);
	void		doOcclusion(LLCamera& camera);
	void		markNotCulled(LLSpatialGroup* group, LLCamera &camera);
	void        markMoved(LLDrawable *drawablep, BOOL damped_motion = FALSE);
	void        markShift(LLDrawable *drawablep);
	void        markTextured(LLDrawable *drawablep);
	void        markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag = LLDrawable::REBUILD_ALL, BOOL priority = FALSE);
		
	//get the object between start and end that's closest to start.  Return the point of collision in collision.
	LLViewerObject* pickObject(const LLVector3 &start, const LLVector3 &end, LLVector3 &collision);

	// Something about these textures has changed.  Dirty them.
	void        dirtyPoolObjectTextures(const std::set<LLViewerImage*>& textures);

	void        resetDrawOrders();

	U32         addObject(LLViewerObject *obj);

	void		enableShadows(const BOOL enable_shadows);

// 	void		setLocalLighting(const BOOL local_lighting);
// 	BOOL		isLocalLightingEnabled() const;
	S32			setLightingDetail(S32 level);
	S32			getLightingDetail() const { return mLightingDetail; }
	S32			getMaxLightingDetail() const;
		
	void		setUseVertexShaders(BOOL use_shaders);
	BOOL		getUseVertexShaders() const { return mVertexShadersEnabled; }
	BOOL		canUseVertexShaders();
	BOOL		canUseWindLightShaders() const;
	BOOL		canUseWindLightShadersOnObjects() const;

	// phases
	void resetFrameStats();

	void updateMoveDampedAsync(LLDrawable* drawablep);
	void updateMoveNormalAsync(LLDrawable* drawablep);
	void updateMovedList(LLDrawable::drawable_vector_t& move_list);
	void updateMove();
	void updateCull(LLCamera& camera, LLCullResult& result, S32 water_clip = 0);  //if water_clip is 0, ignore water plane, 1, cull to above plane, -1, cull to below plane
	void updateGeom(F32 max_dtime);

	//calculate pixel area of given box from vantage point of given camera
	static F32 calcPixelArea(LLVector3 center, LLVector3 size, LLCamera& camera);

	void stateSort(LLCamera& camera, LLCullResult& result);
	void stateSort(LLSpatialGroup* group, LLCamera& camera);
	void stateSort(LLSpatialBridge* bridge, LLCamera& camera);
	void stateSort(LLDrawable* drawablep, LLCamera& camera);
	void postSort(LLCamera& camera);
	void forAllVisibleDrawables(void (*func)(LLDrawable*));

	void renderObjects(U32 type, U32 mask, BOOL texture = TRUE);
	void renderGroups(LLRenderPass* pass, U32 type, U32 mask, BOOL texture);

	void grabReferences(LLCullResult& result);

	void renderGeom(LLCamera& camera, BOOL forceVBOUpdate = FALSE);
	void renderGeomDeferred();

	void processImagery(LLCamera& camera);
	void generateWaterReflection(LLCamera& camera);
	void renderHighlights();
	void renderDebug();

	void renderForSelect(std::set<LLViewerObject*>& objects);
	void renderFaceForUVSelect(LLFace* facep);
	void rebuildPools(); // Rebuild pools

	void findReferences(LLDrawable *drawablep);	// Find the lists which have references to this object
	BOOL verify();						// Verify that all data in the pipeline is "correct"

	S32  getLightCount() const { return mLights.size(); }

	void calcNearbyLights(LLCamera& camera);
	void setupHWLights(LLDrawPool* pool);
	void setupAvatarLights(BOOL for_edit = FALSE);
	void enableLights(U32 mask);
	void enableLightsStatic();
	void enableLightsDynamic();
	void enableLightsAvatar();
	void enableLightsAvatarEdit(const LLColor4& color);
	void enableLightsFullbright(const LLColor4& color);
	void disableLights();

	void shiftObjects(const LLVector3 &offset);

	void setLight(LLDrawable *drawablep, BOOL is_light);
	void setActive(LLDrawable *drawablep, BOOL active);

	BOOL hasRenderBatches(const U32 type) const;
	LLCullResult::drawinfo_list_t::iterator beginRenderMap(U32 type);
	LLCullResult::drawinfo_list_t::iterator endRenderMap(U32 type);
	LLCullResult::sg_list_t::iterator beginAlphaGroups();
	LLCullResult::sg_list_t::iterator endAlphaGroups();

	void addTrianglesDrawn(S32 count);
	BOOL hasRenderType(const U32 type) const				{ return (type && (mRenderTypeMask & (1<<type))) ? TRUE : FALSE; }
	BOOL hasRenderDebugFeatureMask(const U32 mask) const	{ return (mRenderDebugFeatureMask & mask) ? TRUE : FALSE; }
	BOOL hasRenderDebugMask(const U32 mask) const			{ return (mRenderDebugMask & mask) ? TRUE : FALSE; }
	void setRenderTypeMask(const U32 mask)					{ mRenderTypeMask = mask; }
	U32  getRenderTypeMask() const							{ return mRenderTypeMask; }
	static void toggleRenderType(U32 type);

	// For UI control of render features
	static BOOL hasRenderTypeControl(void* data);
	static void toggleRenderDebug(void* data);
	static void toggleRenderDebugFeature(void* data);
	static void toggleRenderTypeControl(void* data);
	static BOOL toggleRenderTypeControlNegated(void* data);
	static BOOL toggleRenderDebugControl(void* data);
	static BOOL toggleRenderDebugFeatureControl(void* data);

	static void setRenderParticleBeacons(BOOL val);
	static void toggleRenderParticleBeacons(void* data);
	static BOOL getRenderParticleBeacons(void* data);

	static void setRenderSoundBeacons(BOOL val);
	static void toggleRenderSoundBeacons(void* data);
	static BOOL getRenderSoundBeacons(void* data);

	static void setRenderPhysicalBeacons(BOOL val);
	static void toggleRenderPhysicalBeacons(void* data);
	static BOOL getRenderPhysicalBeacons(void* data);

	static void setRenderScriptedBeacons(BOOL val);
	static void toggleRenderScriptedBeacons(void* data);
	static BOOL getRenderScriptedBeacons(void* data);

	static void setRenderScriptedTouchBeacons(BOOL val);
	static void toggleRenderScriptedTouchBeacons(void* data);
	static BOOL getRenderScriptedTouchBeacons(void* data);

	static void setRenderBeacons(BOOL val);
	static void toggleRenderBeacons(void* data);
	static BOOL getRenderBeacons(void* data);

	static void setRenderHighlights(BOOL val);
	static void toggleRenderHighlights(void* data);
	static BOOL getRenderHighlights(void* data);

	static BOOL getProcessBeacons(void* data);

private:
	void unloadShaders();
	void addToQuickLookup( LLDrawPool* new_poolp );
	void removeFromQuickLookup( LLDrawPool* poolp );
	BOOL updateDrawableGeom(LLDrawable* drawable, BOOL priority);
	void assertInitializedDoError();
	bool assertInitialized() { const bool is_init = isInit(); if (!is_init) assertInitializedDoError(); return is_init; };
	
public:
	enum {GPU_CLASS_MAX = 3 };

	enum LLRenderTypeMask
	{
		// Following are pool types (some are also object types)
		RENDER_TYPE_SKY			= LLDrawPool::POOL_SKY,
		RENDER_TYPE_WL_SKY		= LLDrawPool::POOL_WL_SKY,
		RENDER_TYPE_GROUND		= LLDrawPool::POOL_GROUND,	
		RENDER_TYPE_TERRAIN		= LLDrawPool::POOL_TERRAIN,
		RENDER_TYPE_SIMPLE		= LLDrawPool::POOL_SIMPLE,
		RENDER_TYPE_BUMP		= LLDrawPool::POOL_BUMP,
		RENDER_TYPE_AVATAR		= LLDrawPool::POOL_AVATAR,
		RENDER_TYPE_TREE		= LLDrawPool::POOL_TREE,
		RENDER_TYPE_INVISIBLE	= LLDrawPool::POOL_INVISIBLE,
		RENDER_TYPE_WATER		= LLDrawPool::POOL_WATER,
 		RENDER_TYPE_ALPHA		= LLDrawPool::POOL_ALPHA,
		RENDER_TYPE_GLOW		= LLDrawPool::POOL_GLOW,
		
		// Following are object types (only used in drawable mRenderType)
		RENDER_TYPE_HUD = LLDrawPool::NUM_POOL_TYPES,
		RENDER_TYPE_VOLUME,
		RENDER_TYPE_GRASS,
		RENDER_TYPE_PARTICLES,
		RENDER_TYPE_CLOUDS,
	};

	enum LLRenderDebugFeatureMask
	{
		RENDER_DEBUG_FEATURE_UI					= 0x0001,
		RENDER_DEBUG_FEATURE_SELECTED			= 0x0002,
		RENDER_DEBUG_FEATURE_HIGHLIGHTED		= 0x0004,
		RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES	= 0x0008,
// 		RENDER_DEBUG_FEATURE_HW_LIGHTING		= 0x0010,
		RENDER_DEBUG_FEATURE_FLEXIBLE			= 0x0010,
		RENDER_DEBUG_FEATURE_FOG				= 0x0020,
		RENDER_DEBUG_FEATURE_PALETTE			= 0x0040,
		RENDER_DEBUG_FEATURE_FR_INFO			= 0x0080,
		RENDER_DEBUG_FEATURE_FOOT_SHADOWS		= 0x0100,
	};

	enum LLRenderDebugMask
	{
		RENDER_DEBUG_COMPOSITION		= 0x000020,
		RENDER_DEBUG_VERIFY				= 0x000080,
		RENDER_DEBUG_BBOXES				= 0x000200,
		RENDER_DEBUG_OCTREE				= 0x000400,
		RENDER_DEBUG_PICKING			= 0x000800,
		RENDER_DEBUG_OCCLUSION			= 0x001000,
		RENDER_DEBUG_POINTS				= 0x002000,
		RENDER_DEBUG_TEXTURE_PRIORITY	= 0x004000,
		RENDER_DEBUG_TEXTURE_AREA		= 0x008000,
		RENDER_DEBUG_FACE_AREA			= 0x010000,
		RENDER_DEBUG_PARTICLES			= 0x020000,
		RENDER_DEBUG_GLOW				= 0x040000,
		RENDER_DEBUG_TEXTURE_ANIM		= 0x080000,
		RENDER_DEBUG_LIGHTS				= 0x100000,
		RENDER_DEBUG_BATCH_SIZE			= 0x200000,
		RENDER_DEBUG_SHAME				= 0x400000,
	};

public:
	
	LLSpatialPartition* getSpatialPartition(LLViewerObject* vobj);

	void updateCamera(BOOL reset = FALSE);
	
	LLVector3				mFlyCamPosition;
	LLQuaternion			mFlyCamRotation;

	BOOL					 mBackfaceCull;
	S32						 mBatchCount;
	S32						 mMatrixOpCount;
	S32						 mTextureMatrixOps;
	S32						 mMaxBatchSize;
	S32						 mMinBatchSize;
	S32						 mMeanBatchSize;
	S32						 mTrianglesDrawn;
	S32						 mNumVisibleNodes;
	LLStat                   mTrianglesDrawnStat;
	S32						 mVerticesRelit;

	S32						 mLightingChanges;
	S32						 mGeometryChanges;

	S32						 mNumVisibleFaces;

	static S32				sCompiles;

	static BOOL				sShowHUDAttachments;
	static S32				sUseOcclusion;  // 0 = no occlusion, 1 = read only, 2 = read/write
	static BOOL				sFastAlpha;
	static BOOL				sDisableShaders; // if TRUE, rendering will be done without shaders
	static BOOL				sRenderBump;
	static BOOL				sUseFBO;
	static BOOL				sUseFarClip;
	static BOOL				sSkipUpdate; //skip lod updates
	static BOOL				sDynamicReflections;
	static BOOL				sWaterReflections;
	static BOOL				sDynamicLOD;
	static BOOL				sReflectionRender;
	static BOOL				sImpostorRender;
	static BOOL				sUnderWaterRender;
	static BOOL				sRenderGlow;
	static BOOL				sTextureBindTest;
	static BOOL				sRenderFrameTest;
	
	//screen texture
	LLRenderTarget			mScreen;
	
	LLVector2				mScreenScale;

	//water reflection texture
	LLRenderTarget				mWaterRef;

	//water distortion texture (refraction)
	LLRenderTarget				mWaterDis;

	//texture for making the glow
	LLRenderTarget				mGlow[3];
	
	//framebuffer objects for off-screen scratch space
	//GLuint					mFramebuffer[4];
	//GLuint					mDepthbuffer[2];

	//dynamic cube map scratch space
	LLPointer<LLCubeMap>	mCubeBuffer;

	//cube map anti-aliasing buffers
	GLuint					mBlurCubeBuffer[3];
	GLuint					mBlurCubeTexture[3];

	//frambuffer object for rendering dynamic cube maps
	GLuint					mCubeFrameBuffer;
	
	//depth buffer object for rendering dynamic cube maps
	GLuint					mCubeDepth;

	
	LLColor4				mSunDiffuse;
	LLVector3				mSunDir;

	BOOL					mInitialized;
	BOOL					mVertexShadersEnabled;
	S32						mVertexShadersLoaded; // 0 = no, 1 = yes, -1 = failed

protected:
	U32						mRenderTypeMask;
	U32						mRenderDebugFeatureMask;
	U32						mRenderDebugMask;

	U32						mOldRenderDebugMask;
	
	/////////////////////////////////////////////
	//
	//
	LLDrawable::drawable_vector_t	mMovedList;
	LLDrawable::drawable_vector_t mMovedBridge;
	LLDrawable::drawable_vector_t	mShiftList;

	/////////////////////////////////////////////
	//
	//
	struct Light
	{
		Light(LLDrawable* ptr, F32 d, F32 f = 0.0f)
			: drawable(ptr),
			  dist(d),
			  fade(f)
		{}
		LLPointer<LLDrawable> drawable;
		F32 dist;
		F32 fade;
		struct compare
		{
			bool operator()(const Light& a, const Light& b) const
			{
				if ( a.dist < b.dist )
					return true;
				else if ( a.dist > b.dist )
					return false;
				else
					return a.drawable < b.drawable;
			}
		};
	};
	typedef std::set< Light, Light::compare > light_set_t;
	
	LLDrawable::drawable_set_t		mLights;
	light_set_t						mNearbyLights; // lights near camera
	LLColor4						mHWLightColors[8];
	
	/////////////////////////////////////////////
	//
	// Different queues of drawables being processed.
	//
	LLDrawable::drawable_list_t 	mBuildQ1; // priority
	LLDrawable::drawable_list_t 	mBuildQ2; // non-priority
		
	LLDrawable::drawable_set_t		mActiveQ;
	
	LLDrawable::drawable_set_t		mRetexturedList;

	//////////////////////////////////////////////////
	//
	// Draw pools are responsible for storing all rendered data,
	// and performing the actual rendering of objects.
	//
	struct compare_pools
	{
		bool operator()(const LLDrawPool* a, const LLDrawPool* b) const
		{
			if (!a)
				return true;
			else if (!b)
				return false;
			else
			{
				S32 atype = a->getType();
				S32 btype = b->getType();
				if (atype < btype)
					return true;
				else if (atype > btype)
					return false;
				else
					return a->getId() < b->getId();
			}
		}
	};
 	typedef std::set<LLDrawPool*, compare_pools > pool_set_t;
	pool_set_t mPools;
	LLDrawPool*	mLastRebuildPool;
	
	// For quick-lookups into mPools (mapped by texture pointer)
	std::map<uintptr_t, LLDrawPool*>	mTerrainPools;
	std::map<uintptr_t, LLDrawPool*>	mTreePools;
	LLDrawPool*					mAlphaPool;
	LLDrawPool*					mSkyPool;
	LLDrawPool*					mTerrainPool;
	LLDrawPool*					mWaterPool;
	LLDrawPool*					mGroundPool;
	LLRenderPass*				mSimplePool;
	LLDrawPool*					mInvisiblePool;
	LLDrawPool*					mGlowPool;
	LLDrawPool*					mBumpPool;
	LLDrawPool*					mWLSkyPool;
	// Note: no need to keep an quick-lookup to avatar pools, since there's only one per avatar
	
public:
	std::vector<LLFace*>		mHighlightFaces;	// highlight faces on physical objects
protected:
	std::vector<LLFace*>		mSelectedFaces;

	LLPointer<LLViewerImage>	mFaceSelectImagep;
	LLPointer<LLViewerImage>	mBloomImagep;
	LLPointer<LLViewerImage>	mBloomImage2p;
	
	U32						mLightMask;
	U32						mLightMovingMask;
	S32						mLightingDetail;
		
	static BOOL				sRenderPhysicalBeacons;
	static BOOL				sRenderScriptedTouchBeacons;
	static BOOL				sRenderScriptedBeacons;
	static BOOL				sRenderParticleBeacons;
	static BOOL				sRenderSoundBeacons;
public:
	static BOOL				sRenderBeacons;
	static BOOL				sRenderHighlight;
	static BOOL				sRenderProcessBeacons;
};

void render_bbox(const LLVector3 &min, const LLVector3 &max);

extern LLPipeline gPipeline;
extern BOOL gRenderForSelect;
extern const LLMatrix4* gGLLastMatrix;

#endif
