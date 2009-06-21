/** 
 * @file pipeline.cpp
 * @brief Rendering pipeline.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"

// library includes
#include "audioengine.h" // For MAX_BUFFERS for debugging.
#include "imageids.h"
#include "llerror.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llmemtype.h"
#include "llnamevalue.h"
#include "llpointer.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "timing.h"
#include "v3color.h"
#include "llui.h" 
#include "llglheaders.h"
#include "llrender.h"
#include "llwindow.h"	// swapBuffers()

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
#include "llvoavatarself.h"
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
#include "llviewershadermgr.h"
#include "llviewerstats.h"
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
//extern BOOL gHideSelectedObjects;
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
	"POOL_BUMP",
	"POOL_TREE",
	"POOL_SKY",
	"POOL_WL_SKY",
	"POOL_GROUND",
	"POOL_INVISIBLE",
	"POOL_AVATAR",
	"POOL_WATER",
	"POOL_GRASS",
	"POOL_FULLBRIGHT",
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

BOOL	LLPipeline::sPickAvatar = TRUE;
BOOL	LLPipeline::sDynamicLOD = TRUE;
BOOL	LLPipeline::sShowHUDAttachments = TRUE;
BOOL	LLPipeline::sRenderPhysicalBeacons = TRUE;
BOOL	LLPipeline::sRenderScriptedBeacons = FALSE;
BOOL	LLPipeline::sRenderScriptedTouchBeacons = TRUE;
BOOL	LLPipeline::sRenderParticleBeacons = FALSE;
BOOL	LLPipeline::sRenderSoundBeacons = FALSE;
BOOL	LLPipeline::sRenderBeacons = FALSE;
BOOL	LLPipeline::sRenderHighlight = TRUE;
BOOL	LLPipeline::sForceOldBakedUpload = FALSE;
S32		LLPipeline::sUseOcclusion = 0;
BOOL	LLPipeline::sDelayVBUpdate = TRUE;
BOOL	LLPipeline::sFastAlpha = TRUE;
BOOL	LLPipeline::sDisableShaders = FALSE;
BOOL	LLPipeline::sRenderBump = TRUE;
BOOL	LLPipeline::sUseFarClip = TRUE;
BOOL	LLPipeline::sShadowRender = FALSE;
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
BOOL	LLPipeline::sRenderDeferred = FALSE;
S32		LLPipeline::sVisibleLightCount = 0;

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

void addDeferredAttachments(LLRenderTarget& target)
{
	target.addColorAttachment(GL_RGBA16F_ARB); //specular
	target.addColorAttachment(GL_RGBA16F_ARB); //normal+z	
	target.addColorAttachment(GL_RGBA16F_ARB); //position
}

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
	mFullbrightPool(NULL),
	mInvisiblePool(NULL),
	mGlowPool(NULL),
	mBumpPool(NULL),
	mWLSkyPool(NULL),
	mLightMask(0),
	mLightMovingMask(0),
	mLightingDetail(0)
{
	mNoiseMap = 0;
}

void LLPipeline::init()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE_INIT);

	sDynamicLOD = gSavedSettings.getBOOL("RenderDynamicLOD");
	sRenderBump = gSavedSettings.getBOOL("RenderObjectBump");
	sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
	sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");

	mInitialized = TRUE;
	
	stop_glerror();

	//create render pass pools
	getPool(LLDrawPool::POOL_ALPHA);
	getPool(LLDrawPool::POOL_SIMPLE);
	getPool(LLDrawPool::POOL_GRASS);
	getPool(LLDrawPool::POOL_FULLBRIGHT);
	getPool(LLDrawPool::POOL_INVISIBLE);
	getPool(LLDrawPool::POOL_BUMP);
	getPool(LLDrawPool::POOL_GLOW);

	LLViewerStats::getInstance()->mTrianglesDrawnStat.reset();
	resetFrameStats();

	mRenderTypeMask = 0xffffffff;	// All render types start on
	mRenderDebugFeatureMask = 0xffffffff; // All debugging features on
	mRenderDebugMask = 0;	// All debug starts off

	// Don't turn on ground when this is set
	// Mac Books with intel 950s need this
	if(!gSavedSettings.getBOOL("RenderGround"))
	{
		toggleRenderType(RENDER_TYPE_GROUND);
	}

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
	delete mFullbrightPool;
	mFullbrightPool = NULL;
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
		GLuint resX = gViewerWindow->getWorldViewWidth();
		GLuint resY = gViewerWindow->getWorldViewHeight();
	
		U32 res_mod = gSavedSettings.getU32("RenderResolutionDivisor");
		if (res_mod > 1 && res_mod < resX && res_mod < resY)
		{
			resX /= res_mod;
			resY /= res_mod;
		}

		allocateScreenBuffer(resX,resY);

		llinfos << "RESIZED SCREEN TEXTURE: " << resX << "x" << resY << llendl;
	}
}

void LLPipeline::allocateScreenBuffer(U32 resX, U32 resY)
{
	U32 samples = gSavedSettings.getU32("RenderFSAASamples");
	if (LLPipeline::sRenderDeferred)
	{
		//allocate deferred rendering color buffers
		mDeferredScreen.allocate(resX, resY, GL_RGBA16F_ARB, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, FALSE);
		addDeferredAttachments(mDeferredScreen);
		mScreen.allocate(resX, resY, GL_RGBA16F_ARB, FALSE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE);		
		
		for (U32 i = 0; i < 2; i++)
		{
			mDeferredLight[i].allocate(resX, resY, GL_RGB, FALSE, FALSE, LLTexUnit::TT_RECT_TEXTURE);
		}
	}
	else
	{
		mScreen.allocate(resX, resY, GL_RGBA, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, FALSE);		
	}
	

	if (gGLManager.mHasFramebufferMultisample && samples > 1)
	{
		if (LLPipeline::sRenderDeferred)
		{
			mSampleBuffer.allocate(resX,resY,GL_RGBA16F_ARB,TRUE,TRUE,LLTexUnit::TT_RECT_TEXTURE,FALSE,samples);
			addDeferredAttachments(mSampleBuffer);
			mDeferredScreen.setSampleBuffer(&mSampleBuffer);
		}
		else
		{
			mSampleBuffer.allocate(resX,resY,GL_RGBA,TRUE,TRUE,LLTexUnit::TT_RECT_TEXTURE,FALSE,samples);
		}

		mScreen.setSampleBuffer(&mSampleBuffer);
		stop_glerror();
	}
	else if (LLPipeline::sRenderDeferred)
	{ //share depth buffer between deferred targets
		mDeferredScreen.shareDepthBuffer(mScreen);
		for (U32 i = 0; i < 2; i++)
		{
			mDeferredScreen.shareDepthBuffer(mDeferredLight[i]);
		}
	}

	gGL.getTexUnit(0)->disable();

	stop_glerror();

}

//static
void LLPipeline::updateRenderDeferred()
{
	BOOL deferred = (gSavedSettings.getBOOL("RenderDeferred") && 
		LLRenderTarget::sUseFBO &&
		gSavedSettings.getBOOL("VertexShaderEnable") && 
		gSavedSettings.getBOOL("RenderAvatarVP") &&
		gSavedSettings.getBOOL("WindLightUseAtmosShaders")) ? TRUE : FALSE;
	
	sRenderDeferred = deferred;			
}

void LLPipeline::releaseGLBuffers()
{
	assertInitialized();
	
	if (mNoiseMap)
	{
		LLImageGL::deleteTextures(1, &mNoiseMap);
		mNoiseMap = 0;
	}

	mWaterRef.release();
	mWaterDis.release();
	mScreen.release();
	mSampleBuffer.releaseSampleBuffer();
	mDeferredScreen.release();
	
	
	for (U32 i = 0; i < 4; i++)
	{
		mSunShadow[i].release();
	}
	for (U32 i = 0; i < 3; i++)
	{
		mGlow[i].release();
	}

	LLVOAvatar::resetImpostors();
}

void LLPipeline::createGLBuffers()
{
	LLMemType mt_cb(LLMemType::MTYPE_PIPELINE_CREATE_BUFFERS);
	assertInitialized();

	updateRenderDeferred();

	if (LLPipeline::sWaterReflections)
	{ //water reflection texture
		U32 res = (U32) gSavedSettings.getS32("RenderWaterRefResolution");
			
		mWaterRef.allocate(res,res,GL_RGBA,TRUE,FALSE);
		mWaterDis.allocate(res,res,GL_RGBA,TRUE,FALSE);
	}


	stop_glerror();

	GLuint resX = gViewerWindow->getWorldViewWidth();
	GLuint resY = gViewerWindow->getWorldViewHeight();
	
	if (LLPipeline::sRenderGlow)
	{ //screen space glow buffers
		const U32 glow_res = llmax(1, 
			llmin(512, 1 << gSavedSettings.getS32("RenderGlowResolutionPow")));

		for (U32 i = 0; i < 3; i++)
		{
			mGlow[i].allocate(512,glow_res,GL_RGBA,FALSE,FALSE);
		}

		allocateScreenBuffer(resX,resY);
	}
	
	if (sRenderDeferred)
	{
		mSunShadow[0].allocate(1024,1024, 0, TRUE, FALSE);
		mSunShadow[1].allocate(1024,1024, 0, TRUE, FALSE);
		mSunShadow[2].allocate(1024,1024, 0, TRUE, FALSE);
		mSunShadow[3].allocate(1024,1024, 0, TRUE, FALSE);
		
		if (!mNoiseMap)
		{
			const U32 noiseRes = 128;
			LLVector3 noise[noiseRes*noiseRes];

			F32 scaler = gSavedSettings.getF32("RenderDeferredNoise")/100.f;
			for (U32 i = 0; i < noiseRes*noiseRes; ++i)
			{
				noise[i] = LLVector3(ll_frand()-0.5f, ll_frand()-0.5f, 0.f);
				noise[i].normVec();
				noise[i].mV[2] = ll_frand()*scaler+1.f-scaler/2.f;
			}

			LLImageGL::generateTextures(1, &mNoiseMap);
			
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mNoiseMap);
			LLImageGL::setManualImage(LLTexUnit::getInternalType(LLTexUnit::TT_TEXTURE), 0, GL_RGB16F_ARB, noiseRes, noiseRes, GL_RGB, GL_FLOAT, noise);
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		}
	}
}

void LLPipeline::restoreGL() 
{
	LLMemType mt_cb(LLMemType::MTYPE_PIPELINE_RESTORE_GL);
	assertInitialized();

	if (mVertexShadersEnabled)
	{
		LLViewerShaderMgr::instance()->setShaders();
	}

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
	if (sDisableShaders ||
		!gGLManager.mHasVertexShader ||
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
	LLMemType mt_us(LLMemType::MTYPE_PIPELINE_UNLOAD_SHADERS);
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
	LLMemType mt_ld(LLMemType::MTYPE_PIPELINE_LIGHTING_DETAIL);
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
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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

	case LLDrawPool::POOL_GRASS:
		poolp = mGrassPool;
		break;

	case LLDrawPool::POOL_FULLBRIGHT:
		poolp = mFullbrightPool;
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
	LLMemType mt_gpt(LLMemType::MTYPE_PIPELINE_GET_POOL_TYPE);
	
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
	LLMemType mt_a(LLMemType::MTYPE_PIPELINE_ADD_POOL);
	assertInitialized();
	mPools.insert(new_poolp);
	addToQuickLookup( new_poolp );
}

void LLPipeline::allocDrawable(LLViewerObject *vobj)
{
	LLMemType mt_ad(LLMemType::MTYPE_PIPELINE_ALLOCATE_DRAWABLE);
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
	LLMemType mt_ao(LLMemType::MTYPE_PIPELINE_ADD_OBJECT);
	if (gNoRender)
	{
		return 0;
	}

	if (gSavedSettings.getBOOL("RenderDelayCreation"))
	{
		mCreateQ.push_back(vobj);
	}
	else
	{
		createObject(vobj);
	}

	return 1;
}

void LLPipeline::createObjects(F32 max_dtime)
{
	LLFastTimer ftm(LLFastTimer::FTM_GEO_UPDATE);
	LLMemType mt(LLMemType::MTYPE_PIPELINE_CREATE_OBJECTS);

	LLTimer update_timer;

	while (!mCreateQ.empty() && update_timer.getElapsedTimeF32() < max_dtime)
	{
		LLViewerObject* vobj = mCreateQ.front();
		if (!vobj->isDead())
		{
			createObject(vobj);
		}
		mCreateQ.pop_front();
	}
	
	//for (LLViewerObject::vobj_list_t::iterator iter = mCreateQ.begin(); iter != mCreateQ.end(); ++iter)
	//{
	//	createObject(*iter);
	//}

	//mCreateQ.clear();
}

void LLPipeline::createObject(LLViewerObject* vobj)
{
	LLDrawable* drawablep = vobj->mDrawable;

	if (!drawablep)
	{
		drawablep = vobj->createDrawable(this);
	}
	else
	{
		llerrs << "Redundant drawable creation!" << llendl;
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

	if (drawablep->getVOVolume() && gSavedSettings.getBOOL("RenderAnimateRes"))
	{
		// fun animated res
		drawablep->updateXform(TRUE);
		drawablep->clearState(LLDrawable::MOVE_UNDAMPED);
		drawablep->setScale(LLVector3(0,0,0));
		drawablep->makeActive();
	}
}


void LLPipeline::resetFrameStats()
{
	assertInitialized();

	LLViewerStats::getInstance()->mTrianglesDrawnStat.addValue(mTrianglesDrawn/1000.f);

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
	LLMemType mt_um(LLMemType::MTYPE_PIPELINE_UPDATE_MOVE);

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

		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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

BOOL LLPipeline::visibleObjectsInFrustum(LLCamera& camera)
{
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
					if (part->visibleObjectsInFrustum(camera))
					{
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

BOOL LLPipeline::getVisibleExtents(LLCamera& camera, LLVector3& min, LLVector3& max)
{
	min = LLVector3(F32_MAX, F32_MAX, F32_MAX);
	max = LLVector3(-F32_MAX, -F32_MAX, -F32_MAX);


	BOOL res = TRUE;

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
					if (!part->getVisibleExtents(camera, min, max))
					{
						res = FALSE;
					}
				}
			}
		}
	}

	return res;
}


void LLPipeline::updateCull(LLCamera& camera, LLCullResult& result, S32 water_clip)
{
	LLFastTimer t(LLFastTimer::FTM_CULL);
	LLMemType mt_uc(LLMemType::MTYPE_PIPELINE_UPDATE_CULL);

	grabReferences(result);

	sCull->clear();

	BOOL to_texture =	LLPipeline::sUseOcclusion > 1 &&
						!hasRenderType(LLPipeline::RENDER_TYPE_HUD) && 
						!sReflectionRender &&
						!sShadowRender &&
						gPipeline.canUseVertexShaders() &&
						sRenderGlow;

	if (to_texture)
	{
		mScreen.bindTarget();
	}

	glPushMatrix();
	gGLLastMatrix = NULL;
	//glLoadMatrixd(gGLModelView);
	glLoadMatrixd(gGLLastModelView);

	LLVertexBuffer::unbind();
	LLGLDisable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	if (sUseOcclusion > 1)
	{
		gGL.setColorMask(false, false);
	}

	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
	
	
	glPopMatrix();

	if (sUseOcclusion > 1)
	{
		gGL.setColorMask(true, false);
	}

	if (to_texture)
	{
		mScreen.flush();
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
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_UPDATE_GEOM);
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_MARK_VISIBLE);
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
	LLMemType mt_mm(LLMemType::MTYPE_PIPELINE_MARK_MOVED);

	if (!drawablep)
	{
		//llerrs << "Sending null drawable to moved list!" << llendl;
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_MARK_SHIFT);

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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_SHIFT_OBJECTS);

	assertInitialized();

	glClear(GL_DEPTH_BUFFER_BIT);
	gDepthDirty = TRUE;
		
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

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_MARK_TEXTURED);

	if (drawablep && !drawablep->isDead() && assertInitialized())
	{
		mRetexturedList.insert(drawablep);
	}
}

void LLPipeline::markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag, BOOL priority)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE_MARK_REBUILD);

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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_STATE_SORT);

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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_STATE_SORT);
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_STATE_SORT);
	if (!sSkipUpdate && bridge->getSpatialGroup()->changeLOD())
	{
		bool force_update = false;
		bridge->updateDistance(camera, force_update);
	}
}

void LLPipeline::stateSort(LLDrawable* drawablep, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE_STATE_SORT);
		
	if (!drawablep
		|| drawablep->isDead() 
		|| !hasRenderType(drawablep->getRenderType()))
	{
		return;
	}
	
	if (LLSelectMgr::getInstance()->mHideSelectedObjects)
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
				bool force_update = false;
				drawablep->updateDistance(camera, force_update);
			}
			else if (drawablep->isAvatar())
			{
				bool force_update = false;
				drawablep->updateDistance(camera, force_update); // calls vobj->updateLOD() which calls LLVOAvatar::updateVisibility()
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_POST_SORT);
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


	const S32 bin_count = 1024*8;
		
	static LLCullResult::drawinfo_list_t alpha_bins[bin_count];
	static U32 bin_size[bin_count];

	//clear one bin per frame to avoid memory bloat
	static S32 clear_idx = 0;
	clear_idx = (1+clear_idx)%bin_count;
	alpha_bins[clear_idx].clear();

	for (U32 j = 0; j < bin_count; j++)
	{
		bin_size[j] = 0;
	}

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
		
	if (!sShadowRender)
	{
		//sort by texture or bump map
		for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; ++i)
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

		std::sort(sCull->beginAlphaGroups(), sCull->endAlphaGroups(), LLSpatialGroup::CompareDepthGreater());
	}
	
	// only render if the flag is set. The flag is only set if we are in edit mode or the toggle is set in the menus
	if (gSavedSettings.getBOOL("BeaconAlwaysOn") && !sShadowRender)
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

	if (!sShadowRender)
	{
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
	}

	LLSpatialGroup::sNoDelete = FALSE;
}


void render_hud_elements()
{
	LLMemType mt_rhe(LLMemType::MTYPE_PIPELINE_RENDER_HUD_ELS);
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_RENDER_HL);

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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_RENDER_GEOM);
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

	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:ForceVBO");
	
	//by bao
	//fake vertex buffer updating
	//to guaranttee at least updating one VBO buffer every frame
	//to walk around the bug caused by ATI card --> DEV-3855
	//
	if(forceVBOUpdate)
		gSky.mVOSkyp->updateDummyVertexBuffer() ;


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

	gGL.getTexUnit(0)->bind(LLViewerImage::sDefaultImagep);
	LLViewerImage::sDefaultImagep->setAddressMode(LLTexUnit::TAM_WRAP);
	
	//////////////////////////////////////////////
	//
	// Actually render all of the geometry
	//
	//	
	stop_glerror();
	
	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderDrawPools");
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (hasRenderType(poolp->getType()))
		{
			poolp->prerender();
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PICKING))
	{
		LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderForSelect");
		gObjectList.renderObjectsForSelect(camera, gViewerWindow->getVirtualWindowRect());
	}
	else
	{
		LLFastTimer t(LLFastTimer::FTM_POOLS);
		
		// HACK: don't calculate local lights if we're rendering the HUD!
		//    Removing this check will cause bad flickering when there are 
		//    HUD elements being rendered AND the user is in flycam mode  -nyx
		if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{
			calcNearbyLights(camera);
			setupHWLights(NULL);
		}

		BOOL occlude = sUseOcclusion > 1;
		U32 cur_type = 0;

		pool_set_t::iterator iter1 = mPools.begin();
		while ( iter1 != mPools.end() )
		{
			LLDrawPool *poolp = *iter1;
			
			cur_type = poolp->getType();

			if (occlude && cur_type >= LLDrawPool::POOL_GRASS)
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
					LLVertexBuffer::unbind();
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
							if (gDebugSession)
							{
								ll_fail("GL matrix stack corrupted.");
							}
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
	
	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderDrawPoolsEnd");

	LLVertexBuffer::unbind();
		
		gGLLastMatrix = NULL;
		glLoadMatrixd(gGLModelView);

		if (occlude)
		{
			occlude = FALSE;
			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);
			doOcclusion(camera);
		}
	}

	LLVertexBuffer::unbind();
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	

	stop_glerror();
		
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderHighlights");

	if (!sReflectionRender)
	{
		renderHighlights();
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();

	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderDebug");
	
	renderDebug();

	LLVertexBuffer::unbind();
	
	if (!LLPipeline::sReflectionRender && !LLPipeline::sRenderDeferred && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		// Render debugging beacons.
		gObjectList.renderObjectBeacons();
		LLHUDObject::renderAll();
		gObjectList.resetObjectBeacons();
	}

	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderGeomEnd");

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

void LLPipeline::renderGeomDeferred(LLCamera& camera)
{
	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderGeomDeferred");

	LLMemType mt_rgd(LLMemType::MTYPE_PIPELINE_RENDER_GEOM_DEFFERRED);
	LLFastTimer t(LLFastTimer::FTM_RENDER_GEOMETRY);

	LLFastTimer t2(LLFastTimer::FTM_POOLS);

	LLGLEnable cull(GL_CULL_FACE);

	LLGLEnable stencil(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
	stop_glerror();
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	stop_glerror();

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (hasRenderType(poolp->getType()))
		{
			poolp->prerender();
		}
	}

	LLGLEnable multisample(GL_MULTISAMPLE_ARB);

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	U32 cur_type = 0;

	gGL.setColorMask(true, true);
	
	pool_set_t::iterator iter1 = mPools.begin();

	while ( iter1 != mPools.end() )
	{
		LLDrawPool *poolp = *iter1;
		
		cur_type = poolp->getType();

		pool_set_t::iterator iter2 = iter1;
		if (hasRenderType(poolp->getType()) && poolp->getNumDeferredPasses() > 0)
		{
			LLFastTimer t(LLFastTimer::FTM_POOLRENDER);

			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);
		
			for( S32 i = 0; i < poolp->getNumDeferredPasses(); i++ )
			{
				LLVertexBuffer::unbind();
				poolp->beginDeferredPass(i);
				for (iter2 = iter1; iter2 != mPools.end(); iter2++)
				{
					LLDrawPool *p = *iter2;
					if (p->getType() != cur_type)
					{
						break;
					}
										
					p->renderDeferred(i);
				}
				poolp->endDeferredPass(i);
				LLVertexBuffer::unbind();

				GLint depth;
				glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
				if (depth > 3)
				{
					llerrs << "GL matrix stack corrupted!" << llendl;
				}
				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();
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

	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLModelView);

	gGL.setColorMask(true, false);
}

void LLPipeline::renderGeomPostDeferred(LLCamera& camera)
{
	LLMemType mt_rgpd(LLMemType::MTYPE_PIPELINE_RENDER_GEOM_POST_DEF);
	LLFastTimer t(LLFastTimer::FTM_POOLS);
	U32 cur_type = 0;

	LLGLEnable cull(GL_CULL_FACE);

	LLGLEnable multisample(GL_MULTISAMPLE_ARB);

	calcNearbyLights(camera);
	setupHWLights(NULL);

	gGL.setColorMask(true, false);

	pool_set_t::iterator iter1 = mPools.begin();
	BOOL occlude = LLPipeline::sUseOcclusion > 1;

	while ( iter1 != mPools.end() )
	{
		LLDrawPool *poolp = *iter1;
		
		cur_type = poolp->getType();

		if (occlude && cur_type >= LLDrawPool::POOL_GRASS)
		{
			occlude = FALSE;
			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);
			doOcclusion(camera);
			gGL.setColorMask(true, false);
		}

		pool_set_t::iterator iter2 = iter1;
		if (hasRenderType(poolp->getType()) && poolp->getNumPostDeferredPasses() > 0)
		{
			LLFastTimer t(LLFastTimer::FTM_POOLRENDER);

			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);
		
			for( S32 i = 0; i < poolp->getNumPostDeferredPasses(); i++ )
			{
				LLVertexBuffer::unbind();
				poolp->beginPostDeferredPass(i);
				for (iter2 = iter1; iter2 != mPools.end(); iter2++)
				{
					LLDrawPool *p = *iter2;
					if (p->getType() != cur_type)
					{
						break;
					}
										
					p->renderPostDeferred(i);
				}
				poolp->endPostDeferredPass(i);
				LLVertexBuffer::unbind();

				GLint depth;
				glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
				if (depth > 3)
				{
					llerrs << "GL matrix stack corrupted!" << llendl;
				}
				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();
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

	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLModelView);

	renderHighlights();
	mHighlightFaces.clear();

	renderDebug();

	LLVertexBuffer::unbind();

	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		// Render debugging beacons.
		gObjectList.renderObjectBeacons();
		LLHUDObject::renderAll();
		gObjectList.resetObjectBeacons();
	}

	if (occlude)
	{
		occlude = FALSE;
		gGLLastMatrix = NULL;
		glLoadMatrixd(gGLModelView);
		doOcclusion(camera);
	}
}

void LLPipeline::renderGeomShadow(LLCamera& camera)
{
	LLMemType mt_rgs(LLMemType::MTYPE_PIPELINE_RENDER_GEOM_SHADOW);
	U32 cur_type = 0;
	
	LLGLEnable cull(GL_CULL_FACE);

	LLVertexBuffer::unbind();

	pool_set_t::iterator iter1 = mPools.begin();
	
	while ( iter1 != mPools.end() )
	{
		LLDrawPool *poolp = *iter1;
		
		cur_type = poolp->getType();

		pool_set_t::iterator iter2 = iter1;
		if (hasRenderType(poolp->getType()) && poolp->getNumShadowPasses() > 0)
		{
			gGLLastMatrix = NULL;
			glLoadMatrixd(gGLModelView);
		
			for( S32 i = 0; i < poolp->getNumShadowPasses(); i++ )
			{
				LLVertexBuffer::unbind();
				poolp->beginShadowPass(i);
				for (iter2 = iter1; iter2 != mPools.end(); iter2++)
				{
					LLDrawPool *p = *iter2;
					if (p->getType() != cur_type)
					{
						break;
					}
										
					p->renderShadow(i);
				}
				poolp->endShadowPass(i);
				LLVertexBuffer::unbind();

				LLGLState::checkStates();
				LLGLState::checkTextureChannels();
				LLGLState::checkClientArrays();
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

	gGLLastMatrix = NULL;
	glLoadMatrixd(gGLModelView);
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
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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

	for (LLCullResult::bridge_list_t::const_iterator i = sCull->beginVisibleBridge(); i != sCull->endVisibleBridge(); ++i)
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

	if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
	{
		gGL.color4f(1,1,1,1);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				
		F32 col[] =
		{
			1,1,0,
			0,1,1,
			1,0,1,
			1,1,1,
			1,0,0,
			0,1,0,
			0,0,1,
			0,0,0
		};

		for (U32 i = 0; i < 8; i++)
		{
			gGL.color3fv(col+i*3);	

			gGL.begin(LLRender::LINES);

			LLVector3* frust = mShadowCamera[i].mAgentFrustum;

			gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[1].mV);
			gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[2].mV);
			gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[3].mV);
			gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[0].mV);

			gGL.vertex3fv(frust[4].mV); gGL.vertex3fv(frust[5].mV);
			gGL.vertex3fv(frust[5].mV); gGL.vertex3fv(frust[6].mV);
			gGL.vertex3fv(frust[6].mV); gGL.vertex3fv(frust[7].mV);
			gGL.vertex3fv(frust[7].mV); gGL.vertex3fv(frust[4].mV);

			gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
			gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[5].mV);
			gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[6].mV);
			gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[7].mV);
			
			if (i < 4)
			{
				LLVector3* ext = mShadowExtents[i];
				
				LLVector3 box[] =
				{
					LLVector3(ext[0][0], ext[0][1], ext[0][2]),
					LLVector3(ext[1][0], ext[0][1], ext[0][2]),
					LLVector3(ext[1][0], ext[1][1], ext[0][2]),
					LLVector3(ext[0][0], ext[1][1], ext[0][2]),
					LLVector3(ext[0][0], ext[0][1], ext[1][2]),
					LLVector3(ext[1][0], ext[0][1], ext[1][2]),
					LLVector3(ext[1][0], ext[1][1], ext[1][2]),
					LLVector3(ext[0][0], ext[1][1], ext[1][2]),
				};
					
				gGL.vertex3fv(box[0].mV); gGL.vertex3fv(box[1].mV);
				gGL.vertex3fv(box[1].mV); gGL.vertex3fv(box[2].mV);
				gGL.vertex3fv(box[2].mV); gGL.vertex3fv(box[3].mV);
				gGL.vertex3fv(box[3].mV); gGL.vertex3fv(box[0].mV);

				gGL.vertex3fv(box[4].mV); gGL.vertex3fv(box[5].mV);
				gGL.vertex3fv(box[5].mV); gGL.vertex3fv(box[6].mV);
				gGL.vertex3fv(box[6].mV); gGL.vertex3fv(box[7].mV);
				gGL.vertex3fv(box[7].mV); gGL.vertex3fv(box[4].mV);

				gGL.vertex3fv(box[0].mV); gGL.vertex3fv(box[4].mV);
				gGL.vertex3fv(box[1].mV); gGL.vertex3fv(box[5].mV);
				gGL.vertex3fv(box[2].mV); gGL.vertex3fv(box[6].mV);
				gGL.vertex3fv(box[3].mV); gGL.vertex3fv(box[7].mV);
			}

			gGL.end();
			
			for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
					iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
			{
				LLViewerRegion* region = *iter;
				for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
				{
					LLSpatialPartition* part = region->getSpatialPartition(j);
					if (part)
					{
						if (hasRenderType(part->mDrawableType))
						{
							part->renderIntersectingBBoxes(&mShadowCamera[i]);
						}
					}
				}
			}
		}
	}

	if (mRenderDebugMask & RENDER_DEBUG_COMPOSITION)
	{
		// Debug composition layers
		F32 x, y;

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		if (gAgent.getRegion())
		{
			gGL.begin(LLRender::POINTS);
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

	LLMemType mt(LLMemType::MTYPE_PIPELINE_RENDER_SELECT);
	
	
	
	glMatrixMode(GL_MODELVIEW);

	LLGLSDefault gls_default;
	LLGLSObjectSelect gls_object_select;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_TRUE,GL_TRUE);
	disableLights();
	
	LLVertexBuffer::unbind();

	//for each drawpool
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
	U32 last_type = 0;
	
	// If we don't do this, we crash something on changing graphics settings
	// from Medium -> Low, because we unload all the shaders and the 
	// draw pools aren't aware.  I don't know if this has to be a separate
	// loop before actual rendering. JC
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->isFacePool() && hasRenderType(poolp->getType()))
		{
			poolp->prerender();
		}
	}
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
					LLVertexBuffer::MAP_TEXCOORD0;

	for (std::set<LLViewerObject*>::iterator i = objects.begin(); i != objects.end(); ++i)
	{
		LLViewerObject* vobj = *i;
		LLDrawable* drawable = vobj->mDrawable;
		if (vobj->isDead() || 
			vobj->isHUDAttachment() ||
			(LLSelectMgr::getInstance()->mHideSelectedObjects && vobj->isSelected()) ||
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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_REBUILD_POOLS);

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
	LLMemType mt(LLMemType::MTYPE_PIPELINE_QUICK_LOOKUP);

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

	case LLDrawPool::POOL_GRASS:
		if (mGrassPool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate grass pool." << llendl;
		}
		else
		{
			mGrassPool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_FULLBRIGHT:
		if (mFullbrightPool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate simple pool." << llendl;
		}
		else
		{
			mFullbrightPool = (LLRenderPass*) new_poolp;
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

	case LLDrawPool::POOL_GRASS:
		llassert(mGrassPool == poolp);
		mGrassPool = NULL;
		break;

	case LLDrawPool::POOL_FULLBRIGHT:
		llassert(mFullbrightPool == poolp);
		mFullbrightPool = NULL;
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
		LLColor4 diffuse(1.f, 1.f, 1.f, 0.f);
		LLVector4 light_pos_cam(-8.f, 0.25f, 10.f, 0.f);  // w==0 => directional light
		LLMatrix4 camera_mat = LLViewerCamera::getInstance()->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();
		LLVector4 light_pos = light_pos_cam * camera_rot;
		
		light_pos.normalize();

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
		backlight_pos.normalize();
			
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
			if (!sRenderAttachedLights && light && light->isAttachment())
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
		mHWLightColors[2] = light_color;
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
		stop_glerror();
		if (!mLightMask)
		{
			glEnable(GL_LIGHTING);
		}
		if (mask)
		{
			stop_glerror();
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
			stop_glerror();
		}
		else
		{
			glDisable(GL_LIGHTING);
		}
		stop_glerror();
		mLightMask = mask;
		LLColor4 ambient = gSky.getTotalAmbientColor();
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient.mV);
		stop_glerror();
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
														BOOL pick_transparent,												
														S32* face_hit,
														LLVector3* intersection,         // return the intersection point
														LLVector2* tex_coord,            // return the texture coordinates of the intersection point
														LLVector3* normal,               // return the surface normal at the intersection point
														LLVector3* bi_normal             // return the surface bi-normal at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	LLVector3 local_end = end;

	LLVector3 position;

	sPickAvatar = FALSE; //LLToolMgr::getInstance()->inBuildMode() ? FALSE : TRUE;
	
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
		{
			if ((j == LLViewerRegion::PARTITION_VOLUME) || 
				(j == LLViewerRegion::PARTITION_BRIDGE) || 
				(j == LLViewerRegion::PARTITION_TERRAIN) ||
				(j == LLViewerRegion::PARTITION_TREE) ||
				(j == LLViewerRegion::PARTITION_GRASS))  // only check these partitions for now
			{
				LLSpatialPartition* part = region->getSpatialPartition(j);
				if (part && hasRenderType(part->mDrawableType))
				{
					LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, face_hit, &position, tex_coord, normal, bi_normal);
					if (hit)
					{
						drawable = hit;
						local_end = position;						
					}
				}
			}
		}
	}
	
	if (!sPickAvatar)
	{
		//save hit info in case we need to restore
		//due to attachment override
		LLVector3 local_normal;
		LLVector3 local_binormal;
		LLVector2 local_texcoord;
		S32 local_face_hit = -1;

		if (face_hit)
		{ 
			local_face_hit = *face_hit;
		}
		if (tex_coord)
		{
			local_texcoord = *tex_coord;
		}
		if (bi_normal)
		{
			local_binormal = *bi_normal;
		}
		if (normal)
		{
			local_normal = *normal;
		}
				
		const F32 ATTACHMENT_OVERRIDE_DIST = 0.1f;

		//check against avatars
		sPickAvatar = TRUE;
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
				iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* region = *iter;

			LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_BRIDGE);
			if (part && hasRenderType(part->mDrawableType))
			{
				LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, face_hit, &position, tex_coord, normal, bi_normal);
				if (hit)
				{
					if (!drawable || 
						!drawable->getVObj()->isAttachment() ||
						(position-local_end).magVec() > ATTACHMENT_OVERRIDE_DIST)
					{ //avatar overrides if previously hit drawable is not an attachment or 
					  //attachment is far enough away from detected intersection
						drawable = hit;
						local_end = position;						
					}
					else
					{ //prioritize attachments over avatars
						position = local_end;

						if (face_hit)
						{
							*face_hit = local_face_hit;
						}
						if (tex_coord)
						{
							*tex_coord = local_texcoord;
						}
						if (bi_normal)
						{
							*bi_normal = local_binormal;
						}
						if (normal)
						{
							*normal = local_normal;
						}
					}
				}
			}
		}
	}

	//check all avatar nametags (silly, isn't it?)
	for (std::vector< LLCharacter* >::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end();
		++iter)
	{
		LLVOAvatar* av = (LLVOAvatar*) *iter;
		if (av->mNameText.notNull() && av->mNameText->lineSegmentIntersect(start, local_end, position))
		{
			drawable = av->mDrawable;
			local_end = position;
		}
	}

	if (intersection)
	{
		*intersection = position;
	}

	return drawable ? drawable->getVObj().get() : NULL;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInHUD(const LLVector3& start, const LLVector3& end,
													  BOOL pick_transparent,													
													  S32* face_hit,
													  LLVector3* intersection,         // return the intersection point
													  LLVector2* tex_coord,            // return the texture coordinates of the intersection point
													  LLVector3* normal,               // return the surface normal at the intersection point
													  LLVector3* bi_normal             // return the surface bi-normal at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		BOOL toggle = FALSE;
		if (!hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{
			toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
			toggle = TRUE;
		}

		LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_HUD);
		if (part)
		{
			LLDrawable* hit = part->lineSegmentIntersect(start, end, pick_transparent, face_hit, intersection, tex_coord, normal, bi_normal);
			if (hit)
			{
				drawable = hit;
			}
		}

		if (toggle)
		{
			toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
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

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
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
	LLMemType mt_ro(LLMemType::MTYPE_PIPELINE_RENDER_OBJECTS);
	assertInitialized();
	glLoadMatrixd(gGLModelView);
	gGLLastMatrix = NULL;
	mSimplePool->pushBatches(type, mask);
	glLoadMatrixd(gGLModelView);
	gGLLastMatrix = NULL;		
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

void LLPipeline::bindScreenToTexture() 
{
	
}

void LLPipeline::renderBloom(BOOL for_snapshot, F32 zoom_factor, int subfield)
{
	LLMemType mt_ru(LLMemType::MTYPE_PIPELINE_RENDER_BLOOM);
	if (!(gPipeline.canUseVertexShaders() &&
		sRenderGlow))
	{
		return;
	}

	LLVertexBuffer::unbind();
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	assertInitialized();

	if (gUseWireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	U32 res_mod = gSavedSettings.getU32("RenderResolutionDivisor");

	LLVector2 tc1(0,0);
	LLVector2 tc2((F32) gViewerWindow->getWorldViewWidth()*2,
				  (F32) gViewerWindow->getWorldViewHeight()*2);

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
		gGL.getTexUnit(0)->bind(&mGlow[1]);
		{
			//LLGLEnable stencil(GL_STENCIL_TEST);
			//glStencilFunc(GL_NOTEQUAL, 255, 0xFFFFFFFF);
			//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			//LLGLDisable blend(GL_BLEND);

			// If the snapshot is constructed from tiles, calculate which
			// tile we're in.
			const S32 num_horizontal_tiles = llceil(zoom_factor);
			const LLVector2 tile(subfield % num_horizontal_tiles,
								 (S32)(subfield / num_horizontal_tiles));
			llassert(zoom_factor > 0.0); // Non-zero, non-negative.
			const F32 tile_size = 1.0/zoom_factor;
			
			tc1 = tile*tile_size; // Top left texture coordinates
			tc2 = (tile+LLVector2(1,1))*tile_size; // Bottom right texture coordinates
			
			LLGLEnable blend(GL_BLEND);
			gGL.setSceneBlendType(LLRender::BT_ADD);
			
				
			gGL.begin(LLRender::TRIANGLE_STRIP);
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
		F32 minLum = llmax(gSavedSettings.getF32("RenderGlowMinLuminance"), 0.0f);
		F32 maxAlpha = gSavedSettings.getF32("RenderGlowMaxExtractAlpha");		
		F32 warmthAmount = gSavedSettings.getF32("RenderGlowWarmthAmount");	
		LLVector3 lumWeights = gSavedSkinSettings.getVector3("RenderGlowLumWeights");
		LLVector3 warmthWeights = gSavedSkinSettings.getVector3("RenderGlowWarmthWeights");
		gGlowExtractProgram.uniform1f("minLuminance", minLum);
		gGlowExtractProgram.uniform1f("maxExtractAlpha", maxAlpha);
		gGlowExtractProgram.uniform3f("lumWeights", lumWeights.mV[0], lumWeights.mV[1], lumWeights.mV[2]);
		gGlowExtractProgram.uniform3f("warmthWeights", warmthWeights.mV[0], warmthWeights.mV[1], warmthWeights.mV[2]);
		gGlowExtractProgram.uniform1f("warmthAmount", warmthAmount);
		LLGLEnable blend_on(GL_BLEND);
		LLGLEnable test(GL_ALPHA_TEST);
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);		
		gGL.getTexUnit(0)->disable();
		gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);
		gGL.getTexUnit(0)->bind(&mScreen);

		gGL.color4f(1,1,1,1);
		gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
		gGL.begin(LLRender::TRIANGLE_STRIP);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,3);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(3,-1);
		
		gGL.end();
		
		gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

		mGlow[2].flush();
	}

	tc1.setVec(0,0);
	tc2.setVec(2,2);



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
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		{
			LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM_FBO);
			mGlow[i%2].bindTarget();
			mGlow[i%2].clear();
		}
			
		if (i == 0)
		{
			gGL.getTexUnit(0)->bind(&mGlow[2]);
		}
		else
		{
			gGL.getTexUnit(0)->bind(&mGlow[(i-1)%2]);
		}

		if (i%2 == 0)
		{
			gGlowProgram.uniform2f("glowDelta", delta, 0);
		}
		else
		{
			gGlowProgram.uniform2f("glowDelta", 0, delta);
		}

		gGL.begin(LLRender::TRIANGLE_STRIP);
		gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
		gGL.vertex2f(-1,-1);
		
		gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
		gGL.vertex2f(-1,3);
		
		gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
		gGL.vertex2f(3,-1);
		
		gGL.end();
		
		mGlow[i%2].flush();
	}

	gGlowProgram.unbind();

	if (LLRenderTarget::sUseFBO)
	{
		LLFastTimer ftm(LLFastTimer::FTM_RENDER_BLOOM_FBO);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	gGLViewport[0] = gViewerWindow->getWorldViewRect().mLeft;
	gGLViewport[1] = gViewerWindow->getWorldViewRect().mBottom;
	gGLViewport[2] = gViewerWindow->getWorldViewRect().getWidth();
	gGLViewport[3] = gViewerWindow->getWorldViewRect().getHeight();
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);

	gGL.flush();
	
	{
		LLVertexBuffer::unbind();

		
		
		tc2.setVec((F32) gViewerWindow->getWorldViewWidth(),
			(F32) gViewerWindow->getWorldViewHeight());

		if (res_mod > 1)
		{
			tc2 /= (F32) res_mod;
		}

		U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;
		LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(mask, 0);
		buff->allocateBuffer(3,0,TRUE);

		LLStrider<LLVector3> v;
		LLStrider<LLVector2> uv1;
		LLStrider<LLVector2> uv2;

		buff->getVertexStrider(v);
		buff->getTexCoord0Strider(uv1);
		buff->getTexCoord1Strider(uv2);
		
		uv1[0] = LLVector2(0, 0);
		uv1[1] = LLVector2(0, 2);
		uv1[2] = LLVector2(2, 0);
		
		uv2[0] = LLVector2(0, 0);
		uv2[1] = LLVector2(0, tc2.mV[1]*2.f);
		uv2[2] = LLVector2(tc2.mV[0]*2.f, 0);
		
		v[0] = LLVector3(-1,-1,0);
		v[1] = LLVector3(-1,3,0);
		v[2] = LLVector3(3,-1,0);
				
		buff->setBuffer(0);

		LLGLDisable blend(GL_BLEND);

		//tex unit 0
		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_COLOR);
	
		gGL.getTexUnit(0)->bind(&mGlow[1]);
		gGL.getTexUnit(1)->activate();
		gGL.getTexUnit(1)->enable(LLTexUnit::TT_RECT_TEXTURE);
		
		//tex unit 1
		gGL.getTexUnit(1)->setTextureColorBlend(LLTexUnit::TBO_ADD, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_PREV_COLOR);
		
		gGL.getTexUnit(1)->bind(&mScreen);
		gGL.getTexUnit(1)->activate();
		
		LLGLEnable multisample(GL_MULTISAMPLE_ARB);
		
		buff->setBuffer(mask);
		buff->drawArrays(LLRender::TRIANGLE_STRIP, 0, 3);
		
		gGL.getTexUnit(1)->disable();
		gGL.getTexUnit(1)->setTextureBlendType(LLTexUnit::TB_MULT);

		gGL.getTexUnit(0)->activate();
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

void LLPipeline::bindDeferredShader(LLGLSLShader& shader, U32 light_index)
{
	shader.bind();
	S32 channel = 0;
	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		mDeferredScreen.bindTexture(0,channel);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_SPECULAR, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		mDeferredScreen.bindTexture(1, channel);
	}

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_NORMAL, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		mDeferredScreen.bindTexture(2, channel);
	}

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_POSITION, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		mDeferredScreen.bindTexture(3, channel);
	}

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_DEPTH, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		gGL.getTexUnit(channel)->bind(&mDeferredScreen, TRUE);
	}

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_NOISE);
	if (channel > -1)
	{
		gGL.getTexUnit(channel)->bindManual(LLTexUnit::TT_TEXTURE, mNoiseMap);
	}

	stop_glerror();

	channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_LIGHT, LLTexUnit::TT_RECT_TEXTURE);
	if (channel > -1)
	{
		mDeferredLight[light_index].bindTexture(0, channel);
	}

	stop_glerror();

	for (U32 i = 0; i < 4; i++)
	{
		channel = shader.enableTexture(LLViewerShaderMgr::DEFERRED_SHADOW0+i);
		stop_glerror();
		if (channel > -1)
		{
			stop_glerror();
			gGL.getTexUnit(channel)->bind(&mSunShadow[i], TRUE);
			gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
			stop_glerror();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			stop_glerror();
		}
	}

	stop_glerror();

	F32 mat[64];
	for (U32 i = 0; i < 16; i++)
	{
		mat[i] = mSunShadowMatrix[0].m[i];
		mat[i+16] = mSunShadowMatrix[1].m[i];
		mat[i+32] = mSunShadowMatrix[2].m[i];
		mat[i+48] = mSunShadowMatrix[3].m[i];
	}

	shader.uniformMatrix4fv("shadow_matrix[0]", 4, FALSE, mat);
	shader.uniformMatrix4fv("shadow_matrix", 4, FALSE, mat);

	stop_glerror();

	channel = shader.enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
	if (channel > -1)
	{
		LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
		if (cube_map)
		{
			cube_map->enable(channel);
			cube_map->bind();
			F64* m = gGLModelView;

			
			F32 mat[] = { m[0], m[1], m[2],
						  m[4], m[5], m[6],
						  m[8], m[9], m[10] };
		
			shader.uniform3fv("env_mat[0]", 3, mat);
			shader.uniform3fv("env_mat", 3, mat);
		}
	}

	shader.uniform4fv("shadow_clip", 1, mSunClipPlanes.mV);
	shader.uniform1f("sun_wash", gSavedSettings.getF32("RenderDeferredSunWash"));
	shader.uniform1f("shadow_noise", gSavedSettings.getF32("RenderShadowNoise"));
	shader.uniform1f("blur_size", gSavedSettings.getF32("RenderShadowBlurSize"));

	shader.uniform1f("ssao_radius", gSavedSettings.getF32("RenderSSAOScale"));
	shader.uniform1f("ssao_max_radius", gSavedSettings.getU32("RenderSSAOMaxScale"));

	F32 ssao_factor = gSavedSettings.getF32("RenderSSAOFactor");
	shader.uniform1f("ssao_factor", ssao_factor);
	shader.uniform1f("ssao_factor_inv", 1.0/ssao_factor);

	LLVector3 ssao_effect = gSavedSettings.getVector3("RenderSSAOEffect");
	F32 matrix_diag = (ssao_effect[0] + 2.0*ssao_effect[1])/3.0;
	F32 matrix_nondiag = (ssao_effect[0] - ssao_effect[1])/3.0;
	// This matrix scales (proj of color onto <1/rt(3),1/rt(3),1/rt(3)>) by
	// value factor, and scales remainder by saturation factor
	F32 ssao_effect_mat[] = {	matrix_diag, matrix_nondiag, matrix_nondiag,
								matrix_nondiag, matrix_diag, matrix_nondiag,
								matrix_nondiag, matrix_nondiag, matrix_diag};
	shader.uniformMatrix3fv("ssao_effect_mat", 1, GL_FALSE, ssao_effect_mat);

	shader.uniform2f("screen_res", mDeferredScreen.getWidth(), mDeferredScreen.getHeight());
	shader.uniform1f("near_clip", LLViewerCamera::getInstance()->getNear()*2.f);
	shader.uniform1f("alpha_soften", gSavedSettings.getF32("RenderDeferredAlphaSoften"));
}

void LLPipeline::renderDeferredLighting()
{
	if (!sCull)
	{
		return;
	}

	LLGLEnable multisample(GL_MULTISAMPLE_ARB);

	if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
	{
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
	}

	//ati doesn't seem to love actually using the stencil buffer on FBO's
	LLGLEnable stencil(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	gGL.setColorMask(true, true);

	mDeferredLight[0].bindTarget();

	//mDeferredLight[0].copyContents(mDeferredScreen, 0, 0, mDeferredScreen.getWidth(), mDeferredScreen.getHeight(),
	//					0, 0, mDeferredLight[0].getWidth(), mDeferredLight[0].getHeight(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);	
	
	//draw a cube around every light
	LLVertexBuffer::unbind();

	glBlendFunc(GL_ONE, GL_ONE);
	LLGLEnable cull(GL_CULL_FACE);
	LLGLEnable blend(GL_BLEND);

	glh::matrix4f mat = glh_copy_matrix(gGLModelView);

	F32 vert[] = 
	{
		-1,1,
		-1,-3,
		3,1,
	};

	bindDeferredShader(gDeferredSunProgram);

	glh::matrix4f inv_trans = glh_get_current_modelview().inverse().transpose();

	const U32 slice = 32;
	F32 offset[slice*3];
	for (U32 i = 0; i < 4; i++)
	{
		for (U32 j = 0; j < 8; j++)
		{
			glh::vec3f v;
			v.set_value(sinf(6.284f/8*j), cosf(6.284f/8*j), -(F32) i);
			v.normalize();
			inv_trans.mult_matrix_vec(v);
			v.normalize();
			offset[(i*8+j)*3+0] = v.v[0];
			offset[(i*8+j)*3+1] = v.v[2];
			offset[(i*8+j)*3+2] = v.v[1];
		}
	}

	gDeferredSunProgram.uniform3fv("offset", slice, offset);
	gDeferredSunProgram.uniform2f("screenRes", mDeferredLight[0].getWidth(), mDeferredLight[0].getHeight());

	setupHWLights(NULL); //to set mSunDir;

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	LLVector4 dir(mSunDir, 0.f);

	glh::vec4f tc(dir.mV);
	mat.mult_matrix_vec(tc);
	glTexCoord4f(tc.v[0], tc.v[1], tc.v[2], 0);
	glColor3f(1,1,1);

	glVertexPointer(2, GL_FLOAT, 0, vert);
	{
		LLGLDisable blend(GL_BLEND);
		LLGLDepthTest depth(GL_FALSE);
		stop_glerror();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		stop_glerror();
	}
	
	unbindDeferredShader(gDeferredSunProgram);

	mDeferredLight[0].flush();

	//blur lightmap
	mDeferredLight[1].bindTarget();

	//mDeferredLight[1].copyContents(mDeferredScreen, 0, 0, mDeferredScreen.getWidth(), mDeferredScreen.getHeight(),
	//					0, 0, mDeferredLight[0].getWidth(), mDeferredLight[0].getHeight(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);	
	
	bindDeferredShader(gDeferredBlurLightProgram);

	LLVector3 gauss[32]; // xweight, yweight, offset

	LLVector3 go = gSavedSettings.getVector3("RenderShadowGaussian");
	U32 kern_length = llclamp(gSavedSettings.getU32("RenderShadowBlurSamples"), (U32) 1, (U32) 16)*2 - 1;
	F32 blur_size = gSavedSettings.getF32("RenderShadowBlurSize");

	// sample symmetrically with the middle sample falling exactly on 0.0
	F32 x = -(kern_length/2.0f) + 0.5f;

	for (U32 i = 0; i < kern_length; i++)
	{
		gauss[i].mV[0] = llgaussian(x, go.mV[0]);
		gauss[i].mV[1] = llgaussian(x, go.mV[1]);
		gauss[i].mV[2] = x;
		x += 1.f;
	}
	/* swap the x=0 position to the start of gauss[] so we can
	   treat it specially as an optimization. */
	LLVector3 swap;
	swap = gauss[kern_length/2];
	gauss[kern_length/2] = gauss[0];
	gauss[0] = swap;
	llassert(gauss[0].mV[2] == 0.0f);

	gDeferredBlurLightProgram.uniform2f("delta", 1.f, 0.f);
	gDeferredBlurLightProgram.uniform3fv("kern[0]", kern_length, gauss[0].mV);
	gDeferredBlurLightProgram.uniform3fv("kern", kern_length, gauss[0].mV);
	gDeferredBlurLightProgram.uniform1i("kern_length", kern_length);
	gDeferredBlurLightProgram.uniform1f("kern_scale", blur_size * (kern_length/2.f - 0.5f));

	{
		LLGLDisable blend(GL_BLEND);
		LLGLDepthTest depth(GL_FALSE);
		stop_glerror();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		stop_glerror();
	}
	
	mDeferredLight[1].flush();
	unbindDeferredShader(gDeferredBlurLightProgram);

	bindDeferredShader(gDeferredBlurLightProgram, 1);
	mDeferredLight[0].bindTarget();

	gDeferredBlurLightProgram.uniform2f("delta", 0.f, 1.f);
	gDeferredBlurLightProgram.uniform3fv("kern[0]", kern_length, gauss[0].mV);
	gDeferredBlurLightProgram.uniform3fv("kern", kern_length, gauss[0].mV);
	gDeferredBlurLightProgram.uniform1i("kern_length", kern_length);
	gDeferredBlurLightProgram.uniform1f("kern_scale", blur_size * (kern_length/2.f - 0.5f));

	{
		LLGLDisable blend(GL_BLEND);
		LLGLDepthTest depth(GL_FALSE);
		stop_glerror();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		stop_glerror();
	}
	mDeferredLight[0].flush();
	unbindDeferredShader(gDeferredBlurLightProgram);

	stop_glerror();
	glPopMatrix();
	stop_glerror();
	glMatrixMode(GL_MODELVIEW);
	stop_glerror();
	glPopMatrix();
	stop_glerror();

	//copy depth and stencil from deferred screen
	//mScreen.copyContents(mDeferredScreen, 0, 0, mDeferredScreen.getWidth(), mDeferredScreen.getHeight(),
	//					0, 0, mScreen.getWidth(), mScreen.getHeight(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	mScreen.bindTarget();
	mScreen.clear(GL_COLOR_BUFFER_BIT);
		
	bindDeferredShader(gDeferredSoftenProgram);	
	{
		LLGLDepthTest depth(GL_FALSE);
		LLGLDisable blend(GL_BLEND);
		LLGLDisable test(GL_ALPHA_TEST);

		//full screen blit
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glVertexPointer(2, GL_FLOAT, 0, vert);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	unbindDeferredShader(gDeferredSoftenProgram);

	bindDeferredShader(gDeferredLightProgram);

	std::list<LLVector4> fullscreen_lights;
	std::list<LLVector4> light_colors;

	F32 v[24];
	glVertexPointer(3, GL_FLOAT, 0, v);
	{
		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		for (LLDrawable::drawable_set_t::iterator iter = mLights.begin(); iter != mLights.end(); ++iter)
		{
			LLDrawable* drawablep = *iter;
			
			LLVOVolume* volume = drawablep->getVOVolume();
			if (!volume)
			{
				continue;
			}

			LLVector3 center = drawablep->getPositionAgent();
			F32* c = center.mV;
			F32 s = volume->getLightRadius()*1.5f;

			if (LLViewerCamera::getInstance()->AABBInFrustumNoFarClip(center, LLVector3(s,s,s)) == 0)
			{
				continue;
			}

			sVisibleLightCount++;
			glh::vec3f tc(c);
			mat.mult_matrix_vec(tc);
			
			LLColor3 col = volume->getLightColor();
			col *= volume->getLightIntensity();

			//vertex positions are encoded so the 3 bits of their vertex index 
			//correspond to their axis facing, with bit position 3,2,1 matching
			//axis facing x,y,z, bit set meaning positive facing, bit clear 
			//meaning negative facing
			v[0] = c[0]-s; v[1]  = c[1]-s; v[2]  = c[2]-s;  // 0 - 0000 
			v[3] = c[0]-s; v[4]  = c[1]-s; v[5]  = c[2]+s;  // 1 - 0001
			v[6] = c[0]-s; v[7]  = c[1]+s; v[8]  = c[2]-s;  // 2 - 0010
			v[9] = c[0]-s; v[10] = c[1]+s; v[11] = c[2]+s;  // 3 - 0011
																							   
			v[12] = c[0]+s; v[13] = c[1]-s; v[14] = c[2]-s; // 4 - 0100
			v[15] = c[0]+s; v[16] = c[1]-s; v[17] = c[2]+s; // 5 - 0101
			v[18] = c[0]+s; v[19] = c[1]+s; v[20] = c[2]-s; // 6 - 0110
			v[21] = c[0]+s; v[22] = c[1]+s; v[23] = c[2]+s; // 7 - 0111

			if (LLViewerCamera::getInstance()->getOrigin().mV[0] > c[0] + s + 0.2f ||
				LLViewerCamera::getInstance()->getOrigin().mV[0] < c[0] - s - 0.2f ||
				LLViewerCamera::getInstance()->getOrigin().mV[1] > c[1] + s + 0.2f ||
				LLViewerCamera::getInstance()->getOrigin().mV[1] < c[1] - s - 0.2f ||
				LLViewerCamera::getInstance()->getOrigin().mV[2] > c[2] + s + 0.2f ||
				LLViewerCamera::getInstance()->getOrigin().mV[2] < c[2] - s - 0.2f)
			{ //draw box if camera is outside box
				glTexCoord4f(tc.v[0], tc.v[1], tc.v[2], s*s);
				glColor4f(col.mV[0], col.mV[1], col.mV[2], volume->getLightFalloff()*0.5f);
				glDrawRangeElements(GL_TRIANGLE_FAN, 0, 7, 8,
					GL_UNSIGNED_BYTE, get_box_fan_indices(LLViewerCamera::getInstance(), center));
			}
			else
			{	
				fullscreen_lights.push_back(LLVector4(tc.v[0], tc.v[1], tc.v[2], s*s));
				light_colors.push_back(LLVector4(col.mV[0], col.mV[1], col.mV[2], volume->getLightFalloff()*0.5f));
			}
		}
	}

	unbindDeferredShader(gDeferredLightProgram);

	if (!fullscreen_lights.empty())
	{
		bindDeferredShader(gDeferredMultiLightProgram);
		LLGLDepthTest depth(GL_FALSE);

		//full screen blit
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		U32 count = 0;

		LLVector4 light[16];
		LLVector4 col[16];

		glVertexPointer(2, GL_FLOAT, 0, vert);

		while (!fullscreen_lights.empty())
		{
			light[count] = fullscreen_lights.front();
			fullscreen_lights.pop_front();
			col[count] = light_colors.front();
			light_colors.pop_front();

			count++;
			if (count == 16 || fullscreen_lights.empty())
			{
				gDeferredMultiLightProgram.uniform1i("light_count", count);
				gDeferredMultiLightProgram.uniform4fv("light[0]", count, (GLfloat*) light);
				gDeferredMultiLightProgram.uniform4fv("light", count, (GLfloat*) light);
				gDeferredMultiLightProgram.uniform4fv("light_col[0]", count, (GLfloat*) col);
				gDeferredMultiLightProgram.uniform4fv("light_col", count, (GLfloat*) col);
				count = 0;
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
			}
		}
		
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		unbindDeferredShader(gDeferredMultiLightProgram);
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	{ //render non-deferred geometry
		LLGLDisable blend(GL_BLEND);
		LLGLDisable stencil(GL_STENCIL_TEST);

		U32 render_mask = mRenderTypeMask;
		mRenderTypeMask =	mRenderTypeMask & 
							((1 << LLPipeline::RENDER_TYPE_SKY) |
							(1 << LLPipeline::RENDER_TYPE_CLOUDS) |
							(1 << LLPipeline::RENDER_TYPE_WL_SKY) |
							(1 << LLPipeline::RENDER_TYPE_ALPHA) |
							(1 << LLPipeline::RENDER_TYPE_AVATAR) |
							(1 << LLPipeline::RENDER_TYPE_WATER) |
							(1 << LLPipeline::RENDER_TYPE_FULLBRIGHT) |
							(1 << LLPipeline::RENDER_TYPE_VOLUME) |
							(1 << LLPipeline::RENDER_TYPE_GLOW) |
							(1 << LLPipeline::RENDER_TYPE_BUMP));
		
		renderGeomPostDeferred(*LLViewerCamera::getInstance());
		mRenderTypeMask = render_mask;
	}

	mScreen.flush();
						
}

void LLPipeline::unbindDeferredShader(LLGLSLShader &shader)
{
	stop_glerror();
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_POSITION, LLTexUnit::TT_RECT_TEXTURE);
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_NORMAL, LLTexUnit::TT_RECT_TEXTURE);
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_DIFFUSE, LLTexUnit::TT_RECT_TEXTURE);
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_SPECULAR, LLTexUnit::TT_RECT_TEXTURE);
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_DEPTH, LLTexUnit::TT_RECT_TEXTURE);
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_LIGHT, LLTexUnit::TT_RECT_TEXTURE);
	for (U32 i = 0; i < 4; i++)
	{
		if (shader.disableTexture(LLViewerShaderMgr::DEFERRED_SHADOW0+i) > -1)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		}
	}
	shader.disableTexture(LLViewerShaderMgr::DEFERRED_NOISE);

	S32 channel = shader.disableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
	if (channel > -1)
	{
		LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
		if (cube_map)
		{
			cube_map->disable();
		}
	}
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(0)->activate();
	shader.unbind();
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
		LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
		if (gAgent.getCameraAnimating() || gAgent.getCameraMode() != CAMERA_MODE_MOUSELOOK)
		{
			avatar = NULL;
		}

		if (avatar)
		{
			avatar->updateAttachmentVisibility(CAMERA_MODE_THIRD_PERSON);
		}
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
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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
				mRenderTypeMask = tmp & ((1 << LLPipeline::RENDER_TYPE_SKY) |
									(1 << LLPipeline::RENDER_TYPE_WL_SKY));
				static LLCullResult result;
				updateCull(camera, result);
				stateSort(camera, result);
				mRenderTypeMask = tmp & ((1 << LLPipeline::RENDER_TYPE_SKY) |
									(1 << LLPipeline::RENDER_TYPE_CLOUDS) |
									(1 << LLPipeline::RENDER_TYPE_WL_SKY));
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

			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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

		gViewerWindow->setup3DViewport();
		mRenderTypeMask = type_mask;
		LLDrawPoolWater::sNeedsReflectionUpdate = FALSE;
		LLDrawPoolWater::sNeedsDistortionUpdate = FALSE;
		LLViewerCamera::getInstance()->setUserClipPlane(LLPlane(-pnorm, -pd));
		LLPipeline::sUseOcclusion = occlusion;
		
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();

		if (avatar)
		{
			avatar->updateAttachmentVisibility(gAgent.getCameraMode());
		}
	}
}

glh::matrix4f look(const LLVector3 pos, const LLVector3 dir, const LLVector3 up)
{
	glh::matrix4f ret;

	LLVector3 dirN;
	LLVector3 upN;
	LLVector3 lftN;

	lftN = dir % up;
	lftN.normVec();
	
	upN = lftN % dir;
	upN.normVec();
	
	dirN = dir;
	dirN.normVec();
	

	ret.m[ 0] = lftN[0];
	ret.m[ 1] = upN[0];
	ret.m[ 2] = -dirN[0];
	ret.m[ 3] = 0.f;

	ret.m[ 4] = lftN[1];
	ret.m[ 5] = upN[1];
	ret.m[ 6] = -dirN[1];
	ret.m[ 7] = 0.f;

	ret.m[ 8] = lftN[2];
	ret.m[ 9] = upN[2];
	ret.m[10] = -dirN[2];
	ret.m[11] = 0.f;

	ret.m[12] = -(lftN*pos);
	ret.m[13] = -(upN*pos);
	ret.m[14] = dirN*pos;
	ret.m[15] = 1.f;

	return ret;
}

glh::matrix4f scale_translate_to_fit(const LLVector3 min, const LLVector3 max)
{
	glh::matrix4f ret;
	ret.m[ 0] = 2/(max[0]-min[0]);
	ret.m[ 4] = 0;
	ret.m[ 8] = 0;
	ret.m[12] = -(max[0]+min[0])/(max[0]-min[0]);

	ret.m[ 1] = 0;
	ret.m[ 5] = 2/(max[1]-min[1]);
	ret.m[ 9] = 0;
	ret.m[13] = -(max[1]+min[1])/(max[1]-min[1]);

	ret.m[ 2] = 0;
	ret.m[ 6] = 0;
	ret.m[10] = 2/(max[2]-min[2]);
	ret.m[14] = -(max[2]+min[2])/(max[2]-min[2]);

	ret.m[ 3] = 0;
	ret.m[ 7] = 0;
	ret.m[11] = 0;
	ret.m[15] = 1;

	return ret;
}

void LLPipeline::generateSunShadow(LLCamera& camera)
{

	if (!sRenderDeferred)
	{
		return;
	}

	//temporary hack to disable shadows but keep local lights
	static BOOL clear = TRUE;
	BOOL gen_shadow = gSavedSettings.getBOOL("RenderDeferredSunShadow");
	if (!gen_shadow)
	{
		if (clear)
		{
			clear = FALSE;
			for (U32 i = 0; i < 4; i++)
			{
				mSunShadow[i].bindTarget();
				mSunShadow[i].clear();
				mSunShadow[i].flush();
			}
		}
		return;
	}
	clear = TRUE;

	gGL.setColorMask(false, false);

	//get sun view matrix
	
	F32 range = 128.f;

	//store current projection/modelview matrix
	glh::matrix4f saved_proj = glh_get_current_projection();
	glh::matrix4f saved_view = glh_get_current_modelview();
	glh::matrix4f inv_view = saved_view.inverse();

	glh::matrix4f view[4];
	glh::matrix4f proj[4];
	LLVector3 up;

	//clip contains parallel split distances for 3 splits
	LLVector3 clip = gSavedSettings.getVector3("RenderShadowClipPlanes");

	//far clip on last split is minimum of camera view distance and 128
	mSunClipPlanes = LLVector4(clip, clip.mV[2] * clip.mV[2]/clip.mV[1]);

	const LLPickInfo& pick_info = gViewerWindow->getLastPick();

	if (!pick_info.mPosGlobal.isExactlyZero())
	{ //squish nearest frustum based on alt-zoom (tighten up nearest frustum when focusing on tiny object
		F32 focus_dist = (F32) (pick_info.mPosGlobal + LLVector3d(pick_info.mObjectOffset) - gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin())).magVec();
		mSunClipPlanes.mV[0] = llclamp(focus_dist*focus_dist, 2.f, mSunClipPlanes.mV[0]);
	}
		
	// convenience array of 4 near clip plane distances
	F32 dist[] = { 0.1f, mSunClipPlanes.mV[0], mSunClipPlanes.mV[1], mSunClipPlanes.mV[2], mSunClipPlanes.mV[3] };

	//currently used for amount to extrude frusta corners for constructing shadow frusta
	LLVector3 n = gSavedSettings.getVector3("RenderShadowNearDist");
	F32 nearDist[] = { n.mV[0], n.mV[1], n.mV[2], n.mV[2] };

	for (S32 j = 0; j < 4; j++)
	{
		//restore render matrices
		glh_set_current_modelview(saved_view);
		glh_set_current_projection(saved_proj);

		//get center of far clip plane (for point of interest later)
		LLVector3 center = camera.getOrigin() + camera.getAtAxis() * range;

		LLVector3 eye = camera.getOrigin();

		//camera used for shadow cull/render
		LLCamera shadow_cam;
		
	    // perspective shadow map
		glh::vec3f p[16];		//point cloud to be contained by shadow projection (light camera space)
		glh::vec3f wp[16];		//point cloud to be contained by shadow projection (world space)
		
		LLVector3 lightDir = -mSunDir;
		glh::vec3f light_dir(lightDir.mV);

		//create light space camera matrix
		LLVector3 at;
		F32 dl = camera.getLeftAxis() * lightDir;
		F32 du = camera.getUpAxis() * lightDir;

		//choose an at axis such that up will be most aligned with lightDir
		if (dl*dl < du*du)
		{
			at = lightDir%camera.getLeftAxis();
		}
		else
		{
			at = lightDir%camera.getUpAxis();
		}

		if (at * camera.getAtAxis() < 0)
		{
			at = -at;
		}
		
		LLVector3 left = lightDir%at;
		up = left%lightDir;
		up.normVec();

		//create world space camera frustum for this split
		shadow_cam = camera;
		shadow_cam.setFar(16.f);
	
		LLViewerCamera::updateFrustumPlanes(shadow_cam);

		LLVector3* frust = shadow_cam.mAgentFrustum;

		LLVector3 pn = shadow_cam.getAtAxis();
		
		LLVector3 frust_center;
		
		LLVector3 min, max;

		//construct 8 corners of split frustum section
		for (U32 i = 0; i < 4; i++)
		{
			LLVector3 delta = frust[i+4]-eye;
			delta.normVec();
			F32 dp = delta*pn;
			frust[i] = eye + (delta*dist[j])/dp;
			frust[i+4] = eye + (delta*dist[j+1])/dp;
			frust_center += frust[i] + frust[i+4];
		}

		//get frustum center
		frust_center /= 8.f;
						
		shadow_cam.calcAgentFrustumPlanes(frust);

		
		if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
		{
			mShadowCamera[j] = shadow_cam;
		}

		if (gPipeline.getVisibleExtents(shadow_cam, min, max))
		{
			//no possible shadow receivers
			if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowExtents[j][0] = LLVector3();
				mShadowExtents[j][1] = LLVector3();
				mShadowCamera[j+4] = shadow_cam;
			}

			continue;
		}

		if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
		{
			mShadowExtents[j][0] = min;
			mShadowExtents[j][1] = max;
		}
				
		view[j] = look(frust_center-lightDir*nearDist[j], lightDir, up);
		F32 shadow_dist = nearDist[j];

		for (U32 i = 0; i < 8; i++)
		{
			//points in worldspace (wp) and light camera space (p)
			//that must be included in shadow generation
			wp[i] = glh::vec3f(frust[i].mV);
			wp[i+8] = wp[i] - light_dir*shadow_dist;
			view[j].mult_matrix_vec(wp[i], p[i]);
			view[j].mult_matrix_vec(wp[i+8], p[i+8]);
		}
	
		min = LLVector3(p[0].v);
		max = LLVector3(p[0].v);

		LLVector3 fmin = min;
		LLVector3 fmax = max;

		for (U32 i = 1; i < 16; i++)
		{ //find camera space AABB of frustum in light camera space
			update_min_max(min, max, LLVector3(p[i].v));
			if (i < 8)
			{
				update_min_max(fmin, fmax, LLVector3(p[i].v));
			}
		}

		//generate perspective matrix that contains frustum
		//proj[j] = matrix_perspective(min, max);
		proj[j] = gl_ortho(min.mV[0], max.mV[0],
							min.mV[1], max.mV[1],
							-max.mV[2], -min.mV[2]);
						
		shadow_cam.setFar(128.f);
		shadow_cam.setOriginAndLookAt(eye, up, center);

		glh_set_current_modelview(view[j]);
		glh_set_current_projection(proj[j]);

		LLViewerCamera::updateFrustumPlanes(shadow_cam, FALSE, FALSE, TRUE);

		proj[j] = gl_ortho(fmin.mV[0], fmax.mV[0],
						   fmin.mV[1], fmax.mV[1],
						   -fmax.mV[2], -fmin.mV[2]);

		//translate and scale to from [-1, 1] to [0, 1]
		glh::matrix4f trans(0.5f, 0.f, 0.f, 0.5f,
						0.f, 0.5f, 0.f, 0.5f,
						0.f, 0.f, 0.5f, 0.5f,
						0.f, 0.f, 0.f, 1.f);

		glh_set_current_modelview(view[j]);
		glh_set_current_projection(proj[j]);

		mSunShadowMatrix[j] = trans*proj[j]*view[j]*inv_view;
		
		U32 type_mask = mRenderTypeMask;
		mRenderTypeMask = type_mask & ((1<<LLPipeline::RENDER_TYPE_SIMPLE) |
									   (1<<LLPipeline::RENDER_TYPE_ALPHA) |
									   (1<<LLPipeline::RENDER_TYPE_GRASS) |
									   (1<<LLPipeline::RENDER_TYPE_FULLBRIGHT) |
									   (1<<LLPipeline::RENDER_TYPE_BUMP) |
									   (1<<LLPipeline::RENDER_TYPE_VOLUME) |
									   (1<<LLPipeline::RENDER_TYPE_AVATAR) |
									   (1<<LLPipeline::RENDER_TYPE_TREE) | 
									   (1<<LLPipeline::RENDER_TYPE_TERRAIN) |
									   0);

		//clip out geometry on the same side of water as the camera
		static LLCullResult result;
		S32 occlude = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = 1;
		LLPipeline::sShadowRender = TRUE;
		//hack to prevent LOD updates from using sun camera origin
		shadow_cam.setOrigin(camera.getOrigin());
		updateCull(shadow_cam, result);
		stateSort(shadow_cam, result);
		
		if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
		{
			LLViewerCamera::updateFrustumPlanes(shadow_cam, FALSE, FALSE, TRUE);
			mShadowCamera[j+4] = shadow_cam;
		}

		LLFastTimer t(LLFastTimer::FTM_SHADOW_RENDER);

		stop_glerror();

		mSunShadow[j].bindTarget();
		mSunShadow[j].getViewport(gGLViewport);

		{
			LLGLDepthTest depth(GL_TRUE);
			mSunShadow[j].clear();
		}

		U32 types[] = { LLRenderPass::PASS_SIMPLE, LLRenderPass::PASS_FULLBRIGHT, LLRenderPass::PASS_SHINY, LLRenderPass::PASS_BUMP };
		LLGLEnable cull(GL_CULL_FACE);

		//generate sun shadow map
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadMatrixf(proj[j].m);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf(view[j].m);

		stop_glerror();
		gGLLastMatrix = NULL;

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				
		glColor4f(1,1,1,1);
		
		glCullFace(GL_FRONT);

		stop_glerror();

		gGL.setColorMask(false, false);

		gDeferredShadowProgram.bind();
		{
			LLFastTimer ftm(LLFastTimer::FTM_SHADOW_SIMPLE);
			LLGLDisable test(GL_ALPHA_TEST);
			gGL.getTexUnit(0)->disable();
			for (U32 i = 0; i < sizeof(types)/sizeof(U32); ++i)
			{
				renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
			}
			gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
		}
		
		{
			LLFastTimer ftm(LLFastTimer::FTM_SHADOW_ALPHA);
			LLGLEnable test(GL_ALPHA_TEST);
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.6f);
			renderObjects(LLRenderPass::PASS_ALPHA_SHADOW, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR, TRUE);
			glColor4f(1,1,1,1);
			renderObjects(LLRenderPass::PASS_GRASS, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, TRUE);
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}
		
		gDeferredShadowProgram.unbind();

		renderGeomShadow(shadow_cam);

		gGL.setColorMask(true, true);

		glCullFace(GL_BACK);
		LLPipeline::sUseOcclusion = occlude;
		LLPipeline::sShadowRender = FALSE;
		mRenderTypeMask = type_mask;
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		gGLLastMatrix = NULL;

		mSunShadow[j].flush();
	}

	if (!gSavedSettings.getBOOL("CameraOffset"))
	{
		glh_set_current_modelview(saved_view);
		glh_set_current_projection(saved_proj);
	}
	else
	{
		glh_set_current_modelview(view[1]);
		glh_set_current_projection(proj[1]);
		glLoadMatrixf(view[1].m);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(proj[1].m);
		glMatrixMode(GL_MODELVIEW);
	}
	gGL.setColorMask(true, false);
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
	LLMemType mt_gi(LLMemType::MTYPE_PIPELINE_GENERATE_IMPOSTOR);
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

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
				(1<<LLPipeline::RENDER_TYPE_FULLBRIGHT) |
				(1<<LLPipeline::RENDER_TYPE_ALPHA) | 
				(1<<LLPipeline::RENDER_TYPE_INVISIBLE);
	}
	
	mask = mask & gPipeline.getRenderTypeMask();
	U32 saved_mask = gPipeline.mRenderTypeMask;
	gPipeline.mRenderTypeMask = mask;

	S32 occlusion = sUseOcclusion;
	sUseOcclusion = 0;
	sReflectionRender = sRenderDeferred ? FALSE : TRUE;
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
	left.normalize();

	LLVector3 up = camera.getUpAxis();
	up *= up;
	up.normalize();

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
		if (LLPipeline::sRenderDeferred)
		{
			avatar->mImpostor.allocate(resX,resY,GL_RGBA16F_ARB,TRUE,TRUE);
			addDeferredAttachments(avatar->mImpostor);
		}
		else
		{
			avatar->mImpostor.allocate(resX,resY,GL_RGBA,TRUE,TRUE);
		}
		gGL.getTexUnit(0)->bind(&avatar->mImpostor);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, resX, resY);
		avatar->mImpostor.bindTarget();
		avatar->mImpostor.clear();
	}
	
	LLGLEnable stencil(GL_STENCIL_TEST);

	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if (LLPipeline::sRenderDeferred)
	{
		stop_glerror();
		renderGeomDeferred(camera);
	}
	else
	{
		renderGeom(camera);
	}
	
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 1, 0xFFFFFF);

	if (!sRenderDeferred || muted)
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
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLGLDepthTest depth(GL_FALSE, GL_FALSE);

		gGL.color4f(1,1,1,1);
		gGL.color4ub(64,64,64,255);
		gGL.begin(LLRender::QUADS);
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

	LLVertexBuffer::unbind();
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
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


