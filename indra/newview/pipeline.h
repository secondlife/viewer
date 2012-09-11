/** 
 * @file pipeline.h
 * @brief Rendering pipeline definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_PIPELINE_H
#define LL_PIPELINE_H

#include "llcamera.h"
#include "llerror.h"
#include "lldarrayptr.h"
#include "lldqueueptr.h"
#include "lldrawpool.h"
#include "llspatialpartition.h"
#include "m4math.h"
#include "llpointer.h"
#include "lldrawpool.h"
#include "llgl.h"
#include "lldrawable.h"
#include "llrendertarget.h"

#include <stack>

#include <stack>

#include <stack>

class LLViewerTexture;
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
class LLGLSLShader;
class LLCurlRequest;

class LLMeshResponder;

typedef enum e_avatar_skinning_method
{
	SKIN_METHOD_SOFTWARE,
	SKIN_METHOD_VERTEX_PROGRAM
} EAvatarSkinningMethod;

BOOL compute_min_max(LLMatrix4& box, LLVector2& min, LLVector2& max); // Shouldn't be defined here!
bool LLRayAABB(const LLVector3 &center, const LLVector3 &size, const LLVector3& origin, const LLVector3& dir, LLVector3 &coord, F32 epsilon = 0);
BOOL setup_hud_matrices(); // use whole screen to render hud
BOOL setup_hud_matrices(const LLRect& screen_region); // specify portion of screen (in pixels) to render hud attachments from (for picking)
glh::matrix4f glh_copy_matrix(F32* src);
glh::matrix4f glh_get_current_modelview();
void glh_set_current_modelview(const glh::matrix4f& mat);
glh::matrix4f glh_get_current_projection();
void glh_set_current_projection(glh::matrix4f& mat);
glh::matrix4f gl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar);
glh::matrix4f gl_perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
glh::matrix4f gl_lookat(LLVector3 eye, LLVector3 center, LLVector3 up);

extern LLFastTimer::DeclareTimer FTM_RENDER_GEOMETRY;
extern LLFastTimer::DeclareTimer FTM_RENDER_GRASS;
extern LLFastTimer::DeclareTimer FTM_RENDER_INVISIBLE;
extern LLFastTimer::DeclareTimer FTM_RENDER_OCCLUSION;
extern LLFastTimer::DeclareTimer FTM_RENDER_SHINY;
extern LLFastTimer::DeclareTimer FTM_RENDER_SIMPLE;
extern LLFastTimer::DeclareTimer FTM_RENDER_TERRAIN;
extern LLFastTimer::DeclareTimer FTM_RENDER_TREES;
extern LLFastTimer::DeclareTimer FTM_RENDER_UI;
extern LLFastTimer::DeclareTimer FTM_RENDER_WATER;
extern LLFastTimer::DeclareTimer FTM_RENDER_WL_SKY;
extern LLFastTimer::DeclareTimer FTM_RENDER_ALPHA;
extern LLFastTimer::DeclareTimer FTM_RENDER_CHARACTERS;
extern LLFastTimer::DeclareTimer FTM_RENDER_BUMP;
extern LLFastTimer::DeclareTimer FTM_RENDER_FULLBRIGHT;
extern LLFastTimer::DeclareTimer FTM_RENDER_GLOW;
extern LLFastTimer::DeclareTimer FTM_STATESORT;
extern LLFastTimer::DeclareTimer FTM_PIPELINE;
extern LLFastTimer::DeclareTimer FTM_CLIENT_COPY;


class LLPipeline
{
public:
	LLPipeline();
	~LLPipeline();

	void destroyGL();
	void restoreGL();
	void resetVertexBuffers();
	void doResetVertexBuffers();
	void resizeScreenTexture();
	void releaseGLBuffers();
	void releaseLUTBuffers();
	void releaseScreenBuffers();
	void createGLBuffers();
	void createLUTBuffers();

	void allocateScreenBuffer(U32 resX, U32 resY);
	bool allocateScreenBuffer(U32 resX, U32 resY, U32 samples);
	void allocatePhysicsBuffer();
	
	void resetVertexBuffers(LLDrawable* drawable);
	void generateImpostor(LLVOAvatar* avatar);
	void bindScreenToTexture();
	void renderBloom(BOOL for_snapshot, F32 zoom_factor = 1.f, int subfield = 0);

	void init();
	void cleanup();
	BOOL isInit() { return mInitialized; };

	/// @brief Get a draw pool from pool type (POOL_SIMPLE, POOL_MEDIA) and texture.
	/// @return Draw pool, or NULL if not found.
	LLDrawPool *findPool(const U32 pool_type, LLViewerTexture *tex0 = NULL);

	/// @brief Get a draw pool for faces of the appropriate type and texture.  Create if necessary.
	/// @return Always returns a draw pool.
	LLDrawPool *getPool(const U32 pool_type, LLViewerTexture *tex0 = NULL);

	/// @brief Figures out draw pool type from texture entry. Creates pool if necessary.
	static LLDrawPool* getPoolFromTE(const LLTextureEntry* te, LLViewerTexture* te_image);
	static U32 getPoolTypeFromTE(const LLTextureEntry* te, LLViewerTexture* imagep);

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
	void		markGLRebuild(LLGLUpdate* glu);
	void		markRebuild(LLSpatialGroup* group, BOOL priority = FALSE);
	void        markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag = LLDrawable::REBUILD_ALL, BOOL priority = FALSE);
	void		markPartitionMove(LLDrawable* drawablep);
	void		markMeshDirty(LLSpatialGroup* group);

	//get the object between start and end that's closest to start.
	LLViewerObject* lineSegmentIntersectInWorld(const LLVector3& start, const LLVector3& end,
												BOOL pick_transparent,
												S32* face_hit,                          // return the face hit
												LLVector3* intersection = NULL,         // return the intersection point
												LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
												LLVector3* normal = NULL,               // return the surface normal at the intersection point
												LLVector3* bi_normal = NULL             // return the surface bi-normal at the intersection point  
		);
	LLViewerObject* lineSegmentIntersectInHUD(const LLVector3& start, const LLVector3& end,
											  BOOL pick_transparent,
											  S32* face_hit,                          // return the face hit
											  LLVector3* intersection = NULL,         // return the intersection point
											  LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
											  LLVector3* normal = NULL,               // return the surface normal at the intersection point
											  LLVector3* bi_normal = NULL             // return the surface bi-normal at the intersection point
		);

	// Something about these textures has changed.  Dirty them.
	void        dirtyPoolObjectTextures(const std::set<LLViewerFetchedTexture*>& textures);

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
	BOOL		canUseAntiAliasing() const;

	// phases
	void resetFrameStats();

	void updateMoveDampedAsync(LLDrawable* drawablep);
	void updateMoveNormalAsync(LLDrawable* drawablep);
	void updateMovedList(LLDrawable::drawable_vector_t& move_list);
	void updateMove();
	BOOL visibleObjectsInFrustum(LLCamera& camera);
	BOOL getVisibleExtents(LLCamera& camera, LLVector3 &min, LLVector3& max);
	BOOL getVisiblePointCloud(LLCamera& camera, LLVector3 &min, LLVector3& max, std::vector<LLVector3>& fp, LLVector3 light_dir = LLVector3(0,0,0));
	void updateCull(LLCamera& camera, LLCullResult& result, S32 water_clip = 0, LLPlane* plane = NULL);  //if water_clip is 0, ignore water plane, 1, cull to above plane, -1, cull to below plane
	void createObjects(F32 max_dtime);
	void createObject(LLViewerObject* vobj);
	void processPartitionQ();
	void updateGeom(F32 max_dtime);
	void updateGL();
	void rebuildPriorityGroups();
	void rebuildGroups();

	//calculate pixel area of given box from vantage point of given camera
	static F32 calcPixelArea(LLVector3 center, LLVector3 size, LLCamera& camera);
	static F32 calcPixelArea(const LLVector4a& center, const LLVector4a& size, LLCamera &camera);

	void stateSort(LLCamera& camera, LLCullResult& result);
	void stateSort(LLSpatialGroup* group, LLCamera& camera);
	void stateSort(LLSpatialBridge* bridge, LLCamera& camera);
	void stateSort(LLDrawable* drawablep, LLCamera& camera);
	void postSort(LLCamera& camera);
	void forAllVisibleDrawables(void (*func)(LLDrawable*));

	void renderObjects(U32 type, U32 mask, BOOL texture = TRUE, BOOL batch_texture = FALSE);
	void renderGroups(LLRenderPass* pass, U32 type, U32 mask, BOOL texture);

	void grabReferences(LLCullResult& result);
	void clearReferences();

	//check references will assert that there are no references in sCullResult to the provided data
	void checkReferences(LLFace* face);
	void checkReferences(LLDrawable* drawable);
	void checkReferences(LLDrawInfo* draw_info);
	void checkReferences(LLSpatialGroup* group);


	void renderGeom(LLCamera& camera, BOOL forceVBOUpdate = FALSE);
	void renderGeomDeferred(LLCamera& camera);
	void renderGeomPostDeferred(LLCamera& camera);
	void renderGeomShadow(LLCamera& camera);
	void bindDeferredShader(LLGLSLShader& shader, U32 light_index = 0, U32 noise_map = 0xFFFFFFFF);
	void setupSpotLight(LLGLSLShader& shader, LLDrawable* drawablep);

	void unbindDeferredShader(LLGLSLShader& shader);
	void renderDeferredLighting();
	
	void generateWaterReflection(LLCamera& camera);
	void generateSunShadow(LLCamera& camera);
	void generateHighlight(LLCamera& camera);
	void renderHighlight(const LLViewerObject* obj, F32 fade);
	void setHighlightObject(LLDrawable* obj) { mHighlightObject = obj; }


	void renderShadow(glh::matrix4f& view, glh::matrix4f& proj, LLCamera& camera, LLCullResult& result, BOOL use_shader, BOOL use_occlusion, U32 target_width);
	void renderHighlights();
	void renderDebug();
	void renderPhysicsDisplay();

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
	void enableLightsPreview();
	void enableLightsAvatarEdit(const LLColor4& color);
	void enableLightsFullbright(const LLColor4& color);
	void disableLights();

	void shiftObjects(const LLVector3 &offset);

	void setLight(LLDrawable *drawablep, BOOL is_light);
	
	BOOL hasRenderBatches(const U32 type) const;
	LLCullResult::drawinfo_list_t::iterator beginRenderMap(U32 type);
	LLCullResult::drawinfo_list_t::iterator endRenderMap(U32 type);
	LLCullResult::sg_list_t::iterator beginAlphaGroups();
	LLCullResult::sg_list_t::iterator endAlphaGroups();
	

	void addTrianglesDrawn(S32 index_count, U32 render_type = LLRender::TRIANGLES);

	BOOL hasRenderDebugFeatureMask(const U32 mask) const	{ return (mRenderDebugFeatureMask & mask) ? TRUE : FALSE; }
	BOOL hasRenderDebugMask(const U32 mask) const			{ return (mRenderDebugMask & mask) ? TRUE : FALSE; }
	


	BOOL hasRenderType(const U32 type) const;
	BOOL hasAnyRenderType(const U32 type, ...) const;

	void setRenderTypeMask(U32 type, ...);
	void orRenderTypeMask(U32 type, ...);
	void andRenderTypeMask(U32 type, ...);
	void clearRenderTypeMask(U32 type, ...);
	
	void pushRenderTypeMask();
	void popRenderTypeMask();

	static void toggleRenderType(U32 type);

	// For UI control of render features
	static BOOL hasRenderTypeControl(void* data);
	static void toggleRenderDebug(void* data);
	static void toggleRenderDebugFeature(void* data);
	static void toggleRenderTypeControl(void* data);
	static BOOL toggleRenderTypeControlNegated(void* data);
	static BOOL toggleRenderDebugControl(void* data);
	static BOOL toggleRenderDebugFeatureControl(void* data);
	static void setRenderDebugFeatureControl(U32 bit, bool value);

	static void setRenderParticleBeacons(BOOL val);
	static void toggleRenderParticleBeacons(void* data);
	static BOOL getRenderParticleBeacons(void* data);

	static void setRenderSoundBeacons(BOOL val);
	static void toggleRenderSoundBeacons(void* data);
	static BOOL getRenderSoundBeacons(void* data);

	static void setRenderMOAPBeacons(BOOL val);
	static void toggleRenderMOAPBeacons(void * data);
	static BOOL getRenderMOAPBeacons(void * data);

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

	static void updateRenderDeferred();
	static void refreshCachedSettings();

	static void throttleNewMemoryAllocation(BOOL disable);

	void addDebugBlip(const LLVector3& position, const LLColor4& color);

	void hidePermanentObjects( std::vector<U32>& restoreList );
	void restorePermanentObjects( const std::vector<U32>& restoreList );
	void skipRenderingOfTerrain( BOOL flag );
	void hideObject( const LLUUID& id );
	void restoreHiddenObject( const LLUUID& id );

private:
	void unloadShaders();
	void addToQuickLookup( LLDrawPool* new_poolp );
	void removeFromQuickLookup( LLDrawPool* poolp );
	BOOL updateDrawableGeom(LLDrawable* drawable, BOOL priority);
	void assertInitializedDoError();
	bool assertInitialized() { const bool is_init = isInit(); if (!is_init) assertInitializedDoError(); return is_init; };
	void hideDrawable( LLDrawable *pDrawable );
	void unhideDrawable( LLDrawable *pDrawable );
public:
	enum {GPU_CLASS_MAX = 3 };

	enum LLRenderTypeMask
	{
		// Following are pool types (some are also object types)
		RENDER_TYPE_SKY							= LLDrawPool::POOL_SKY,
		RENDER_TYPE_WL_SKY						= LLDrawPool::POOL_WL_SKY,
		RENDER_TYPE_GROUND						= LLDrawPool::POOL_GROUND,	
		RENDER_TYPE_TERRAIN						= LLDrawPool::POOL_TERRAIN,
		RENDER_TYPE_SIMPLE						= LLDrawPool::POOL_SIMPLE,
		RENDER_TYPE_GRASS						= LLDrawPool::POOL_GRASS,
		RENDER_TYPE_FULLBRIGHT					= LLDrawPool::POOL_FULLBRIGHT,
		RENDER_TYPE_BUMP						= LLDrawPool::POOL_BUMP,
		RENDER_TYPE_AVATAR						= LLDrawPool::POOL_AVATAR,
		RENDER_TYPE_TREE						= LLDrawPool::POOL_TREE,
		RENDER_TYPE_INVISIBLE					= LLDrawPool::POOL_INVISIBLE,
		RENDER_TYPE_VOIDWATER					= LLDrawPool::POOL_VOIDWATER,
		RENDER_TYPE_WATER						= LLDrawPool::POOL_WATER,
 		RENDER_TYPE_ALPHA						= LLDrawPool::POOL_ALPHA,
		RENDER_TYPE_GLOW						= LLDrawPool::POOL_GLOW,
		RENDER_TYPE_PASS_SIMPLE 				= LLRenderPass::PASS_SIMPLE,
		RENDER_TYPE_PASS_GRASS					= LLRenderPass::PASS_GRASS,
		RENDER_TYPE_PASS_FULLBRIGHT				= LLRenderPass::PASS_FULLBRIGHT,
		RENDER_TYPE_PASS_INVISIBLE				= LLRenderPass::PASS_INVISIBLE,
		RENDER_TYPE_PASS_INVISI_SHINY			= LLRenderPass::PASS_INVISI_SHINY,
		RENDER_TYPE_PASS_FULLBRIGHT_SHINY		= LLRenderPass::PASS_FULLBRIGHT_SHINY,
		RENDER_TYPE_PASS_SHINY					= LLRenderPass::PASS_SHINY,
		RENDER_TYPE_PASS_BUMP					= LLRenderPass::PASS_BUMP,
		RENDER_TYPE_PASS_POST_BUMP				= LLRenderPass::PASS_POST_BUMP,
		RENDER_TYPE_PASS_GLOW					= LLRenderPass::PASS_GLOW,
		RENDER_TYPE_PASS_ALPHA					= LLRenderPass::PASS_ALPHA,
		RENDER_TYPE_PASS_ALPHA_MASK				= LLRenderPass::PASS_ALPHA_MASK,
		RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK	= LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK,
		// Following are object types (only used in drawable mRenderType)
		RENDER_TYPE_HUD = LLRenderPass::NUM_RENDER_TYPES,
		RENDER_TYPE_VOLUME,
		RENDER_TYPE_PARTICLES,
		RENDER_TYPE_CLOUDS,
		RENDER_TYPE_HUD_PARTICLES,
		NUM_RENDER_TYPES,
		END_RENDER_TYPES = NUM_RENDER_TYPES
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
		RENDER_DEBUG_FEATURE_FR_INFO			= 0x0080,
		RENDER_DEBUG_FEATURE_FOOT_SHADOWS		= 0x0100,
	};

	enum LLRenderDebugMask
	{
		RENDER_DEBUG_COMPOSITION		= 0x00000001,
		RENDER_DEBUG_VERIFY				= 0x00000002,
		RENDER_DEBUG_BBOXES				= 0x00000004,
		RENDER_DEBUG_OCTREE				= 0x00000008,
		RENDER_DEBUG_WIND_VECTORS		= 0x00000010,
		RENDER_DEBUG_OCCLUSION			= 0x00000020,
		RENDER_DEBUG_POINTS				= 0x00000040,
		RENDER_DEBUG_TEXTURE_PRIORITY	= 0x00000080,
		RENDER_DEBUG_TEXTURE_AREA		= 0x00000100,
		RENDER_DEBUG_FACE_AREA			= 0x00000200,
		RENDER_DEBUG_PARTICLES			= 0x00000400,
		RENDER_DEBUG_GLOW				= 0x00000800,
		RENDER_DEBUG_TEXTURE_ANIM		= 0x00001000,
		RENDER_DEBUG_LIGHTS				= 0x00002000,
		RENDER_DEBUG_BATCH_SIZE			= 0x00004000,
		RENDER_DEBUG_ALPHA_BINS			= 0x00008000,
		RENDER_DEBUG_RAYCAST            = 0x00010000,
		RENDER_DEBUG_SHAME				= 0x00020000,
		RENDER_DEBUG_SHADOW_FRUSTA		= 0x00040000,
		RENDER_DEBUG_SCULPTED           = 0x00080000,
		RENDER_DEBUG_AVATAR_VOLUME      = 0x00100000,
		RENDER_DEBUG_BUILD_QUEUE		= 0x00200000,
		RENDER_DEBUG_AGENT_TARGET       = 0x00400000,
		RENDER_DEBUG_UPDATE_TYPE		= 0x00800000,
		RENDER_DEBUG_PHYSICS_SHAPES     = 0x01000000,
		RENDER_DEBUG_NORMALS	        = 0x02000000,
		RENDER_DEBUG_LOD_INFO	        = 0x04000000,
		RENDER_DEBUG_RENDER_COMPLEXITY  = 0x08000000,
		RENDER_DEBUG_ATTACHMENT_BYTES	= 0x10000000,
		RENDER_DEBUG_TEXEL_DENSITY		= 0x20000000
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
	S32						 mVerticesRelit;

	S32						 mDebugTextureUploadCost;
	S32						 mDebugSculptUploadCost;
	S32						 mDebugMeshUploadCost;

	S32						 mLightingChanges;
	S32						 mGeometryChanges;

	S32						 mNumVisibleFaces;

	static S32				sCompiles;

	static BOOL				sShowHUDAttachments;
	static BOOL				sForceOldBakedUpload; // If true will not use capabilities to upload baked textures.
	static S32				sUseOcclusion;  // 0 = no occlusion, 1 = read only, 2 = read/write
	static BOOL				sDelayVBUpdate;
	static BOOL				sAutoMaskAlphaDeferred;
	static BOOL				sAutoMaskAlphaNonDeferred;
	static BOOL				sDisableShaders; // if TRUE, rendering will be done without shaders
	static BOOL				sRenderBump;
	static BOOL				sBakeSunlight;
	static BOOL				sNoAlpha;
	static BOOL				sUseTriStrips;
	static BOOL				sUseFarClip;
	static BOOL				sShadowRender;
	static BOOL				sWaterReflections;
	static BOOL				sDynamicLOD;
	static BOOL				sPickAvatar;
	static BOOL				sReflectionRender;
	static BOOL				sImpostorRender;
	static BOOL				sUnderWaterRender;
	static BOOL				sRenderGlow;
	static BOOL				sTextureBindTest;
	static BOOL				sRenderFrameTest;
	static BOOL				sRenderAttachedLights;
	static BOOL				sRenderAttachedParticles;
	static BOOL				sRenderDeferred;
	static BOOL             sMemAllocationThrottled;
	static S32				sVisibleLightCount;
	static F32				sMinRenderSize;	

	//screen texture
	U32 					mScreenWidth;
	U32 					mScreenHeight;
	
	LLRenderTarget			mScreen;
	LLRenderTarget			mUIScreen;
	LLRenderTarget			mDeferredScreen;
	LLRenderTarget			mFXAABuffer;
	LLRenderTarget			mEdgeMap;
	LLRenderTarget			mDeferredDepth;
	LLRenderTarget			mDeferredLight;
	LLRenderTarget			mHighlight;
	LLRenderTarget			mPhysicsDisplay;

	//utility buffer for rendering post effects, gets abused by renderDeferredLighting
	LLPointer<LLVertexBuffer> mDeferredVB;

	//utility buffer for rendering cubes, 8 vertices are corners of a cube [-1, 1]
	LLPointer<LLVertexBuffer> mCubeVB;

	//sun shadow map
	LLRenderTarget			mShadow[6];
	std::vector<LLVector3>	mShadowFrustPoints[4];
	LLVector4				mShadowError;
	LLVector4				mShadowFOV;
	LLVector3				mShadowFrustOrigin[4];
	LLCamera				mShadowCamera[8];
	LLVector3				mShadowExtents[4][2];
	glh::matrix4f			mSunShadowMatrix[6];
	glh::matrix4f			mShadowModelview[6];
	glh::matrix4f			mShadowProjection[6];
	glh::matrix4f			mGIMatrix;
	glh::matrix4f			mGIMatrixProj;
	glh::matrix4f			mGIModelview;
	glh::matrix4f			mGIProjection;
	glh::matrix4f			mGINormalMatrix;
	glh::matrix4f			mGIInvProj;
	LLVector2				mGIRange;
	F32						mGILightRadius;
	
	LLPointer<LLDrawable>				mShadowSpotLight[2];
	F32									mSpotLightFade[2];
	LLPointer<LLDrawable>				mTargetShadowSpotLight[2];

	LLVector4				mSunClipPlanes;
	LLVector4				mSunOrthoClipPlanes;

	LLVector2				mScreenScale;

	//water reflection texture
	LLRenderTarget				mWaterRef;

	//water distortion texture (refraction)
	LLRenderTarget				mWaterDis;

	//texture for making the glow
	LLRenderTarget				mGlow[3];

	//noise map
	U32					mNoiseMap;
	U32					mTrueNoiseMap;
	U32					mLightFunc;

	LLColor4				mSunDiffuse;
	LLVector3				mSunDir;
	LLVector3				mTransformedSunDir;

	BOOL					mInitialized;
	BOOL					mVertexShadersEnabled;
	S32						mVertexShadersLoaded; // 0 = no, 1 = yes, -1 = failed

	U32						mTransformFeedbackPrimitives; //number of primitives expected to be generated by transform feedback
protected:
	BOOL					mRenderTypeEnabled[NUM_RENDER_TYPES];
	std::stack<std::string> mRenderTypeEnableStack;

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
	LLSpatialGroup::sg_vector_t		mGroupQ1; //priority
	LLSpatialGroup::sg_vector_t		mGroupQ2; // non-priority

	LLSpatialGroup::sg_vector_t		mMeshDirtyGroup; //groups that need rebuildMesh called
	U32 mMeshDirtyQueryObject;

	LLDrawable::drawable_list_t		mPartitionQ; //drawables that need to update their spatial partition radius 

	bool mGroupQ2Locked;
	bool mGroupQ1Locked;

	bool mResetVertexBuffers; //if true, clear vertex buffers on next update

	LLViewerObject::vobj_list_t		mCreateQ;
		
	LLDrawable::drawable_set_t		mRetexturedList;

	class HighlightItem
	{
	public:
		const LLPointer<LLDrawable> mItem;
		mutable F32 mFade;

		HighlightItem(LLDrawable* item)
		: mItem(item), mFade(0)
		{
		}

		bool operator<(const HighlightItem& rhs) const
		{
			return mItem < rhs.mItem;
		}

		bool operator==(const HighlightItem& rhs) const
		{
			return mItem == rhs.mItem;
		}

		void incrFade(F32 val) const
		{
			mFade = llclamp(mFade+val, 0.f, 1.f);
		}
	};

	std::set<HighlightItem> mHighlightSet;
	LLPointer<LLDrawable> mHighlightObject;

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
	LLRenderPass*				mGrassPool;
	LLRenderPass*				mFullbrightPool;
	LLDrawPool*					mInvisiblePool;
	LLDrawPool*					mGlowPool;
	LLDrawPool*					mBumpPool;
	LLDrawPool*					mWLSkyPool;
	// Note: no need to keep an quick-lookup to avatar pools, since there's only one per avatar
	
public:
	std::vector<LLFace*>		mHighlightFaces;	// highlight faces on physical objects
protected:
	std::vector<LLFace*>		mSelectedFaces;

	class DebugBlip
	{
	public:
		LLColor4 mColor;
		LLVector3 mPosition;
		F32 mAge;

		DebugBlip(const LLVector3& position, const LLColor4& color)
			: mColor(color), mPosition(position), mAge(0.f)
		{ }
	};

	std::list<DebugBlip> mDebugBlips;

	LLPointer<LLViewerFetchedTexture>	mFaceSelectImagep;
	
	U32						mLightMask;
	U32						mLightMovingMask;
	S32						mLightingDetail;
		
	static BOOL				sRenderPhysicalBeacons;
	static BOOL				sRenderMOAPBeacons;
	static BOOL				sRenderScriptedTouchBeacons;
	static BOOL				sRenderScriptedBeacons;
	static BOOL				sRenderParticleBeacons;
	static BOOL				sRenderSoundBeacons;
public:
	static BOOL				sRenderBeacons;
	static BOOL				sRenderHighlight;

	//debug use
	static U32              sCurRenderPoolType ;

	//cached settings
	static BOOL WindLightUseAtmosShaders;
	static BOOL VertexShaderEnable;
	static BOOL RenderAvatarVP;
	static BOOL RenderDeferred;
	static F32 RenderDeferredSunWash;
	static U32 RenderFSAASamples;
	static U32 RenderResolutionDivisor;
	static BOOL RenderUIBuffer;
	static S32 RenderShadowDetail;
	static BOOL RenderDeferredSSAO;
	static F32 RenderShadowResolutionScale;
	static BOOL RenderLocalLights;
	static BOOL RenderDelayCreation;
	static BOOL RenderAnimateRes;
	static BOOL FreezeTime;
	static S32 DebugBeaconLineWidth;
	static F32 RenderHighlightBrightness;
	static LLColor4 RenderHighlightColor;
	static F32 RenderHighlightThickness;
	static BOOL RenderSpotLightsInNondeferred;
	static LLColor4 PreviewAmbientColor;
	static LLColor4 PreviewDiffuse0;
	static LLColor4 PreviewSpecular0;
	static LLColor4 PreviewDiffuse1;
	static LLColor4 PreviewSpecular1;
	static LLColor4 PreviewDiffuse2;
	static LLColor4 PreviewSpecular2;
	static LLVector3 PreviewDirection0;
	static LLVector3 PreviewDirection1;
	static LLVector3 PreviewDirection2;
	static F32 RenderGlowMinLuminance;
	static F32 RenderGlowMaxExtractAlpha;
	static F32 RenderGlowWarmthAmount;
	static LLVector3 RenderGlowLumWeights;
	static LLVector3 RenderGlowWarmthWeights;
	static S32 RenderGlowResolutionPow;
	static S32 RenderGlowIterations;
	static F32 RenderGlowWidth;
	static F32 RenderGlowStrength;
	static BOOL RenderDepthOfField;
	static F32 CameraFocusTransitionTime;
	static F32 CameraFNumber;
	static F32 CameraFocalLength;
	static F32 CameraFieldOfView;
	static F32 RenderShadowNoise;
	static F32 RenderShadowBlurSize;
	static F32 RenderSSAOScale;
	static U32 RenderSSAOMaxScale;
	static F32 RenderSSAOFactor;
	static LLVector3 RenderSSAOEffect;
	static F32 RenderShadowOffsetError;
	static F32 RenderShadowBiasError;
	static F32 RenderShadowOffset;
	static F32 RenderShadowBias;
	static F32 RenderSpotShadowOffset;
	static F32 RenderSpotShadowBias;
	static F32 RenderEdgeDepthCutoff;
	static F32 RenderEdgeNormCutoff;
	static LLVector3 RenderShadowGaussian;
	static F32 RenderShadowBlurDistFactor;
	static BOOL RenderDeferredAtmospheric;
	static S32 RenderReflectionDetail;
	static F32 RenderHighlightFadeTime;
	static LLVector3 RenderShadowClipPlanes;
	static LLVector3 RenderShadowOrthoClipPlanes;
	static LLVector3 RenderShadowNearDist;
	static F32 RenderFarClip;
	static LLVector3 RenderShadowSplitExponent;
	static F32 RenderShadowErrorCutoff;
	static F32 RenderShadowFOVCutoff;
	static BOOL CameraOffset;
	static F32 CameraMaxCoF;
	static F32 CameraDoFResScale;
	static F32 RenderAutoHideSurfaceAreaLimit;
};

void render_bbox(const LLVector3 &min, const LLVector3 &max);
void render_hud_elements();

extern LLPipeline gPipeline;
extern BOOL gDebugPipeline;
extern const LLMatrix4* gGLLastMatrix;

#endif
