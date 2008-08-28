/** 
 * @file pipeline.cpp
 * @brief Rendering pipeline.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"

// library includes
#include "audioengine.h" // For MAX_BUFFERS for debugging.
#include "imageids.h"
#include "llerror.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llmemory.h"
#include "llmemtype.h"
#include "llnamevalue.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "timing.h"
#include "v3color.h"
#include "llui.h" 
#include "llglheaders.h"
#include "llrender.h"

// newview includes
#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolground.h"
#include "lldrawpoolbump.h"
#include "lldrawpooltree.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfloatertelehub.h"
#include "llframestats.h"
#include "llgldbg.h"
#include "llhudmanager.h"
#include "lllightconstants.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltracker.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h" // for audio debugging.
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatar.h"
#include "llvoground.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "llvotree.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "llcubemap.h"
#include "lldebugmessagebox.h"
#include "llviewershadermgr.h"
#include "llviewerjoystick.h"
#include "llviewerdisplay.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llspatialpartition.h"
#include "llmutelist.h"

#ifdef _DEBUG
// Debug indices is disabled for now for debug performance - djs 4/24/02
//#define DEBUG_INDICES
#else
//#define DEBUG_INDICES
#endif

const F32 BACKLIGHT_DAY_MAGNITUDE_AVATAR = 0.2f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_AVATAR = 0.1f;
const F32 BACKLIGHT_DAY_MAGNITUDE_OBJECT = 0.1f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_OBJECT = 0.08f;
const S32 MAX_ACTIVE_OBJECT_QUIET_FRAMES = 40;
const S32 MAX_OFFSCREEN_GEOMETRY_CHANGES_PER_FRAME = 10;
const U32 REFLECTION_MAP_RES = 128;

// Max number of occluders to search for. JC
const S32 MAX_OCCLUDER_COUNT = 2;

extern S32 gBoxFrame;
extern BOOL gRenderLightGlows;
extern BOOL gHideSelectedObjects;
extern BOOL gDisplaySwapBuffers;
extern BOOL gDebugGL;

// hack counter for rendering a fixed number of frames after toggling
// fullscreen to work around DEV-5361
static S32 sDelayedVBOEnable = 0;

BOOL	gAvatarBacklight = FALSE;

BOOL	gRenderForSelect = FALSE;

BOOL	gDebugPipeline = FALSE;
LLPipeline gPipeline;
const LLMatrix4* gGLLastMatrix = NULL;

//----------------------------------------

std::string gPoolNames[] = 
{
	// Correspond to LLDrawpool enum render type
	"NONE",
	"POOL_SIMPLE",
	"POOL_TERRAIN",	
	"POOL_TREE",
	"POOL_SKY",
	"POOL_WL_SKY",
	"POOL_GROUND",
	"POOL_BUMP",
	"POOL_INVISIBLE",
	"POOL_AVATAR",
	"POOL_WATER",
	"POOL_GLOW",
	"POOL_ALPHA",
};

U32 nhpo2(U32 v) 
{
	U32 r = 1;
	while (r < v) {
		r *= 2;
	}
	return r;
}

glh::matrix4f glh_copy_matrix(GLdouble* src)
{
	glh::matrix4f ret;
	for (U32 i = 0; i < 16; i++)
	{
		ret.m[i] = (F32) src[i];
	}
	return ret;
}

glh::matrix4f glh_get_current_modelview()
{
	return glh_copy_matrix(gGLModelView);
}

glh::matrix4f glh_get_current_projection()
{
	return glh_copy_matrix(gGLProjection);
}

void glh_copy_matrix(const glh::matrix4f& src, GLdouble* dst)
{
	for (U32 i = 0; i < 16; i++)
	{
		dst[i] = src.m[i];
	}
}

void glh_set_current_modelview(const glh::matrix4f& mat)
{
	glh_copy_matrix(mat, gGLModelView);
}

void glh_set_current_projection(glh::matrix4f& mat)
{
	glh_copy_matrix(mat, gGLProjection);
}

glh::matrix4f gl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar)
{
	glh::matrix4f ret(
		2.f/(right-left), 0.f, 0.f, -(right+left)/(right-left),
		0.f, 2.f/(top-bottom), 0.f, -(top+bottom)/(top-bottom),
		0.f, 0.f, -2.f/(zfar-znear),  -(zfar+znear)/(zfar-znear),
		0.f, 0.f, 0.f, 1.f);

	return ret;
}

void display_update_camera();
//----------------------------------------

S32		LLPipeline::sCompiles = 0;

BOOL	LLPipeline::sDynamicLOD = TRUE;
BOOL	LLPipeline::sShowHUDAttachments = TRUE;
BOOL	LLPipeline::sRenderPhysicalBeacons = TRUE;
BOOL	LLPipeline::sRenderScriptedBeacons = FALSE;
BOOL	LLPipeline::sRenderScriptedTouchBeacons = TRUE;
BOOL	LLPipeline::sRenderParticleBeacons = FALSE;
BOOL	LLPipeline::sRenderSoundBeacons = FALSE;
BOOL	LLPipeline::sRenderBeacons = FALSE;
BOOL	LLPipeline::sRenderHighlight = TRUE;
S32		LLPipeline::sUseOcclusion = 0;
BOOL	LLPipeline::sFastAlpha = TRUE;
BOOL	LLPipeline::sDisableShaders = FALSE;
BOOL	LLPipeline::sRenderBump = TRUE;
BOOL	LLPipeline::sUseFarClip = TRUE;
BOOL	LLPipeline::sSkipUpdate = FALSE;
BOOL	LLPipeline::sWaterReflections = FALSE;
BOOL	LLPipeline::sRenderGlow = FALSE;
BOOL	LLPipeline::sReflectionRender = FALSE;
BOOL	LLPipeline::sImpostorRender = FALSE;
BOOL	LLPipeline::sUnderWaterRender = FALSE;
BOOL	LLPipeline::sTextureBindTest = FALSE;
BOOL	LLPipeline::sRenderFrameTest = FALSE;
BOOL	LLPipeline::sRenderAttachedLights = TRUE;
BOOL	LLPipeline::sRenderAttachedParticles = TRUE;

static LLCullResult* sCull = NULL;

static const U32 gl_cube_face[] = 
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
};

void validate_framebuffer_object();

LLPipeline::LLPipeline() :
	mBackfaceCull(FALSE),
	mBatchCount(0),
	mMatrixOpCount(0),
	mTextureMatrixOps(0),
	mMaxBatchSize(0),
	mMinBatchSize(0),
	mMeanBatchSize(0),
	mTrianglesDrawn(0),
	mNumVisibleNodes(0),
	mVerticesRelit(0),
	mLightingChanges(0),
	mGeometryChanges(0),
	mNumVisibleFaces(0),

	mCubeBuffer(NULL),
	mCubeFrameBuffer(0),
	mCubeDepth(0),
	mInitialized(FALSE),
	mVertexShadersEnabled(FALSE),
	mVertexShadersLoaded(0),
	mRenderTypeMask(0),
	mRenderDebugFeatureMask(0),
	mRenderDebugMask(0),
	mOldRenderDebugMask(0),
	mLastRebuildPool(NULL),
	mAlphaPool(NULL),
	mSkyPool(NULL),
	mTerrainPool(NULL),
	mWaterPool(NULL),
	mGroundPool(NULL),
	mSimplePool(NULL),
	mInvisiblePool(NULL),
	mGlowPool(NULL),
	mBumpPool(NULL),
	mWLSkyPool(NULL),
	mLightMask(0),
	mLightMovingMask(0),
	mLightingDetail(0)
{
	mBlurCubeBuffer[0] = mBlurCubeBuffer[1] = mBlurCubeBuffer[2] = 0;
	mBlurCubeTexture[0] = mBlurCubeTexture[1] = mBlurCubeTexture[2] = 0;
}

void LLPipeline::init()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	sDynamicLOD = gSavedSettings.getBOOL("RenderDynamicLOD");
	sRenderBump = gSavedSettings.getBOOL("RenderObjectBump");
	sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
	sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");

	mInitialized = TRUE;
	
	stop_glerror();

	//create render pass pools
	getPool(LLDrawPool::POOL_ALPHA);
	getPool(LLDrawPool::POOL_SIMPLE);
	getPool(LLDrawPool::POOL_INVISIBLE);
	getPool(LLDrawPool::POOL_BUMP);
	getPool(LLDrawPool::POOL_GLOW);

	mTrianglesDrawnStat.reset();
	resetFrameStats();

	mRenderTypeMask = 0xffffffff;	// All render types start on
	mRenderDebugFeatureMask = 0xffffffff; // All debugging features on
	mRenderDebugMask = 0;	// All debug starts off

	mOldRenderDebugMask = mRenderDebugMask;
	
	mBackfaceCull = TRUE;

	stop_glerror();
	
	// Enable features
		
	LLViewerShaderMgr::instance()->setShaders();

	stop_glerror();
}

LLPipeline::~LLPipeline()
{

}

void LLPipeline::cleanup()
{
	assertInitialized();

	for(pool_set_t::iterator iter = mPools.begin();
		iter != mPools.end(); )
	{
		pool_set_t::iterator curiter = iter++;
		LLDrawPool* poolp = *curiter;
		if (poolp->isFacePool())
		{
			LLFacePool* face_pool = (LLFacePool*) poolp;
			if (face_pool->mReferences.empty())
			{
				mPools.erase(curiter);
				removeFromQuickLookup( poolp );
				delete poolp;
			}
		}
		else
		{
			mPools.erase(curiter);
			removeFromQuickLookup( poolp );
			delete poolp;
		}
	}
	
	if (!mTerrainPools.empty())
	{
		llwarns << "Terrain Pools not cleaned up" << llendl;
	}
	if (!mTreePools.empty())
	{
		llwarns << "Tree Pools not cleaned up" << llendl;
	}
		
	delete mAlphaPool;
	mAlphaPool = NULL;
	delete mSkyPool;
	mSkyPool = NULL;
	delete mTerrainPool;
	mTerrainPool = NULL;
	delete mWaterPool;
	mWaterPool = NULL;
	delete mGroundPool;
	mGroundPool = NULL;
	delete mSimplePool;
	mSimplePool = NULL;
	delete mInvisiblePool;
	mInvisiblePool = NULL;
	delete mGlowPool;
	mGlowPool = NULL;
	delete mBumpPool;
	mBumpPool = NULL;
	// don't delete wl sky pool it was handled above in the for loop
	//delete mWLSkyPool;
	mWLSkyPool = NULL;

	releaseGLBuffers();

	mBloomImagep = NULL;
	mBloomImage2p = NULL;
	mFaceSelectImagep = NULL;

	mMovedBridge.clear();

	mInitialized = FALSE;
}

//============================================================================

void LLPipeline::destroyGL() 
{
	stop_glerror();
	unloadShaders();
	mHighlightFaces.clear();
	
	resetDrawOrders();

	resetVertexBuffers();

	releaseGLBuffers();

	if (LLVertexBuffer::sEnableVBOs)
	{
		// render 30 frames after switching to work around DEV-5361
		sDelayedVBOEnable = 30;
		LLVertexBuffer::sEnableVBOs = FALSE;
	}
}

void LLPipeline::resizeScreenTexture()
{
	if (gPipeline.canUseVertexShaders() && assertInitialized())
	{
		GLuint resX = gViewerWindow->getWindowDisplayWidth();
		GLuint resY = gViewerWindow->getWindowDisplayHeight();
	
		U32 res_mod = gSavedSettings.getU32("RenderResolutionDivisor");
		if (res_mod > 1)
		{
			resX /= res_mod;
			resY /= res_mod;
		}
	
		mScreen.release();
		mScreen.allocate(resX, resY, GL_RGBA, TRUE, GL_TEXTURE_RECTANGLE_ARB);		

		llinfos << "RESIZED SCREEN TEXTURE: " << resX << "x" << resY << llendl;
	}
}


void LLPipeline::releaseGLBuffers()
{
	assertInitialized();
	
	if (mCubeBuffer)
	{
		mCubeBuffer = NULL;
	}

	if (mCubeFrameBuffer)
	{
		glDeleteFramebuffersEXT(1, &mCubeFrameBuffer);
		glDeleteRenderbuffersEXT(1, &mCubeDepth);
		mCubeDepth = mCubeFrameBuffer = 0;
	}

	if (mBlurCubeBuffer[0])
	{
		glDeleteFramebuffersEXT(3, mBlurCubeBuffer);
		mBlurCubeBuffer[0] = mBlurCubeBuffer[1] = mBlurCubeBuffer[2] = 0;
	}

	if (mBlurCubeTexture[0])
	{
		glDeleteTextures(3, mBlurCubeTexture);
		mBlurCubeTexture[0] = mBlurCubeTexture[1] = mBlurCubeTexture[2] = 0;
	}

	mWaterRef.release();
	mWaterDis.release();
	mScreen.release();

	for (U32 i = 0; i < 3; i++)
	{
		mGlow[i].release();
	}

	LLVOAvatar::resetImpostors();
}

void LLPipeline::createGLBuffers()
{
	assertInitialized();

	if (LLPipeline::sWaterReflections)
	{ //water reflection texture
		U32 res = (U32) gSavedSettings.getS32("RenderWaterRefResolution");
			
		mWaterRef.allocate(res,res,GL_RGBA,TRUE);
		mWaterDis.allocate(res,res,GL_RGBA,TRUE);

#if 0 //cube map buffers (keep for future work)
		{
			//reflection map generation buffers
			if (mCubeFrameBuffer == 0)
			{
				glGenFramebuffersEXT(1, &mCubeFrameBuffer);
				glGenRenderbuffersEXT(1, &mCubeDepth);

				U32 res = REFLECTION_MAP_RES;

				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mCubeDepth);
						
				glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,res,res);
							
				glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
			}

			if (mCubeBuffer.isNull())
			{
				res = 128;
				mCubeBuffer = new LLCubeMap();
				mCubeBuffer->initGL();
				mCubeBuffer->setReflection();
					
				for (U32 i = 0; i < 6; i++)
				{
					glTexImage2D(gl_cube_face[i], 0, GL_RGBA, res, res, 0, GL_RGBA, GL_FLOAT, NULL); 
				}
			}

			if (mBlurCubeBuffer[0] == 0)
			{
				glGenFramebuffersEXT(3, mBlurCubeBuffer);
			}

			if (mBlurCubeTexture[0] == 0)
			{
				glGenTextures(3, mBlurCubeTexture);
			}

			res = (U32) gSavedSettings.getS32("RenderReflectionRes");

			for (U32 j = 0; j < 3; j++)
			{
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, mBlurCubeTexture[j]);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					
				for (U32 i = 0; i < 6; i++)
				{
					glTexImage2D(gl_cube_face[i], 0, GL_RGBA, res, res, 0, GL_RGBA, GL_FLOAT, NULL); 
				}
			}
		}
#endif
	}

	stop_glerror();

	if (LLPipeline::sRenderGlow)
	{ //screen space glow buffers
		const U32 glow_res = llmax(1, 
			llmin(512, 1 << gSavedSettings.getS32("RenderGlowResolutionPow")));

		for (U32 i = 0; i < 3; i++)
		{
			mGlow[i].allocate(512,glow_res,GL_RGBA,FALSE);
		}
		
		GLuint resX = gViewerWindow->getWindowDisplayWidth();
		GLuint resY = gViewerWindow->getWindowDisplayHeight();
		
		mScreen.allocate(resX, resY, GL_RGBA, TRUE, GL_TEXTURE_RECTANGLE_ARB);
	}
}

void LLPipeline::restoreGL() 
{
	assertInitialized();

	if (mVertexShadersEnabled)
	{
		LLViewerShaderMgr::instance()->setShaders();
	}

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				part->restoreGL();
			}
		}
	}
}


BOOL LLPipeline::canUseVertexShaders()
{
	if (!gGLManager.mHasVertexShader ||
		!gGLManager.mHasFragmentShader ||
		!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable") ||
		(assertInitialized() && mVertexShadersLoaded != 1) )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL LLPipeline::canUseWindLightShaders() const
{
	return (!LLPipeline::sDisableShaders &&
			gWLSkyProgram.mProgramObject != 0 &&
			LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_WINDLIGHT) > 1);
}

BOOL LLPipeline::canUseWindLightShadersOnObjects() const
{
	return (canUseWindLightShaders() 
		&& LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0);
}

void LLPipeline::unloadShaders()
{
	LLViewerShaderMgr::instance()->unloadShaders();

	mVertexShadersLoaded = 0;
}

void LLPipeline::assertInitializedDoError()
{
	llerrs << "LLPipeline used when uninitialized." << llendl;
}

//============================================================================

void LLPipeline::enableShadows(const BOOL enable_shadows)
{
	//should probably do something here to wrangle shadows....	
}

S32 LLPipeline::getMaxLightingDetail() const
{
	/*if (mVertexShaderLevel[SHADER_OBJECT] >= LLDrawPoolSimple::SHADER_LEVEL_LOCAL_LIGHTS)
	{
		return 3;
	}
	else*/
	{
		return 1;
	}
}

S32 LLPipeline::setLightingDetail(S32 level)
{
	assertInitialized();

	if (level < 0)
	{
		level = gSavedSettings.getS32("RenderLightingDetail");
	}
	level = llclamp(level, 0, getMaxLightingDetail());
	if (level != mLightingDetail)
	{
		gSavedSettings.setS32("RenderLightingDetail", level);
		
		mLightingDetail = level;

		if (mVertexShadersLoaded == 1)
		{
			LLViewerShaderMgr::instance()->setShaders();
		}
	}
	return mLightingDetail;
}

class LLOctreeDirtyTexture : public LLOctreeTraveler<LLDrawable>
{
public:
	const std::set<LLViewerImage*>& mTextures;

	LLOctreeDirtyTexture(const std::set<LLViewerImage*>& textures) : mTextures(textures) { }

	virtual void visit(const LLOctreeNode<LLDrawable>* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);

		if (!group->isState(LLSpatialGroup::GEOM_DIRTY) && !group->getData().empty())
		{
			for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
			{
				for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j) 
				{
					LLDrawInfo* params = *j;
					if (mTextures.find(params->mTexture) != mTextures.end())
					{ 
						group->setState(LLSpatialGroup::GEOM_DIRTY);
					}
				}
			}
		}

		for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			traverse(bridge->mOctree);
		}
	}
};

// Called when a texture changes # of channels (causes faces to move to alpha pool)
void LLPipeline::dirtyPoolObjectTextures(const std::set<LLViewerImage*>& textures)
{
	assertInitialized();

	// *TODO: This is inefficient and causes frame spikes; need a better way to do this
	//        Most of the time is spent in dirty.traverse.

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->isFacePool())
		{
			((LLFacePool*) poolp)->dirtyTextures(textures);
		}
	}
	
	LLOctreeDirtyTexture dirty(textures);
	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				dirty.traverse(part->mOctree);
			}
		}
	}
}

LLDrawPool *LLPipeline::findPool(const U32 type, LLViewerImage *tex0)
{
	assertInitialized();

	LLDrawPool *poolp = NULL;
	switch( type )
	{
	case LLDrawPool::POOL_SIMPLE:
		poolp = mSimplePool;
		break;

	case LLDrawPool::POOL_INVISIBLE:
		poolp = mInvisiblePool;
		break;

	case LLDrawPool::POOL_GLOW:
		poolp = mGlowPool;
		break;

	case LLDrawPool::POOL_TREE:
		poolp = get_if_there(mTreePools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_TERRAIN:
		poolp = get_if_there(mTerrainPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_BUMP:
		poolp = mBumpPool;
		break;

	case LLDrawPool::POOL_ALPHA:
		poolp = mAlphaPool;
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		poolp = mSkyPool;
		break;

	case LLDrawPool::POOL_WATER:
		poolp = mWaterPool;
		break;

	case LLDrawPool::POOL_GROUND:
		poolp = mGroundPool;
		break;

	case LLDrawPool::POOL_WL_SKY:
		poolp = mWLSkyPool;
		break;

	default:
		llassert(0);
		llerrs << "Invalid Pool Type in  LLPipeline::findPool() type=" << type << llendl;
		break;
	}

	return poolp;
}


LLDrawPool *LLPipeline::getPool(const U32 type,	LLViewerImage *tex0)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLDrawPool *poolp = findPool(type, tex0);
	if (poolp)
	{
		return poolp;
	}

	LLDrawPool *new_poolp = LLDrawPool::createPool(type, tex0);
	addPool( new_poolp );

	return new_poolp;
}


// static
LLDrawPool* LLPipeline::getPoolFromTE(const LLTextureEntry* te, LLViewerImage* imagep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	U32 type = getPoolTypeFromTE(te, imagep);
	return gPipeline.getPool(type, imagep);
}

//static 
U32 LLPipeline::getPoolTypeFromTE(const LLTextureEntry* te, LLViewerImage* imagep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	if (!te || !imagep)
	{
		return 0;
	}
		
	bool alpha = te->getColor().mV[3] < 0.999f;
	if (imagep)
	{
		alpha = alpha || (imagep->getComponents() == 4 && ! imagep->mIsMediaTexture) || (imagep->getComponents() == 2);
	}

	if (alpha)
	{
		return LLDrawPool::POOL_ALPHA;
	}
	else if ((te->getBumpmap() || te->getShiny()))
	{
		return LLDrawPool::POOL_BUMP;
	}
	else
	{
		return LLDrawPool::POOL_SIMPLE;
	}
}


void LLPipeline::addPool(LLDrawPool *new_poolp)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	assertInitialized();
	mPools.insert(new_poolp);
	addToQuickLookup( new_poolp );
}

void LLPipeline::allocDrawable(LLViewerObject *vobj)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	LLDrawable *drawable = new LLDrawable();
	vobj->mDrawable = drawable;
	
	drawable->mVObjp     = vobj;
	
	//encompass completely sheared objects by taking 
	//the most extreme point possible (<1,1,0.5>)
	drawable->setRadius(LLVector3(1,1,0.5f).scaleVec(vobj->getScale()).length());
	if (vobj->isOrphaned())
	{
		drawable->setState(LLDrawable::FORCE_INVISIBLE);
	}
	drawable->updateXform(TRUE);
}


void LLPipeline::unlinkDrawable(LLDrawable *drawable)
{
	LLFastTimer t(LLFastTimer::FTM_PIPELINE);

	assertInitialized();

	LLPointer<LLDrawable> drawablep = drawable; // make sure this doesn't get deleted before we are done
	
	// Based on flags, remove the drawable from the queues that it's on.
	if (drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		LLDrawable::drawable_vector_t::iterator iter = std::find(mMovedList.begin(), mMovedList.end(), drawablep);
		if (iter != mMovedList.end())
		{
			mMovedList.erase(iter);
		}
	}

	if (drawablep->getSpatialGroup())
	{
		if (!drawablep->getSpatialGroup()->mSpatialPartition->remove(drawablep, drawablep->getSpatialGroup()))
		{
#ifdef LL_RELEASE_FOR_DOWNLOAD
			llwarns << "Couldn't remove object from spatial group!" << llendl;
#else
			llerrs << "Couldn't remove object from spatial group!" << llendl;
#endif
		}
	}

	mLights.erase(drawablep);
	for (light_set_t::iterator iter = mNearbyLights.begin();
				iter != mNearbyLights.end(); iter++)
	{
		if (iter->drawable == drawablep)
		{
			mNearbyLights.erase(iter);
			break;
		}
	}
}

U32 LLPipeline::addObject(LLViewerObject *vobj)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	if (gNoRender)
	{
		return 0;
	}

	LLDrawable* drawablep = vobj->mDrawable;

	if (!drawablep)
	{
		drawablep = vobj->createDrawable(this);
	}
	
	llassert(drawablep);

	if (vobj->getParent())
	{
		vobj->setDrawableParent(((LLViewerObject*)vobj->getParent())->mDrawable); // LLPipeline::addObject 1
	}
	else
	{
		vobj->setDrawableParent(NULL); // LLPipeline::addObject 2
	}

	markRebuild(drawablep, LLDrawable::REBUILD_ALL, TRUE);

	return 1;
}


void LLPipeline::resetFrameStats()
{
	assertInitialized();

	mTrianglesDrawnStat.addValue(mTrianglesDrawn/1000.f);

	if (mBatchCount > 0)
	{
		mMeanBatchSize = gPipeline.mTrianglesDrawn/gPipeline.mBatchCount;
	}
	mTrianglesDrawn = 0;
	sCompiles        = 0;
	mVerticesRelit   = 0;
	mLightingChanges = 0;
	mGeometryChanges = 0;
	mNumVisibleFaces = 0;

	if (mOldRenderDebugMask != mRenderDebugMask)
	{
		gObjectList.clearDebugText();
		mOldRenderDebugMask = mRenderDebugMask;
	}
		
}

//external functions for asynchronous updating
void LLPipeline::updateMoveDampedAsync(LLDrawable* drawablep)
{
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}
	if (!drawablep)
	{
		llerrs << "updateMove called with NULL drawablep" << llendl;
		return;
	}
	if (drawablep->isState(LLDrawable::EARLY_MOVE))
	{
		return;
	}

	assertInitialized();

	// update drawable now
	drawablep->clearState(LLDrawable::MOVE_UNDAMPED); // force to DAMPED
	drawablep->updateMove(); // returns done
	drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
	// Put on move list so that EARLY_MOVE gets cleared
	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.push_back(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
}

void LLPipeline::updateMoveNormalAsync(LLDrawable* drawablep)
{
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}
	if (!drawablep)
	{
		llerrs << "updateMove called with NULL drawablep" << llendl;
	}
	if (drawablep->isState(LLDrawable::EARLY_MOVE))
	{
		return;
	}

	assertInitialized();

	// update drawable now
	drawablep->setState(LLDrawable::MOVE_UNDAMPED); // force to UNDAMPED
	drawablep->updateMove();
	drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
	// Put on move list so that EARLY_MOVE gets cleared
	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.push_back(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
}

void LLPipeline::updateMovedList(LLDrawable::drawable_vector_t& moved_list)
{
	for (LLDrawable::drawable_vector_t::iterator iter = moved_list.begin();
		 iter != moved_list.end(); )
	{
		LLDrawable::drawable_vector_t::iterator curiter = iter++;
		LLDrawable *drawablep = *curiter;
		BOOL done = TRUE;
		if (!drawablep->isDead() && (!drawablep->isState(LLDrawable::EARLY_MOVE)))
		{
			done = drawablep->updateMove();
		}
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED);
		if (done)
		{
			drawablep->clearState(LLDrawable::ON_MOVE_LIST);
			iter = moved_list.erase(curiter);
		}
	}
}

void LLPipeline::updateMove()
{
	LLFastTimer t(LLFastTimer::FTM_UPDATE_MOVE);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}

	assertInitialized();

	for (LLDrawable::drawable_set_t::iterator iter = mRetexturedList.begin();
		 iter != mRetexturedList.end(); ++iter)
	{
		LLDrawable* drawablep = *iter;
		if (drawablep && !drawablep->isDead())
		{
			drawablep->updateTexture();
		}
	}
	mRetexturedList.clear();

	updateMovedList(mMovedList);

	for (LLDrawable::drawable_set_t::iterator iter = mActiveQ.begin();
		 iter != mActiveQ.end(); )
	{
		LLDrawable::drawable_set_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		if (drawablep && !drawablep->isDead()) 
		{
			if (drawablep->isRoot() && 
				drawablep->mQuietCount++ > MAX_ACTIVE_OBJECT_QUIET_FRAMES && 
				(!drawablep->getParent() || !drawablep->getParent()->isActive()))
			{
				drawablep->makeStatic(); // removes drawable and its children from mActiveQ
				iter = mActiveQ.upper_bound(drawablep); // next valid entry
			}
		}
		else
		{
			mActiveQ.erase(curiter);
		}
	}

	//balance octrees
	{
 		LLFastTimer ot(LLFastTimer::FTM_OCTREE_BALANCE);

		for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* region = *iter;
			for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
			{
				LLSpatialPartition* part = region->getSpatialPartition(i);
				if (part)
				{
					part->mOctree->balance();
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Culling and occlusion testing
/////////////////////////////////////////////////////////////////////////////

//static
F32 LLPipeline::calcPixelArea(LLVector3 center, LLVector3 size, LLCamera &camera)
{
	LLVector3 lookAt = center - camera.getOrigin();
	F32 dist = lookAt.length();

	//ramp down distance for nearby objects
	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	//get area of circle around node
	F32 app_angle = atanf(size.length()/dist);
	F32 radius = app_angle*LLDrawable::sCurPixelAngle;
	return radius*radius * 3.14159f;
}

void LLPipeline::grabReferences(LLCullResult& result)
{
	sCull = &result;
}

void LLPipeline::updateCull(LLCamera& camera, LLCullResult& result, S32 water_clip)
{
	LLFastTimer t(LLFastTimer::FTM_CULL);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	grabReferences(result);

	sCull->clear();

	BOOL to_texture =	LLPipeline::sUseOcclusion > 1 &&
						!hasRenderType(LLPipeline::RENDER_TYPE_HUD) && 
						!sReflectionRender &&
						gPipeline.canUseVertexShaders() &&
						sRenderGlow;

	if (to_texture)
	{
		mScreen.bindTarget();
	}

	glPushMatrix();
	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLLastModelView);

	LLVertexBuffer::unbind();
	LLGLDisable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);

	gGL.setColorMask(false, false);
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		if (water_clip != 0)
		{
			LLPlane plane(LLVector3(0,0, (F32) -water_clip), (F32) water_clip*region->getWaterHeight());
			camera.setUserClipPlane(plane);
		}
		else
		{
			camera.disableUserClipPlane();
		}

		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				if (hasRenderType(part->mDrawableType))
				{
					part->cull(camera);
				}
			}
		}
	}

	camera.disableUserClipPlane();

	if (gSky.mVOSkyp.notNull() && gSky.mVOSkyp->mDrawable.notNull())
	{
		// Hack for sky - always visible.
		if (hasRenderType(LLPipeline::RENDER_TYPE_SKY)) 
		{
			gSky.mVOSkyp->mDrawable->setVisible(camera);
			sCull->pushDrawable(gSky.mVOSkyp->mDrawable);
			gSky.updateCull();
			stop_glerror();
		}
	}
	else
	{
		llinfos << "No sky drawable!" << llendl;
	}

	if (hasRenderType(LLPipeline::RENDER_TYPE_GROUND) && 
		!gPipeline.canUseWindLightShaders() &&
		gSky.mVOGroundp.notNull() && 
		gSky.mVOGroundp->mDrawable.notNull() &&
		!LLPipeline::sWaterReflections)
	{
		gSky.mVOGroundp->mDrawable->setVisible(camera);
		sCull->pushDrawable(gSky.mVOGroundp->mDrawable);
	}
	
	gGL.setColorMask(true, false);
	glPopMatrix();

	if (to_texture)
	{
		mScreen.flush();
		LLRenderTarget::unbindTarget();
	}
	else if (LLPipeline::sUseOcclusion > 1)
	{
		glFlush();
	}
}

void LLPipeline::markNotCulled(LLSpatialGroup* group, LLCamera& camera)
{
	if (group->getData().empty())
	{ 
		return;
	}
	
	group->setVisible();

	if (!sSkipUpdate)
	{
		group->updateDistance(camera);
	}
	
	const F32 MINIMUM_PIXEL_AREA = 16.f;

	if (group->mPixelArea < MINIMUM_PIXEL_AREA)
	{
		return;
	}

	assertInitialized();
	
	if (!group->mSpatialPartition->mRenderByGroup)
	{ //render by drawable
		sCull->pushDrawableGroup(group);
	}
	else
	{   //render by group
		sCull->pushVisibleGroup(group);
	}

	mNumVisibleNodes++;
}

void LLPipeline::markOccluder(LLSpatialGroup* group)
{
	if (sUseOcclusion > 1 && group && !group->isState(LLSpatialGroup::ACTIVE_OCCLUSION))
	{
		LLSpatialGroup* parent = group->getParent();

		if (!parent || !parent->isState(LLSpatialGroup::OCCLUDED))
		{ //only mark top most occluders as active occlusion
			sCull->pushOcclusionGroup(group);
			group->setState(LLSpatialGroup::ACTIVE_OCCLUSION);
				
			if (parent && 
				!parent->isState(LLSpatialGroup::ACTIVE_OCCLUSION) &&
				parent->getElementCount() == 0 &&
				parent->needsUpdate())
			{
				sCull->pushOcclusionGroup(group);
				parent->setState(LLSpatialGroup::ACTIVE_OCCLUSION);
			}
		}
	}
}

void LLPipeline::doOcclusion(LLCamera& camera)
{
	LLVertexBuffer::unbind();
	if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
	{
		gGL.setColorMask(true, false, false, false);
	}
	else
	{
		gGL.setColorMask(false, false);
	}
	LLGLDisable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	if (LLPipeline::sUseOcclusion > 1)
	{
		for (LLCullResult::sg_list_t::iterator iter = sCull->beginOcclusionGroups(); iter != sCull->endOcclusionGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			group->doOcclusion(&camera);
			group->clearState(LLSpatialGroup::ACTIVE_OCCLUSION);
		}
	}

	gGL.setColorMask(true, false);
	glFlush();
}
	
BOOL LLPipeline::updateDrawableGeom(LLDrawable* drawablep, BOOL priority)
{
	BOOL update_complete = drawablep->updateGeometry(priority);
	if (update_complete && assertInitialized())
	{
		drawablep->setState(LLDrawable::BUILT);
		mGeometryChanges++;
	}
	return update_complete;
}

void LLPipeline::updateGeom(F32 max_dtime)
{
	LLTimer update_timer;
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLPointer<LLDrawable> drawablep;

	LLFastTimer t(LLFastTimer::FTM_GEO_UPDATE);

	assertInitialized();

	if (sDelayedVBOEnable > 0)
	{
		if (--sDelayedVBOEnable <= 0)
		{
			resetVertexBuffers();
			LLVertexBuffer::sEnableVBOs = TRUE;
		}
	}

	// notify various object types to reset internal cost metrics, etc.
	// for now, only LLVOVolume does this to throttle LOD changes
	LLVOVolume::preUpdateGeom();

	// Iterate through all drawables on the priority build queue,
	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ1.begin();
		 iter != mBuildQ1.end();)
	{
		LLDrawable::drawable_list_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		if (drawablep && !drawablep->isDead())
		{
			if (drawablep->isState(LLDrawable::IN_REBUILD_Q2))
			{
				drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
				LLDrawable::drawable_list_t::iterator find = std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep);
				if (find != mBuildQ2.end())
				{
					mBuildQ2.erase(find);
				}
			}

			if (updateDrawableGeom(drawablep, TRUE))
			{
				drawablep->clearState(LLDrawable::IN_REBUILD_Q1);
				mBuildQ1.erase(curiter);
			}
		}
		else
		{
			mBuildQ1.erase(curiter);
		}
	}
		
	// Iterate through some drawables on the non-priority build queue
	S32 min_count = 16;
	S32 size = (S32) mBuildQ2.size();
	if (size > 1024)
	{
		min_count = llclamp((S32) (size * (F32) size/4096), 16, size);
	}
		
	S32 count = 0;
	
	max_dtime = llmax(update_timer.getElapsedTimeF32()+0.001f, max_dtime);
	LLSpatialGroup* last_group = NULL;
	LLSpatialBridge* last_bridge = NULL;

	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ2.begin();
		 iter != mBuildQ2.end(); )
	{
		LLDrawable::drawable_list_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;

		LLSpatialBridge* bridge = drawablep->isRoot() ? drawablep->getSpatialBridge() :
									drawablep->getParent()->getSpatialBridge();

		if (drawablep->getSpatialGroup() != last_group && 
			(!last_bridge || bridge != last_bridge) &&
			(update_timer.getElapsedTimeF32() >= max_dtime) && count > min_count)
		{
			break;
		}

		//make sure updates don't stop in the middle of a spatial group
		//to avoid thrashing (objects are enqueued by group)
		last_group = drawablep->getSpatialGroup();
		last_bridge = bridge;

		BOOL update_complete = TRUE;
		if (!drawablep->isDead())
		{
			update_complete = updateDrawableGeom(drawablep, FALSE);
			count++;
		}
		if (update_complete)
		{
			drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
			mBuildQ2.erase(curiter);
		}
	}	

	updateMovedList(mMovedBridge);
}

void LLPipeline::markVisible(LLDrawable *drawablep, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if(!drawablep || drawablep->isDead())
	{
		return;
	}
	
	if (drawablep->isSpatialBridge())
	{
		sCull->pushBridge((LLSpatialBridge*) drawablep);
	}
	else
	{
		sCull->pushDrawable(drawablep);
	}

	drawablep->setVisible(camera);
}

void LLPipeline::markMoved(LLDrawable *drawablep, BOOL damped_motion)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (!drawablep)
	{
		llerrs << "Sending null drawable to moved list!" << llendl;
		return;
	}
	
	if (drawablep->isDead())
	{
		llwarns << "Marking NULL or dead drawable moved!" << llendl;
		return;
	}
	
	if (drawablep->getParent()) 
	{
		//ensure that parent drawables are moved first
		markMoved(drawablep->getParent(), damped_motion);
	}

	assertInitialized();

	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		if (drawablep->isSpatialBridge())
		{
			mMovedBridge.push_back(drawablep);
		}
		else
		{
			mMovedList.push_back(drawablep);
		}
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
	if (damped_motion == FALSE)
	{
		drawablep->setState(LLDrawable::MOVE_UNDAMPED); // UNDAMPED trumps DAMPED
	}
	else if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
	{
		drawablep->clearState(LLDrawable::MOVE_UNDAMPED);
	}
}

void LLPipeline::markShift(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (!drawablep || drawablep->isDead())
	{
		return;
	}

	assertInitialized();

	if (!drawablep->isState(LLDrawable::ON_SHIFT_LIST))
	{
		drawablep->getVObj()->setChanged(LLXform::SHIFTED | LLXform::SILHOUETTE);
		if (drawablep->getParent()) 
		{
			markShift(drawablep->getParent());
		}
		mShiftList.push_back(drawablep);
		drawablep->setState(LLDrawable::ON_SHIFT_LIST);
	}
}

void LLPipeline::shiftObjects(const LLVector3 &offset)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	assertInitialized();

	glClear(GL_DEPTH_BUFFER_BIT);
	gDepthDirty = FALSE;

	for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
		 iter != mShiftList.end(); iter++)
	{
		LLDrawable *drawablep = *iter;
		if (drawablep->isDead())
		{
			continue;
		}	
		drawablep->shiftPos(offset);	
		drawablep->clearState(LLDrawable::ON_SHIFT_LIST);
	}
	mShiftList.resize(0);

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				part->shift(offset);
			}
		}
	}

	LLHUDText::shiftAll(offset);
	display_update_camera();
}

void LLPipeline::markTextured(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (drawablep && !drawablep->isDead() && assertInitialized())
	{
		mRetexturedList.insert(drawablep);
	}
}

void LLPipeline::markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag, BOOL priority)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (drawablep && !drawablep->isDead() && assertInitialized())
	{
		if (!drawablep->isState(LLDrawable::BUILT))
		{
			priority = TRUE;
		}
		if (priority)
		{
			if (!drawablep->isState(LLDrawable::IN_REBUILD_Q1))
			{
				mBuildQ1.push_back(drawablep);
				drawablep->setState(LLDrawable::IN_REBUILD_Q1); // mark drawable as being in priority queue
			}
		}
		else if (!drawablep->isState(LLDrawable::IN_REBUILD_Q2))
		{
			mBuildQ2.push_back(drawablep);
			drawablep->setState(LLDrawable::IN_REBUILD_Q2); // need flag here because it is just a list
		}
		if (flag & (LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION))
		{
			drawablep->getVObj()->setChanged(LLXform::SILHOUETTE);
		}
		drawablep->setState(flag);
	}
}

void LLPipeline::stateSort(LLCamera& camera, LLCullResult &result)
{
	const U32 face_mask = (1 << LLPipeline::RENDER_TYPE_AVATAR) |
						  (1 << LLPipeline::RENDER_TYPE_GROUND) |
						  (1 << LLPipeline::RENDER_TYPE_TERRAIN) |
						  (1 << LLPipeline::RENDER_TYPE_TREE) |
						  (1 << LLPipeline::RENDER_TYPE_SKY) |
						  (1 << LLPipeline::RENDER_TYPE_WATER);

	if (mRenderTypeMask & face_mask)
	{
		//clear faces from face pools
		LLFastTimer t(LLFastTimer::FTM_RESET_DRAWORDER);
		gPipeline.resetDrawOrders();
	}

	LLFastTimer ftm(LLFastTimer::FTM_STATESORT);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	//LLVertexBuffer::unbind();

	grabReferences(result);

	{
		for (LLCullResult::sg_list_t::iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			group->checkOcclusion();
			if (sUseOcclusion && group->isState(LLSpatialGroup::OCCLUDED))
			{
				markOccluder(group);
			}
			else
			{
				group->setVisible();
				for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
				{
					markVisible(*i, camera);
				}
			}
		}

		for (LLCullResult::sg_list_t::iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			group->checkOcclusion();
			if (sUseOcclusion && group->isState(LLSpatialGroup::OCCLUDED))
			{
				markOccluder(group);
			}
			else
			{
				group->setVisible();
				stateSort(group, camera);
			}
		}
	}

	{
		for (LLCullResult::bridge_list_t::iterator i = sCull->beginVisibleBridge(); i != sCull->endVisibleBridge(); ++i)
		{
			LLCullResult::bridge_list_t::iterator cur_iter = i;
			LLSpatialBridge* bridge = *cur_iter;
			LLSpatialGroup* group = bridge->getSpatialGroup();
			if (!bridge->isDead() && group && !group->isState(LLSpatialGroup::OCCLUDED))
			{
				stateSort(bridge, camera);
			}
		}
	}

	{
		LLFastTimer ftm(LLFastTimer::FTM_STATESORT_DRAWABLE);
		for (LLCullResult::drawable_list_t::iterator iter = sCull->beginVisibleList();
			 iter != sCull->endVisibleList(); ++iter)
		{
			LLDrawable *drawablep = *iter;
			if (!drawablep->isDead())
			{
				stateSort(drawablep, camera);
			}
		}
	}

	{
		LLFastTimer ftm(LLFastTimer::FTM_CLIENT_COPY);
		LLVertexBuffer::clientCopy();
	}

	postSort(camera);
}

void LLPipeline::stateSort(LLSpatialGroup* group, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!sSkipUpdate && group->changeLOD())
	{
		for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
		{
			LLDrawable* drawablep = *i;
			stateSort(drawablep, camera);
		}
	}
}

void LLPipeline::stateSort(LLSpatialBridge* bridge, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!sSkipUpdate && bridge->getSpatialGroup()->changeLOD())
	{
		bridge->updateDistance(camera);
	}
}

void LLPipeline::stateSort(LLDrawable* drawablep, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
		
	if (!drawablep
		|| drawablep->isDead() 
		|| !hasRenderType(drawablep->getRenderType()))
	{
		return;
	}
	
	if (gHideSelectedObjects)
	{
		if (drawablep->getVObj().notNull() &&
			drawablep->getVObj()->isSelected())
		{
			return;
		}
	}

	if (drawablep->isAvatar())
	{ //don't draw avatars beyond render distance or if we don't have a spatial group.
		if ((drawablep->getSpatialGroup() == NULL) || 
			(drawablep->getSpatialGroup()->mDistance > LLVOAvatar::sRenderDistance))
		{
			return;
		}

		LLVOAvatar* avatarp = (LLVOAvatar*) drawablep->getVObj().get();
		if (!avatarp->isVisible())
		{
			return;
		}
	}

	assertInitialized();

	if (hasRenderType(drawablep->mRenderType))
	{
		if (!drawablep->isState(LLDrawable::INVISIBLE|LLDrawable::FORCE_INVISIBLE))
		{
			drawablep->setVisible(camera, NULL, FALSE);
		}
		else if (drawablep->isState(LLDrawable::CLEAR_INVISIBLE))
		{
			// clear invisible flag here to avoid single frame glitch
			drawablep->clearState(LLDrawable::FORCE_INVISIBLE|LLDrawable::CLEAR_INVISIBLE);
		}
	}

	LLSpatialGroup* group = drawablep->getSpatialGroup();
	if (!group || group->changeLOD())
	{
		if (drawablep->isVisible() && !sSkipUpdate)
		{
			if (!drawablep->isActive())
			{
				drawablep->updateDistance(camera);
			}
			else if (drawablep->isAvatar())
			{
				drawablep->updateDistance(camera); // calls vobj->updateLOD() which calls LLVOAvatar::updateVisibility()
		}
		}
	}

	for (LLDrawable::face_list_t::iterator iter = drawablep->mFaces.begin();
			iter != drawablep->mFaces.end(); iter++)
	{
		LLFace* facep = *iter;

		if (facep->hasGeometry())
		{
			if (facep->getPool())
			{
				facep->getPool()->enqueue(facep);
			}
			else
			{
				break;
			}
		}
	}

	mNumVisibleFaces += drawablep->getNumFaces();
}


void forAllDrawables(LLCullResult::sg_list_t::iterator begin, 
					 LLCullResult::sg_list_t::iterator end,
					 void (*func)(LLDrawable*))
{
	for (LLCullResult::sg_list_t::iterator i = begin; i != end; ++i)
	{
		for (LLSpatialGroup::element_iter j = (*i)->getData().begin(); j != (*i)->getData().end(); ++j)
		{
			func(*j);	
		}
	}
}

void LLPipeline::forAllVisibleDrawables(void (*func)(LLDrawable*))
{
	forAllDrawables(sCull->beginDrawableGroups(), sCull->endDrawableGroups(), func);
	forAllDrawables(sCull->beginVisibleGroups(), sCull->endVisibleGroups(), func);
}

//function for creating scripted beacons
void renderScriptedBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& !vobj->isAvatar() 
		&& !vobj->getParent()
		&& vobj->flagScripted())
	{
		if (gPipeline.sRenderBeacons)
		{
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), gSavedSettings.getS32("DebugBeaconLineWidth"));
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void renderScriptedTouchBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& !vobj->isAvatar() 
		&& !vobj->getParent()
		&& vobj->flagScripted()
		&& vobj->flagHandleTouch())
	{
		if (gPipeline.sRenderBeacons)
		{
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), gSavedSettings.getS32("DebugBeaconLineWidth"));
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void renderPhysicalBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& !vobj->isAvatar() 
		//&& !vobj->getParent()
		&& vobj->usePhysics())
	{
		if (gPipeline.sRenderBeacons)
		{
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(0.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), gSavedSettings.getS32("DebugBeaconLineWidth"));
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void renderParticleBeacons(LLDrawable* drawablep)
{
	// Look for attachments, objects, etc.
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& vobj->isParticleSource())
	{
		if (gPipeline.sRenderBeacons)
		{
			LLColor4 light_blue(0.5f, 0.5f, 1.f, 0.5f);
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", light_blue, LLColor4(1.f, 1.f, 1.f, 0.5f), gSavedSettings.getS32("DebugBeaconLineWidth"));
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void renderSoundHighlights(LLDrawable* drawablep)
{
	// Look for attachments, objects, etc.
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj && vobj->isAudioSource())
	{
		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void LLPipeline::postSort(LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLFastTimer ftm(LLFastTimer::FTM_STATESORT_POSTSORT);

	assertInitialized();

	//rebuild drawable geometry
	for (LLCullResult::sg_list_t::iterator i = sCull->beginDrawableGroups(); i != sCull->endDrawableGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!sUseOcclusion || 
			!group->isState(LLSpatialGroup::OCCLUDED))
		{
			group->rebuildGeom();
		}
	}

	//rebuild groups
	sCull->assertDrawMapsEmpty();

	LLSpatialGroup::sNoDelete = FALSE;
	for (LLCullResult::sg_list_t::iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (sUseOcclusion && 
			group->isState(LLSpatialGroup::OCCLUDED))
		{
			continue;
		}
		
		group->rebuildGeom();
	}
	LLSpatialGroup::sNoDelete = TRUE;

	//build render map
	for (LLCullResult::sg_list_t::iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (sUseOcclusion && 
			group->isState(LLSpatialGroup::OCCLUDED))
		{
			continue;
		}
		
		for (LLSpatialGroup::draw_map_t::iterator j = group->mDrawMap.begin(); j != group->mDrawMap.end(); ++j)
		{
			LLSpatialGroup::drawmap_elem_t& src_vec = j->second;	
			
			for (LLSpatialGroup::drawmap_elem_t::iterator k = src_vec.begin(); k != src_vec.end(); ++k)
			{
				sCull->pushDrawInfo(j->first, *k);
			}
		}
		
		LLSpatialGroup::draw_map_t::iterator alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA);
		
		if (alpha != group->mDrawMap.end())
		{ //store alpha groups for sorting
			LLSpatialBridge* bridge = group->mSpatialPartition->asBridge();
			if (!sSkipUpdate)
			{
				if (bridge)
				{
					LLCamera trans_camera = bridge->transformCamera(camera);
					group->updateDistance(trans_camera);
				}
				else
				{
					group->updateDistance(camera);
				}
			}
			
			if (hasRenderType(LLDrawPool::POOL_ALPHA))
			{
				sCull->pushAlphaGroup(group);
			}
		}
	}
		
	{
		//sort by texture or bump map
		for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; ++i)
		{
			//if (!mRenderMap[i].empty())
			{
				if (i == LLRenderPass::PASS_BUMP)
				{
					std::sort(sCull->beginRenderMap(i), sCull->endRenderMap(i), LLDrawInfo::CompareBump());
				}
				else 
				{
					std::sort(sCull->beginRenderMap(i), sCull->endRenderMap(i), LLDrawInfo::CompareTexturePtrMatrix());
				}
			}
		}

		std::sort(sCull->beginAlphaGroups(), sCull->endAlphaGroups(), LLSpatialGroup::CompareDepthGreater());
	}

	// only render if the flag is set. The flag is only set if we are in edit mode or the toggle is set in the menus
	if (gSavedSettings.getBOOL("BeaconAlwaysOn"))
	{
		if (sRenderScriptedTouchBeacons)
		{
			// Only show the beacon on the root object.
			forAllVisibleDrawables(renderScriptedTouchBeacons);
		}
		else
		if (sRenderScriptedBeacons)
		{
			// Only show the beacon on the root object.
			forAllVisibleDrawables(renderScriptedBeacons);
		}

		if (sRenderPhysicalBeacons)
		{
			// Only show the beacon on the root object.
			forAllVisibleDrawables(renderPhysicalBeacons);
		}

		if (sRenderParticleBeacons)
		{
			forAllVisibleDrawables(renderParticleBeacons);
		}

		// If god mode, also show audio cues
		if (sRenderSoundBeacons && gAudiop)
		{
			// Walk all sound sources and render out beacons for them. Note, this isn't done in the ForAllVisibleDrawables function, because some are not visible.
			LLAudioEngine::source_map::iterator iter;
			for (iter = gAudiop->mAllSources.begin(); iter != gAudiop->mAllSources.end(); ++iter)
			{
				LLAudioSource *sourcep = iter->second;

				LLVector3d pos_global = sourcep->getPositionGlobal();
				LLVector3 pos = gAgent.getPosAgentFromGlobal(pos_global);
				if (gPipeline.sRenderBeacons)
				{
					//pos += LLVector3(0.f, 0.f, 0.2f);
					gObjectList.addDebugBeacon(pos, "", LLColor4(1.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), gSavedSettings.getS32("DebugBeaconLineWidth"));
				}
			}
			// now deal with highlights for all those seeable sound sources
			forAllVisibleDrawables(renderSoundHighlights);
		}
	}

	// If managing your telehub, draw beacons at telehub and currently selected spawnpoint.
	if (LLFloaterTelehub::renderBeacons())
	{
		LLFloaterTelehub::addBeacons();
	}

	mSelectedFaces.clear();
	
	// Draw face highlights for selected faces.
	if (LLSelectMgr::getInstance()->getTEMode())
	{
		struct f : public LLSelectedTEFunctor
		{
			virtual bool apply(LLViewerObject* object, S32 te)
			{
				if (object->mDrawable)
				{
					gPipeline.mSelectedFaces.push_back(object->mDrawable->getFace(te));
				}
				return true;
			}
		} func;
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);
	}

	LLSpatialGroup::sNoDelete = FALSE;
}


void render_hud_elements()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_UI);
	gPipeline.disableLights();		
	
	LLGLDisable fog(GL_FOG);
	LLGLSUIDefault gls_ui;

	LLGLEnable stencil(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 255, 0xFFFFFFFF);
	glStencilMask(0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	
	gGL.color4f(1,1,1,1);
	if (!LLPipeline::sReflectionRender && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		LLGLEnable multisample(GL_MULTISAMPLE_ARB);
		gViewerWindow->renderSelections(FALSE, FALSE, FALSE); // For HUD version in render_ui_3d()
	
		// Draw the tracking overlays
		LLTracker::render3D();
		
		// Show the property lines
		LLWorld::getInstance()->renderPropertyLines();
		LLViewerParcelMgr::getInstance()->render();
		LLViewerParcelMgr::getInstance()->renderParcelCollision();
	
		// Render debugging beacons.
		//gObjectList.renderObjectBeacons();
		//LLHUDObject::renderAll();
		//gObjectList.resetObjectBeacons();
	}
	else if (gForceRenderLandFence)
	{
		// This is only set when not rendering the UI, for parcel snapshots
		LLViewerParcelMgr::getInstance()->render();
	}
	else if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
	{
		LLHUDText::renderAllHUD();
	}
	gGL.flush();
}

void LLPipeline::renderHighlights()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	assertInitialized();

	// Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
	// Render highlighted faces.
	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLColor4 color(1.f, 1.f, 1.f, 0.5f);
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	disableLights();

	if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
	{
		gHighlightProgram.bind();
		gHighlightProgram.vertexAttrib4f(LLViewerShaderMgr::MATERIAL_COLOR,1,1,1,0.5f);
	}
	
	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Make sure the selection image gets downloaded and decoded
		if (!mFaceSelectImagep)
		{
			mFaceSelectImagep = gImageList.getImage(IMG_FACE_SELECT);
		}
		mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

		U32 count = mSelectedFaces.size();
		for (U32 i = 0; i < count; i++)
		{
			LLFace *facep = mSelectedFaces[i];
			if (!facep || facep->getDrawable()->isDead())
			{
				llerrs << "Bad face on selection" << llendl;
				return;
			}
			
			facep->renderSelected(mFaceSelectImagep, color);
		}
	}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Paint 'em red!
		color.setVec(1.f, 0.f, 0.f, 0.5f);
		if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
		{
			gHighlightProgram.vertexAttrib4f(LLViewerShaderMgr::MATERIAL_COLOR,1,0,0,0.5f);
		}
		int count = mHighlightFaces.size();
		for (S32 i = 0; i < count; i++)
		{
			LLFace* facep = mHighlightFaces[i];
			facep->renderSelected(LLViewerImage::sNullImagep, color);
		}
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();

	if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0)
	{
		gHighlightProgram.unbind();
	}
}

void LLPipeline::renderGeom(LLCamera& camera, BOOL forceVBOUpdate)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLFastTimer t(LLFastTimer::FTM_RENDER_GEOMETRY);

	assertInitialized();

	F64 saved_modelview[16];
	F64 saved_projection[16];

	//HACK: preserve/restore matrices around HUD render
	if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
	{
		for (U32 i = 0; i < 16; i++)
		{
			saved_modelview[i] = gGLModelView[i];
			saved_projection[i] = gGLProjection[i];
		}
	}

	///////////////////////////////////////////
	//
	// Sync and verify GL state
	//
	//

	stop_glerror();
	gFrameStats.start(LLFrameStats::RENDER_SYNC);

	glEnableClientState(GL_VERTEX_ARRAY);

	LLVertexBuffer::unbind();

	// Do verification of GL state
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
	if (mRenderDebugMask & RENDER_DEBUG_VERIFY)
	{
		if (!verify())
		{
			llerrs << "Pipeline verification failed!" << llendl;
		}
	}

	
	
	//by bao
	//fake vertex buffer updating
	//to guaranttee at least updating one VBO buffer every frame
	//to walk around the bug caused by ATI card --> DEV-3855
	//
	if(forceVBOUpdate)
		gSky.mVOSkyp->updateDummyVertexBuffer() ;

	gFrameStats.start(LLFrameStats::RENDER_GEOM);

	// Initialize lots of GL state to "safe" values
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	LLGLSPipeline gls_pipeline;
	LLGLEnable multisample(GL_MULTISAMPLE_ARB);

	LLGLState gls_color_material(GL_COLOR_MATERIAL, mLightingDetail < 2);
				
	// Toggle backface culling for debugging
	LLGLEnable cull_face(mBackfaceCull ? GL_CULL_FACE : 0);
	// Set fog
	BOOL use_fog = hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOG);
	LLGLEnable fog_enable(use_fog &&
						  !gPipeline.canUseWindLightShadersOnObjects() ? GL_FOG : 0);
	gSky.updateFog(camera.getFar());
	if (!use_fog)
	{
		sUnderWaterRender = FALSE;
	}

	LLViewerImage::sDefaultImagep->bind(0);
	LLViewerImage::sDefaultImagep->setClamp(FALSE, FALSE);
	
	//////////////////////////////////////////////
	//
	// Actually render all of the geometry
	//
	//	
	stop_glerror();
	BOOL occlude = sUseOcclusion > 1;
	
	U32 cur_type = 0;

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PICKING))
	{
		gObjectList.renderObjectsForSelect(camera, gViewerWindow->getVirtualWindowRect());
	}
	else if (gSavedSettings.getBOOL("RenderDeferred"))
	{
		renderGeomDeferred();
	}
	else
	{
		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			if (hasRenderType(poolp->getType()))
			{
				poolp->prerender();
			}
		}


		LLFastTimer t(LLFastTimer::FTM_POOLS);
		calcNearbyLights(camera);
		setupHWLights(NULL);

		pool_set_t::iterator iter1 = mPools.begin();
		while ( iter1 != mPools.end() )
		{
			LLDrawPool *poolp = *iter1;
			
			cur_type = poolp->getType();

			if (occlude && cur_type > LLDrawPool::POOL_AVATAR)
			{
				occlude = FALSE;
				gGLLastMatrix = NULL;
				glLoadMatrixd(gGLModelView);
				doOcclusion(camera);
			}

			pool_set_t::iterator iter2 = iter1;
			if (hasRenderType(poolp->getType()) && poolp->getNumPasses() > 0)
			{
				LLFastTimer t(LLFastTimer::FTM_POOLRENDER);

				gGLLastMatrix = NULL;
				glLoadMatrixd(gGLModelView);
			
				for( S32 i = 0; i < poolp->getNumPasses(); i++ )
				{
					poolp->beginRenderPass(i);
					for (iter2 = iter1; iter2 != mPools.end(); iter2++)
					{
						LLDrawPool *p = *iter2;
						if (p->getType() != cur_type)
						{
							break;
						}
						
						p->render(i);
					}
					poolp->endRenderPass(i);
					LLVertexBuffer::unbind();
					if (gDebugGL || gDebugPipeline)
					{
						GLint depth;
						glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
						if (depth > 3)
						{
							llerrs << "GL matrix stack corrupted!" << llendl;
						}
						std::string msg = llformat("%s pass %d", gPoolNames[cur_type].c_str(), i);
						LLGLState::checkStates(msg);
						LLGLState::checkTextureChannels(msg);
						LLGLState::checkClientArrays(msg);
					}
				}
			}
			else
			{
				// Skip all pools of this type
				for (iter2 = iter1; iter2 != mPools.end(); iter2++)
				{
					LLDrawPool *p = *iter2;
					if (p->getType() != cur_type)
					{
						break;
					}
				}
			}
			iter1 = iter2;
			stop_glerror();
		}
	}

	LLVertexBuffer::unbind();
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLModelView);

	if (occlude)
	{
		occlude = FALSE;
		gGLLastMatrix = NULL;
		glLoadMatrixd(gGLModelView);
		doOcclusion(camera);
	}

	stop_glerror();
		
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	if (!sReflectionRender)
	{
		renderHighlights();
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();

	renderDebug();

	LLVertexBuffer::unbind();
	
	if (!LLPipeline::sReflectionRender && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		// Render debugging beacons.
		gObjectList.renderObjectBeacons();
		LLHUDObject::renderAll();
		gObjectList.resetObjectBeacons();
	}

	//HACK: preserve/restore matrices around HUD render
	if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
	{
		for (U32 i = 0; i < 16; i++)
		{
			gGLModelView[i] = saved_modelview[i];
			gGLProjection[i] = saved_projection[i];
		}
	}

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
}

void LLPipeline::renderGeomDeferred()
{
	gDeferredDiffuseProgram.bind();
	gPipeline.renderObjects(LLRenderPass::PASS_SIMPLE, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_NORMAL, TRUE);
	gDeferredDiffuseProgram.unbind();
}

void LLPipeline::addTrianglesDrawn(S32 count)
{
	assertInitialized();
	mTrianglesDrawn += count;
	mBatchCount++;
	mMaxBatchSize = llmax(mMaxBatchSize, count);
	mMinBatchSize = llmin(mMinBatchSize, count);

	if (LLPipeline::sRenderFrameTest)
	{
		gViewerWindow->getWindow()->swapBuffers();
		ms_sleep(16);
	}
}

void LLPipeline::renderDebug()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	assertInitialized();

	gGL.color4f(1,1,1,1);

	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLModelView);
	gGL.setColorMask(true, false);

	// Debug stuff.
	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				if (hasRenderType(part->mDrawableType))
				{
					part->renderDebug();
				}
			}
		}
	}

	for (LLCullResult::bridge_list_t::iterator i = sCull->beginVisibleBridge(); i != sCull->endVisibleBridge(); ++i)
	{
		LLSpatialBridge* bridge = *i;
		if (!bridge->isDead() && !bridge->isState(LLSpatialGroup::OCCLUDED) && hasRenderType(bridge->mDrawableType))
		{
			glPushMatrix();
			glMultMatrixf((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
			bridge->renderDebug();
			glPopMatrix();
		}
	}

	if (mRenderDebugMask & RENDER_DEBUG_COMPOSITION)
	{
		// Debug composition layers
		F32 x, y;

		LLGLSNoTexture gls_no_texture;

		if (gAgent.getRegion())
		{
			gGL.begin(LLVertexBuffer::POINTS);
			// Draw the composition layer for the region that I'm in.
			for (x = 0; x <= 260; x++)
			{
				for (y = 0; y <= 260; y++)
				{
					if ((x > 255) || (y > 255))
					{
						gGL.color4f(1.f, 0.f, 0.f, 1.f);
					}
					else
					{
						gGL.color4f(0.f, 0.f, 1.f, 1.f);
					}
					F32 z = gAgent.getRegion()->getCompositionXY((S32)x, (S32)y);
					z *= 5.f;
					z += 50.f;
					gGL.vertex3f(x, y, z);
				}
			}
			gGL.end();
		}
	}
	gGL.flush();
}

void LLPipeline::renderForSelect(std::set<LLViewerObject*>& objects, BOOL render_transparent, const LLRect& screen_rect)
{
	assertInitialized();

	gGL.setColorMask(true, false);
	gPipeline.resetDrawOrders();

	for (std::set<LLViewerObject*>::iterator iter = objects.begin(); iter != objects.end(); ++iter)
	{
		stateSort((*iter)->mDrawable, *LLViewerCamera::getInstance());
	}

	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	
	
	glMatrixMode(GL_MODELVIEW);

	LLGLSDefault gls_default;
	LLGLSObjectSelect gls_object_select;
	LLGLDepthTest gls_depth(GL_TRUE,GL_TRUE);
	disableLights();
	
	LLVertexBuffer::unbind();

	//for each drawpool
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
	U32 last_type = 0;
	
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->isFacePool() && hasRenderType(poolp->getType()))
		{
			LLFacePool* face_pool = (LLFacePool*) poolp;
			face_pool->renderForSelect();
			LLVertexBuffer::unbind();
			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);

			if (poolp->getType() != last_type)
			{
				last_type = poolp->getType();
				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();
			}
		}
	}	

	LLGLEnable alpha_test(GL_ALPHA_TEST);
	if (render_transparent)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.f);
	}
	else
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.2f);
	}

	gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_VERT_COLOR);
	gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_ALPHA, LLTexUnit::TBS_VERT_ALPHA);

	U32 prim_mask = LLVertexBuffer::MAP_VERTEX | 
					LLVertexBuffer::MAP_TEXCOORD;

	for (std::set<LLViewerObject*>::iterator i = objects.begin(); i != objects.end(); ++i)
	{
		LLViewerObject* vobj = *i;
		LLDrawable* drawable = vobj->mDrawable;
		if (vobj->isDead() || 
			vobj->isHUDAttachment() ||
			(gHideSelectedObjects && vobj->isSelected()) ||
			drawable->isDead() || 
			!hasRenderType(drawable->getRenderType()))
		{
			continue;
		}

		for (S32 j = 0; j < drawable->getNumFaces(); ++j)
		{
			LLFace* facep = drawable->getFace(j);
			if (!facep->getPool())
			{
				facep->renderForSelect(prim_mask);
			}
		}
	}

	// pick HUD objects
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp && sShowHUDAttachments)
	{
		glh::matrix4f save_proj(glh_get_current_projection());
		glh::matrix4f save_model(glh_get_current_modelview());

		setup_hud_matrices(screen_rect);
		for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
			 iter != avatarp->mAttachmentPoints.end(); )
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachmentp = curiter->second;
			if (attachmentp->getIsHUDAttachment())
			{
				LLViewerObject* objectp = attachmentp->getObject();
				if (objectp)
				{
					LLDrawable* drawable = objectp->mDrawable;
					if (drawable->isDead())
					{
						continue;
					}

					for (S32 j = 0; j < drawable->getNumFaces(); ++j)
					{
						LLFace* facep = drawable->getFace(j);
						if (!facep->getPool())
						{
							facep->renderForSelect(prim_mask);
						}
					}

					//render child faces
					LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
					for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
						 iter != child_list.end(); iter++)
					{
						LLViewerObject* child = *iter;
						LLDrawable* child_drawable = child->mDrawable;
						for (S32 l = 0; l < child_drawable->getNumFaces(); ++l)
						{
							LLFace* facep = child_drawable->getFace(l);
							if (!facep->getPool())
							{
								facep->renderForSelect(prim_mask);
							}
						}
					}
				}	
			}
		}

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(save_proj.m);
		glh_set_current_projection(save_proj);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(save_model.m);
		glh_set_current_modelview(save_model);

	
	}

	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	
	LLVertexBuffer::unbind();
	
	gGL.setColorMask(true, true);
}

void LLPipeline::rebuildPools()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	assertInitialized();

	S32 max_count = mPools.size();
	pool_set_t::iterator iter1 = mPools.upper_bound(mLastRebuildPool);
	while(max_count > 0 && mPools.size() > 0) // && num_rebuilds < MAX_REBUILDS)
	{
		if (iter1 == mPools.end())
		{
			iter1 = mPools.begin();
		}
		LLDrawPool* poolp = *iter1;

		if (poolp->isDead())
		{
			mPools.erase(iter1++);
			removeFromQuickLookup( poolp );
			if (poolp == mLastRebuildPool)
			{
				mLastRebuildPool = NULL;
			}
			delete poolp;
		}
		else
		{
			mLastRebuildPool = poolp;
			iter1++;
		}
		max_count--;
	}

	if (gAgent.getAvatarObject())
	{
		gAgent.getAvatarObject()->rebuildHUD();
	}
}

void LLPipeline::addToQuickLookup( LLDrawPool* new_poolp )
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	assertInitialized();

	switch( new_poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		if (mSimplePool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate simple pool." << llendl;
		}
		else
		{
			mSimplePool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_INVISIBLE:
		if (mInvisiblePool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate simple pool." << llendl;
		}
		else
		{
			mInvisiblePool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_GLOW:
		if (mGlowPool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate glow pool." << llendl;
		}
		else
		{
			mGlowPool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_TREE:
		mTreePools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;
 
	case LLDrawPool::POOL_TERRAIN:
		mTerrainPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_BUMP:
		if (mBumpPool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate bump pool." << llendl;
		}
		else
		{
			mBumpPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_ALPHA:
		if( mAlphaPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Alpha pool" << llendl;
		}
		else
		{
			mAlphaPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		if( mSkyPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Sky pool" << llendl;
		}
		else
		{
			mSkyPool = new_poolp;
		}
		break;
	
	case LLDrawPool::POOL_WATER:
		if( mWaterPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Water pool" << llendl;
		}
		else
		{
			mWaterPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_GROUND:
		if( mGroundPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Ground Pool" << llendl;
		}
		else
		{ 
			mGroundPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_WL_SKY:
		if( mWLSkyPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate WLSky Pool" << llendl;
		}
		else
		{ 
			mWLSkyPool = new_poolp;
		}
		break;

	default:
		llassert(0);
		llwarns << "Invalid Pool Type in  LLPipeline::addPool()" << llendl;
		break;
	}
}

void LLPipeline::removePool( LLDrawPool* poolp )
{
	assertInitialized();
	removeFromQuickLookup(poolp);
	mPools.erase(poolp);
	delete poolp;
}

void LLPipeline::removeFromQuickLookup( LLDrawPool* poolp )
{
	assertInitialized();
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	switch( poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		llassert(mSimplePool == poolp);
		mSimplePool = NULL;
		break;

	case LLDrawPool::POOL_INVISIBLE:
		llassert(mInvisiblePool == poolp);
		mInvisiblePool = NULL;
		break;

	case LLDrawPool::POOL_WL_SKY:
		llassert(mWLSkyPool == poolp);
		mWLSkyPool = NULL;
		break;

	case LLDrawPool::POOL_GLOW:
		llassert(mGlowPool == poolp);
		mGlowPool = NULL;
		break;

	case LLDrawPool::POOL_TREE:
		#ifdef _DEBUG
			{
				BOOL found = mTreePools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTreePools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_TERRAIN:
		#ifdef _DEBUG
			{
				BOOL found = mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_BUMP:
		llassert( poolp == mBumpPool );
		mBumpPool = NULL;
		break;
	
	case LLDrawPool::POOL_ALPHA:
		llassert( poolp == mAlphaPool );
		mAlphaPool = NULL;
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		llassert( poolp == mSkyPool );
		mSkyPool = NULL;
		break;

	case LLDrawPool::POOL_WATER:
		llassert( poolp == mWaterPool );
		mWaterPool = NULL;
		break;

	case LLDrawPool::POOL_GROUND:
		llassert( poolp == mGroundPool );
		mGroundPool = NULL;
		break;

	default:
		llassert(0);
		llwarns << "Invalid Pool Type in  LLPipeline::removeFromQuickLookup() type=" << poolp->getType() << llendl;
		break;
	}
}

void LLPipeline::resetDrawOrders()
{
	assertInitialized();
	// Iterate through all of the draw pools and rebuild them.
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		poolp->resetDrawOrders();
	}
}

//============================================================================
// Once-per-frame setup of hardware lights,
// including sun/moon, avatar backlight, and up to 6 local lights

void LLPipeline::setupAvatarLights(BOOL for_edit)
{
	assertInitialized();

	if (for_edit)
	{
		LLColor4 diffuse(0.8f, 0.8f, 0.8f, 0.f);
		LLVector4 light_pos_cam(-8.f, 0.25f, 10.f, 0.f);  // w==0 => directional light
		LLMatrix4 camera_mat = LLViewerCamera::getInstance()->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();
		LLVector4 light_pos = light_pos_cam * camera_rot;
		
		light_pos.normVec();

		mHWLightColors[1] = diffuse;
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  diffuse.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, LLColor4::black.mV);
		glLightfv(GL_LIGHT1, GL_POSITION, light_pos.mV); 
		glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION, 	 0.0f);
		glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 		 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_CUTOFF, 			 180.0f);
	}
	else if (gAvatarBacklight) // Always true (unless overridden in a devs .ini)
	{
		LLVector3 opposite_pos = -1.f * mSunDir;
		LLVector3 orthog_light_pos = mSunDir % LLVector3::z_axis;
		LLVector4 backlight_pos = LLVector4(lerp(opposite_pos, orthog_light_pos, 0.3f), 0.0f);
		backlight_pos.normVec();
			
		LLColor4 light_diffuse = mSunDiffuse;
		LLColor4 backlight_diffuse(1.f - light_diffuse.mV[VRED], 1.f - light_diffuse.mV[VGREEN], 1.f - light_diffuse.mV[VBLUE], 1.f);
		F32 max_component = 0.001f;
		for (S32 i = 0; i < 3; i++)
		{
			if (backlight_diffuse.mV[i] > max_component)
			{
				max_component = backlight_diffuse.mV[i];
			}
		}
		F32 backlight_mag;
		if (gSky.getSunDirection().mV[2] >= NIGHTTIME_ELEVATION_COS)
		{
			backlight_mag = BACKLIGHT_DAY_MAGNITUDE_OBJECT;
		}
		else
		{
			backlight_mag = BACKLIGHT_NIGHT_MAGNITUDE_OBJECT;
		}
		backlight_diffuse *= backlight_mag / max_component;

		mHWLightColors[1] = backlight_diffuse;
		glLightfv(GL_LIGHT1, GL_POSITION, backlight_pos.mV); // this is just sun/moon direction
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  backlight_diffuse.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, LLColor4::black.mV);
		glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_EXPONENT,         0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,           180.0f);
	}
	else
	{
		mHWLightColors[1] = LLColor4::black;
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  LLColor4::black.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, LLColor4::black.mV);
	}
}

static F32 calc_light_dist(LLVOVolume* light, const LLVector3& cam_pos, F32 max_dist)
{
	F32 inten = light->getLightIntensity();
	if (inten < .001f)
	{
		return max_dist;
	}
	F32 radius = light->getLightRadius();
	BOOL selected = light->isSelected();
	LLVector3 dpos = light->getRenderPosition() - cam_pos;
	F32 dist2 = dpos.lengthSquared();
	if (!selected && dist2 > (max_dist + radius)*(max_dist + radius))
	{
		return max_dist;
	}
	F32 dist = fsqrtf(dist2);
	dist *= 1.f / inten;
	dist -= radius;
	if (selected)
	{
		dist -= 10000.f; // selected lights get highest priority
	}
	if (light->mDrawable.notNull() && light->mDrawable->isState(LLDrawable::ACTIVE))
	{
		// moving lights get a little higher priority (too much causes artifacts)
		dist -= light->getLightRadius()*0.25f;
	}
	return dist;
}

void LLPipeline::calcNearbyLights(LLCamera& camera)
{
	assertInitialized();

	if (LLPipeline::sReflectionRender)
	{
		return;
	}

	if (mLightingDetail >= 1)
	{
		// mNearbyLight (and all light_set_t's) are sorted such that
		// begin() == the closest light and rbegin() == the farthest light
		const S32 MAX_LOCAL_LIGHTS = 6;
// 		LLVector3 cam_pos = gAgent.getCameraPositionAgent();
		LLVector3 cam_pos = LLViewerJoystick::getInstance()->getOverrideCamera() ?
						camera.getOrigin() : 
						gAgent.getPositionAgent();

		F32 max_dist = LIGHT_MAX_RADIUS * 4.f; // ignore enitrely lights > 4 * max light rad
		
		// UPDATE THE EXISTING NEARBY LIGHTS
		if (!LLPipeline::sSkipUpdate)
		{
			light_set_t cur_nearby_lights;
			for (light_set_t::iterator iter = mNearbyLights.begin();
				iter != mNearbyLights.end(); iter++)
			{
				const Light* light = &(*iter);
				LLDrawable* drawable = light->drawable;
				LLVOVolume* volight = drawable->getVOVolume();
				if (!volight || !drawable->isState(LLDrawable::LIGHT))
				{
					drawable->clearState(LLDrawable::NEARBY_LIGHT);
					continue;
				}
				if (light->fade <= -LIGHT_FADE_TIME)
				{
					drawable->clearState(LLDrawable::NEARBY_LIGHT);
					continue;
				}
				if (!sRenderAttachedLights && volight && volight->isAttachment())
				{
					drawable->clearState(LLDrawable::NEARBY_LIGHT);
					continue;
				}

				F32 dist = calc_light_dist(volight, cam_pos, max_dist);
				cur_nearby_lights.insert(Light(drawable, dist, light->fade));
			}
			mNearbyLights = cur_nearby_lights;
		}
		
		// FIND NEW LIGHTS THAT ARE IN RANGE
		light_set_t new_nearby_lights;
		for (LLDrawable::drawable_set_t::iterator iter = mLights.begin();
			 iter != mLights.end(); ++iter)
		{
			LLDrawable* drawable = *iter;
			LLVOVolume* light = drawable->getVOVolume();
			if (!light || drawable->isState(LLDrawable::NEARBY_LIGHT))
			{
				continue;
			}
			if (light->isHUDAttachment())
			{
				continue; // no lighting from HUD objects
			}
			F32 dist = calc_light_dist(light, cam_pos, max_dist);
			if (dist >= max_dist)
			{
				continue;
			}
			new_nearby_lights.insert(Light(drawable, dist, 0.f));
			if (new_nearby_lights.size() > (U32)MAX_LOCAL_LIGHTS)
			{
				new_nearby_lights.erase(--new_nearby_lights.end());
				const Light& last = *new_nearby_lights.rbegin();
				max_dist = last.dist;
			}
		}

		// INSERT ANY NEW LIGHTS
		for (light_set_t::iterator iter = new_nearby_lights.begin();
			 iter != new_nearby_lights.end(); iter++)
		{
			const Light* light = &(*iter);
			if (mNearbyLights.size() < (U32)MAX_LOCAL_LIGHTS)
			{
				mNearbyLights.insert(*light);
				((LLDrawable*) light->drawable)->setState(LLDrawable::NEARBY_LIGHT);
			}
			else
			{
				// crazy cast so that we can overwrite the fade value
				// even though gcc enforces sets as const
				// (fade value doesn't affect sort so this is safe)
				Light* farthest_light = ((Light*) (&(*(mNearbyLights.rbegin()))));
				if (light->dist < farthest_light->dist)
				{
					if (farthest_light->fade >= 0.f)
					{
						farthest_light->fade = -gFrameIntervalSeconds;
					}
				}
				else
				{
					break; // none of the other lights are closer
				}
			}
		}
		
	}
}

void LLPipeline::setupHWLights(LLDrawPool* pool)
{
	assertInitialized();

	// Ambient
	LLColor4 ambient = gSky.getTotalAmbientColor();
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient.mV);

	// Light 0 = Sun or Moon (All objects)
	{
		if (gSky.getSunDirection().mV[2] >= NIGHTTIME_ELEVATION_COS)
		{
			mSunDir.setVec(gSky.getSunDirection());
			mSunDiffuse.setVec(gSky.getSunDiffuseColor());
		}
		else
		{
			mSunDir.setVec(gSky.getMoonDirection());
			mSunDiffuse.setVec(gSky.getMoonDiffuseColor());
		}

		F32 max_color = llmax(mSunDiffuse.mV[0], mSunDiffuse.mV[1], mSunDiffuse.mV[2]);
		if (max_color > 1.f)
		{
			mSunDiffuse *= 1.f/max_color;
		}
		mSunDiffuse.clamp();

		LLVector4 light_pos(mSunDir, 0.0f);
		LLColor4 light_diffuse = mSunDiffuse;
		mHWLightColors[0] = light_diffuse;
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos.mV); // this is just sun/moon direction
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse.mV);
		glLightfv(GL_LIGHT0, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(GL_LIGHT0, GL_SPECULAR, LLColor4::black.mV);
		glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT0, GL_SPOT_EXPONENT,         0.0f);
		glLightf (GL_LIGHT0, GL_SPOT_CUTOFF,           180.0f);
	}
	
	// Light 1 = Backlight (for avatars)
	// (set by enableLightsAvatar)
	
	S32 cur_light = 2;
	
	// Nearby lights = LIGHT 2-7

	mLightMovingMask = 0;
	
	if (mLightingDetail >= 1)
	{
		for (light_set_t::iterator iter = mNearbyLights.begin();
			 iter != mNearbyLights.end(); ++iter)
		{
			LLDrawable* drawable = iter->drawable;
			LLVOVolume* light = drawable->getVOVolume();
			if (!light)
			{
				continue;
			}
			if (drawable->isState(LLDrawable::ACTIVE))
			{
				mLightMovingMask |= (1<<cur_light);
			}
			
			LLColor4  light_color = light->getLightColor();
			light_color.mV[3] = 0.0f;

			F32 fade = iter->fade;
			if (fade < LIGHT_FADE_TIME)
			{
				// fade in/out light
				if (fade >= 0.f)
				{
					fade = fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade += gFrameIntervalSeconds;
				}
				else
				{
					fade = 1.f + fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade -= gFrameIntervalSeconds;
				}
				fade = llclamp(fade,0.f,1.f);
				light_color *= fade;
			}

			LLVector3 light_pos(light->getRenderPosition());
			LLVector4 light_pos_gl(light_pos, 1.0f);
	
			F32 light_radius = llmax(light->getLightRadius(), 0.001f);
			F32 atten, quad;

#if 0 //1.9.1
			if (pool->getVertexShaderLevel() > 0)
			{
				atten = light_radius;
				quad = llmax(light->getLightFalloff(), 0.0001f);
			}
			else
#endif
			{
				F32 x = (3.f * (1.f + light->getLightFalloff()));
				atten = x / (light_radius); // % of brightness at radius
				quad = 0.0f;
			}
			mHWLightColors[cur_light] = light_color;
			S32 gllight = GL_LIGHT0+cur_light;
			glLightfv(gllight, GL_POSITION, light_pos_gl.mV);
			glLightfv(gllight, GL_DIFFUSE,  light_color.mV);
			glLightfv(gllight, GL_AMBIENT,  LLColor4::black.mV);
			glLightfv(gllight, GL_SPECULAR, LLColor4::black.mV);
			glLightf (gllight, GL_CONSTANT_ATTENUATION,   0.0f);
			glLightf (gllight, GL_LINEAR_ATTENUATION,     atten);
			glLightf (gllight, GL_QUADRATIC_ATTENUATION,  quad);
			glLightf (gllight, GL_SPOT_EXPONENT,          0.0f);
			glLightf (gllight, GL_SPOT_CUTOFF,            180.0f);
			cur_light++;
			if (cur_light >= 8)
			{
				break; // safety
			}
		}
	}
	for ( ; cur_light < 8 ; cur_light++)
	{
		mHWLightColors[cur_light] = LLColor4::black;
		S32 gllight = GL_LIGHT0+cur_light;
		glLightfv(gllight, GL_DIFFUSE,  LLColor4::black.mV);
		glLightfv(gllight, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(gllight, GL_SPECULAR, LLColor4::black.mV);
	}

	if (gAgent.getAvatarObject() &&
		gAgent.getAvatarObject()->mSpecialRenderMode == 3)
	{
		LLColor4  light_color = LLColor4::white;
		light_color.mV[3] = 0.0f;

		LLVector3 light_pos(LLViewerCamera::getInstance()->getOrigin());
		LLVector4 light_pos_gl(light_pos, 1.0f);

		F32 light_radius = 16.f;
		F32 atten, quad;

		{
			F32 x = 3.f;
			atten = x / (light_radius); // % of brightness at radius
			quad = 0.0f;
		}
		//mHWLightColors[cur_light] = light_color;
		S32 gllight = GL_LIGHT2;
		glLightfv(gllight, GL_POSITION, light_pos_gl.mV);
		glLightfv(gllight, GL_DIFFUSE,  light_color.mV);
		glLightfv(gllight, GL_AMBIENT,  LLColor4::black.mV);
		glLightfv(gllight, GL_SPECULAR, LLColor4::black.mV);
		glLightf (gllight, GL_CONSTANT_ATTENUATION,   0.0f);
		glLightf (gllight, GL_LINEAR_ATTENUATION,     atten);
		glLightf (gllight, GL_QUADRATIC_ATTENUATION,  quad);
		glLightf (gllight, GL_SPOT_EXPONENT,          0.0f);
		glLightf (gllight, GL_SPOT_CUTOFF,            180.0f);
	}

	// Init GL state
	glDisable(GL_LIGHTING);
	for (S32 gllight=GL_LIGHT0; gllight<=GL_LIGHT7; gllight++)
	{
		glDisable(gllight);
	}
	mLightMask = 0;
}

void LLPipeline::enableLights(U32 mask)
{
	assertInitialized();
	if (mLightingDetail == 0)
	{
		mask &= 0xf003; // sun and backlight only (and fullbright bit)
	}
	if (mLightMask != mask)
	{
		if (!mLightMask)
		{
			glEnable(GL_LIGHTING);
		}
		if (mask)
		{
			for (S32 i=0; i<8; i++)
			{
				if (mask & (1<<i))
				{
					glEnable(GL_LIGHT0 + i);
					glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  mHWLightColors[i].mV);
				}
				else
				{
					glDisable(GL_LIGHT0 + i);
					glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  LLColor4::black.mV);
				}
			}
		}
		else
		{
			glDisable(GL_LIGHTING);
		}
		mLightMask = mask;
		LLColor4 ambient = gSky.getTotalAmbientColor();
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient.mV);
	}
}

void LLPipeline::enableLightsStatic()
{
	assertInitialized();
	U32 mask = 0x01; // Sun
	if (mLightingDetail >= 2)
	{
		mask |= mLightMovingMask; // Hardware moving lights
		glColor4f(0.f, 0.f, 0.f, 1.0f); // no local lighting by default
	}
	else
	{
		mask |= 0xff & (~2); // Hardware local lights
	}
	enableLights(mask);
}

void LLPipeline::enableLightsDynamic()
{
	assertInitialized();
	U32 mask = 0xff & (~2); // Local lights
	enableLights(mask);
	if (mLightingDetail >= 2)
	{
		glColor4f(0.f, 0.f, 0.f, 1.f); // no local lighting by default
	}

	LLVOAvatar* avatarp = gAgent.getAvatarObject();

	if (avatarp && getLightingDetail() <= 0)
	{
		if (avatarp->mSpecialRenderMode == 0) // normal
		{
			gPipeline.enableLightsAvatar();
		}
		else if (avatarp->mSpecialRenderMode >= 1)  // anim preview
		{
			gPipeline.enableLightsAvatarEdit(LLColor4(0.7f, 0.6f, 0.3f, 1.f));
		}
	}
}

void LLPipeline::enableLightsAvatar()
{
	U32 mask = 0xff; // All lights
	setupAvatarLights(FALSE);
	enableLights(mask);
}

void LLPipeline::enableLightsAvatarEdit(const LLColor4& color)
{
	U32 mask = 0x2002; // Avatar backlight only, set ambient
	setupAvatarLights(TRUE);
	enableLights(mask);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,color.mV);
}

void LLPipeline::enableLightsFullbright(const LLColor4& color)
{
	assertInitialized();
	U32 mask = 0x1000; // Non-0 mask, set ambient
	enableLights(mask);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,color.mV);
	if (mLightingDetail >= 2)
	{
		glColor4f(0.f, 0.f, 0.f, 1.f); // no local lighting by default
	}
}

void LLPipeline::disableLights()
{
	enableLights(0); // no lighting (full bright)
	glColor4f(1.f, 1.f, 1.f, 1.f); // lighting color = white by default
}

//============================================================================

class LLMenuItemGL;
class LLInvFVBridge;
struct cat_folder_pair;
class LLVOBranch;
class LLVOLeaf;

void LLPipeline::findReferences(LLDrawable *drawablep)
{
	assertInitialized();
	if (mLights.find(drawablep) != mLights.end())
	{
		llinfos << "In mLights" << llendl;
	}
	if (std::find(mMovedList.begin(), mMovedList.end(), drawablep) != mMovedList.end())
	{
		llinfos << "In mMovedList" << llendl;
	}
	if (std::find(mShiftList.begin(), mShiftList.end(), drawablep) != mShiftList.end())
	{
		llinfos << "In mShiftList" << llendl;
	}
	if (mRetexturedList.find(drawablep) != mRetexturedList.end())
	{
		llinfos << "In mRetexturedList" << llendl;
	}
	
	if (mActiveQ.find(drawablep) != mActiveQ.end())
	{
		llinfos << "In mActiveQ" << llendl;
	}
	if (std::find(mBuildQ1.begin(), mBuildQ1.end(), drawablep) != mBuildQ1.end())
	{
		llinfos << "In mBuildQ1" << llendl;
	}
	if (std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep) != mBuildQ2.end())
	{
		llinfos << "In mBuildQ2" << llendl;
	}

	S32 count;
	
	count = gObjectList.findReferences(drawablep);
	if (count)
	{
		llinfos << "In other drawables: " << count << " references" << llendl;
	}
}

BOOL LLPipeline::verify()
{
	BOOL ok = assertInitialized();
	if (ok) 
	{
		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			if (!poolp->verify())
			{
				ok = FALSE;
			}
		}
	}

	if (!ok)
	{
		llwarns << "Pipeline verify failed!" << llendl;
	}
	return ok;
}

//////////////////////////////
//
// Collision detection
//
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	A method to compute a ray-AABB intersection.
 *	Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
 *	Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
 *	Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
 *
 *	Hence this version is faster as well as more robust than the original one.
 *
 *	Should work provided:
 *	1) the integer representation of 0.0f is 0x00000000
 *	2) the sign bit of the float is the most significant one
 *
 *	Report bugs: p.terdiman@codercorner.com
 *
 *	\param		aabb		[in] the axis-aligned bounding box
 *	\param		origin		[in] ray origin
 *	\param		dir			[in] ray direction
 *	\param		coord		[out] impact coordinates
 *	\return		true if ray intersects AABB
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define RAYAABB_EPSILON 0.00001f
#define IR(x)	((U32&)x)

bool LLRayAABB(const LLVector3 &center, const LLVector3 &size, const LLVector3& origin, const LLVector3& dir, LLVector3 &coord, F32 epsilon)
{
	BOOL Inside = TRUE;
	LLVector3 MinB = center - size;
	LLVector3 MaxB = center + size;
	LLVector3 MaxT;
	MaxT.mV[VX]=MaxT.mV[VY]=MaxT.mV[VZ]=-1.0f;

	// Find candidate planes.
	for(U32 i=0;i<3;i++)
	{
		if(origin.mV[i] < MinB.mV[i])
		{
			coord.mV[i]	= MinB.mV[i];
			Inside		= FALSE;

			// Calculate T distances to candidate planes
			if(IR(dir.mV[i]))	MaxT.mV[i] = (MinB.mV[i] - origin.mV[i]) / dir.mV[i];
		}
		else if(origin.mV[i] > MaxB.mV[i])
		{
			coord.mV[i]	= MaxB.mV[i];
			Inside		= FALSE;

			// Calculate T distances to candidate planes
			if(IR(dir.mV[i]))	MaxT.mV[i] = (MaxB.mV[i] - origin.mV[i]) / dir.mV[i];
		}
	}

	// Ray origin inside bounding box
	if(Inside)
	{
		coord = origin;
		return true;
	}

	// Get largest of the maxT's for final choice of intersection
	U32 WhichPlane = 0;
	if(MaxT.mV[1] > MaxT.mV[WhichPlane])	WhichPlane = 1;
	if(MaxT.mV[2] > MaxT.mV[WhichPlane])	WhichPlane = 2;

	// Check final candidate actually inside box
	if(IR(MaxT.mV[WhichPlane])&0x80000000) return false;

	for(U32 i=0;i<3;i++)
	{
		if(i!=WhichPlane)
		{
			coord.mV[i] = origin.mV[i] + MaxT.mV[WhichPlane] * dir.mV[i];
			if (epsilon > 0)
			{
				if(coord.mV[i] < MinB.mV[i] - epsilon || coord.mV[i] > MaxB.mV[i] + epsilon)	return false;
			}
			else
			{
				if(coord.mV[i] < MinB.mV[i] || coord.mV[i] > MaxB.mV[i])	return false;
			}
		}
	}
	return true;	// ray hits box
}

//////////////////////////////
//
// Macros, functions, and inline methods from other classes
//
//

void LLPipeline::setLight(LLDrawable *drawablep, BOOL is_light)
{
	if (drawablep && assertInitialized())
	{
		if (is_light)
		{
			mLights.insert(drawablep);
			drawablep->setState(LLDrawable::LIGHT);
		}
		else
		{
			drawablep->clearState(LLDrawable::LIGHT);
			mLights.erase(drawablep);
		}
	}
}

void LLPipeline::setActive(LLDrawable *drawablep, BOOL active)
{
	assertInitialized();
	if (active)
	{
		mActiveQ.insert(drawablep);
	}
	else
	{
		mActiveQ.erase(drawablep);
	}
}

//static
void LLPipeline::toggleRenderType(U32 type)
{
	U32 bit = (1<<type);
	gPipeline.mRenderTypeMask ^= bit;
}

//static
void LLPipeline::toggleRenderTypeControl(void* data)
{
	U32 type = (U32)(intptr_t)data;
	U32 bit = (1<<type);
	if (gPipeline.hasRenderType(type))
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.toggleRenderType(type);
}

//static
BOOL LLPipeline::hasRenderTypeControl(void* data)
{
	U32 type = (U32)(intptr_t)data;
	return gPipeline.hasRenderType(type);
}

// Allows UI items labeled "Hide foo" instead of "Show foo"
//static
BOOL LLPipeline::toggleRenderTypeControlNegated(void* data)
{
	S32 type = (S32)(intptr_t)data;
	return !gPipeline.hasRenderType(type);
}

//static
void LLPipeline::toggleRenderDebug(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	if (gPipeline.hasRenderDebugMask(bit))
	{
		llinfos << "Toggling render debug mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render debug mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.mRenderDebugMask ^= bit;
}


//static
BOOL LLPipeline::toggleRenderDebugControl(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	return gPipeline.hasRenderDebugMask(bit);
}

//static
void LLPipeline::toggleRenderDebugFeature(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	gPipeline.mRenderDebugFeatureMask ^= bit;
}


//static
BOOL LLPipeline::toggleRenderDebugFeatureControl(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	return gPipeline.hasRenderDebugFeatureMask(bit);
}

// static
void LLPipeline::setRenderScriptedBeacons(BOOL val)
{
	sRenderScriptedBeacons = val;
}

// static
void LLPipeline::toggleRenderScriptedBeacons(void*)
{
	sRenderScriptedBeacons = !sRenderScriptedBeacons;
}

// static
BOOL LLPipeline::getRenderScriptedBeacons(void*)
{
	return sRenderScriptedBeacons;
}

// static
void LLPipeline::setRenderScriptedTouchBeacons(BOOL val)
{
	sRenderScriptedTouchBeacons = val;
}

// static
void LLPipeline::toggleRenderScriptedTouchBeacons(void*)
{
	sRenderScriptedTouchBeacons = !sRenderScriptedTouchBeacons;
}

// static
BOOL LLPipeline::getRenderScriptedTouchBeacons(void*)
{
	return sRenderScriptedTouchBeacons;
}

// static
void LLPipeline::setRenderPhysicalBeacons(BOOL val)
{
	sRenderPhysicalBeacons = val;
}

// static
void LLPipeline::toggleRenderPhysicalBeacons(void*)
{
	sRenderPhysicalBeacons = !sRenderPhysicalBeacons;
}

// static
BOOL LLPipeline::getRenderPhysicalBeacons(void*)
{
	return sRenderPhysicalBeacons;
}

// static
void LLPipeline::setRenderParticleBeacons(BOOL val)
{
	sRenderParticleBeacons = val;
}

// static
void LLPipeline::toggleRenderParticleBeacons(void*)
{
	sRenderParticleBeacons = !sRenderParticleBeacons;
}

// static
BOOL LLPipeline::getRenderParticleBeacons(void*)
{
	return sRenderParticleBeacons;
}

// static
void LLPipeline::setRenderSoundBeacons(BOOL val)
{
	sRenderSoundBeacons = val;
}

// static
void LLPipeline::toggleRenderSoundBeacons(void*)
{
	sRenderSoundBeacons = !sRenderSoundBeacons;
}

// static
BOOL LLPipeline::getRenderSoundBeacons(void*)
{
	return sRenderSoundBeacons;
}

// static
void LLPipeline::setRenderBeacons(BOOL val)
{
	sRenderBeacons = val;
}

// static
void LLPipeline::toggleRenderBeacons(void*)
{
	sRenderBeacons = !sRenderBeacons;
}

// static
BOOL LLPipeline::getRenderBeacons(void*)
{
	return sRenderBeacons;
}

// static
void LLPipeline::setRenderHighlights(BOOL val)
{
	sRenderHighlight = val;
}

// static
void LLPipeline::toggleRenderHighlights(void*)
{
	sRenderHighlight = !sRenderHighlight;
}

// static
BOOL LLPipeline::getRenderHighlights(void*)
{
	return sRenderHighlight;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInWorld(const LLVector3& start, const LLVector3& end,
														S32* face_hit,
														LLVector3* intersection,         // return the intersection point
														LLVector2* tex_coord,            // return the texture coordinates of the intersection point
														LLVector3* normal,               // return the surface normal at the intersection point
														LLVector3* bi_normal             // return the surface bi-normal at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
		{
			if ((j == LLViewerRegion::PARTITION_VOLUME) || (j == LLViewerRegion::PARTITION_BRIDGE))  // only check these partitions for now
			{
				LLSpatialPartition* part = region->getSpatialPartition(j);
				if (part)
				{
					LLDrawable* hit = part->lineSegmentIntersect(start, end, face_hit, intersection, tex_coord, normal, bi_normal);
					if (hit)
					{
						drawable = hit;
					}
				}
			}
		}
	}
	return drawable ? drawable->getVObj().get() : NULL;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInHUD(const LLVector3& start, const LLVector3& end,
													  S32* face_hit,
													  LLVector3* intersection,         // return the intersection point
													  LLVector2* tex_coord,            // return the texture coordinates of the intersection point
													  LLVector3* normal,               // return the surface normal at the intersection point
													  LLVector3* bi_normal             // return the surface bi-normal at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_HUD);
		if (part)
		{
			LLDrawable* hit = part->lineSegmentIntersect(start, end, face_hit, intersection, tex_coord, normal, bi_normal);
			if (hit)
			{
				drawable = hit;
			}
		}
	}
	return drawable ? drawable->getVObj().get() : NULL;
}

LLSpatialPartition* LLPipeline::getSpatialPartition(LLViewerObject* vobj)
{
	if (vobj)
	{
		LLViewerRegion* region = vobj->getRegion();
		if (region)
		{
			return region->getSpatialPartition(vobj->getPartitionType());
		}
	}
	return NULL;
}


void LLPipeline::resetVertexBuffers(LLDrawable* drawable)
{
	if (!drawable || drawable->isDead())
	{
		return;
	}

	for (S32 i = 0; i < drawable->getNumFaces(); i++)
	{
		LLFace* facep = drawable->getFace(i);
		facep->mVertexBuffer = NULL;
		facep->mLastVertexBuffer = NULL;
	}
}

void LLPipeline::resetVertexBuffers()
{
	sRenderBump = gSavedSettings.getBOOL("RenderObjectBump");

	for (LLWorld::region_list_t::iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;
		for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
		{
			LLSpatialPartition* part = region->getSpatialPartition(i);
			if (part)
			{
				part->resetVertexBuffers();
			}
		}
	}

	resetDrawOrders();

	gSky.resetVertexBuffers();

	if (LLVertexBuffer::sGLCount > 0)
	{
		LLVertexBuffer::cleanupClass();
	}

	//delete all name pool caches
	LLGLNamePool::cleanupPools();

	if (LLVertexBuffer::sGLCount > 0)
	{
		llwarns << "VBO wipe failed." << llendl;
	}

	if (!LLVertexBuffer::sStreamIBOPool.mNameList.empty() ||
		!LLVertexBuffer::sStreamVBOPool.mNameList.empty() ||
		!LLVertexBuffer::sDynamicIBOPool.mNameList.empty() ||
		!LLVertexBuffer::sDynamicVBOPool.mNameList.empty())
	{
		llwarns << "VBO name pool cleanup failed." << llendl;
	}

	LLVertexBuffer::unbind();

	LLPipeline::sTextureBindTest = gSavedSettings.getBOOL("RenderDebugTextureBind");
}

void LLPipeline::renderObjects(U32 type, U32 mask, BOOL texture)
{
	assertInitialized();
	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLLastModelView);
	mSimplePool->renderGroups(type, mask, texture);
	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLLastModelView);
}

void LLPipeline::setUseVBO(BOOL use_vbo)
{
	if (use_vbo != LLVertexBuffer::sEnableVBOs)
	{
		if (use_vbo)
		{
			llinfos << "Enabling VBO." << llendl;
		}
		else
		{ 
			llinfos << "Disabling VBO." << llendl;
		}
		
		resetVertexBuffers();
		LLVertexBuffer::initClass(use_vbo);
	}
}

void apply_cube_face_rotation(U32 face)
{
	switch (face)
	{
		case 0: 
			glRotatef(90.f, 0, 1, 0);
			glRotatef(180.f, 1, 0, 0);
		break;
		case 2: 
			glRotatef(-90.f, 1, 0, 0);
		break;
		case 4:
			glRotatef(180.f, 0, 1, 0);
			glRotatef(180.f, 0, 0, 1);
		break;
		case 1: 
			glRotatef(-90.f, 0, 1, 0);
			glRotatef(180.f, 1, 0, 0);
		break;
		case 3:
			glRotatef(90, 1, 0, 0);
		break;
		case 5: 
			glRotatef(180, 0, 0, 1);
		break;
	}
}
void LLPipeline::generateReflectionMap(LLCubeMap* cube_map, LLCamera& cube_cam)
{
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	assertInitialized();

	//render dynamic cube map
	U32 type_mask = gPipeline.getRenderTypeMask();
	S32 use_occlusion = LLPipeline::sUseOcclusion;
	LLPipeline::sUseOcclusion = 0;
	LLPipeline::sSkipUpdate = TRUE;
	U32 res = REFLECTION_MAP_RES;

	LLPipeline::sReflectionRender = TRUE;

	cube_map->bind();
	GLint width;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_TEXTURE_WIDTH, &width);
	if (width != res)
	{
		cube_map->setReflection();
		
		for (U32 i = 0; i < 6; i++)
		{
			glTexImage2D(gl_cube_face[i], 0, GL_RGBA, res, res, 0, GL_RGBA, GL_FLOAT, NULL); 
		}
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
	cube_map->disable();

	BOOL toggle_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
	if (toggle_ui)
	{
		gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	
	U32 cube_mask = (1 << LLPipeline::RENDER_TYPE_SIMPLE) |
					(1 << LLPipeline::RENDER_TYPE_WATER) |
					//(1 << LLPipeline::RENDER_TYPE_BUMP) |
					(1 << LLPipeline::RENDER_TYPE_ALPHA) |
					(1 << LLPipeline::RENDER_TYPE_TREE) |
					//(1 << LLPipeline::RENDER_TYPE_PARTICLES) |
					(1 << LLPipeline::RENDER_TYPE_CLOUDS) |
					//(1 << LLPipeline::RENDER_TYPE_STARS) |
					//(1 << LLPipeline::RENDER_TYPE_AVATAR) |
					(1 << LLPipeline::RENDER_TYPE_GLOW) |
					(1 << LLPipeline::RENDER_TYPE_GRASS) |
					(1 << LLPipeline::RENDER_TYPE_VOLUME) |
					(1 << LLPipeline::RENDER_TYPE_TERRAIN) |
					(1 << LLPipeline::RENDER_TYPE_SKY) |
					(1 << LLPipeline::RENDER_TYPE_WL_SKY) |
					(1 << LLPipeline::RENDER_TYPE_GROUND);
	
	LLDrawPoolWater::sSkipScreenCopy = TRUE;
	LLPipeline::sSkipUpdate = TRUE;
	cube_mask = cube_mask & type_mask;
	gPipeline.setRenderTypeMask(cube_mask);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glViewport(0,0,res,res);
	
	glClearColor(0,0,0,0);			
	
	LLVector3 origin = cube_cam.getOrigin();

	gPipeline.calcNearbyLights(cube_cam);

	stop_glerror();
	LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mCubeFrameBuffer);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
										GL_RENDERBUFFER_EXT, mCubeDepth);		
	stop_glerror();

	for (S32 i = 0; i < 6; i++)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mCubeFrameBuffer);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
									gl_cube_face[i], cube_map->getGLName(), 0);
		validate_framebuffer_object();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.f, 1.f, 0.1f, 1024.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		apply_cube_face_rotation(i);

		glTranslatef(-origin.mV[0], -origin.mV[1], -origin.mV[2]);
		cube_cam.setOrigin(origin);
		LLViewerCamera::updateFrustumPlanes(cube_cam);
		cube_cam.setOrigin(LLViewerCamera::getInstance()->getOrigin());
		static LLCullResult result;
		gPipeline.updateCull(cube_cam, result);
		gPipeline.stateSort(cube_cam, result);
		
		glClearColor(0,0,0,0);
		gGL.setColorMask(true, true);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		gGL.setColorMask(true, false);
		stop_glerror();
		gPipeline.renderGeom(cube_cam);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	cube_cam.setOrigin(origin);
	gShinyOrigin.setVec(cube_cam.getOrigin(), cube_cam.getFar()*2.f);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	gViewerWindow->setupViewport();

	gPipeline.setRenderTypeMask(type_mask);
	LLPipeline::sUseOcclusion = use_occlusion;
	LLPipeline::sSkipUpdate = FALSE;

	if (toggle_ui)
	{
		gPipeline.toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	LLDrawPoolWater::sSkipScreenCopy = FALSE;
	LLPipeline::sSkipUpdate = FALSE;
	LLPipeline::sReflectionRender = FALSE;

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
}

//send cube map vertices and texture coordinates
void render_cube_map()
{
	U16 idx[36];

	idx[0] = 1; idx[1] = 0; idx[2] = 2; //front
	idx[3] = 3; idx[4] = 2; idx[5] = 0;

	idx[6] = 4; idx[7] = 5; idx[8] = 1; //top
	idx[9] = 0; idx[10] = 1; idx[11] = 5; 

	idx[12] = 5; idx[13] = 4; idx[14] = 6; //back
	idx[15] = 7; idx[16] = 6; idx[17] = 4;

	idx[18] = 6; idx[19] = 7; idx[20] = 3; //bottom
	idx[21] = 2; idx[22] = 3; idx[23] = 7;

	idx[24] = 0; idx[25] = 5; idx[26] = 3; //left
	idx[27] = 6; idx[28] = 3; idx[29] = 5;

	idx[30] = 4; idx[31] = 1; idx[32] = 7; //right
	idx[33] = 2; idx[34] = 7; idx[35] = 1;

	LLVector3 vert[8];
	LLVector3 r = LLVector3(1,1,1);

	vert[0] = r.scaledVec(LLVector3(-1,1,1)); //   0 - left top front
	vert[1] = r.scaledVec(LLVector3(1,1,1));  //   1 - right top front
	vert[2] = r.scaledVec(LLVector3(1,-1,1)); //   2 - right bottom front
	vert[3] = r.scaledVec(LLVector3(-1,-1,1)); //  3 - left bottom front

	vert[4] = r.scaledVec(LLVector3(1,1,-1)); //  4 - left top back
	vert[5] = r.scaledVec(LLVector3(-1,1,-1)); //   5 - right top back
	vert[6] = r.scaledVec(LLVector3(-1,-1,-1)); //  6 - right bottom back
	vert[7] = r.scaledVec(LLVector3(1,-1,-1)); // 7 -left bottom back

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(3, GL_FLOAT, 0, vert);
	glVertexPointer(3, GL_FLOAT, 0, vert);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (GLushort*) idx);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void validate_framebuffer_object()
{                                                           
	GLenum status;                                            
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); 
	switch(status) 
	{                                          
		case GL_FRAMEBUFFER_COMPLETE_EXT:                       
			//framebuffer OK, no error.
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			// frame buffer not OK: probably means unsupported depth buffer format
			llerrs << "Framebuffer Incomplete Dimensions." << llendl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			// frame buffer not OK: probably means unsupported depth buffer format
			llerrs << "Framebuffer Incomplete Attachment." << llendl;
			break; 
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    
			/* choose different formats */                        
			llerrs << "Framebuffer unsupported." << llendl;
			break;                                                
		default:                                                
			llerrs << "Unknown framebuffer status." << llendl;
			break;
	}
}

void LLPipeline::blurReflectionMap(LLCubeMap* cube_in, LLCubeMap* cube_out)
{
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	assertInitialized();

	U32 res = (U32) gSavedSettings.getS32("RenderReflectionRes");
	enableLightsFullbright(LLColor4::white);
	LLGLDepthTest depth(GL_FALSE);
	gGL.setColorMask(true, true);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(90.f+45.f/res, 1.f, 0.1f, 1024.f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	cube_out->enableTexture(0);
	cube_out->bind();
	GLint width;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_TEXTURE_WIDTH, &width);
	if (width != res)
	{
		cube_out->setReflection();
		
		for (U32 i = 0; i < 6; i++)
		{
			glTexImage2D(gl_cube_face[i], 0, GL_RGBA, res, res, 0, GL_RGBA, GL_FLOAT, NULL); 
		}
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);

	glViewport(0, 0, res, res);
	LLGLEnable blend(GL_BLEND);
	
	S32 kernel = 2;
	F32 step = 90.f/res;
	F32 alpha = 1.f / ((kernel*2)+1);

	gGL.color4f(alpha,alpha,alpha,alpha*1.25f);
	
	LLVector3 axis[] = 
	{
		LLVector3(1,0,0),
		LLVector3(0,1,0),
		LLVector3(0,0,1)
	};

	stop_glerror();
	glViewport(0,0,res, res);
	gGL.setSceneBlendType(LLRender::BT_ADD);
	cube_in->enableTexture(0);
	//3-axis blur
	for (U32 j = 0; j < 3; j++)
	{
		stop_glerror();

		if (j == 0)
		{
			cube_in->bind();
		}
		else
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, mBlurCubeTexture[j-1]);
		}

		stop_glerror();

		LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mBlurCubeBuffer[j]);
		stop_glerror();

		for (U32 i = 0; i < 6; i++)
		{
			stop_glerror();
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
								GL_COLOR_ATTACHMENT0_EXT,
								gl_cube_face[i], 
								j < 2 ? mBlurCubeTexture[j] : cube_out->getGLName(), 0);
			validate_framebuffer_object();
			gGL.setColorMask(true, true);
			glClear(GL_COLOR_BUFFER_BIT);
			glLoadIdentity();
			apply_cube_face_rotation(i);
			for (S32 x = -kernel; x <= kernel; ++x)
			{
				glPushMatrix();
				glRotatef(x*step, axis[j].mV[0], axis[j].mV[1], axis[j].mV[2]);
				render_cube_map();
				glPopMatrix();
			}
			stop_glerror();
		}	
	}

	stop_glerror();

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	gGL.setColorMask(true, false);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	cube_in->disableTexture();
	gViewerWindow->setupViewport();
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
}

void LLPipeline::bindScreenToTexture() 
{
	
}

void LLPipeline::renderBloom(BOOL for_snapshot)
{
	if (!(gPipeline.canUseVertexShaders() &&
		sRenderGlow))
	{
		return;
	}

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	assertInitialized();

	if (gUseWireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	U32 res_mod = gSavedSettings.getU32("RenderResolutionDivisor");

	LLVector2 tc1(0,0);
	LLVector2 tc2((F32) gViewerWindow->getWindowDisplayWidth(),
					(F32) gViewerWindow->getWindowDisplayHeight());

	if (res_mod > 1)
	{
		tc2 /= (F32) res_mod;
	}

	gGL.setColorMask(true, true);
		
	LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM);
	gGL.color4f(1,1,1,1);
	LLGLDepthTest depth(GL_FALSE);
	LLGLDisable blend(GL_BLEND);
	LLGLDisable cull(GL_CULL_FACE);
	
	enableLightsFullbright(LLColor4(1,1,1,1));

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLDisable test(GL_ALPHA_TEST);

	gGL.setColorMask(true, true);
	glClearColor(0,0,0,0);

	if (for_snapshot)
	{
		mGlow[1].bindTexture();
		{
			//LLGLEnable stencil(GL_STENCIL_TEST);
			//glStencilFunc(GL_NOTEQUAL, 255, 0xFFFFFFFF);
			//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			//LLGLDisable blend(GL_BLEND);
			LLGLEnable blend(GL_BLEND);
			gGL.setSceneBlendType(LLRender::BT_ADD);
			tc2.setVec(1,1);				
			gGL.begin(LLVertexBuffer::TRIANGLE_STRIP);
			gGL.color4f(1,1,1,1);
			gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
			gGL.vertex2f(-1,-1);
			
			gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
			gGL.vertex2f(-1,1);
			
			gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
			gGL.vertex2f(1,-1);
			
			gGL.texCoord2f(tc2.mV[0], tc2.mV[1]);
			gGL.vertex2f(1,1);
			gGL.end();

			gGL.flush();
			gGL.setSceneBlendType(LLRender::BT_ALPHA);
		}

		gGL.flush();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		return;
	}
	
	{
		{
			LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM_FBO);
			mGlow[2].bindTarget();
			mGlow[2].clear();
		}
		
		gGlowExtractProgram.bind();
		F32 minLum = llclamp(gSavedSettings.getF32("RenderGlowMinLuminance"), 0.0f, 1.0f);
		F32 maxAlpha = gSavedSettings.getF32("RenderGlowMaxExtractAlpha");		
		F32 warmthAmount = gSavedSettings.getF32("RenderGlowWarmthAmount");	
		LLVector3 lumWeights = gSavedSettings.getVector3("RenderGlowLumWeights");
		LLVector3 warmthWeights = gSavedSettings.getVector3("RenderGlowWarmthWeights");
		gGlowExtractProgram.uniform1f("minLuminance", minLum);
		gGlowExtractProgram.uniform1f("maxExtractAlpha", maxAlpha);
		gGlowExtractProgram.uniform3f("lumWeights", lumWeights.mV[0], lumWeights.mV[1], lumWeights.mV[2]);
		gGlowExtractProgram.uniform3f("warmthWeights", warmthWeights.mV[0], warmthWeights.mV[1], warmthWeights.mV[2]);
		gGlowExtractProgram.uniform1f("warmthAmount", warmthAmount);
		LLGLEnable blend_on(GL_BLEND);
		LLGLEnable test(GL_ALPHA_TEST);
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
		LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
		
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		mScreen.bindTexture();

		gGL.color4f(1,1,1,1);
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		gGL.begin(LLVertexBuffer::TRIANGLE_STRIP);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,1);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(1,-1);
		
		gGL.texCoord2f(tc2.mV[0], tc2.mV[1]);
		gGL.vertex2f(1,1);
		gGL.end();
		
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);

		mGlow[2].flush();
	}

	tc1.setVec(0,0);
	tc2.setVec(1,1);



	// power of two between 1 and 1024
	U32 glowResPow = gSavedSettings.getS32("RenderGlowResolutionPow");
	const U32 glow_res = llmax(1, 
		llmin(1024, 1 << glowResPow));

	S32 kernel = gSavedSettings.getS32("RenderGlowIterations")*2;
	F32 delta = gSavedSettings.getF32("RenderGlowWidth") / glow_res;
	// Use half the glow width if we have the res set to less than 9 so that it looks
	// almost the same in either case.
	if (glowResPow < 9)
	{
		delta *= 0.5f;
	}
	F32 strength = gSavedSettings.getF32("RenderGlowStrength");

	gGlowProgram.bind();
	gGlowProgram.uniform1f("glowStrength", strength);

	for (S32 i = 0; i < kernel; i++)
	{
		LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
		{
			LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM_FBO);
			mGlow[i%2].bindTarget();
			mGlow[i%2].clear();
		}
			
		if (i == 0)
		{
			mGlow[2].bindTexture();
		}
		else
		{
			mGlow[(i-1)%2].bindTexture();
		}

		if (i%2 == 0)
		{
			gGlowProgram.uniform2f("glowDelta", delta, 0);
		}
		else
		{
			gGlowProgram.uniform2f("glowDelta", 0, delta);
		}

		gGL.begin(LLVertexBuffer::TRIANGLE_STRIP);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,1);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(1,-1);
		
		gGL.texCoord2f(tc2.mV[0], tc2.mV[1]);
		gGL.vertex2f(1,1);
		gGL.end();
		
		mGlow[i%2].flush();
	}

	gGlowProgram.unbind();

	if (LLRenderTarget::sUseFBO)
	{
		LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM_FBO);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	gViewerWindow->setupViewport();

	/*mGlow[1].bindTexture();
	{
		LLGLEnable stencil(GL_STENCIL_TEST);
		glStencilFunc(GL_NOTEQUAL, 255, 0xFFFFFFFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		LLGLDisable blend(GL_BLEND);
			
		gGL.begin(LLVertexBuffer::TRIANGLE_STRIP);
		gGL.color4f(1,1,1,1);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,1);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(1,-1);
		
		gGL.texCoord2f(tc2.mV[0], tc2.mV[1]);
		gGL.vertex2f(1,1);
		gGL.end();

		gGL.flush();
	}

	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_GLOW))
	{
		tc2.setVec((F32) gViewerWindow->getWindowDisplayWidth(),
				(F32) gViewerWindow->getWindowDisplayHeight());

		if (res_mod > 1)
		{
			tc2 /= (F32) res_mod;
		}

		LLGLEnable blend(GL_BLEND);
		gGL.blendFunc(GL_ONE, GL_ONE);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		mScreen.bindTexture();
		
		gGL.begin(LLVertexBuffer::TRIANGLE_STRIP);
		gGL.color4f(1,1,1,1);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,1);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(1,-1);
		
		gGL.texCoord2f(tc2.mV[0], tc2.mV[1]);
		gGL.vertex2f(1,1);
		gGL.end();

		gGL.flush();
		
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);

		gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}*/
	gGL.flush();
	
	{
		LLVertexBuffer::unbind();

		F32 uv0[] = 
		{
				tc1.mV[0], tc1.mV[1],
				tc1.mV[0], tc2.mV[1],
				tc2.mV[0], tc1.mV[1],
				tc2.mV[0], tc2.mV[1]
		};
		
		tc2.setVec((F32) gViewerWindow->getWindowDisplayWidth(),
			(F32) gViewerWindow->getWindowDisplayHeight());

		if (res_mod > 1)
		{
			tc2 /= (F32) res_mod;
		}

		F32 uv1[] = 
		{
				tc1.mV[0], tc1.mV[1],
				tc1.mV[0], tc2.mV[1],
				tc2.mV[0], tc1.mV[1],
				tc2.mV[0], tc2.mV[1]
		};

		F32 v[] = 
		{
			-1,-1,
			-1,1,
			1,-1,
			1,1
		};
		
		LLGLDisable blend(GL_BLEND);

		//tex unit 0
		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_COLOR);
		
		mGlow[1].bindTexture();
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, uv0);
		gGL.getTexUnit(1)->activate();
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		
		//tex unit 1
		gGL.getTexUnit(1)->setTextureColorBlend(LLTexUnit::TBO_ADD, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_PREV_COLOR);
		
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, uv1);

		glVertexPointer(2, GL_FLOAT, 0, v);
		
		mScreen.bindTexture();
		
		LLGLEnable multisample(GL_MULTISAMPLE_ARB);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		gGL.getTexUnit(1)->setTextureBlendType(LLTexUnit::TB_MULT);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		gGL.getTexUnit(0)->activate();
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
	
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

}

inline float sgn(float a)
{
    if (a > 0.0F) return (1.0F);
    if (a < 0.0F) return (-1.0F);
    return (0.0F);
}

void LLPipeline::generateWaterReflection(LLCamera& camera_in)
{
	if (LLPipeline::sWaterReflections && assertInitialized() && LLDrawPoolWater::sNeedsReflectionUpdate)
	{
		LLVertexBuffer::unbind();

		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();

		LLCamera camera = camera_in;
		camera.setFar(camera.getFar()*0.87654321f);
		LLPipeline::sReflectionRender = TRUE;
		S32 occlusion = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = llmin(occlusion, 1);
		U32 type_mask = gPipeline.mRenderTypeMask;

		glh::matrix4f projection = glh_get_current_projection();
		glh::matrix4f mat;

		stop_glerror();
		LLPlane plane;

		F32 height = gAgent.getRegion()->getWaterHeight(); 
		F32 to_clip = fabsf(camera.getOrigin().mV[2]-height);
		F32 pad = -to_clip*0.05f; //amount to "pad" clip plane by

		//plane params
		LLVector3 pnorm;
		F32 pd;

		S32 water_clip = 0;
		if (!LLViewerCamera::getInstance()->cameraUnderWater())
		{ //camera is above water, clip plane points up
			pnorm.setVec(0,0,1);
			pd = -height;
			plane.setVec(pnorm, pd);
			water_clip = -1;
		}
		else
		{	//camera is below water, clip plane points down
			pnorm = LLVector3(0,0,-1);
			pd = height;
			plane.setVec(pnorm, pd);
			water_clip = 1;
		}



		if (!LLViewerCamera::getInstance()->cameraUnderWater())
		{	//generate planar reflection map
			LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
			glClearColor(0,0,0,0);
			gGL.setColorMask(true, true);
			mWaterRef.bindTarget();
			mWaterRef.getViewport(gGLViewport);
			mWaterRef.clear();
			gGL.setColorMask(true, false);

			stop_glerror();

			LLVector3 origin = camera.getOrigin();

			glPushMatrix();

			mat.set_scale(glh::vec3f(1,1,-1));
			mat.set_translate(glh::vec3f(0,0,height*2.f));
			
			glh::matrix4f current = glh_get_current_modelview();

			mat = current * mat;

			glh_set_current_modelview(mat);
			glLoadMatrixf(mat.m);

			LLViewerCamera::updateFrustumPlanes(camera, FALSE, TRUE);

			glCullFace(GL_FRONT);

			//initial sky pass (no user clip plane)
			{ //mask out everything but the sky
				U32 tmp = mRenderTypeMask;
				mRenderTypeMask &= ((1 << LLPipeline::RENDER_TYPE_SKY) |
									(1 << LLPipeline::RENDER_TYPE_CLOUDS) |
									(1 << LLPipeline::RENDER_TYPE_WL_SKY));

				static LLCullResult result;
				updateCull(camera, result);
				stateSort(camera, result);
				renderGeom(camera, TRUE);

				mRenderTypeMask = tmp;
			}

			if (LLDrawPoolWater::sNeedsDistortionUpdate)
			{
				mRenderTypeMask &=	~((1<<LLPipeline::RENDER_TYPE_WATER) |
									  (1<<LLPipeline::RENDER_TYPE_GROUND) |
									  (1<<LLPipeline::RENDER_TYPE_SKY) |
									  (1<<LLPipeline::RENDER_TYPE_CLOUDS));	

				if (gSavedSettings.getBOOL("RenderWaterReflections"))
				{ //mask out selected geometry based on reflection detail

					S32 detail = gSavedSettings.getS32("RenderReflectionDetail");
					if (detail < 3)
					{
						mRenderTypeMask &= ~(1 << LLPipeline::RENDER_TYPE_PARTICLES);
						if (detail < 2)
						{
							mRenderTypeMask &= ~(1 << LLPipeline::RENDER_TYPE_AVATAR);
							if (detail < 1)
							{
								mRenderTypeMask &= ~(1 << LLPipeline::RENDER_TYPE_VOLUME);
							}
						}
					}

					LLSpatialPartition::sFreezeState = TRUE;
					LLPipeline::sSkipUpdate = TRUE;
					LLGLUserClipPlane clip_plane(plane, mat, projection);
					static LLCullResult result;
					updateCull(camera, result, 1);
					stateSort(camera, result);
					renderGeom(camera);
					LLSpatialPartition::sFreezeState = FALSE;
					LLPipeline::sSkipUpdate = FALSE;
				}
			}	
			glCullFace(GL_BACK);
			glPopMatrix();
			mWaterRef.flush();

			glh_set_current_modelview(current);
		}

		//render distortion map
		static BOOL last_update = TRUE;
		if (last_update)
		{
			camera.setFar(camera_in.getFar());
			mRenderTypeMask = type_mask & (~(1<<LLPipeline::RENDER_TYPE_WATER) |
											(1<<LLPipeline::RENDER_TYPE_GROUND));	
			stop_glerror();

			LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater() ? FALSE : TRUE;

			if (LLPipeline::sUnderWaterRender)
			{
				mRenderTypeMask &=	~((1<<LLPipeline::RENDER_TYPE_GROUND) |
									  (1<<LLPipeline::RENDER_TYPE_SKY) |
									  (1<<LLPipeline::RENDER_TYPE_CLOUDS) |
									  (1<<LLPipeline::RENDER_TYPE_WL_SKY));		
			}
			LLViewerCamera::updateFrustumPlanes(camera);

			LLViewerImage::unbindTexture(0, GL_TEXTURE_2D);
			LLColor4& col = LLDrawPoolWater::sWaterFogColor;
			glClearColor(col.mV[0], col.mV[1], col.mV[2], 0.f);
			gGL.setColorMask(true, true);
			mWaterDis.bindTarget();
			mWaterDis.getViewport(gGLViewport);
			mWaterDis.clear();
			gGL.setColorMask(true, false);

			if (!LLPipeline::sUnderWaterRender || LLDrawPoolWater::sNeedsReflectionUpdate)
			{
				//clip out geometry on the same side of water as the camera
				mat = glh_get_current_modelview();
				LLGLUserClipPlane clip_plane(LLPlane(-pnorm, -(pd+pad)), mat, projection);
				static LLCullResult result;
				updateCull(camera, result, water_clip);
				stateSort(camera, result);
				renderGeom(camera);
			}

			LLPipeline::sUnderWaterRender = FALSE;
			mWaterDis.flush();
		}
		last_update = LLDrawPoolWater::sNeedsReflectionUpdate && LLDrawPoolWater::sNeedsDistortionUpdate;

		LLRenderTarget::unbindTarget();
		LLPipeline::sReflectionRender = FALSE;

		if (!LLRenderTarget::sUseFBO)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		glClearColor(0.f, 0.f, 0.f, 0.f);

		gViewerWindow->setupViewport();
		mRenderTypeMask = type_mask;
		LLDrawPoolWater::sNeedsReflectionUpdate = FALSE;
		LLDrawPoolWater::sNeedsDistortionUpdate = FALSE;
		LLViewerCamera::getInstance()->setUserClipPlane(LLPlane(-pnorm, -pd));
		LLPipeline::sUseOcclusion = occlusion;
		
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();
	}
}

void LLPipeline::renderGroups(LLRenderPass* pass, U32 type, U32 mask, BOOL texture)
{
	for (LLCullResult::sg_list_t::iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() &&
			(!sUseOcclusion || !group->isState(LLSpatialGroup::OCCLUDED)) &&
			gPipeline.hasRenderType(group->mSpatialPartition->mDrawableType) &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			pass->renderGroup(group,type,mask,texture);
		}
	}
}

void LLPipeline::generateImpostor(LLVOAvatar* avatar)
{
	static LLCullResult result;
	result.clear();
	grabReferences(result);
	
	if (!avatar || !avatar->mDrawable)
	{
		return;
	}

	assertInitialized();

	U32 mask;
	BOOL muted = LLMuteList::getInstance()->isMuted(avatar->getID());

	if (muted)
	{
		mask  = 1 << LLPipeline::RENDER_TYPE_AVATAR;
	}
	else
	{
		mask  = (1<<LLPipeline::RENDER_TYPE_VOLUME) |
				(1<<LLPipeline::RENDER_TYPE_AVATAR) |
				(1<<LLPipeline::RENDER_TYPE_BUMP) |
				(1<<LLPipeline::RENDER_TYPE_GRASS) |
				(1<<LLPipeline::RENDER_TYPE_SIMPLE) |
				(1<<LLPipeline::RENDER_TYPE_ALPHA) | 
				(1<<LLPipeline::RENDER_TYPE_INVISIBLE);
	}
	
	mask = mask & gPipeline.getRenderTypeMask();
	U32 saved_mask = gPipeline.mRenderTypeMask;
	gPipeline.mRenderTypeMask = mask;

	S32 occlusion = sUseOcclusion;
	sUseOcclusion = 0;
	sReflectionRender = TRUE;
	sImpostorRender = TRUE;

	markVisible(avatar->mDrawable, *LLViewerCamera::getInstance());
	LLVOAvatar::sUseImpostors = FALSE;

	LLVOAvatar::attachment_map_t::iterator iter;
	for (iter = avatar->mAttachmentPoints.begin();
		iter != avatar->mAttachmentPoints.end();
		++iter)
	{
		LLViewerObject* object = iter->second->getObject();
		if (object)
		{
			markVisible(object->mDrawable->getSpatialBridge(), *LLViewerCamera::getInstance());
		}
	}

	stateSort(*LLViewerCamera::getInstance(), result);
	
	const LLVector3* ext = avatar->mDrawable->getSpatialExtents();
	LLVector3 pos(avatar->getRenderPosition()+avatar->getImpostorOffset());

	LLCamera camera = *LLViewerCamera::getInstance();

	camera.lookAt(LLViewerCamera::getInstance()->getOrigin(), pos, LLViewerCamera::getInstance()->getUpAxis());
	
	LLVector2 tdim;

	LLVector3 half_height = (ext[1]-ext[0])*0.5f;

	LLVector3 left = camera.getLeftAxis();
	left *= left;
	left.normVec();

	LLVector3 up = camera.getUpAxis();
	up *= up;
	up.normVec();

	tdim.mV[0] = fabsf(half_height * left);
	tdim.mV[1] = fabsf(half_height * up);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	//glh::matrix4f ortho = gl_ortho(-tdim.mV[0], tdim.mV[0], -tdim.mV[1], tdim.mV[1], 1.0, 256.0);
	F32 distance = (pos-camera.getOrigin()).length();
	F32 fov = atanf(tdim.mV[1]/distance)*2.f*RAD_TO_DEG;
	F32 aspect = tdim.mV[0]/tdim.mV[1]; //128.f/256.f;
	glh::matrix4f persp = gl_perspective(fov, aspect, 1.f, 256.f);
	glh_set_current_projection(persp);
	glLoadMatrixf(persp.m);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glh::matrix4f mat;
	camera.getOpenGLTransform(mat.m);

	mat = glh::matrix4f((GLfloat*) OGL_TO_CFR_ROTATION) * mat;

	glLoadMatrixf(mat.m);
	glh_set_current_modelview(mat);

	glClearColor(0.0f,0.0f,0.0f,0.0f);
	gGL.setColorMask(true, true);
	glStencilMask(0xFFFFFFFF);
	glClearStencil(0);

	// get the number of pixels per angle
	F32 pa = gViewerWindow->getWindowDisplayHeight() / (RAD_TO_DEG * LLViewerCamera::getInstance()->getView());

	//get resolution based on angle width and height of impostor (double desired resolution to prevent aliasing)
	U32 resY = llmin(nhpo2((U32) (fov*pa)), (U32) 512);
	U32 resX = llmin(nhpo2((U32) (atanf(tdim.mV[0]/distance)*2.f*RAD_TO_DEG*pa)), (U32) 512);

	if (!avatar->mImpostor.isComplete() || resX != avatar->mImpostor.getWidth() ||
		resY != avatar->mImpostor.getHeight())
	{
		avatar->mImpostor.allocate(resX,resY,GL_RGBA,TRUE);
		avatar->mImpostor.bindTexture();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	}

	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, resX, resY);
		avatar->mImpostor.bindTarget();
		avatar->mImpostor.getViewport(gGLViewport);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	
	LLGLEnable stencil(GL_STENCIL_TEST);

	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	renderGeom(camera);
	
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 1, 0xFFFFFF);

	{
		LLVector3 left = camera.getLeftAxis()*tdim.mV[0]*2.f;
		LLVector3 up = camera.getUpAxis()*tdim.mV[1]*2.f;

		LLGLEnable blend(muted ? 0 : GL_BLEND);

		if (muted)
		{
			gGL.setColorMask(true, true);
		}
		else
		{
			gGL.setColorMask(false, true);
		}
		
		gGL.setSceneBlendType(LLRender::BT_ADD);
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);

		LLGLDepthTest depth(GL_FALSE, GL_FALSE);

		gGL.color4f(1,1,1,1);
		gGL.color4ub(64,64,64,255);
		gGL.begin(LLVertexBuffer::QUADS);
		gGL.vertex3fv((pos+left-up).mV);
		gGL.vertex3fv((pos-left-up).mV);
		gGL.vertex3fv((pos-left+up).mV);
		gGL.vertex3fv((pos+left+up).mV);
		gGL.end();
		gGL.flush();


		gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}


	avatar->mImpostor.flush();

	avatar->setImpostorDim(tdim);

	LLVOAvatar::sUseImpostors = TRUE;
	sUseOcclusion = occlusion;
	sReflectionRender = FALSE;
	sImpostorRender = FALSE;
	gPipeline.mRenderTypeMask = saved_mask;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	avatar->mNeedsImpostorUpdate = FALSE;
	avatar->cacheImpostorValues();
}

BOOL LLPipeline::hasRenderBatches(const U32 type) const
{
	return sCull->getRenderMapSize(type) > 0;
}

LLCullResult::drawinfo_list_t::iterator LLPipeline::beginRenderMap(U32 type)
{
	return sCull->beginRenderMap(type);
}

LLCullResult::drawinfo_list_t::iterator LLPipeline::endRenderMap(U32 type)
{
	return sCull->endRenderMap(type);
}

LLCullResult::sg_list_t::iterator LLPipeline::beginAlphaGroups()
{
	return sCull->beginAlphaGroups();
}

LLCullResult::sg_list_t::iterator LLPipeline::endAlphaGroups()
{
	return sCull->endAlphaGroups();
}

