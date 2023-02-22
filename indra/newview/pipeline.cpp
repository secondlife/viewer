/** 
 * @file pipeline.cpp
 * @brief Rendering pipeline.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"

// library includes
#include "llaudioengine.h" // For debugging.
#include "llerror.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llnamevalue.h"
#include "llpointer.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "v3color.h"
#include "llui.h" 
#include "llglheaders.h"
#include "llrender.h"
#include "llwindow.h"	// swapBuffers()

// newview includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"
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
#include "llfloaterreg.h"
#include "llgldbg.h"
#include "llhudmanager.h"
#include "llhudnametag.h"
#include "llhudtext.h"
#include "lllightconstants.h"
#include "llmeshrepository.h"
#include "llpipelinelistener.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltracker.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewermediafocus.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h" // for audio debugging.
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatarself.h"
#include "llvocache.h"
#include "llvoground.h"
#include "llvosky.h"
#include "llvowlsky.h"
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
#include "llspatialpartition.h"
#include "llmutelist.h"
#include "lltoolpie.h"
#include "llnotifications.h"
#include "llpathinglib.h"
#include "llfloaterpathfindingconsole.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloatertools.h"
#include "llpanelface.h"
#include "llpathfindingpathtool.h"
#include "llscenemonitor.h"
#include "llprogressview.h"
#include "llcleanup.h"

#include "llenvironment.h"
#include "llsettingsvo.h"

#ifdef _DEBUG
// Debug indices is disabled for now for debug performance - djs 4/24/02
//#define DEBUG_INDICES
#else
//#define DEBUG_INDICES
#endif

// Expensive and currently broken
//
#define MATERIALS_IN_REFLECTIONS 0

// NOTE: Keep in sync with indra/newview/skins/default/xui/en/floater_preferences_graphics_advanced.xml
// NOTE: Unused consts are commented out since some compilers (on macOS) may complain about unused variables.
//  const S32 WATER_REFLECT_NONE_WATER_OPAQUE       = -2;
    const S32 WATER_REFLECT_NONE_WATER_TRANSPARENT  = -1;
    const S32 WATER_REFLECT_MINIMAL                 =  0;
//  const S32 WATER_REFLECT_TERRAIN                 =  1;
    const S32 WATER_REFLECT_STATIC_OBJECTS          =  2;
    const S32 WATER_REFLECT_AVATARS                 =  3;
    const S32 WATER_REFLECT_EVERYTHING              =  4;

bool gShiftFrame = false;

//cached settings
bool LLPipeline::WindLightUseAtmosShaders;
bool LLPipeline::RenderDeferred;
F32 LLPipeline::RenderDeferredSunWash;
U32 LLPipeline::RenderFSAASamples;
U32 LLPipeline::RenderResolutionDivisor;
bool LLPipeline::RenderUIBuffer;
S32 LLPipeline::RenderShadowDetail;
bool LLPipeline::RenderDeferredSSAO;
F32 LLPipeline::RenderShadowResolutionScale;
bool LLPipeline::RenderLocalLights;
bool LLPipeline::RenderDelayCreation;
bool LLPipeline::RenderAnimateRes;
bool LLPipeline::FreezeTime;
S32 LLPipeline::DebugBeaconLineWidth;
F32 LLPipeline::RenderHighlightBrightness;
LLColor4 LLPipeline::RenderHighlightColor;
F32 LLPipeline::RenderHighlightThickness;
bool LLPipeline::RenderSpotLightsInNondeferred;
LLColor4 LLPipeline::PreviewAmbientColor;
LLColor4 LLPipeline::PreviewDiffuse0;
LLColor4 LLPipeline::PreviewSpecular0;
LLColor4 LLPipeline::PreviewDiffuse1;
LLColor4 LLPipeline::PreviewSpecular1;
LLColor4 LLPipeline::PreviewDiffuse2;
LLColor4 LLPipeline::PreviewSpecular2;
LLVector3 LLPipeline::PreviewDirection0;
LLVector3 LLPipeline::PreviewDirection1;
LLVector3 LLPipeline::PreviewDirection2;
F32 LLPipeline::RenderGlowMinLuminance;
F32 LLPipeline::RenderGlowMaxExtractAlpha;
F32 LLPipeline::RenderGlowWarmthAmount;
LLVector3 LLPipeline::RenderGlowLumWeights;
LLVector3 LLPipeline::RenderGlowWarmthWeights;
S32 LLPipeline::RenderGlowResolutionPow;
S32 LLPipeline::RenderGlowIterations;
F32 LLPipeline::RenderGlowWidth;
F32 LLPipeline::RenderGlowStrength;
bool LLPipeline::RenderDepthOfField;
bool LLPipeline::RenderDepthOfFieldInEditMode;
F32 LLPipeline::CameraFocusTransitionTime;
F32 LLPipeline::CameraFNumber;
F32 LLPipeline::CameraFocalLength;
F32 LLPipeline::CameraFieldOfView;
F32 LLPipeline::RenderShadowNoise;
F32 LLPipeline::RenderShadowBlurSize;
F32 LLPipeline::RenderSSAOScale;
U32 LLPipeline::RenderSSAOMaxScale;
F32 LLPipeline::RenderSSAOFactor;
LLVector3 LLPipeline::RenderSSAOEffect;
F32 LLPipeline::RenderShadowOffsetError;
F32 LLPipeline::RenderShadowBiasError;
F32 LLPipeline::RenderShadowOffset;
F32 LLPipeline::RenderShadowBias;
F32 LLPipeline::RenderSpotShadowOffset;
F32 LLPipeline::RenderSpotShadowBias;
LLDrawable* LLPipeline::RenderSpotLight = nullptr;
F32 LLPipeline::RenderEdgeDepthCutoff;
F32 LLPipeline::RenderEdgeNormCutoff;
LLVector3 LLPipeline::RenderShadowGaussian;
F32 LLPipeline::RenderShadowBlurDistFactor;
bool LLPipeline::RenderDeferredAtmospheric;
S32 LLPipeline::RenderReflectionDetail;
F32 LLPipeline::RenderHighlightFadeTime;
LLVector3 LLPipeline::RenderShadowClipPlanes;
LLVector3 LLPipeline::RenderShadowOrthoClipPlanes;
LLVector3 LLPipeline::RenderShadowNearDist;
F32 LLPipeline::RenderFarClip;
LLVector3 LLPipeline::RenderShadowSplitExponent;
F32 LLPipeline::RenderShadowErrorCutoff;
F32 LLPipeline::RenderShadowFOVCutoff;
bool LLPipeline::CameraOffset;
F32 LLPipeline::CameraMaxCoF;
F32 LLPipeline::CameraDoFResScale;
F32 LLPipeline::RenderAutoHideSurfaceAreaLimit;
LLTrace::EventStatHandle<S64> LLPipeline::sStatBatchSize("renderbatchsize");

const F32 BACKLIGHT_DAY_MAGNITUDE_OBJECT = 0.1f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_OBJECT = 0.08f;
const F32 DEFERRED_LIGHT_FALLOFF = 0.5f;
const U32 DEFERRED_VB_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;

extern S32 gBoxFrame;
//extern BOOL gHideSelectedObjects;
extern BOOL gDisplaySwapBuffers;
extern BOOL gDebugGL;

bool	gAvatarBacklight = false;

bool	gDebugPipeline = false;
LLPipeline gPipeline;
const LLMatrix4* gGLLastMatrix = NULL;

LLTrace::BlockTimerStatHandle FTM_RENDER_GEOMETRY("Render Geometry");
LLTrace::BlockTimerStatHandle FTM_RENDER_GRASS("Grass");
LLTrace::BlockTimerStatHandle FTM_RENDER_INVISIBLE("Invisible");
LLTrace::BlockTimerStatHandle FTM_RENDER_SHINY("Shiny");
LLTrace::BlockTimerStatHandle FTM_RENDER_SIMPLE("Simple");
LLTrace::BlockTimerStatHandle FTM_RENDER_TERRAIN("Terrain");
LLTrace::BlockTimerStatHandle FTM_RENDER_TREES("Trees");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI("UI");
LLTrace::BlockTimerStatHandle FTM_RENDER_WATER("Water");
LLTrace::BlockTimerStatHandle FTM_RENDER_WL_SKY("Windlight Sky");
LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA("Alpha Objects");
LLTrace::BlockTimerStatHandle FTM_RENDER_CHARACTERS("Avatars");
LLTrace::BlockTimerStatHandle FTM_RENDER_BUMP("Bump");
LLTrace::BlockTimerStatHandle FTM_RENDER_MATERIALS("Render Materials");
LLTrace::BlockTimerStatHandle FTM_RENDER_FULLBRIGHT("Fullbright");
LLTrace::BlockTimerStatHandle FTM_RENDER_GLOW("Glow");
LLTrace::BlockTimerStatHandle FTM_GEO_UPDATE("Geo Update");
LLTrace::BlockTimerStatHandle FTM_POOLRENDER("RenderPool");
LLTrace::BlockTimerStatHandle FTM_POOLS("Pools");
LLTrace::BlockTimerStatHandle FTM_DEFERRED_POOLRENDER("RenderPool (Deferred)");
LLTrace::BlockTimerStatHandle FTM_DEFERRED_POOLS("Pools (Deferred)");
LLTrace::BlockTimerStatHandle FTM_POST_DEFERRED_POOLRENDER("RenderPool (Post)");
LLTrace::BlockTimerStatHandle FTM_POST_DEFERRED_POOLS("Pools (Post)");
LLTrace::BlockTimerStatHandle FTM_STATESORT("Sort Draw State");
LLTrace::BlockTimerStatHandle FTM_PIPELINE("Pipeline");
LLTrace::BlockTimerStatHandle FTM_CLIENT_COPY("Client Copy");
LLTrace::BlockTimerStatHandle FTM_RENDER_DEFERRED("Deferred Shading");

LLTrace::BlockTimerStatHandle FTM_RENDER_UI_HUD("HUD");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI_3D("3D");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI_2D("2D");

static LLTrace::BlockTimerStatHandle FTM_STATESORT_DRAWABLE("Sort Drawables");

static LLStaticHashedString sTint("tint");
static LLStaticHashedString sAmbiance("ambiance");
static LLStaticHashedString sAlphaScale("alpha_scale");
static LLStaticHashedString sNormMat("norm_mat");
static LLStaticHashedString sOffset("offset");
static LLStaticHashedString sScreenRes("screenRes");
static LLStaticHashedString sDelta("delta");
static LLStaticHashedString sDistFactor("dist_factor");
static LLStaticHashedString sKern("kern");
static LLStaticHashedString sKernScale("kern_scale");

//----------------------------------------
std::string gPoolNames[] = 
{
	// Correspond to LLDrawpool enum render type
	"NONE",
	"POOL_SIMPLE",
	"POOL_GROUND",
	"POOL_FULLBRIGHT",
	"POOL_BUMP",
	"POOL_MATERIALS",
	"POOL_TERRAIN,"	
	"POOL_SKY",
	"POOL_WL_SKY",
	"POOL_TREE",
	"POOL_ALPHA_MASK",
	"POOL_FULLBRIGHT_ALPHA_MASK",
	"POOL_GRASS",
	"POOL_INVISIBLE",
	"POOL_AVATAR",
	"POOL_VOIDWATER",
	"POOL_WATER",
	"POOL_GLOW",
	"POOL_ALPHA"
};

void drawBox(const LLVector4a& c, const LLVector4a& r);
void drawBoxOutline(const LLVector3& pos, const LLVector3& size);
U32 nhpo2(U32 v);
LLVertexBuffer* ll_create_cube_vb(U32 type_mask, U32 usage);

void display_update_camera();
//----------------------------------------

S32		LLPipeline::sCompiles = 0;

bool	LLPipeline::sPickAvatar = true;
bool	LLPipeline::sDynamicLOD = true;
bool	LLPipeline::sShowHUDAttachments = true;
bool	LLPipeline::sRenderMOAPBeacons = false;
bool	LLPipeline::sRenderPhysicalBeacons = true;
bool	LLPipeline::sRenderScriptedBeacons = false;
bool	LLPipeline::sRenderScriptedTouchBeacons = true;
bool	LLPipeline::sRenderParticleBeacons = false;
bool	LLPipeline::sRenderSoundBeacons = false;
bool	LLPipeline::sRenderBeacons = false;
bool	LLPipeline::sRenderHighlight = true;
LLRender::eTexIndex LLPipeline::sRenderHighlightTextureChannel = LLRender::DIFFUSE_MAP;
bool	LLPipeline::sForceOldBakedUpload = false;
S32		LLPipeline::sUseOcclusion = 0;
bool	LLPipeline::sDelayVBUpdate = true;
bool	LLPipeline::sAutoMaskAlphaDeferred = true;
bool	LLPipeline::sAutoMaskAlphaNonDeferred = false;
bool	LLPipeline::sRenderTransparentWater = true;
bool	LLPipeline::sRenderBump = true;
bool	LLPipeline::sBakeSunlight = false;
bool	LLPipeline::sNoAlpha = false;
bool	LLPipeline::sUseTriStrips = true;
bool	LLPipeline::sUseFarClip = true;
bool	LLPipeline::sShadowRender = false;
bool	LLPipeline::sWaterReflections = false;
bool	LLPipeline::sRenderGlow = false;
bool	LLPipeline::sReflectionRender = false;
bool    LLPipeline::sDistortionRender = false;
bool	LLPipeline::sImpostorRender = false;
bool	LLPipeline::sImpostorRenderAlphaDepthPass = false;
bool	LLPipeline::sUnderWaterRender = false;
bool	LLPipeline::sTextureBindTest = false;
bool	LLPipeline::sRenderFrameTest = false;
bool	LLPipeline::sRenderAttachedLights = true;
bool	LLPipeline::sRenderAttachedParticles = true;
bool	LLPipeline::sRenderDeferred = false;
S32		LLPipeline::sVisibleLightCount = 0;
bool	LLPipeline::sRenderingHUDs;
F32     LLPipeline::sDistortionWaterClipPlaneMargin = 1.0125f;

// EventHost API LLPipeline listener.
static LLPipelineListener sPipelineListener;

static LLCullResult* sCull = NULL;

void validate_framebuffer_object();

// Add color attachments for deferred rendering
// target -- RenderTarget to add attachments to
// for_impostor -- whether or not these render targets are for an impostor (if true, avoids implicit sRGB conversions)
bool addDeferredAttachments(LLRenderTarget& target, bool for_impostor = false)
{
	return target.addColorAttachment(for_impostor ? GL_RGBA : GL_SRGB8_ALPHA8) && //specular
			target.addColorAttachment(GL_RGB10_A2); //normal+z
}

LLPipeline::LLPipeline() :
	mBackfaceCull(false),
	mMatrixOpCount(0),
	mTextureMatrixOps(0),
	mNumVisibleNodes(0),
	mNumVisibleFaces(0),

	mInitialized(false),
	mShadersLoaded(false),
	mTransformFeedbackPrimitives(0),
	mRenderDebugFeatureMask(0),
	mRenderDebugMask(0),
	mOldRenderDebugMask(0),
	mMeshDirtyQueryObject(0),
	mGroupQ1Locked(false),
	mGroupQ2Locked(false),
	mResetVertexBuffers(false),
	mLastRebuildPool(NULL),
	mAlphaPool(NULL),
	mSkyPool(NULL),
	mTerrainPool(NULL),
	mWaterPool(NULL),
	mGroundPool(NULL),
	mSimplePool(NULL),
	mGrassPool(NULL),
	mAlphaMaskPool(NULL),
	mFullbrightAlphaMaskPool(NULL),
	mFullbrightPool(NULL),
	mInvisiblePool(NULL),
	mGlowPool(NULL),
	mBumpPool(NULL),
	mMaterialsPool(NULL),
	mWLSkyPool(NULL),
	mLightMask(0),
	mLightMovingMask(0),
	mLightingDetail(0),
	mScreenWidth(0),
	mScreenHeight(0)
{
	mNoiseMap = 0;
	mTrueNoiseMap = 0;
	mLightFunc = 0;

    for(U32 i = 0; i < 8; i++)
    {
        mHWLightColors[i] = LLColor4::black;
    }
}

void LLPipeline::connectRefreshCachedSettingsSafe(const std::string name)
{
	LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl(name);
	if ( cntrl_ptr.isNull() )
	{
		LL_WARNS() << "Global setting name not found:" << name << LL_ENDL;
	}
	else
	{
		cntrl_ptr->getCommitSignal()->connect(boost::bind(&LLPipeline::refreshCachedSettings));
	}
}

void LLPipeline::init()
{
	refreshCachedSettings();

	gOctreeMaxCapacity = gSavedSettings.getU32("OctreeMaxNodeCapacity");
	gOctreeMinSize = gSavedSettings.getF32("OctreeMinimumNodeSize");
	sDynamicLOD = gSavedSettings.getBOOL("RenderDynamicLOD");
	sRenderBump = gSavedSettings.getBOOL("RenderObjectBump");
	sUseTriStrips = gSavedSettings.getBOOL("RenderUseTriStrips");
	LLVertexBuffer::sUseStreamDraw = gSavedSettings.getBOOL("RenderUseStreamVBO");
	LLVertexBuffer::sUseVAO = gSavedSettings.getBOOL("RenderUseVAO");
	LLVertexBuffer::sPreferStreamDraw = gSavedSettings.getBOOL("RenderPreferStreamDraw");
	sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
	sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");

	mInitialized = true;
	
	stop_glerror();

	//create render pass pools
	getPool(LLDrawPool::POOL_ALPHA);
	getPool(LLDrawPool::POOL_SIMPLE);
	getPool(LLDrawPool::POOL_ALPHA_MASK);
	getPool(LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK);
	getPool(LLDrawPool::POOL_GRASS);
	getPool(LLDrawPool::POOL_FULLBRIGHT);
	getPool(LLDrawPool::POOL_INVISIBLE);
	getPool(LLDrawPool::POOL_BUMP);
	getPool(LLDrawPool::POOL_MATERIALS);
	getPool(LLDrawPool::POOL_GLOW);

	resetFrameStats();

	if (gSavedSettings.getBOOL("DisableAllRenderFeatures"))
	{
		clearAllRenderDebugFeatures();
	}
	else
	{
		setAllRenderDebugFeatures(); // By default, all debugging features on
	}
	clearAllRenderDebugDisplays(); // All debug displays off

	if (gSavedSettings.getBOOL("DisableAllRenderTypes"))
	{
		clearAllRenderTypes();
	}
	else if (gNonInteractive)
	{
		clearAllRenderTypes();
	}
	else
	{
		setAllRenderTypes(); // By default, all rendering types start enabled
		// Don't turn on ground when this is set
		// Mac Books with intel 950s need this
		if(!gSavedSettings.getBOOL("RenderGround"))
		{
			toggleRenderType(RENDER_TYPE_GROUND);
		}
	}

	// make sure RenderPerformanceTest persists (hackity hack hack)
	// disables non-object rendering (UI, sky, water, etc)
	if (gSavedSettings.getBOOL("RenderPerformanceTest"))
	{
		gSavedSettings.setBOOL("RenderPerformanceTest", FALSE);
		gSavedSettings.setBOOL("RenderPerformanceTest", TRUE);
	}

	mOldRenderDebugMask = mRenderDebugMask;

	mBackfaceCull = true;

	stop_glerror();
	
	// Enable features
		
	LLViewerShaderMgr::instance()->setShaders();

	stop_glerror();

	for (U32 i = 0; i < 2; ++i)
	{
		mSpotLightFade[i] = 1.f;
	}

	if (mCubeVB.isNull())
	{
		mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX, GL_STATIC_DRAW_ARB);
	}

	mDeferredVB = new LLVertexBuffer(DEFERRED_VB_MASK, 0);
	mDeferredVB->allocateBuffer(8, 0, true);
	setLightingDetail(-1);
	
	//
	// Update all settings to trigger a cached settings refresh
	//
	connectRefreshCachedSettingsSafe("RenderAutoMaskAlphaDeferred");
	connectRefreshCachedSettingsSafe("RenderAutoMaskAlphaNonDeferred");
	connectRefreshCachedSettingsSafe("RenderUseFarClip");
	connectRefreshCachedSettingsSafe("RenderAvatarMaxNonImpostors");
	connectRefreshCachedSettingsSafe("RenderDelayVBUpdate");
	connectRefreshCachedSettingsSafe("UseOcclusion");
	connectRefreshCachedSettingsSafe("WindLightUseAtmosShaders");
	connectRefreshCachedSettingsSafe("RenderDeferred");
	connectRefreshCachedSettingsSafe("RenderDeferredSunWash");
	connectRefreshCachedSettingsSafe("RenderFSAASamples");
	connectRefreshCachedSettingsSafe("RenderResolutionDivisor");
	connectRefreshCachedSettingsSafe("RenderUIBuffer");
	connectRefreshCachedSettingsSafe("RenderShadowDetail");
	connectRefreshCachedSettingsSafe("RenderDeferredSSAO");
	connectRefreshCachedSettingsSafe("RenderShadowResolutionScale");
	connectRefreshCachedSettingsSafe("RenderLocalLights");
	connectRefreshCachedSettingsSafe("RenderDelayCreation");
	connectRefreshCachedSettingsSafe("RenderAnimateRes");
	connectRefreshCachedSettingsSafe("FreezeTime");
	connectRefreshCachedSettingsSafe("DebugBeaconLineWidth");
	connectRefreshCachedSettingsSafe("RenderHighlightBrightness");
	connectRefreshCachedSettingsSafe("RenderHighlightColor");
	connectRefreshCachedSettingsSafe("RenderHighlightThickness");
	connectRefreshCachedSettingsSafe("RenderSpotLightsInNondeferred");
	connectRefreshCachedSettingsSafe("PreviewAmbientColor");
	connectRefreshCachedSettingsSafe("PreviewDiffuse0");
	connectRefreshCachedSettingsSafe("PreviewSpecular0");
	connectRefreshCachedSettingsSafe("PreviewDiffuse1");
	connectRefreshCachedSettingsSafe("PreviewSpecular1");
	connectRefreshCachedSettingsSafe("PreviewDiffuse2");
	connectRefreshCachedSettingsSafe("PreviewSpecular2");
	connectRefreshCachedSettingsSafe("PreviewDirection0");
	connectRefreshCachedSettingsSafe("PreviewDirection1");
	connectRefreshCachedSettingsSafe("PreviewDirection2");
	connectRefreshCachedSettingsSafe("RenderGlowMinLuminance");
	connectRefreshCachedSettingsSafe("RenderGlowMaxExtractAlpha");
	connectRefreshCachedSettingsSafe("RenderGlowWarmthAmount");
	connectRefreshCachedSettingsSafe("RenderGlowLumWeights");
	connectRefreshCachedSettingsSafe("RenderGlowWarmthWeights");
	connectRefreshCachedSettingsSafe("RenderGlowResolutionPow");
	connectRefreshCachedSettingsSafe("RenderGlowIterations");
	connectRefreshCachedSettingsSafe("RenderGlowWidth");
	connectRefreshCachedSettingsSafe("RenderGlowStrength");
	connectRefreshCachedSettingsSafe("RenderDepthOfField");
	connectRefreshCachedSettingsSafe("RenderDepthOfFieldInEditMode");
	connectRefreshCachedSettingsSafe("CameraFocusTransitionTime");
	connectRefreshCachedSettingsSafe("CameraFNumber");
	connectRefreshCachedSettingsSafe("CameraFocalLength");
	connectRefreshCachedSettingsSafe("CameraFieldOfView");
	connectRefreshCachedSettingsSafe("RenderShadowNoise");
	connectRefreshCachedSettingsSafe("RenderShadowBlurSize");
	connectRefreshCachedSettingsSafe("RenderSSAOScale");
	connectRefreshCachedSettingsSafe("RenderSSAOMaxScale");
	connectRefreshCachedSettingsSafe("RenderSSAOFactor");
	connectRefreshCachedSettingsSafe("RenderSSAOEffect");
	connectRefreshCachedSettingsSafe("RenderShadowOffsetError");
	connectRefreshCachedSettingsSafe("RenderShadowBiasError");
	connectRefreshCachedSettingsSafe("RenderShadowOffset");
	connectRefreshCachedSettingsSafe("RenderShadowBias");
	connectRefreshCachedSettingsSafe("RenderSpotShadowOffset");
	connectRefreshCachedSettingsSafe("RenderSpotShadowBias");
	connectRefreshCachedSettingsSafe("RenderEdgeDepthCutoff");
	connectRefreshCachedSettingsSafe("RenderEdgeNormCutoff");
	connectRefreshCachedSettingsSafe("RenderShadowGaussian");
	connectRefreshCachedSettingsSafe("RenderShadowBlurDistFactor");
	connectRefreshCachedSettingsSafe("RenderDeferredAtmospheric");
	connectRefreshCachedSettingsSafe("RenderReflectionDetail");
	connectRefreshCachedSettingsSafe("RenderHighlightFadeTime");
	connectRefreshCachedSettingsSafe("RenderShadowClipPlanes");
	connectRefreshCachedSettingsSafe("RenderShadowOrthoClipPlanes");
	connectRefreshCachedSettingsSafe("RenderShadowNearDist");
	connectRefreshCachedSettingsSafe("RenderFarClip");
	connectRefreshCachedSettingsSafe("RenderShadowSplitExponent");
	connectRefreshCachedSettingsSafe("RenderShadowErrorCutoff");
	connectRefreshCachedSettingsSafe("RenderShadowFOVCutoff");
	connectRefreshCachedSettingsSafe("CameraOffset");
	connectRefreshCachedSettingsSafe("CameraMaxCoF");
	connectRefreshCachedSettingsSafe("CameraDoFResScale");
	connectRefreshCachedSettingsSafe("RenderAutoHideSurfaceAreaLimit");
	gSavedSettings.getControl("RenderAutoHideSurfaceAreaLimit")->getCommitSignal()->connect(boost::bind(&LLPipeline::refreshCachedSettings));
}

LLPipeline::~LLPipeline()
{

}

void LLPipeline::cleanup()
{
	assertInitialized();

	mGroupQ1.clear() ;
	mGroupQ2.clear() ;

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
		LL_WARNS() << "Terrain Pools not cleaned up" << LL_ENDL;
	}
	if (!mTreePools.empty())
	{
		LL_WARNS() << "Tree Pools not cleaned up" << LL_ENDL;
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

	mFaceSelectImagep = NULL;

    mMovedList.clear();
    mMovedBridge.clear();
    mShiftList.clear();

	mInitialized = false;

	mDeferredVB = NULL;

	mCubeVB = NULL;
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
		LLVertexBuffer::sEnableVBOs = FALSE;
	}

	if (mMeshDirtyQueryObject)
	{
		glDeleteQueriesARB(1, &mMeshDirtyQueryObject);
		mMeshDirtyQueryObject = 0;
	}
}

void LLPipeline::requestResizeScreenTexture()
{
    gResizeScreenTexture = TRUE;
}

void LLPipeline::requestResizeShadowTexture()
{
    gResizeShadowTexture = TRUE;
}

void LLPipeline::resizeShadowTexture()
{
    releaseShadowTargets();
    allocateShadowBuffer(mScreenWidth, mScreenHeight);
    gResizeShadowTexture = FALSE;
}

void LLPipeline::resizeScreenTexture()
{
	if (gPipeline.shadersLoaded())
	{
		GLuint resX = gViewerWindow->getWorldViewWidthRaw();
		GLuint resY = gViewerWindow->getWorldViewHeightRaw();
	
		if (gResizeScreenTexture || (resX != mScreen.getWidth()) || (resY != mScreen.getHeight()))
		{
			releaseScreenBuffers();
            releaseShadowTargets();
		    allocateScreenBuffer(resX,resY);
            gResizeScreenTexture = FALSE;
		}
	}
}

void LLPipeline::allocatePhysicsBuffer()
{
	GLuint resX = gViewerWindow->getWorldViewWidthRaw();
	GLuint resY = gViewerWindow->getWorldViewHeightRaw();

	if (mPhysicsDisplay.getWidth() != resX || mPhysicsDisplay.getHeight() != resY)
	{
		mPhysicsDisplay.allocate(resX, resY, GL_RGBA, TRUE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE);
	}
}

bool LLPipeline::allocateScreenBuffer(U32 resX, U32 resY)
{
	refreshCachedSettings();
	
	bool save_settings = sRenderDeferred;
	if (save_settings)
	{
		// Set this flag in case we crash while resizing window or allocating space for deferred rendering targets
		gSavedSettings.setBOOL("RenderInitError", TRUE);
		gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );
	}

	eFBOStatus ret = doAllocateScreenBuffer(resX, resY);

	if (save_settings)
	{
		// don't disable shaders on next session
		gSavedSettings.setBOOL("RenderInitError", FALSE);
		gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );
	}
	
	if (ret == FBO_FAILURE)
	{ //FAILSAFE: screen buffer allocation failed, disable deferred rendering if it's enabled
		//NOTE: if the session closes successfully after this call, deferred rendering will be 
		// disabled on future sessions
		if (LLPipeline::sRenderDeferred)
		{
			gSavedSettings.setBOOL("RenderDeferred", FALSE);
			LLPipeline::refreshCachedSettings();
		}
	}

	return ret == FBO_SUCCESS_FULLRES;
}


LLPipeline::eFBOStatus LLPipeline::doAllocateScreenBuffer(U32 resX, U32 resY)
{
	// try to allocate screen buffers at requested resolution and samples
	// - on failure, shrink number of samples and try again
	// - if not multisampled, shrink resolution and try again (favor X resolution over Y)
	// Make sure to call "releaseScreenBuffers" after each failure to cleanup the partially loaded state

	U32 samples = RenderFSAASamples;

	eFBOStatus ret = FBO_SUCCESS_FULLRES;
	if (!allocateScreenBuffer(resX, resY, samples))
	{
		//failed to allocate at requested specification, return false
		ret = FBO_FAILURE;

		releaseScreenBuffers();
		//reduce number of samples 
		while (samples > 0)
		{
			samples /= 2;
			if (allocateScreenBuffer(resX, resY, samples))
			{ //success
				return FBO_SUCCESS_LOWRES;
			}
			releaseScreenBuffers();
		}

		samples = 0;

		//reduce resolution
		while (resY > 0 && resX > 0)
		{
			resY /= 2;
			if (allocateScreenBuffer(resX, resY, samples))
			{
				return FBO_SUCCESS_LOWRES;
			}
			releaseScreenBuffers();

			resX /= 2;
			if (allocateScreenBuffer(resX, resY, samples))
			{
				return FBO_SUCCESS_LOWRES;
			}
			releaseScreenBuffers();
		}

		LL_WARNS() << "Unable to allocate screen buffer at any resolution!" << LL_ENDL;
	}

	return ret;
}

bool LLPipeline::allocateScreenBuffer(U32 resX, U32 resY, U32 samples)
{
	refreshCachedSettings();

	// remember these dimensions
	mScreenWidth = resX;
	mScreenHeight = resY;
	
	U32 res_mod = RenderResolutionDivisor;

	if (res_mod > 1 && res_mod < resX && res_mod < resY)
	{
		resX /= res_mod;
		resY /= res_mod;
	}

	if (RenderUIBuffer)
	{
		if (!mUIScreen.allocate(resX,resY, GL_RGBA, FALSE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE))
		{
			return false;
		}
	}	

	if (LLPipeline::sRenderDeferred)
	{
		S32 shadow_detail = RenderShadowDetail;
		bool ssao = RenderDeferredSSAO;
		
		const U32 occlusion_divisor = 3;

		//allocate deferred rendering color buffers
		if (!mDeferredScreen.allocate(resX, resY, GL_SRGB8_ALPHA8, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, FALSE, samples)) return false;
		if (!mDeferredDepth.allocate(resX, resY, 0, TRUE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE, samples)) return false;
		if (!mOcclusionDepth.allocate(resX/occlusion_divisor, resY/occlusion_divisor, 0, TRUE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE, samples)) return false;
		if (!addDeferredAttachments(mDeferredScreen)) return false;
	
		GLuint screenFormat = GL_RGBA16;
		if (gGLManager.mIsAMD)
		{
			screenFormat = GL_RGBA12;
		}

		if (gGLManager.mGLVersion < 4.f && gGLManager.mIsNVIDIA)
		{
			screenFormat = GL_RGBA16F_ARB;
		}
        
		if (!mScreen.allocate(resX, resY, screenFormat, FALSE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE, samples)) return false;
		if (samples > 0)
		{
			if (!mFXAABuffer.allocate(resX, resY, GL_RGBA, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE, samples)) return false;
		}
		else
		{
			mFXAABuffer.release();
		}
		
		if (shadow_detail > 0 || ssao || RenderDepthOfField || samples > 0)
		{ //only need mDeferredLight for shadows OR ssao OR dof OR fxaa
			if (!mDeferredLight.allocate(resX, resY, GL_RGBA, FALSE, FALSE, LLTexUnit::TT_RECT_TEXTURE, FALSE)) return false;
		}
		else
		{
			mDeferredLight.release();
		}

        allocateShadowBuffer(resX, resY);

        //HACK make screenbuffer allocations start failing after 30 seconds
        if (gSavedSettings.getBOOL("SimulateFBOFailure"))
        {
            return false;
        }
    }
    else
    {
        mDeferredLight.release();

        releaseShadowTargets();

		mFXAABuffer.release();
		mScreen.release();
		mDeferredScreen.release(); //make sure to release any render targets that share a depth buffer with mDeferredScreen first
		mDeferredDepth.release();
		mOcclusionDepth.release();
						
		if (!mScreen.allocate(resX, resY, GL_RGBA, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, FALSE)) return false;		
	}
	
	if (LLPipeline::sRenderDeferred)
	{ //share depth buffer between deferred targets
		mDeferredScreen.shareDepthBuffer(mScreen);
	}

	gGL.getTexUnit(0)->disable();

	stop_glerror();

	return true;
}

// must be even to avoid a stripe in the horizontal shadow blur
inline U32 BlurHappySize(U32 x, F32 scale) { return U32( x * scale + 16.0f) & ~0xF; }

bool LLPipeline::allocateShadowBuffer(U32 resX, U32 resY)
{
	refreshCachedSettings();
	
	if (LLPipeline::sRenderDeferred)
	{
		S32 shadow_detail = RenderShadowDetail;

		const U32 occlusion_divisor = 3;

		F32 scale = llmax(0.f,RenderShadowResolutionScale);
		U32 sun_shadow_map_width  = BlurHappySize(resX, scale);
		U32 sun_shadow_map_height = BlurHappySize(resY, scale);

		if (shadow_detail > 0)
		{ //allocate 4 sun shadow maps
			for (U32 i = 0; i < 4; i++)
			{
				if (!mShadow[i].allocate(sun_shadow_map_width, sun_shadow_map_height, 0, TRUE, FALSE, LLTexUnit::TT_TEXTURE))
                {
                    return false;
                }

                if (!mShadowOcclusion[i].allocate(sun_shadow_map_width/occlusion_divisor, sun_shadow_map_height/occlusion_divisor, 0, TRUE, FALSE, LLTexUnit::TT_TEXTURE))
                {
                    return false;
                }
			}
		}
		else
		{
			for (U32 i = 0; i < 4; i++)
			{
                releaseShadowTarget(i);
			}
		}

		U32 width = (U32) (resX*scale);
		U32 height = width;

		if (shadow_detail > 1)
		{ //allocate two spot shadow maps
			U32 spot_shadow_map_width = width;
            U32 spot_shadow_map_height = height;
			for (U32 i = 4; i < 6; i++)
			{
                if (!mShadow[i].allocate(spot_shadow_map_width, spot_shadow_map_height, 0, TRUE, FALSE))
		{
                    return false;
			}
                if (!mShadowOcclusion[i].allocate(spot_shadow_map_width/occlusion_divisor, height/occlusion_divisor, 0, TRUE, FALSE))
		{
			return false;
		}
	}
        }
	else
	{
            for (U32 i = 4; i < 6; i++)
		{
                releaseShadowTarget(i);
		}
	}
	}

	return true;
}

//static
void LLPipeline::updateRenderTransparentWater()
{
    sRenderTransparentWater = gSavedSettings.getBOOL("RenderTransparentWater");
}

//static
void LLPipeline::updateRenderBump()
{
	sRenderBump = gSavedSettings.getBOOL("RenderObjectBump");
}

// static
void LLPipeline::updateRenderDeferred()
{
    sRenderDeferred = !gUseWireframe &&
                      RenderDeferred &&
                      LLRenderTarget::sUseFBO &&
                      LLPipeline::sRenderBump &&
                      WindLightUseAtmosShaders &&
                      (bool) LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred");
}

// static
void LLPipeline::refreshCachedSettings()
{
	LLPipeline::sAutoMaskAlphaDeferred = gSavedSettings.getBOOL("RenderAutoMaskAlphaDeferred");
	LLPipeline::sAutoMaskAlphaNonDeferred = gSavedSettings.getBOOL("RenderAutoMaskAlphaNonDeferred");
	LLPipeline::sUseFarClip = gSavedSettings.getBOOL("RenderUseFarClip");
	LLVOAvatar::sMaxNonImpostors = gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
	LLVOAvatar::updateImpostorRendering(LLVOAvatar::sMaxNonImpostors);
	LLPipeline::sDelayVBUpdate = gSavedSettings.getBOOL("RenderDelayVBUpdate");

	LLPipeline::sUseOcclusion = 
			(!gUseWireframe
			&& LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") 
			&& gSavedSettings.getBOOL("UseOcclusion") 
			&& gGLManager.mHasOcclusionQuery) ? 2 : 0;
	
	WindLightUseAtmosShaders = gSavedSettings.getBOOL("WindLightUseAtmosShaders");
	RenderDeferred = gSavedSettings.getBOOL("RenderDeferred");
	RenderDeferredSunWash = gSavedSettings.getF32("RenderDeferredSunWash");
	RenderFSAASamples = gSavedSettings.getU32("RenderFSAASamples");
	RenderResolutionDivisor = gSavedSettings.getU32("RenderResolutionDivisor");
	RenderUIBuffer = gSavedSettings.getBOOL("RenderUIBuffer");
	RenderShadowDetail = gSavedSettings.getS32("RenderShadowDetail");
	RenderDeferredSSAO = gSavedSettings.getBOOL("RenderDeferredSSAO");
	RenderShadowResolutionScale = gSavedSettings.getF32("RenderShadowResolutionScale");
	RenderLocalLights = gSavedSettings.getBOOL("RenderLocalLights");
	RenderDelayCreation = gSavedSettings.getBOOL("RenderDelayCreation");
	RenderAnimateRes = gSavedSettings.getBOOL("RenderAnimateRes");
	FreezeTime = gSavedSettings.getBOOL("FreezeTime");
	DebugBeaconLineWidth = gSavedSettings.getS32("DebugBeaconLineWidth");
	RenderHighlightBrightness = gSavedSettings.getF32("RenderHighlightBrightness");
	RenderHighlightColor = gSavedSettings.getColor4("RenderHighlightColor");
	RenderHighlightThickness = gSavedSettings.getF32("RenderHighlightThickness");
	RenderSpotLightsInNondeferred = gSavedSettings.getBOOL("RenderSpotLightsInNondeferred");
	PreviewAmbientColor = gSavedSettings.getColor4("PreviewAmbientColor");
	PreviewDiffuse0 = gSavedSettings.getColor4("PreviewDiffuse0");
	PreviewSpecular0 = gSavedSettings.getColor4("PreviewSpecular0");
	PreviewDiffuse1 = gSavedSettings.getColor4("PreviewDiffuse1");
	PreviewSpecular1 = gSavedSettings.getColor4("PreviewSpecular1");
	PreviewDiffuse2 = gSavedSettings.getColor4("PreviewDiffuse2");
	PreviewSpecular2 = gSavedSettings.getColor4("PreviewSpecular2");
	PreviewDirection0 = gSavedSettings.getVector3("PreviewDirection0");
	PreviewDirection1 = gSavedSettings.getVector3("PreviewDirection1");
	PreviewDirection2 = gSavedSettings.getVector3("PreviewDirection2");
	RenderGlowMinLuminance = gSavedSettings.getF32("RenderGlowMinLuminance");
	RenderGlowMaxExtractAlpha = gSavedSettings.getF32("RenderGlowMaxExtractAlpha");
	RenderGlowWarmthAmount = gSavedSettings.getF32("RenderGlowWarmthAmount");
	RenderGlowLumWeights = gSavedSettings.getVector3("RenderGlowLumWeights");
	RenderGlowWarmthWeights = gSavedSettings.getVector3("RenderGlowWarmthWeights");
	RenderGlowResolutionPow = gSavedSettings.getS32("RenderGlowResolutionPow");
	RenderGlowIterations = gSavedSettings.getS32("RenderGlowIterations");
	RenderGlowWidth = gSavedSettings.getF32("RenderGlowWidth");
	RenderGlowStrength = gSavedSettings.getF32("RenderGlowStrength");
	RenderDepthOfField = gSavedSettings.getBOOL("RenderDepthOfField");
	RenderDepthOfFieldInEditMode = gSavedSettings.getBOOL("RenderDepthOfFieldInEditMode");
	CameraFocusTransitionTime = gSavedSettings.getF32("CameraFocusTransitionTime");
	CameraFNumber = gSavedSettings.getF32("CameraFNumber");
	CameraFocalLength = gSavedSettings.getF32("CameraFocalLength");
	CameraFieldOfView = gSavedSettings.getF32("CameraFieldOfView");
	RenderShadowNoise = gSavedSettings.getF32("RenderShadowNoise");
	RenderShadowBlurSize = gSavedSettings.getF32("RenderShadowBlurSize");
	RenderSSAOScale = gSavedSettings.getF32("RenderSSAOScale");
	RenderSSAOMaxScale = gSavedSettings.getU32("RenderSSAOMaxScale");
	RenderSSAOFactor = gSavedSettings.getF32("RenderSSAOFactor");
	RenderSSAOEffect = gSavedSettings.getVector3("RenderSSAOEffect");
	RenderShadowOffsetError = gSavedSettings.getF32("RenderShadowOffsetError");
	RenderShadowBiasError = gSavedSettings.getF32("RenderShadowBiasError");
	RenderShadowOffset = gSavedSettings.getF32("RenderShadowOffset");
	RenderShadowBias = gSavedSettings.getF32("RenderShadowBias");
	RenderSpotShadowOffset = gSavedSettings.getF32("RenderSpotShadowOffset");
	RenderSpotShadowBias = gSavedSettings.getF32("RenderSpotShadowBias");
	RenderEdgeDepthCutoff = gSavedSettings.getF32("RenderEdgeDepthCutoff");
	RenderEdgeNormCutoff = gSavedSettings.getF32("RenderEdgeNormCutoff");
	RenderShadowGaussian = gSavedSettings.getVector3("RenderShadowGaussian");
	RenderShadowBlurDistFactor = gSavedSettings.getF32("RenderShadowBlurDistFactor");
	RenderDeferredAtmospheric = gSavedSettings.getBOOL("RenderDeferredAtmospheric");
	RenderReflectionDetail = gSavedSettings.getS32("RenderReflectionDetail");
	RenderHighlightFadeTime = gSavedSettings.getF32("RenderHighlightFadeTime");
	RenderShadowClipPlanes = gSavedSettings.getVector3("RenderShadowClipPlanes");
	RenderShadowOrthoClipPlanes = gSavedSettings.getVector3("RenderShadowOrthoClipPlanes");
	RenderShadowNearDist = gSavedSettings.getVector3("RenderShadowNearDist");
	RenderFarClip = gSavedSettings.getF32("RenderFarClip");
	RenderShadowSplitExponent = gSavedSettings.getVector3("RenderShadowSplitExponent");
	RenderShadowErrorCutoff = gSavedSettings.getF32("RenderShadowErrorCutoff");
	RenderShadowFOVCutoff = gSavedSettings.getF32("RenderShadowFOVCutoff");
	CameraOffset = gSavedSettings.getBOOL("CameraOffset");
	CameraMaxCoF = gSavedSettings.getF32("CameraMaxCoF");
	CameraDoFResScale = gSavedSettings.getF32("CameraDoFResScale");
	RenderAutoHideSurfaceAreaLimit = gSavedSettings.getF32("RenderAutoHideSurfaceAreaLimit");
	RenderSpotLight = nullptr;
	updateRenderDeferred();

	if (gNonInteractive)
	{
		LLVOAvatar::sMaxNonImpostors = 1;
		LLVOAvatar::updateImpostorRendering(LLVOAvatar::sMaxNonImpostors);
	}
}

void LLPipeline::releaseGLBuffers()
{
	assertInitialized();
	
	if (mNoiseMap)
	{
		LLImageGL::deleteTextures(1, &mNoiseMap);
		mNoiseMap = 0;
	}

	if (mTrueNoiseMap)
	{
		LLImageGL::deleteTextures(1, &mTrueNoiseMap);
		mTrueNoiseMap = 0;
	}

	releaseLUTBuffers();

	mWaterRef.release();
	mWaterDis.release();
    mBake.release();
	mHighlight.release();
	
	for (U32 i = 0; i < 3; i++)
	{
		mGlow[i].release();
	}

	releaseScreenBuffers();

	gBumpImageList.destroyGL();
	LLVOAvatar::resetImpostors();
}

void LLPipeline::releaseLUTBuffers()
{
	if (mLightFunc)
	{
		LLImageGL::deleteTextures(1, &mLightFunc);
		mLightFunc = 0;
	}
}

void LLPipeline::releaseShadowBuffers()
{
    releaseShadowTargets();
}

void LLPipeline::releaseScreenBuffers()
{
	mUIScreen.release();
	mScreen.release();
	mFXAABuffer.release();
	mPhysicsDisplay.release();
	mDeferredScreen.release();
	mDeferredDepth.release();
	mDeferredLight.release();
	mOcclusionDepth.release();
}
		
		
void LLPipeline::releaseShadowTarget(U32 index)
{
    mShadow[index].release();
    mShadowOcclusion[index].release();
}

void LLPipeline::releaseShadowTargets()
{
	for (U32 i = 0; i < 6; i++)
	{
        releaseShadowTarget(i);
	}
}

void LLPipeline::createGLBuffers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    stop_glerror();
	assertInitialized();

	updateRenderDeferred();
	if (LLPipeline::sWaterReflections)
	{ //water reflection texture
		U32 res = (U32) llmax(gSavedSettings.getS32("RenderWaterRefResolution"), 512);
		mWaterRef.allocate(res,res,GL_RGBA,TRUE,FALSE);
        mWaterDis.allocate(res,res,GL_RGBA,TRUE,FALSE,LLTexUnit::TT_TEXTURE);
	}

    // Use FBO for bake tex
    mBake.allocate(512, 512, GL_RGBA, TRUE, FALSE, LLTexUnit::TT_TEXTURE, true); // SL-12781 Build > Upload > Model; 3D Preview

	mHighlight.allocate(256,256,GL_RGBA, FALSE, FALSE);

	stop_glerror();

	GLuint resX = gViewerWindow->getWorldViewWidthRaw();
	GLuint resY = gViewerWindow->getWorldViewHeightRaw();

    // allocate screen space glow buffers
    const U32 glow_res = llmax(1, llmin(512, 1 << gSavedSettings.getS32("RenderGlowResolutionPow")));
    for (U32 i = 0; i < 3; i++)
    {
        mGlow[i].allocate(512, glow_res, GL_RGBA, FALSE, FALSE);
    }

    allocateScreenBuffer(resX, resY);
    mScreenWidth = 0;
    mScreenHeight = 0;

    if (sRenderDeferred)
    {
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
			LLImageGL::setManualImage(LLTexUnit::getInternalType(LLTexUnit::TT_TEXTURE), 0, GL_RGB16F_ARB, noiseRes, noiseRes, GL_RGB, GL_FLOAT, noise, false);
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		}

		if (!mTrueNoiseMap)
		{
			const U32 noiseRes = 128;
			F32 noise[noiseRes*noiseRes*3];
			for (U32 i = 0; i < noiseRes*noiseRes*3; i++)
			{
				noise[i] = ll_frand()*2.0-1.0;
			}

			LLImageGL::generateTextures(1, &mTrueNoiseMap);
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mTrueNoiseMap);
			LLImageGL::setManualImage(LLTexUnit::getInternalType(LLTexUnit::TT_TEXTURE), 0, GL_RGB16F_ARB, noiseRes, noiseRes, GL_RGB,GL_FLOAT, noise, false);
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		}

		createLUTBuffers();
	}

	gBumpImageList.restoreGL();
}

F32 lerpf(F32 a, F32 b, F32 w)
{
	return a + w * (b - a);
}

void LLPipeline::createLUTBuffers()
{
	if (sRenderDeferred)
	{
		if (!mLightFunc)
		{
			U32 lightResX = gSavedSettings.getU32("RenderSpecularResX");
			U32 lightResY = gSavedSettings.getU32("RenderSpecularResY");
			F32* ls = new F32[lightResX*lightResY];
			F32 specExp = gSavedSettings.getF32("RenderSpecularExponent");
            // Calculate the (normalized) blinn-phong specular lookup texture. (with a few tweaks)
			for (U32 y = 0; y < lightResY; ++y)
			{
				for (U32 x = 0; x < lightResX; ++x)
				{
					ls[y*lightResX+x] = 0;
					F32 sa = (F32) x/(lightResX-1);
					F32 spec = (F32) y/(lightResY-1);
					F32 n = spec * spec * specExp;
					
					// Nothing special here.  Just your typical blinn-phong term.
					spec = powf(sa, n);
					
					// Apply our normalization function.
					// Note: This is the full equation that applies the full normalization curve, not an approximation.
					// This is fine, given we only need to create our LUT once per buffer initialization.
					spec *= (((n + 2) * (n + 4)) / (8 * F_PI * (powf(2, -n/2) + n)));

					// Since we use R16F, we no longer have a dynamic range issue we need to work around here.
					// Though some older drivers may not like this, newer drivers shouldn't have this problem.
					ls[y*lightResX+x] = spec;
				}
			}
			
			U32 pix_format = GL_R16F;
#if LL_DARWIN
			// Need to work around limited precision with 10.6.8 and older drivers
			//
			pix_format = GL_R32F;
#endif
			LLImageGL::generateTextures(1, &mLightFunc);
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mLightFunc);
			LLImageGL::setManualImage(LLTexUnit::getInternalType(LLTexUnit::TT_TEXTURE), 0, pix_format, lightResX, lightResY, GL_RED, GL_FLOAT, ls, false);
			gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_TRILINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			
			delete [] ls;
		}
	}
}


void LLPipeline::restoreGL()
{
	assertInitialized();

	LLViewerShaderMgr::instance()->setShaders();

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

bool LLPipeline::shadersLoaded()
{
    return (assertInitialized() && mShadersLoaded);
}

bool LLPipeline::canUseWindLightShaders() const
{
	return (gWLSkyProgram.mProgramObject != 0 &&
			LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_WINDLIGHT) > 1);
}

bool LLPipeline::canUseWindLightShadersOnObjects() const
{
	return (canUseWindLightShaders() 
		&& LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0);
}

bool LLPipeline::canUseAntiAliasing() const
{
	return true;
}

void LLPipeline::unloadShaders()
{
	LLViewerShaderMgr::instance()->unloadShaders();
	mShadersLoaded = false;
}

void LLPipeline::assertInitializedDoError()
{
	LL_ERRS() << "LLPipeline used when uninitialized." << LL_ENDL;
}

//============================================================================

void LLPipeline::enableShadows(const bool enable_shadows)
{
	//should probably do something here to wrangle shadows....	
}

S32 LLPipeline::getMaxLightingDetail() const
{
	/*if (mShaderLevel[SHADER_OBJECT] >= LLDrawPoolSimple::SHADER_LEVEL_LOCAL_LIGHTS)
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
	refreshCachedSettings();

	if (level < 0)
	{
		if (RenderLocalLights)
		{
			level = 1;
		}
		else
		{
			level = 0;
		}
	}
	level = llclamp(level, 0, getMaxLightingDetail());
	mLightingDetail = level;
	
	return mLightingDetail;
}

class LLOctreeDirtyTexture : public OctreeTraveler
{
public:
	const std::set<LLViewerFetchedTexture*>& mTextures;

	LLOctreeDirtyTexture(const std::set<LLViewerFetchedTexture*>& textures) : mTextures(textures) { }

	virtual void visit(const OctreeNode* node)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);

		if (!group->hasState(LLSpatialGroup::GEOM_DIRTY) && !group->isEmpty())
		{
			for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
			{
				for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j) 
				{
					LLDrawInfo* params = *j;
					LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(params->mTexture);
					if (tex && mTextures.find(tex) != mTextures.end())
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
void LLPipeline::dirtyPoolObjectTextures(const std::set<LLViewerFetchedTexture*>& textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
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

LLDrawPool *LLPipeline::findPool(const U32 type, LLViewerTexture *tex0)
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

	case LLDrawPool::POOL_ALPHA_MASK:
		poolp = mAlphaMaskPool;
		break;

	case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
		poolp = mFullbrightAlphaMaskPool;
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
	case LLDrawPool::POOL_MATERIALS:
		poolp = mMaterialsPool;
		break;
	case LLDrawPool::POOL_ALPHA:
		poolp = mAlphaPool;
		break;

	case LLDrawPool::POOL_AVATAR:
	case LLDrawPool::POOL_CONTROL_AV:
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
		LL_ERRS() << "Invalid Pool Type in  LLPipeline::findPool() type=" << type << LL_ENDL;
		break;
	}

	return poolp;
}


LLDrawPool *LLPipeline::getPool(const U32 type,	LLViewerTexture *tex0)
{
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
LLDrawPool* LLPipeline::getPoolFromTE(const LLTextureEntry* te, LLViewerTexture* imagep)
{
	U32 type = getPoolTypeFromTE(te, imagep);
	return gPipeline.getPool(type, imagep);
}

//static 
U32 LLPipeline::getPoolTypeFromTE(const LLTextureEntry* te, LLViewerTexture* imagep)
{
	if (!te || !imagep)
	{
		return 0;
	}
		
	LLMaterial* mat = te->getMaterialParams().get();

	bool color_alpha = te->getColor().mV[3] < 0.999f;
	bool alpha = color_alpha;
	if (imagep)
	{
		alpha = alpha || (imagep->getComponents() == 4 && imagep->getType() != LLViewerTexture::MEDIA_TEXTURE) || (imagep->getComponents() == 2);
	}

	if (alpha && mat)
	{
		switch (mat->getDiffuseAlphaMode())
		{
			case 1:
				alpha = true; // Material's alpha mode is set to blend.  Toss it into the alpha draw pool.
				break;
			case 0: //alpha mode set to none, never go to alpha pool
			case 3: //alpha mode set to emissive, never go to alpha pool
				alpha = color_alpha;
				break;
			default: //alpha mode set to "mask", go to alpha pool if fullbright
				alpha = color_alpha; // Material's alpha mode is set to none, mask, or emissive.  Toss it into the opaque material draw pool.
				break;
		}
	}
	
	if (alpha)
	{
		return LLDrawPool::POOL_ALPHA;
	}
	else if ((te->getBumpmap() || te->getShiny()) && (!mat || mat->getNormalID().isNull()))
	{
		return LLDrawPool::POOL_BUMP;
	}
	else if (mat && !alpha)
	{
		return LLDrawPool::POOL_MATERIALS;
	}
	else
	{
		return LLDrawPool::POOL_SIMPLE;
	}
}


void LLPipeline::addPool(LLDrawPool *new_poolp)
{
	assertInitialized();
	mPools.insert(new_poolp);
	addToQuickLookup( new_poolp );
}

void LLPipeline::allocDrawable(LLViewerObject *vobj)
{
	LLDrawable *drawable = new LLDrawable(vobj);
	vobj->mDrawable = drawable;
	
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

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
		if (!drawablep->getSpatialGroup()->getSpatialPartition()->remove(drawablep, drawablep->getSpatialGroup()))
		{
#ifdef LL_RELEASE_FOR_DOWNLOAD
			LL_WARNS() << "Couldn't remove object from spatial group!" << LL_ENDL;
#else
			LL_ERRS() << "Couldn't remove object from spatial group!" << LL_ENDL;
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

	HighlightItem item(drawablep);
	mHighlightSet.erase(item);

	if (mHighlightObject == drawablep)
	{
		mHighlightObject = NULL;
	}

	for (U32 i = 0; i < 2; ++i)
	{
		if (mShadowSpotLight[i] == drawablep)
		{
			mShadowSpotLight[i] = NULL;
		}

		if (mTargetShadowSpotLight[i] == drawablep)
		{
			mTargetShadowSpotLight[i] = NULL;
		}
	}
}

//static
void LLPipeline::removeMutedAVsLights(LLVOAvatar* muted_avatar)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    light_set_t::iterator iter = gPipeline.mNearbyLights.begin();

    while (iter != gPipeline.mNearbyLights.end())
    {
        if (iter->drawable->getVObj()->isAttachment() && iter->drawable->getVObj()->getAvatar() == muted_avatar)
        {
            gPipeline.mLights.erase(iter->drawable);
            iter = gPipeline.mNearbyLights.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

U32 LLPipeline::addObject(LLViewerObject *vobj)
{
	if (RenderDelayCreation)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	LLDrawable* drawablep = vobj->mDrawable;

	if (!drawablep)
	{
		drawablep = vobj->createDrawable(this);
	}
	else
	{
		LL_ERRS() << "Redundant drawable creation!" << LL_ENDL;
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

	if (drawablep->getVOVolume() && RenderAnimateRes)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	assertInitialized();

	sCompiles        = 0;
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
    LL_PROFILE_ZONE_SCOPED;
	if (FreezeTime)
	{
		return;
	}
	if (!drawablep)
	{
		LL_ERRS() << "updateMove called with NULL drawablep" << LL_ENDL;
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
    LL_PROFILE_ZONE_SCOPED;
	if (FreezeTime)
	{
		return;
	}
	if (!drawablep)
	{
		LL_ERRS() << "updateMove called with NULL drawablep" << LL_ENDL;
		return;
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
    LL_PROFILE_ZONE_SCOPED;
	for (LLDrawable::drawable_vector_t::iterator iter = moved_list.begin();
		 iter != moved_list.end(); )
	{
		LLDrawable::drawable_vector_t::iterator curiter = iter++;
		LLDrawable *drawablep = *curiter;
		bool done = true;
		if (!drawablep->isDead() && (!drawablep->isState(LLDrawable::EARLY_MOVE)))
		{
			done = drawablep->updateMove();
		}
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED);
		if (done)
		{
			if (drawablep->isRoot() && !drawablep->isState(LLDrawable::ACTIVE))
			{
				drawablep->makeStatic();
			}
			drawablep->clearState(LLDrawable::ON_MOVE_LIST);
			if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
			{ //will likely not receive any future world matrix updates
				// -- this keeps attachments from getting stuck in space and falling off your avatar
				drawablep->clearState(LLDrawable::ANIMATED_CHILD);
				markRebuild(drawablep, LLDrawable::REBUILD_VOLUME, TRUE);
				if (drawablep->getVObj())
				{
					drawablep->getVObj()->dirtySpatialGroup(TRUE);
				}
			}
			iter = moved_list.erase(curiter);
		}
	}
}

void LLPipeline::updateMove()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

	if (FreezeTime)
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

	//balance octrees
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

		//balance the VO Cache tree
		LLVOCachePartition* vo_part = region->getVOCachePartition();
		if(vo_part)
		{
			vo_part->mOctree->balance();
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
	//shrink dist by dist/16.
	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	//get area of circle around node
	F32 app_angle = atanf(size.length()/dist);
	F32 radius = app_angle*LLDrawable::sCurPixelAngle;
	return radius*radius * F_PI;
}

//static
F32 LLPipeline::calcPixelArea(const LLVector4a& center, const LLVector4a& size, LLCamera &camera)
{
	LLVector4a origin;
	origin.load3(camera.getOrigin().mV);

	LLVector4a lookAt;
	lookAt.setSub(center, origin);
	F32 dist = lookAt.getLength3().getF32();

	//ramp down distance for nearby objects
	//shrink dist by dist/16.
	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	//get area of circle around node
	F32 app_angle = atanf(size.getLength3().getF32()/dist);
	F32 radius = app_angle*LLDrawable::sCurPixelAngle;
	return radius*radius * F_PI;
}

void LLPipeline::grabReferences(LLCullResult& result)
{
	sCull = &result;
}

void LLPipeline::clearReferences()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	sCull = NULL;
	mGroupSaveQ1.clear();
}

void check_references(LLSpatialGroup* group, LLDrawable* drawable)
{
	for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
	{
        LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
		if (drawable == drawablep)
		{
			LL_ERRS() << "LLDrawable deleted while actively reference by LLPipeline." << LL_ENDL;
		}
	}			
}

void check_references(LLDrawable* drawable, LLFace* face)
{
	for (S32 i = 0; i < drawable->getNumFaces(); ++i)
	{
		if (drawable->getFace(i) == face)
		{
			LL_ERRS() << "LLFace deleted while actively referenced by LLPipeline." << LL_ENDL;
		}
	}
}

void check_references(LLSpatialGroup* group, LLFace* face)
{
	for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
	{
		LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
		if(drawable)
		{
		check_references(drawable, face);
	}			
}
}

void LLPipeline::checkReferences(LLFace* face)
{
#if 0
	if (sCull)
	{
		for (LLCullResult::sg_iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, face);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginAlphaGroups(); iter != sCull->endAlphaGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, face);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, face);
		}

		for (LLCullResult::drawable_iterator iter = sCull->beginVisibleList(); iter != sCull->endVisibleList(); ++iter)
		{
			LLDrawable* drawable = *iter;
			check_references(drawable, face);	
		}
	}
#endif
}

void LLPipeline::checkReferences(LLDrawable* drawable)
{
#if 0
	if (sCull)
	{
		for (LLCullResult::sg_iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, drawable);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginAlphaGroups(); iter != sCull->endAlphaGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, drawable);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, drawable);
		}

		for (LLCullResult::drawable_iterator iter = sCull->beginVisibleList(); iter != sCull->endVisibleList(); ++iter)
		{
			if (drawable == *iter)
			{
				LL_ERRS() << "LLDrawable deleted while actively referenced by LLPipeline." << LL_ENDL;
			}
		}
	}
#endif
}

void check_references(LLSpatialGroup* group, LLDrawInfo* draw_info)
{
	for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
	{
		LLSpatialGroup::drawmap_elem_t& draw_vec = i->second;
		for (LLSpatialGroup::drawmap_elem_t::iterator j = draw_vec.begin(); j != draw_vec.end(); ++j)
		{
			LLDrawInfo* params = *j;
			if (params == draw_info)
			{
				LL_ERRS() << "LLDrawInfo deleted while actively referenced by LLPipeline." << LL_ENDL;
			}
		}
	}
}


void LLPipeline::checkReferences(LLDrawInfo* draw_info)
{
#if 0
	if (sCull)
	{
		for (LLCullResult::sg_iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, draw_info);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginAlphaGroups(); iter != sCull->endAlphaGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, draw_info);
		}

		for (LLCullResult::sg_iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			check_references(group, draw_info);
		}
	}
#endif
}

void LLPipeline::checkReferences(LLSpatialGroup* group)
{
#if CHECK_PIPELINE_REFERENCES
	if (sCull)
	{
		for (LLCullResult::sg_iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
		{
			if (group == *iter)
			{
				LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
			}
		}

		for (LLCullResult::sg_iterator iter = sCull->beginAlphaGroups(); iter != sCull->endAlphaGroups(); ++iter)
		{
			if (group == *iter)
			{
				LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
			}
		}

		for (LLCullResult::sg_iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
		{
			if (group == *iter)
			{
				LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
			}
		}
	}
#endif
}


bool LLPipeline::visibleObjectsInFrustum(LLCamera& camera)
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
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool LLPipeline::getVisibleExtents(LLCamera& camera, LLVector3& min, LLVector3& max)
{
	const F32 X = 65536.f;

	min = LLVector3(X,X,X);
	max = LLVector3(-X,-X,-X);

	LLViewerCamera::eCameraID saved_camera_id = LLViewerCamera::sCurCameraID;
	LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

	bool res = true;

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
						res = false;
					}
				}
			}
		}
	}

	LLViewerCamera::sCurCameraID = saved_camera_id;

	return res;
}

static LLTrace::BlockTimerStatHandle FTM_CULL("Object Culling");

void LLPipeline::updateCull(LLCamera& camera, LLCullResult& result, LLPlane* planep)
{
	static LLCachedControl<bool> use_occlusion(gSavedSettings,"UseOcclusion");
	static bool can_use_occlusion = LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") 
									&& gGLManager.mHasOcclusionQuery;

    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_CULL);

    if (planep != nullptr)
    {
        camera.setUserClipPlane(*planep);
    }
    else
    {
        camera.disableUserClipPlane();
    }

	grabReferences(result);

	sCull->clear();

	bool to_texture = LLPipeline::sUseOcclusion > 1 && gPipeline.shadersLoaded();

	if (to_texture)
	{
		if (LLPipeline::sRenderDeferred && can_use_occlusion)
		{
			mOcclusionDepth.bindTarget();
		}
		else
		{
			mScreen.bindTarget();
		}
	}

	if (sUseOcclusion > 1)
	{
		gGL.setColorMask(false, false);
	}

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(gGLLastProjection);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGLLastMatrix = NULL;
	gGL.loadMatrix(gGLLastModelView);

	LLGLDisable blend(GL_BLEND);
	LLGLDisable test(GL_ALPHA_TEST);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	bool bound_shader = false;
	if (gPipeline.shadersLoaded() && LLGLSLShader::sCurBoundShader == 0)
	{ //if no shader is currently bound, use the occlusion shader instead of fixed function if we can
		// (shadow render uses a special shader that clamps to clip planes)
		bound_shader = true;
		gOcclusionCubeProgram.bind();
	}
	
	if (sUseOcclusion > 1)
	{
		if (mCubeVB.isNull())
		{ //cube VB will be used for issuing occlusion queries
			mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX, GL_STATIC_DRAW_ARB);
		}
		mCubeVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
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
				if (hasRenderType(part->mDrawableType))
				{
					part->cull(camera);
				}
			}
		}

		//scan the VO Cache tree
		LLVOCachePartition* vo_part = region->getVOCachePartition();
		if(vo_part)
		{
            bool do_occlusion_cull = can_use_occlusion && use_occlusion && !gUseWireframe;
			vo_part->cull(camera, do_occlusion_cull);
		}
	}

	if (bound_shader)
	{
		gOcclusionCubeProgram.unbind();
	}

	if (hasRenderType(LLPipeline::RENDER_TYPE_SKY) && 
		gSky.mVOSkyp.notNull() && 
		gSky.mVOSkyp->mDrawable.notNull())
	{
		gSky.mVOSkyp->mDrawable->setVisible(camera);
		sCull->pushDrawable(gSky.mVOSkyp->mDrawable);
		gSky.updateCull();
		stop_glerror();
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
	
	
    if (hasRenderType(LLPipeline::RENDER_TYPE_WL_SKY) && 
        gPipeline.canUseWindLightShaders() &&
        gSky.mVOWLSkyp.notNull() && 
        gSky.mVOWLSkyp->mDrawable.notNull())
    {
        gSky.mVOWLSkyp->mDrawable->setVisible(camera);
        sCull->pushDrawable(gSky.mVOWLSkyp->mDrawable);
    }

    bool render_water = !sReflectionRender && (hasRenderType(LLPipeline::RENDER_TYPE_WATER) || hasRenderType(LLPipeline::RENDER_TYPE_VOIDWATER));

    if (render_water)
    {
        LLWorld::getInstance()->precullWaterObjects(camera, sCull, render_water);
    }
	
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

	if (sUseOcclusion > 1)
	{
		gGL.setColorMask(true, false);
	}

	if (to_texture)
	{
		if (LLPipeline::sRenderDeferred && can_use_occlusion)
		{
			mOcclusionDepth.flush();
		}
		else
		{
			mScreen.flush();
		}
	}
}

void LLPipeline::markNotCulled(LLSpatialGroup* group, LLCamera& camera)
{
	if (group->isEmpty())
	{ 
		return;
	}
	
	group->setVisible();

	if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
	{
		group->updateDistance(camera);
	}
	
	assertInitialized();
	
	if (!group->getSpatialPartition()->mRenderByGroup)
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
	if (sUseOcclusion > 1 && group && !group->isOcclusionState(LLSpatialGroup::ACTIVE_OCCLUSION))
	{
		LLSpatialGroup* parent = group->getParent();

		if (!parent || !parent->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{ //only mark top most occluders as active occlusion
			sCull->pushOcclusionGroup(group);
			group->setOcclusionState(LLSpatialGroup::ACTIVE_OCCLUSION);
				
			if (parent && 
				!parent->isOcclusionState(LLSpatialGroup::ACTIVE_OCCLUSION) &&
				parent->getElementCount() == 0 &&
				parent->needsUpdate())
			{
				sCull->pushOcclusionGroup(group);
				parent->setOcclusionState(LLSpatialGroup::ACTIVE_OCCLUSION);
			}
		}
	}
}

void LLPipeline::downsampleDepthBuffer(LLRenderTarget& source, LLRenderTarget& dest, LLRenderTarget* scratch_space)
{
	LLGLSLShader* last_shader = LLGLSLShader::sCurBoundShaderPtr;

	LLGLSLShader* shader = NULL;

	if (scratch_space)
	{
        GLint bits = 0;
        bits |= (source.hasStencil() && dest.hasStencil()) ? GL_STENCIL_BUFFER_BIT : 0;
        bits |= GL_DEPTH_BUFFER_BIT;
		scratch_space->copyContents(source, 
									0, 0, source.getWidth(), source.getHeight(), 
									0, 0, scratch_space->getWidth(), scratch_space->getHeight(), bits, GL_NEAREST);
	}

	dest.bindTarget();
	dest.clear(GL_DEPTH_BUFFER_BIT);

	LLStrider<LLVector3> vert; 
	mDeferredVB->getVertexStrider(vert);
	LLStrider<LLVector2> tc0;
		
	vert[0].set(-1,1,0);
	vert[1].set(-1,-3,0);
	vert[2].set(3,1,0);
	
	if (source.getUsage() == LLTexUnit::TT_RECT_TEXTURE)
	{
		shader = &gDownsampleDepthRectProgram;
		shader->bind();
		shader->uniform2f(sDelta, 1.f, 1.f);
		shader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, source.getWidth(), source.getHeight());
	}
	else
	{
		shader = &gDownsampleDepthProgram;
		shader->bind();
		shader->uniform2f(sDelta, 1.f/source.getWidth(), 1.f/source.getHeight());
		shader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, 1.f, 1.f);
	}

	gGL.getTexUnit(0)->bind(scratch_space ? scratch_space : &source, TRUE);

	{
		LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);
		mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
		mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
	}
	
	dest.flush();
	
	if (last_shader)
	{
		last_shader->bind();
	}
	else
	{
		shader->unbind();
	}
}

void LLPipeline::doOcclusion(LLCamera& camera, LLRenderTarget& source, LLRenderTarget& dest, LLRenderTarget* scratch_space)
{
	downsampleDepthBuffer(source, dest, scratch_space);
	dest.bindTarget();
	doOcclusion(camera);
	dest.flush();
}

void LLPipeline::doOcclusion(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (LLPipeline::sUseOcclusion > 1 && !LLSpatialPartition::sTeleportRequested &&
		(sCull->hasOcclusionGroups() || LLVOCachePartition::sNeedsOcclusionCheck))
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

		LLGLDisable cull(GL_CULL_FACE);

		
		bool bind_shader = (LLGLSLShader::sCurBoundShader == 0);
		if (bind_shader)
		{
			if (LLPipeline::sShadowRender)
			{
				gDeferredShadowCubeProgram.bind();
			}
			else
			{
				gOcclusionCubeProgram.bind();
			}
		}

		if (mCubeVB.isNull())
		{ //cube VB will be used for issuing occlusion queries
			mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX, GL_STATIC_DRAW_ARB);
		}
		mCubeVB->setBuffer(LLVertexBuffer::MAP_VERTEX);

		for (LLCullResult::sg_iterator iter = sCull->beginOcclusionGroups(); iter != sCull->endOcclusionGroups(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			group->doOcclusion(&camera);
			group->clearOcclusionState(LLSpatialGroup::ACTIVE_OCCLUSION);
		}
	
		//apply occlusion culling to object cache tree
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLVOCachePartition* vo_part = (*iter)->getVOCachePartition();
			if(vo_part)
			{
				vo_part->processOccluders(&camera);
			}
		}

		if (bind_shader)
		{
			if (LLPipeline::sShadowRender)
			{
				gDeferredShadowCubeProgram.unbind();
			}
			else
			{
				gOcclusionCubeProgram.unbind();
			}
		}

		gGL.setColorMask(true, false);
	}
}
	
bool LLPipeline::updateDrawableGeom(LLDrawable* drawablep, bool priority)
{
	bool update_complete = drawablep->updateGeometry(priority);
	if (update_complete && assertInitialized())
	{
		drawablep->setState(LLDrawable::BUILT);
	}
	return update_complete;
}

void LLPipeline::updateGL()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	{
		while (!LLGLUpdate::sGLQ.empty())
		{
			LLGLUpdate* glu = LLGLUpdate::sGLQ.front();
			glu->updateGL();
			glu->mInQ = FALSE;
			LLGLUpdate::sGLQ.pop_front();
		}
	}

	{ //seed VBO Pools
		LLVertexBuffer::seedPools();
	}
}

void LLPipeline::clearRebuildGroups()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	LLSpatialGroup::sg_vector_t	hudGroups;

	mGroupQ1Locked = true;
	// Iterate through all drawables on the priority build queue,
	for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ1.begin();
		 iter != mGroupQ1.end(); ++iter)
	{
		LLSpatialGroup* group = *iter;

		// If the group contains HUD objects, save the group
		if (group->isHUDGroup())
		{
			hudGroups.push_back(group);
		}
		// Else, no HUD objects so clear the build state
		else
		{
			group->clearState(LLSpatialGroup::IN_BUILD_Q1);
		}
	}

	// Clear the group
	mGroupQ1.clear();

	// Copy the saved HUD groups back in
	mGroupQ1.assign(hudGroups.begin(), hudGroups.end());
	mGroupQ1Locked = false;

	// Clear the HUD groups
	hudGroups.clear();

	mGroupQ2Locked = true;
	for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ2.begin();
		 iter != mGroupQ2.end(); ++iter)
	{
		LLSpatialGroup* group = *iter;

		// If the group contains HUD objects, save the group
		if (group->isHUDGroup())
		{
			hudGroups.push_back(group);
		}
		// Else, no HUD objects so clear the build state
		else
		{
			group->clearState(LLSpatialGroup::IN_BUILD_Q2);
		}
	}	
	// Clear the group
	mGroupQ2.clear();

	// Copy the saved HUD groups back in
	mGroupQ2.assign(hudGroups.begin(), hudGroups.end());
	mGroupQ2Locked = false;
}

void LLPipeline::clearRebuildDrawables()
{
	// Clear all drawables on the priority build queue,
	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ1.begin();
		 iter != mBuildQ1.end(); ++iter)
	{
		LLDrawable* drawablep = *iter;
		if (drawablep && !drawablep->isDead())
		{
			drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
			drawablep->clearState(LLDrawable::IN_REBUILD_Q1);
		}
	}
	mBuildQ1.clear();

	// clear drawables on the non-priority build queue
	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ2.begin();
		 iter != mBuildQ2.end(); ++iter)
	{
		LLDrawable* drawablep = *iter;
		if (!drawablep->isDead())
		{
			drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
		}
	}	
	mBuildQ2.clear();
	
	//clear all moving bridges
	for (LLDrawable::drawable_vector_t::iterator iter = mMovedBridge.begin();
		 iter != mMovedBridge.end(); ++iter)
	{
		LLDrawable *drawablep = *iter;
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD);
	}
	mMovedBridge.clear();

	//clear all moving drawables
	for (LLDrawable::drawable_vector_t::iterator iter = mMovedList.begin();
		 iter != mMovedList.end(); ++iter)
	{
		LLDrawable *drawablep = *iter;
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD);
	}
	mMovedList.clear();

    for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
        iter != mShiftList.end(); ++iter)
    {
        LLDrawable *drawablep = *iter;
        drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD | LLDrawable::ON_SHIFT_LIST);
    }
    mShiftList.clear();
}

void LLPipeline::rebuildPriorityGroups()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	LLTimer update_timer;
	assertInitialized();

	gMeshRepo.notifyLoadedMeshes();

	mGroupQ1Locked = true;
	// Iterate through all drawables on the priority build queue,
	for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ1.begin();
		 iter != mGroupQ1.end(); ++iter)
	{
		LLSpatialGroup* group = *iter;
		group->rebuildGeom();
		group->clearState(LLSpatialGroup::IN_BUILD_Q1);
	}

	mGroupSaveQ1 = mGroupQ1;
	mGroupQ1.clear();
	mGroupQ1Locked = false;

}

void LLPipeline::rebuildGroups()
{
	if (mGroupQ2.empty())
	{
		return;
	}

    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	mGroupQ2Locked = true;
	// Iterate through some drawables on the non-priority build queue
	S32 size = (S32) mGroupQ2.size();
	S32 min_count = llclamp((S32) ((F32) (size * size)/4096*0.25f), 1, size);
			
	S32 count = 0;
	
	std::sort(mGroupQ2.begin(), mGroupQ2.end(), LLSpatialGroup::CompareUpdateUrgency());

	LLSpatialGroup::sg_vector_t::iterator iter;
	LLSpatialGroup::sg_vector_t::iterator last_iter = mGroupQ2.begin();

	for (iter = mGroupQ2.begin();
		 iter != mGroupQ2.end() && count <= min_count; ++iter)
	{
		LLSpatialGroup* group = *iter;
		last_iter = iter;

		if (!group->isDead())
		{
			group->rebuildGeom();
			
			if (group->getSpatialPartition()->mRenderByGroup)
			{
				count++;
			}
		}

		group->clearState(LLSpatialGroup::IN_BUILD_Q2);
	}	

	mGroupQ2.erase(mGroupQ2.begin(), ++last_iter);

	mGroupQ2Locked = false;

	updateMovedList(mMovedBridge);
}

void LLPipeline::updateGeom(F32 max_dtime)
{
	LLTimer update_timer;
	LLPointer<LLDrawable> drawablep;

	LL_RECORD_BLOCK_TIME(FTM_GEO_UPDATE);

	assertInitialized();

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

			if (drawablep->isUnload())
			{
				drawablep->unload();
				drawablep->clearState(LLDrawable::FOR_UNLOAD);
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
	
	max_dtime = llmax(update_timer.getElapsedTimeF32()+0.001f, F32SecondsImplicit(max_dtime));
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

		bool update_complete = true;
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
	if(drawablep && !drawablep->isDead())
	{
		if (drawablep->isSpatialBridge())
		{
			const LLDrawable* root = ((LLSpatialBridge*) drawablep)->mDrawable;
			llassert(root); // trying to catch a bad assumption
					
			if (root && //  // this test may not be needed, see above
					root->getVObj()->isAttachment())
			{
				LLDrawable* rootparent = root->getParent();
				if (rootparent) // this IS sometimes NULL
				{
					LLViewerObject *vobj = rootparent->getVObj();
					llassert(vobj); // trying to catch a bad assumption
					if (vobj) // this test may not be needed, see above
					{
						LLVOAvatar* av = vobj->asAvatar();
						if (av &&
							((!sImpostorRender && av->isImpostor()) //ignore impostor flag during impostor pass
							 || av->isInMuteList() 
							 || (LLVOAvatar::AOA_JELLYDOLL == av->getOverallAppearance() && !av->needsImpostorUpdate()) ))
						{
							return;
						}
					}
				}
			}
			sCull->pushBridge((LLSpatialBridge*) drawablep);
		}
		else
		{
		
			sCull->pushDrawable(drawablep);
		}

		drawablep->setVisible(camera);
	}
}

void LLPipeline::markMoved(LLDrawable *drawablep, bool damped_motion)
{
	if (!drawablep)
	{
		//LL_ERRS() << "Sending null drawable to moved list!" << LL_ENDL;
		return;
	}
	
	if (drawablep->isDead())
	{
		LL_WARNS() << "Marking NULL or dead drawable moved!" << LL_ENDL;
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
	if (! damped_motion)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	assertInitialized();

	glClear(GL_DEPTH_BUFFER_BIT);
	gDepthDirty = true;
		
	LLVector4a offseta;
	offseta.load3(offset.mV);

	for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
			iter != mShiftList.end(); iter++)
	{
		LLDrawable *drawablep = *iter;
		if (drawablep->isDead())
		{
			continue;
		}	
		drawablep->shiftPos(offseta);	
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
				part->shift(offseta);
			}
		}
	}

	LLHUDText::shiftAll(offset);
	LLHUDNameTag::shiftAll(offset);

	display_update_camera();
}

void LLPipeline::markTextured(LLDrawable *drawablep)
{
	if (drawablep && !drawablep->isDead() && assertInitialized())
	{
		mRetexturedList.insert(drawablep);
	}
}

void LLPipeline::markGLRebuild(LLGLUpdate* glu)
{
	if (glu && !glu->mInQ)
	{
		LLGLUpdate::sGLQ.push_back(glu);
		glu->mInQ = TRUE;
	}
}

void LLPipeline::markPartitionMove(LLDrawable* drawable)
{
	if (!drawable->isState(LLDrawable::PARTITION_MOVE) && 
		!drawable->getPositionGroup().equals3(LLVector4a::getZero()))
	{
		drawable->setState(LLDrawable::PARTITION_MOVE);
		mPartitionQ.push_back(drawable);
	}
}

void LLPipeline::processPartitionQ()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	for (LLDrawable::drawable_list_t::iterator iter = mPartitionQ.begin(); iter != mPartitionQ.end(); ++iter)
	{
		LLDrawable* drawable = *iter;
		if (!drawable->isDead())
		{
			drawable->updateBinRadius();
			drawable->movePartition();
		}
		drawable->clearState(LLDrawable::PARTITION_MOVE);
	}

	mPartitionQ.clear();
}

void LLPipeline::markMeshDirty(LLSpatialGroup* group)
{
	mMeshDirtyGroup.push_back(group);
}

void LLPipeline::markRebuild(LLSpatialGroup* group, bool priority)
{
	if (group && !group->isDead() && group->getSpatialPartition())
	{
		if (group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD)
		{
			priority = true;
		}

		if (priority)
		{
			if (!group->hasState(LLSpatialGroup::IN_BUILD_Q1))
			{
				llassert_always(!mGroupQ1Locked);

				mGroupQ1.push_back(group);
				group->setState(LLSpatialGroup::IN_BUILD_Q1);

				if (group->hasState(LLSpatialGroup::IN_BUILD_Q2))
				{
					LLSpatialGroup::sg_vector_t::iterator iter = std::find(mGroupQ2.begin(), mGroupQ2.end(), group);
					if (iter != mGroupQ2.end())
					{
						mGroupQ2.erase(iter);
					}
					group->clearState(LLSpatialGroup::IN_BUILD_Q2);
				}
			}
		}
		else if (!group->hasState(LLSpatialGroup::IN_BUILD_Q2 | LLSpatialGroup::IN_BUILD_Q1))
		{
			llassert_always(!mGroupQ2Locked);
			mGroupQ2.push_back(group);
			group->setState(LLSpatialGroup::IN_BUILD_Q2);

		}
	}
}

void LLPipeline::markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag, bool priority)
{
	if (drawablep && !drawablep->isDead() && assertInitialized())
	{
        if (debugLoggingEnabled("AnimatedObjectsLinkset"))
        {
            LLVOVolume *vol_obj = drawablep->getVOVolume();
            if (vol_obj && vol_obj->isAnimatedObject() && vol_obj->isRiggedMesh())
            {
                std::string vobj_name = llformat("Vol%p", vol_obj);
                F32 est_tris = vol_obj->getEstTrianglesMax();
                LL_DEBUGS("AnimatedObjectsLinkset") << vobj_name << " markRebuild, tris " << est_tris 
                                                    << " priority " << (S32) priority << " flag " << std::hex << flag << LL_ENDL; 
            }
        }
    
		if (!drawablep->isState(LLDrawable::BUILT))
		{
			priority = true;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

	if (hasAnyRenderType(LLPipeline::RENDER_TYPE_AVATAR,
					  LLPipeline::RENDER_TYPE_CONTROL_AV,
					  LLPipeline::RENDER_TYPE_GROUND,
					  LLPipeline::RENDER_TYPE_TERRAIN,
					  LLPipeline::RENDER_TYPE_TREE,
					  LLPipeline::RENDER_TYPE_SKY,
					  LLPipeline::RENDER_TYPE_VOIDWATER,
					  LLPipeline::RENDER_TYPE_WATER,
					  LLPipeline::END_RENDER_TYPES))
	{
		//clear faces from face pools
		gPipeline.resetDrawOrders();
	}

	//LLVertexBuffer::unbind();

	grabReferences(result);
	for (LLCullResult::sg_iterator iter = sCull->beginDrawableGroups(); iter != sCull->endDrawableGroups(); ++iter)
	{
		LLSpatialGroup* group = *iter;
		group->checkOcclusion();
		if (sUseOcclusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{
			markOccluder(group);
		}
		else
		{
			group->setVisible();
			for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
			{
                LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
				markVisible(drawablep, camera);
			}

			if (!sDelayVBUpdate)
			{ //rebuild mesh as soon as we know it's visible
				group->rebuildMesh();
			}
		}
	}

	if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
	{
		LLSpatialGroup* last_group = NULL;
		BOOL fov_changed = LLViewerCamera::getInstance()->isDefaultFOVChanged();
		for (LLCullResult::bridge_iterator i = sCull->beginVisibleBridge(); i != sCull->endVisibleBridge(); ++i)
		{
			LLCullResult::bridge_iterator cur_iter = i;
			LLSpatialBridge* bridge = *cur_iter;
			LLSpatialGroup* group = bridge->getSpatialGroup();

			if (last_group == NULL)
			{
				last_group = group;
			}

			if (!bridge->isDead() && group && !group->isOcclusionState(LLSpatialGroup::OCCLUDED))
			{
				stateSort(bridge, camera, fov_changed);
			}

			if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD &&
				last_group != group && last_group->changeLOD())
			{
				last_group->mLastUpdateDistance = last_group->mDistance;
			}

			last_group = group;
		}

		if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD &&
			last_group && last_group->changeLOD())
		{
			last_group->mLastUpdateDistance = last_group->mDistance;
		}
	}

	for (LLCullResult::sg_iterator iter = sCull->beginVisibleGroups(); iter != sCull->endVisibleGroups(); ++iter)
	{
		LLSpatialGroup* group = *iter;
		group->checkOcclusion();
		if (sUseOcclusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{
			markOccluder(group);
		}
		else
		{
			group->setVisible();
			stateSort(group, camera);

			if (!sDelayVBUpdate)
			{ //rebuild mesh as soon as we know it's visible
				group->rebuildMesh();
			}
		}
	}
	
	{
		LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWABLE("stateSort"); // LL_RECORD_BLOCK_TIME(FTM_STATESORT_DRAWABLE);
		for (LLCullResult::drawable_iterator iter = sCull->beginVisibleList();
			 iter != sCull->endVisibleList(); ++iter)
		{
			LLDrawable *drawablep = *iter;
			if (!drawablep->isDead())
			{
				stateSort(drawablep, camera);
			}
		}
	}
		
	postSort(camera);	
}

void LLPipeline::stateSort(LLSpatialGroup* group, LLCamera& camera)
{
	if (group->changeLOD())
	{
		for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
		{
            LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();            
			stateSort(drawablep, camera);
		}

		if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
		{ //avoid redundant stateSort calls
			group->mLastUpdateDistance = group->mDistance;
		}
	}
}

void LLPipeline::stateSort(LLSpatialBridge* bridge, LLCamera& camera, BOOL fov_changed)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (bridge->getSpatialGroup()->changeLOD() || fov_changed)
	{
		bool force_update = false;
		bridge->updateDistance(camera, force_update);
	}
}

void LLPipeline::stateSort(LLDrawable* drawablep, LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (!drawablep
		|| drawablep->isDead() 
		|| !hasRenderType(drawablep->getRenderType()))
	{
		return;
	}
	
    // SL-11353
    // ignore our own geo when rendering spotlight shadowmaps...
    // 
    if (RenderSpotLight && drawablep == RenderSpotLight)
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
	}

	if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
	{
		//if (drawablep->isVisible()) isVisible() check here is redundant, if it wasn't visible, it wouldn't be here
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

	if (!drawablep->getVOVolume())
	{
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
	}
	
	mNumVisibleFaces += drawablep->getNumFaces();
}


void forAllDrawables(LLCullResult::sg_iterator begin, 
					 LLCullResult::sg_iterator end,
					 void (*func)(LLDrawable*))
{
	for (LLCullResult::sg_iterator i = begin; i != end; ++i)
	{
		for (LLSpatialGroup::element_iter j = (*i)->getDataBegin(); j != (*i)->getDataEnd(); ++j)
		{
			if((*j)->hasDrawable())
			{
				func((LLDrawable*)(*j)->getDrawable());	
			}
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
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				LLFace * facep = drawablep->getFace(face_id);
				if (facep) 
				{
					gPipeline.mHighlightFaces.push_back(facep);
				}
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
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				LLFace * facep = drawablep->getFace(face_id);
				if (facep)
				{
					gPipeline.mHighlightFaces.push_back(facep);
			}
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
		&& vobj->flagUsePhysics())
	{
		if (gPipeline.sRenderBeacons)
		{
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(0.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				LLFace * facep = drawablep->getFace(face_id);
				if (facep)
				{
					gPipeline.mHighlightFaces.push_back(facep);
			}
		}
	}
}
}

void renderMOAPBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();

	if(!vobj || vobj->isAvatar())
		return;

	bool beacon=false;
	U8 tecount=vobj->getNumTEs();
	for(int x=0;x<tecount;x++)
	{
		if(vobj->getTE(x)->hasMedia())
		{
			beacon=true;
			break;
		}
	}
	if(beacon)
	{
		if (gPipeline.sRenderBeacons)
		{
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 1.f, 1.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				LLFace * facep = drawablep->getFace(face_id);
				if (facep)
				{
					gPipeline.mHighlightFaces.push_back(facep);
			}
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
			gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", light_blue, LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
		}

		if (gPipeline.sRenderHighlight)
		{
			S32 face_id;
			S32 count = drawablep->getNumFaces();
			for (face_id = 0; face_id < count; face_id++)
			{
				LLFace * facep = drawablep->getFace(face_id);
				if (facep)
				{
					gPipeline.mHighlightFaces.push_back(facep);
			}
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
				LLFace * facep = drawablep->getFace(face_id);
				if (facep)
				{
					gPipeline.mHighlightFaces.push_back(facep);
			}
		}
	}
}
}

void LLPipeline::touchTexture(LLViewerTexture* tex, F32 vsize)
{
    if (tex)
    {
        LLImageGL* gl_tex = tex->getGLTexture();
        if (gl_tex && gl_tex->updateBindStats(gl_tex->mTextureMemory))
        {
            tex->setActive();
            tex->addTextureStats(vsize);
        }
    }


}
void LLPipeline::touchTextures(LLDrawInfo* info)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    for (int i = 0; i < info->mTextureList.size(); ++i)
    {
        touchTexture(info->mTextureList[i], info->mTextureListVSize[i]);
    }

    touchTexture(info->mTexture, info->mVSize);
    touchTexture(info->mSpecularMap, info->mVSize);
    touchTexture(info->mNormalMap, info->mVSize);
}

void LLPipeline::postSort(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

	assertInitialized();

	LL_PUSH_CALLSTACKS();
	//rebuild drawable geometry
	for (LLCullResult::sg_iterator i = sCull->beginDrawableGroups(); i != sCull->endDrawableGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!sUseOcclusion || 
			!group->isOcclusionState(LLSpatialGroup::OCCLUDED))
		{
			group->rebuildGeom();
		}
	}
	LL_PUSH_CALLSTACKS();
	//rebuild groups
	sCull->assertDrawMapsEmpty();

	rebuildPriorityGroups();
	LL_PUSH_CALLSTACKS();

	
	//build render map
	for (LLCullResult::sg_iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if ((sUseOcclusion && 
			group->isOcclusionState(LLSpatialGroup::OCCLUDED)) ||
			(RenderAutoHideSurfaceAreaLimit > 0.f && 
			group->mSurfaceArea > RenderAutoHideSurfaceAreaLimit*llmax(group->mObjectBoxSize, 10.f)))
		{
			continue;
		}

		if (group->hasState(LLSpatialGroup::NEW_DRAWINFO) && group->hasState(LLSpatialGroup::GEOM_DIRTY))
		{ //no way this group is going to be drawable without a rebuild
			group->rebuildGeom();
		}

		for (LLSpatialGroup::draw_map_t::iterator j = group->mDrawMap.begin(); j != group->mDrawMap.end(); ++j)
		{
			LLSpatialGroup::drawmap_elem_t& src_vec = j->second;	
			if (!hasRenderType(j->first))
			{
				continue;
			}
			
			for (LLSpatialGroup::drawmap_elem_t::iterator k = src_vec.begin(); k != src_vec.end(); ++k)
			{
                LLDrawInfo* info = *k;
				
				sCull->pushDrawInfo(j->first, info);
                if (!sShadowRender && !sReflectionRender)
                {
                    touchTextures(info);
                    addTrianglesDrawn(info->mCount, info->mDrawMode);
                }
			}
		}

		if (hasRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA))
		{
			LLSpatialGroup::draw_map_t::iterator alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA);
			
			if (alpha != group->mDrawMap.end())
			{ //store alpha groups for sorting
				LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
				if (LLViewerCamera::sCurCameraID == LLViewerCamera::CAMERA_WORLD)
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

            LLSpatialGroup::draw_map_t::iterator rigged_alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA_RIGGED);

            if (rigged_alpha != group->mDrawMap.end())
            { //store rigged alpha groups for LLDrawPoolAlpha prepass (skip distance update, rigged attachments use depth buffer)
                if (hasRenderType(LLDrawPool::POOL_ALPHA))
                {
                    sCull->pushRiggedAlphaGroup(group);
                }
            }
		}
	}
	
	//flush particle VB
	if (LLVOPartGroup::sVB)
	{
		LLVOPartGroup::sVB->flush();
	}
	else
	{
		LL_WARNS_ONCE() << "Missing particle buffer" << LL_ENDL;
	}

	/*bool use_transform_feedback = gTransformPositionProgram.mProgramObject && !mMeshDirtyGroup.empty();

	if (use_transform_feedback)
	{ //place a query around potential transform feedback code for synchronization
		mTransformFeedbackPrimitives = 0;

		if (!mMeshDirtyQueryObject)
		{
			glGenQueriesARB(1, &mMeshDirtyQueryObject);
		}

		
		glBeginQueryARB(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mMeshDirtyQueryObject);
	}*/

	//pack vertex buffers for groups that chose to delay their updates
	for (LLSpatialGroup::sg_vector_t::iterator iter = mMeshDirtyGroup.begin(); iter != mMeshDirtyGroup.end(); ++iter)
	{
		(*iter)->rebuildMesh();
	}

	/*if (use_transform_feedback)
	{
		glEndQueryARB(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	}*/
	
	mMeshDirtyGroup.clear();

	if (!sShadowRender)
	{
        // order alpha groups by distance
		std::sort(sCull->beginAlphaGroups(), sCull->endAlphaGroups(), LLSpatialGroup::CompareDepthGreater());

        // order rigged alpha groups by avatar attachment order
        std::sort(sCull->beginRiggedAlphaGroups(), sCull->endRiggedAlphaGroups(), LLSpatialGroup::CompareRenderOrder());
	}

	LL_PUSH_CALLSTACKS();
	// only render if the flag is set. The flag is only set if we are in edit mode or the toggle is set in the menus
	if (LLFloaterReg::instanceVisible("beacons") && !sShadowRender)
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

		if(sRenderMOAPBeacons)
		{
			forAllVisibleDrawables(renderMOAPBeacons);
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
					gObjectList.addDebugBeacon(pos, "", LLColor4(1.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), DebugBeaconLineWidth);
				}
			}
			// now deal with highlights for all those seeable sound sources
			forAllVisibleDrawables(renderSoundHighlights);
		}
	}
	LL_PUSH_CALLSTACKS();
	// If managing your telehub, draw beacons at telehub and currently selected spawnpoint.
	if (LLFloaterTelehub::renderBeacons() && !sShadowRender)
	{
		LLFloaterTelehub::addBeacons();
	}

	if (!sShadowRender)
	{
		mSelectedFaces.clear();

		if (!gNonInteractive)
		{
			LLPipeline::setRenderHighlightTextureChannel(gFloaterTools->getPanelFace()->getTextureChannelToEdit());
		}

		// Draw face highlights for selected faces.
		if (LLSelectMgr::getInstance()->getTEMode())
		{
			struct f : public LLSelectedTEFunctor
			{
				virtual bool apply(LLViewerObject* object, S32 te)
				{
					if (object->mDrawable)
					{
						LLFace * facep = object->mDrawable->getFace(te);
						if (facep)
						{
							gPipeline.mSelectedFaces.push_back(facep);
					}
					}
					return true;
				}
			} func;
			LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);
		}
	}

	//LLSpatialGroup::sNoDelete = FALSE;
	LL_PUSH_CALLSTACKS();
}


void render_hud_elements()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI; //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
	gPipeline.disableLights();
	
	LLGLDisable fog(GL_FOG);
	LLGLSUIDefault gls_ui;

	LLGLEnable stencil(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 255, 0xFFFFFFFF);
	glStencilMask(0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	
	gGL.color4f(1,1,1,1);
	
	gUIProgram.bind();
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);

	if (!LLPipeline::sReflectionRender && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		LLGLEnable multisample(LLPipeline::RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);
		gViewerWindow->renderSelections(FALSE, FALSE, FALSE); // For HUD version in render_ui_3d()
	
		// Draw the tracking overlays
		LLTracker::render3D();
		
        if (LLWorld::instanceExists())
        {
            // Show the property lines
            LLWorld::getInstance()->renderPropertyLines();
        }
		LLViewerParcelMgr::getInstance()->render();
		LLViewerParcelMgr::getInstance()->renderParcelCollision();
	
		// Render name tags.
		LLHUDObject::renderAll();
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

	gUIProgram.unbind();
	gGL.flush();
}

void LLPipeline::renderHighlights()
{
	assertInitialized();

	// Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
	// Render highlighted faces.
	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLColor4 color(1.f, 1.f, 1.f, 0.5f);
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	disableLights();

	if (!hasRenderType(LLPipeline::RENDER_TYPE_HUD) && !mHighlightSet.empty())
	{ //draw blurry highlight image over screen
		LLGLEnable blend(GL_BLEND);
		LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
		LLGLDisable test(GL_ALPHA_TEST);

		LLGLEnable stencil(GL_STENCIL_TEST);
		gGL.flush();
		glStencilMask(0xFFFFFFFF);
		glClearStencil(1);
		glClear(GL_STENCIL_BUFFER_BIT);

		glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

		gGL.setColorMask(false, false);

        gHighlightProgram.bind();

		for (std::set<HighlightItem>::iterator iter = mHighlightSet.begin(); iter != mHighlightSet.end(); ++iter)
		{
			renderHighlight(iter->mItem->getVObj(), 1.f);
		}
		gGL.setColorMask(true, false);

		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
		
		//gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);

		gGL.pushMatrix();
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.pushMatrix();
		gGL.loadIdentity();

		gGL.getTexUnit(0)->bind(&mHighlight);

		LLVector2 tc1;
		LLVector2 tc2;

		tc1.setVec(0,0);
		tc2.setVec(2,2);

		gGL.begin(LLRender::TRIANGLES);
				
		F32 scale = RenderHighlightBrightness;
		LLColor4 color = RenderHighlightColor;
		F32 thickness = RenderHighlightThickness;

		for (S32 pass = 0; pass < 2; ++pass)
		{
			if (pass == 0)
			{
				gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);
			}
			else
			{
				gGL.setSceneBlendType(LLRender::BT_ALPHA);
			}

			for (S32 i = 0; i < 8; ++i)
			{
				for (S32 j = 0; j < 8; ++j)
				{
					LLVector2 tc(i-4+0.5f, j-4+0.5f);

					F32 dist = 1.f-(tc.length()/sqrtf(32.f));
					dist *= scale/64.f;

					tc *= thickness;
					tc.mV[0] = (tc.mV[0])/mHighlight.getWidth();
					tc.mV[1] = (tc.mV[1])/mHighlight.getHeight();

					gGL.color4f(color.mV[0],
								color.mV[1],
								color.mV[2],
								color.mV[3]*dist);
					
					gGL.texCoord2f(tc.mV[0]+tc1.mV[0], tc.mV[1]+tc2.mV[1]);
					gGL.vertex2f(-1,3);
					
					gGL.texCoord2f(tc.mV[0]+tc1.mV[0], tc.mV[1]+tc1.mV[1]);
					gGL.vertex2f(-1,-1);
					
					gGL.texCoord2f(tc.mV[0]+tc2.mV[0], tc.mV[1]+tc1.mV[1]);
					gGL.vertex2f(3,-1);
				}
			}
		}

		gGL.end();

		gGL.popMatrix();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.popMatrix();
		
		//gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}

	if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
	{
		gHighlightProgram.bind();
		gGL.diffuseColor4f(1,1,1,0.5f);
	}
	
	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED) && !mFaceSelectImagep)
		{
			mFaceSelectImagep = LLViewerTextureManager::getFetchedTexture(IMG_FACE_SELECT);
		}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED) && (sRenderHighlightTextureChannel == LLRender::DIFFUSE_MAP))
	{
		// Make sure the selection image gets downloaded and decoded
		mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

		U32 count = mSelectedFaces.size();
		for (U32 i = 0; i < count; i++)
		{
			LLFace *facep = mSelectedFaces[i];
			if (!facep || facep->getDrawable()->isDead())
			{
				LL_ERRS() << "Bad face on selection" << LL_ENDL;
				return;
			}
			
			facep->renderSelected(mFaceSelectImagep, color);
		}
	}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Paint 'em red!
		color.setVec(1.f, 0.f, 0.f, 0.5f);
		
		int count = mHighlightFaces.size();
		for (S32 i = 0; i < count; i++)
		{
			LLFace* facep = mHighlightFaces[i];
			facep->renderSelected(LLViewerTexture::sNullImagep, color);
		}
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();

	if (LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0)
	{
		gHighlightProgram.unbind();
	}


	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED) && (sRenderHighlightTextureChannel == LLRender::NORMAL_MAP))
	{
		color.setVec(1.0f, 0.5f, 0.5f, 0.5f);
		if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
		{
			gHighlightNormalProgram.bind();
			gGL.diffuseColor4f(1,1,1,0.5f);
		}

		mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

		U32 count = mSelectedFaces.size();
		for (U32 i = 0; i < count; i++)
		{
			LLFace *facep = mSelectedFaces[i];
			if (!facep || facep->getDrawable()->isDead())
			{
				LL_ERRS() << "Bad face on selection" << LL_ENDL;
				return;
			}

			facep->renderSelected(mFaceSelectImagep, color);
		}

		if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
		{
			gHighlightNormalProgram.unbind();
		}
	}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED) && (sRenderHighlightTextureChannel == LLRender::SPECULAR_MAP))
	{
		color.setVec(0.0f, 0.3f, 1.0f, 0.8f);
		if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
		{
			gHighlightSpecularProgram.bind();
			gGL.diffuseColor4f(1,1,1,0.5f);
		}

		mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

		U32 count = mSelectedFaces.size();
		for (U32 i = 0; i < count; i++)
		{
			LLFace *facep = mSelectedFaces[i];
			if (!facep || facep->getDrawable()->isDead())
			{
				LL_ERRS() << "Bad face on selection" << LL_ENDL;
				return;
			}

			facep->renderSelected(mFaceSelectImagep, color);
		}

		if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
		{
			gHighlightSpecularProgram.unbind();
		}
	}
}

//debug use
U32 LLPipeline::sCurRenderPoolType = 0 ;

void LLPipeline::renderGeom(LLCamera& camera, bool forceVBOUpdate)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GEOMETRY);

	assertInitialized();

	F32 saved_modelview[16];
	F32 saved_projection[16];

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
	if (mRenderDebugMask & RENDER_DEBUG_VERIFY)
	{
		if (!verify())
		{
			LL_ERRS() << "Pipeline verification failed!" << LL_ENDL;
		}
	}

	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:ForceVBO");
	
	// Initialize lots of GL state to "safe" values
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	LLGLSPipeline gls_pipeline;
	LLGLEnable multisample(RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);

	LLGLState gls_color_material(GL_COLOR_MATERIAL, mLightingDetail < 2);
				
	// Toggle backface culling for debugging
	LLGLEnable cull_face(mBackfaceCull ? GL_CULL_FACE : 0);
	// Set fog
	bool use_fog = hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOG);
	LLGLEnable fog_enable(use_fog &&
						  !gPipeline.canUseWindLightShadersOnObjects() ? GL_FOG : 0);
	gSky.updateFog(camera.getFar());
	if (!use_fog)
	{
		sUnderWaterRender = false;
	}

	gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sDefaultImagep);
	LLViewerFetchedTexture::sDefaultImagep->setAddressMode(LLTexUnit::TAM_WRAP);
	

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

	{
		LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pools"); //LL_RECORD_BLOCK_TIME(FTM_POOLS);
		
		// HACK: don't calculate local lights if we're rendering the HUD!
		//    Removing this check will cause bad flickering when there are 
		//    HUD elements being rendered AND the user is in flycam mode  -nyx
		if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{
			calcNearbyLights(camera);
			setupHWLights(NULL);
		}

		bool occlude = sUseOcclusion > 1;
		U32 cur_type = 0;

		pool_set_t::iterator iter1 = mPools.begin();
		while ( iter1 != mPools.end() )
		{
			LLDrawPool *poolp = *iter1;
			
			cur_type = poolp->getType();

			//debug use
			sCurRenderPoolType = cur_type ;

			if (occlude && cur_type >= LLDrawPool::POOL_GRASS)
			{
				occlude = false;
				gGLLastMatrix = NULL;
				gGL.loadMatrix(gGLModelView);
				LLGLSLShader::bindNoShader();
				doOcclusion(camera);
			}

			pool_set_t::iterator iter2 = iter1;
			if (hasRenderType(poolp->getType()) && poolp->getNumPasses() > 0)
			{
				LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pool render"); //LL_RECORD_BLOCK_TIME(FTM_POOLRENDER);

				gGLLastMatrix = NULL;
				gGL.loadMatrix(gGLModelView);
			
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
						
						if ( !p->getSkipRenderFlag() ) { p->render(i); }
					}
					poolp->endRenderPass(i);
					LLVertexBuffer::unbind();
					if (gDebugGL)
					{
						std::string msg = llformat("pass %d", i);
						LLGLState::checkStates(msg);
						//LLGLState::checkTextureChannels(msg);
						//LLGLState::checkClientArrays(msg);
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
		gGL.loadMatrix(gGLModelView);

		if (occlude)
		{
			occlude = false;
			gGLLastMatrix = NULL;
			gGL.loadMatrix(gGLModelView);
			LLGLSLShader::bindNoShader();
			doOcclusion(camera);
		}
	}

	LLVertexBuffer::unbind();
	LLGLState::checkStates();

	if (!LLPipeline::sImpostorRender)
	{
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
	
		if (!LLPipeline::sReflectionRender && !LLPipeline::sRenderDeferred)
		{
			if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
			{
				// Render debugging beacons.
				gObjectList.renderObjectBeacons();
				gObjectList.resetObjectBeacons();
                gSky.addSunMoonBeacons();
			}
			else
			{
				// Make sure particle effects disappear
				LLHUDObject::renderAllForTimer();
			}
		}
		else
		{
			// Make sure particle effects disappear
			LLHUDObject::renderAllForTimer();
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
	}

	LLVertexBuffer::unbind();

	LLGLState::checkStates();
//	LLGLState::checkTextureChannels();
//	LLGLState::checkClientArrays();
}

void LLPipeline::renderGeomDeferred(LLCamera& camera)
{
	LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderGeomDeferred");

	LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GEOMETRY);
	{
		// SL-15709 -- NOTE: Tracy only allows one ZoneScoped per function.
		// Solutions are:
		// 1. Use a new scope
		// 2. Use named zones
		// 3. Use transient zones
		LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred pools"); //LL_RECORD_BLOCK_TIME(FTM_DEFERRED_POOLS);

		LLGLEnable cull(GL_CULL_FACE);

		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			if (hasRenderType(poolp->getType()))
			{
				poolp->prerender();
			}
		}

		LLGLEnable multisample(RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);

		LLVertexBuffer::unbind();

		LLGLState::checkStates();
		LLGLState::checkTextureChannels();

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
				LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred pool render"); //LL_RECORD_BLOCK_TIME(FTM_DEFERRED_POOLRENDER);

				gGLLastMatrix = NULL;
				gGL.loadMatrix(gGLModelView);
		
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

						if ( !p->getSkipRenderFlag() ) { p->renderDeferred(i); }
					}
					poolp->endDeferredPass(i);
					LLVertexBuffer::unbind();

					if (gDebugGL || gDebugPipeline)
					{
						LLGLState::checkStates();
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

		gGLLastMatrix = NULL;
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.loadMatrix(gGLModelView);

		gGL.setColorMask(true, false);

	} // Tracy ZoneScoped
}

void LLPipeline::renderGeomPostDeferred(LLCamera& camera, bool do_occlusion)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_POST_DEFERRED_POOLS);
	U32 cur_type = 0;

	LLGLEnable cull(GL_CULL_FACE);

	LLGLEnable multisample(RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);

	calcNearbyLights(camera);
	setupHWLights(NULL);

	gGL.setColorMask(true, false);

	pool_set_t::iterator iter1 = mPools.begin();
	bool occlude = LLPipeline::sUseOcclusion > 1 && do_occlusion;

	while ( iter1 != mPools.end() )
	{
		LLDrawPool *poolp = *iter1;
		
		cur_type = poolp->getType();

		if (occlude && cur_type >= LLDrawPool::POOL_GRASS)
		{
			occlude = false;
			gGLLastMatrix = NULL;
			gGL.loadMatrix(gGLModelView);
			LLGLSLShader::bindNoShader();
			doOcclusion(camera, mScreen, mOcclusionDepth, &mDeferredDepth);
			gGL.setColorMask(true, false);
		}

		pool_set_t::iterator iter2 = iter1;
		if (hasRenderType(poolp->getType()) && poolp->getNumPostDeferredPasses() > 0)
		{
			LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred poolrender"); //LL_RECORD_BLOCK_TIME(FTM_POST_DEFERRED_POOLRENDER);

			gGLLastMatrix = NULL;
			gGL.loadMatrix(gGLModelView);
		
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

				if (gDebugGL || gDebugPipeline)
				{
					LLGLState::checkStates();
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

	gGLLastMatrix = NULL;
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(gGLModelView);

	if (occlude)
	{
		occlude = false;
		LLGLSLShader::bindNoShader();
		doOcclusion(camera);
		gGLLastMatrix = NULL;
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.loadMatrix(gGLModelView);
	}
}

void LLPipeline::renderGeomShadow(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
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
			poolp->prerender() ;

			gGLLastMatrix = NULL;
			gGL.loadMatrix(gGLModelView);
		
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
	gGL.loadMatrix(gGLModelView);
}


void LLPipeline::addTrianglesDrawn(S32 index_count, U32 render_type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	assertInitialized();
	S32 count = 0;
	if (render_type == LLRender::TRIANGLE_STRIP)
	{
		count = index_count-2;
	}
	else
	{
		count = index_count/3;
	}

	record(sStatBatchSize, count);
	add(LLStatViewer::TRIANGLES_DRAWN, LLUnits::Triangles::fromValue(count));

	if (LLPipeline::sRenderFrameTest)
	{
		gViewerWindow->getWindow()->swapBuffers();
		ms_sleep(16);
	}
}

void LLPipeline::renderPhysicsDisplay()
{
	if (!hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PHYSICS_SHAPES))
	{
		return;
	}

	allocatePhysicsBuffer();

	gGL.flush();
	mPhysicsDisplay.bindTarget();
	glClearColor(0,0,0,1);
	gGL.setColorMask(true, true);
	mPhysicsDisplay.clear();
	glClearColor(0,0,0,0);

	gGL.setColorMask(true, false);

	gDebugProgram.bind();

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
					part->renderPhysicsShapes();
				}
			}
		}
	}

	gGL.flush();

	gDebugProgram.unbind();
	mPhysicsDisplay.flush();
}

extern std::set<LLSpatialGroup*> visible_selected_groups;

void LLPipeline::renderDebug()
{
	assertInitialized();

	bool hud_only = hasRenderType(LLPipeline::RENDER_TYPE_HUD);

	if (!hud_only )
	{
		//Render any navmesh geometry	
		LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
		if ( llPathingLibInstance != NULL ) 
		{
			//character floater renderables
			
			LLHandle<LLFloaterPathfindingCharacters> pathfindingCharacterHandle = LLFloaterPathfindingCharacters::getInstanceHandle();
			if ( !pathfindingCharacterHandle.isDead() )
			{
				LLFloaterPathfindingCharacters *pathfindingCharacter = pathfindingCharacterHandle.get();

				if ( pathfindingCharacter->getVisible() || gAgentCamera.cameraMouselook() )			
				{	
					gPathfindingProgram.bind();			
					gPathfindingProgram.uniform1f(sTint, 1.f);
					gPathfindingProgram.uniform1f(sAmbiance, 1.f);
					gPathfindingProgram.uniform1f(sAlphaScale, 1.f);

					//Requried character physics capsule render parameters
					LLUUID id;					
					LLVector3 pos;
					LLQuaternion rot;
				
					if ( pathfindingCharacter->isPhysicsCapsuleEnabled( id, pos, rot ) )
					{
						//remove blending artifacts
						gGL.setColorMask(false, false);
						llPathingLibInstance->renderSimpleShapeCapsuleID( gGL, id, pos, rot );				
						gGL.setColorMask(true, false);
						LLGLEnable blend(GL_BLEND);
						gPathfindingProgram.uniform1f(sAlphaScale, 0.90f);
						llPathingLibInstance->renderSimpleShapeCapsuleID( gGL, id, pos, rot );
						gPathfindingProgram.bind();
					}
				}
			}
			

			//pathing console renderables
			LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
			if (!pathfindingConsoleHandle.isDead())
			{
				LLFloaterPathfindingConsole *pathfindingConsole = pathfindingConsoleHandle.get();

				if ( pathfindingConsole->getVisible() || gAgentCamera.cameraMouselook() )
				{				
					F32 ambiance = gSavedSettings.getF32("PathfindingAmbiance");

					gPathfindingProgram.bind();
			
					gPathfindingProgram.uniform1f(sTint, 1.f);
					gPathfindingProgram.uniform1f(sAmbiance, ambiance);
					gPathfindingProgram.uniform1f(sAlphaScale, 1.f);

					if ( !pathfindingConsole->isRenderWorld() )
					{
						const LLColor4 clearColor = gSavedSettings.getColor4("PathfindingNavMeshClear");
						gGL.setColorMask(true, true);
						glClearColor(clearColor.mV[0],clearColor.mV[1],clearColor.mV[2],0);
						glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);					
						gGL.setColorMask(true, false);
						glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
					}

					//NavMesh
					if ( pathfindingConsole->isRenderNavMesh() )
					{	
						gGL.flush();
						glLineWidth(2.0f);	
						LLGLEnable cull(GL_CULL_FACE);
						LLGLDisable blend(GL_BLEND);
						
						if ( pathfindingConsole->isRenderWorld() )
						{					
							LLGLEnable blend(GL_BLEND);
							gPathfindingProgram.uniform1f(sAlphaScale, 0.66f);
							llPathingLibInstance->renderNavMesh();
						}
						else
						{
							llPathingLibInstance->renderNavMesh();
						}
						
						//render edges
						gPathfindingNoNormalsProgram.bind();
						gPathfindingNoNormalsProgram.uniform1f(sTint, 1.f);
						gPathfindingNoNormalsProgram.uniform1f(sAlphaScale, 1.f);
						llPathingLibInstance->renderNavMeshEdges();
						gPathfindingProgram.bind();

						gGL.flush();
						glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
						glLineWidth(1.0f);	
						gGL.flush();
					}
					//User designated path
					if ( LLPathfindingPathTool::getInstance()->isRenderPath() )
					{
						//The path
						gUIProgram.bind();
						gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
						llPathingLibInstance->renderPath();
						gPathfindingProgram.bind();

                        //The bookends
						//remove blending artifacts
						gGL.setColorMask(false, false);
						llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_START );
						llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_END );
						
						gGL.setColorMask(true, false);
						//render the bookends
						LLGLEnable blend(GL_BLEND);
						gPathfindingProgram.uniform1f(sAlphaScale, 0.90f);
						llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_START );
						llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_END );
						gPathfindingProgram.bind();
					}
				
					if ( pathfindingConsole->isRenderWaterPlane() )
					{	
						LLGLEnable blend(GL_BLEND);
						gPathfindingProgram.uniform1f(sAlphaScale, 0.90f);
						llPathingLibInstance->renderSimpleShapes( gGL, gAgent.getRegion()->getWaterHeight() );
					}
				//physics/exclusion shapes
				if ( pathfindingConsole->isRenderAnyShapes() )
				{					
						U32 render_order[] = {
							1 << LLPathingLib::LLST_ObstacleObjects,
							1 << LLPathingLib::LLST_WalkableObjects,
							1 << LLPathingLib::LLST_ExclusionPhantoms,	
							1 << LLPathingLib::LLST_MaterialPhantoms,
						};

						U32 flags = pathfindingConsole->getRenderShapeFlags();

						for (U32 i = 0; i < 4; i++)
						{
							if (!(flags & render_order[i]))
							{
								continue;
							}

							//turn off backface culling for volumes so they are visible when camera is inside volume
							LLGLDisable cull(i >= 2 ? GL_CULL_FACE : 0);
						
							gGL.flush();
							glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
				
							//get rid of some z-fighting
							LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
							glPolygonOffset(1.0f, 1.0f);

							//render to depth first to avoid blending artifacts
							gGL.setColorMask(false, false);
							llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );		
							gGL.setColorMask(true, false);

							//get rid of some z-fighting
							glPolygonOffset(0.f, 0.f);

							LLGLEnable blend(GL_BLEND);
				
							{
								gPathfindingProgram.uniform1f(sAmbiance, ambiance);

								{ //draw solid overlay
									LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
									llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );				
									gGL.flush();				
								}
				
								LLGLEnable lineOffset(GL_POLYGON_OFFSET_LINE);
								glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );	
						
								F32 offset = gSavedSettings.getF32("PathfindingLineOffset");

								if (pathfindingConsole->isRenderXRay())
								{
									gPathfindingProgram.uniform1f(sTint, gSavedSettings.getF32("PathfindingXRayTint"));
									gPathfindingProgram.uniform1f(sAlphaScale, gSavedSettings.getF32("PathfindingXRayOpacity"));
									LLGLEnable blend(GL_BLEND);
									LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);
								
									glPolygonOffset(offset, -offset);
								
									if (gSavedSettings.getBOOL("PathfindingXRayWireframe"))
									{ //draw hidden wireframe as darker and less opaque
										gPathfindingProgram.uniform1f(sAmbiance, 1.f);
										llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );				
									}
									else
									{
										glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
										gPathfindingProgram.uniform1f(sAmbiance, ambiance);
										llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );				
										glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
									}
								}

								{ //draw visible wireframe as brighter, thicker and more opaque
									glPolygonOffset(offset, offset);
									gPathfindingProgram.uniform1f(sAmbiance, 1.f);
									gPathfindingProgram.uniform1f(sTint, 1.f);
									gPathfindingProgram.uniform1f(sAlphaScale, 1.f);

									glLineWidth(gSavedSettings.getF32("PathfindingLineWidth"));
									LLGLDisable blendOut(GL_BLEND);
									llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );				
									gGL.flush();
									glLineWidth(1.f);
								}
				
								glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
							}
						}
					}

					glPolygonOffset(0.f, 0.f);

					if ( pathfindingConsole->isRenderNavMesh() && pathfindingConsole->isRenderXRay() )
					{	//render navmesh xray
						F32 ambiance = gSavedSettings.getF32("PathfindingAmbiance");

						LLGLEnable lineOffset(GL_POLYGON_OFFSET_LINE);
						LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
											
						F32 offset = gSavedSettings.getF32("PathfindingLineOffset");
						glPolygonOffset(offset, -offset);

						LLGLEnable blend(GL_BLEND);
						LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);
						gGL.flush();				
						glLineWidth(2.0f);	
						LLGLEnable cull(GL_CULL_FACE);
																		
						gPathfindingProgram.uniform1f(sTint, gSavedSettings.getF32("PathfindingXRayTint"));
						gPathfindingProgram.uniform1f(sAlphaScale, gSavedSettings.getF32("PathfindingXRayOpacity"));
								
						if (gSavedSettings.getBOOL("PathfindingXRayWireframe"))
						{ //draw hidden wireframe as darker and less opaque
							glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );	
							gPathfindingProgram.uniform1f(sAmbiance, 1.f);
							llPathingLibInstance->renderNavMesh();
							glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	
						}	
						else
						{
							gPathfindingProgram.uniform1f(sAmbiance, ambiance);
							llPathingLibInstance->renderNavMesh();
						}

						//render edges
						gPathfindingNoNormalsProgram.bind();
						gPathfindingNoNormalsProgram.uniform1f(sTint, gSavedSettings.getF32("PathfindingXRayTint"));
						gPathfindingNoNormalsProgram.uniform1f(sAlphaScale, gSavedSettings.getF32("PathfindingXRayOpacity"));
						llPathingLibInstance->renderNavMeshEdges();
						gPathfindingProgram.bind();
					
						gGL.flush();
						glLineWidth(1.0f);	
					}
			
					glPolygonOffset(0.f, 0.f);

					gGL.flush();
					gPathfindingProgram.unbind();
				}
			}
		}
	}

	gGL.color4f(1,1,1,1);

	gGLLastMatrix = NULL;
	gGL.loadMatrix(gGLModelView);
	gGL.setColorMask(true, false);

	
	if (!hud_only && !mDebugBlips.empty())
	{ //render debug blips
		gUIProgram.bind();

		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep, true);

		glPointSize(8.f);
		LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

		gGL.begin(LLRender::POINTS);
		for (std::list<DebugBlip>::iterator iter = mDebugBlips.begin(); iter != mDebugBlips.end(); )
		{
			DebugBlip& blip = *iter;

			blip.mAge += gFrameIntervalSeconds.value();
			if (blip.mAge > 2.f)
			{
				mDebugBlips.erase(iter++);
			}
			else
			{
				iter++;
			}

			blip.mPosition.mV[2] += gFrameIntervalSeconds.value()*2.f;

			gGL.color4fv(blip.mColor.mV);
			gGL.vertex3fv(blip.mPosition.mV);
		}
		gGL.end();
		gGL.flush();
		glPointSize(1.f);
	}


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
				if ( (hud_only && (part->mDrawableType == RENDER_TYPE_HUD || part->mDrawableType == RENDER_TYPE_HUD_PARTICLES)) ||
					 (!hud_only && hasRenderType(part->mDrawableType)) )
				{
					part->renderDebug();
				}
			}
		}
	}

	for (LLCullResult::bridge_iterator i = sCull->beginVisibleBridge(); i != sCull->endVisibleBridge(); ++i)
	{
		LLSpatialBridge* bridge = *i;
		if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
		{
			gGL.pushMatrix();
			gGL.multMatrix((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
			bridge->renderDebug();
			gGL.popMatrix();
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
	{ //render visible selected group occlusion geometry
		gDebugProgram.bind();
		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		gGL.diffuseColor3f(1,0,1);
		for (std::set<LLSpatialGroup*>::iterator iter = visible_selected_groups.begin(); iter != visible_selected_groups.end(); ++iter)
		{
			LLSpatialGroup* group = *iter;

			LLVector4a fudge;
			fudge.splat(0.25f); //SG_OCCLUSION_FUDGE

			LLVector4a size;
			const LLVector4a* bounds = group->getBounds();
			size.setAdd(fudge, bounds[1]);
			
			drawBox(bounds[0], size);
		}
	}

	visible_selected_groups.clear();

	gUIProgram.bind();

	if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST) && !hud_only)
	{ //draw crosshairs on particle intersection
		if (gDebugRaycastParticle)
		{
			gDebugProgram.bind();

			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			LLVector3 center(gDebugRaycastParticleIntersection.getF32ptr());
			LLVector3 size(0.1f, 0.1f, 0.1f);

			LLVector3 p[6];

			p[0] = center + size.scaledVec(LLVector3(1,0,0));
			p[1] = center + size.scaledVec(LLVector3(-1,0,0));
			p[2] = center + size.scaledVec(LLVector3(0,1,0));
			p[3] = center + size.scaledVec(LLVector3(0,-1,0));
			p[4] = center + size.scaledVec(LLVector3(0,0,1));
			p[5] = center + size.scaledVec(LLVector3(0,0,-1));
				
			gGL.begin(LLRender::LINES);
			gGL.diffuseColor3f(1.f, 1.f, 0.f);
			for (U32 i = 0; i < 6; i++)
			{
				gGL.vertex3fv(p[i].mV);
			}
			gGL.end();
			gGL.flush();

			gDebugProgram.unbind();
		}
	}

	if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
	{
		LLVertexBuffer::unbind();

		LLGLEnable blend(GL_BLEND);
		LLGLDepthTest depth(TRUE, FALSE);
		LLGLDisable cull(GL_CULL_FACE);

		gGL.color4f(1,1,1,1);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				
		F32 a = 0.1f;

		F32 col[] =
		{
			1,0,0,a,
			0,1,0,a,
			0,0,1,a,
			1,0,1,a,
			
			1,1,0,a,
			0,1,1,a,
			1,1,1,a,
			1,0,1,a,
		};

		for (U32 i = 0; i < 8; i++)
		{
			LLVector3* frust = mShadowCamera[i].mAgentFrustum;

			if (i > 3)
			{ //render shadow frusta as volumes
				if (mShadowFrustPoints[i-4].empty())
				{
					continue;
				}

				gGL.color4fv(col+(i-4)*4);	
			
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
				gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[5].mV);
				gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[6].mV);
				gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[7].mV);
				gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
				gGL.end();
				
				
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3fv(frust[0].mV);
				gGL.vertex3fv(frust[1].mV);
				gGL.vertex3fv(frust[3].mV);
				gGL.vertex3fv(frust[2].mV);
				gGL.end();
				
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3fv(frust[4].mV);
				gGL.vertex3fv(frust[5].mV);
				gGL.vertex3fv(frust[7].mV);
				gGL.vertex3fv(frust[6].mV);
				gGL.end();		
			}

	
			if (i < 4)
			{
				
				//if (i == 0 || !mShadowFrustPoints[i].empty())
				{
					//render visible point cloud
					gGL.flush();
					glPointSize(8.f);
					gGL.begin(LLRender::POINTS);
					
					F32* c = col+i*4;
					gGL.color3fv(c);

					for (U32 j = 0; j < mShadowFrustPoints[i].size(); ++j)
						{
							gGL.vertex3fv(mShadowFrustPoints[i][j].mV);
						
						}
					gGL.end();

					gGL.flush();
					glPointSize(1.f);

					LLVector3* ext = mShadowExtents[i]; 
					LLVector3 pos = (ext[0]+ext[1])*0.5f;
					LLVector3 size = (ext[1]-ext[0])*0.5f;
					drawBoxOutline(pos, size);

					//render camera frustum splits as outlines
					gGL.begin(LLRender::LINES);
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
					gGL.end();
				}
			}

			/*gGL.flush();
			glLineWidth(16-i*2);
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
			gGL.flush();
			glLineWidth(1.f);*/
		}
	}

	if (mRenderDebugMask & RENDER_DEBUG_WIND_VECTORS)
	{
		gAgent.getRegion()->mWind.renderVectors();
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

	if (mRenderDebugMask & LLPipeline::RENDER_DEBUG_BUILD_QUEUE)
	{
		U32 count = 0;
		U32 size = mGroupQ2.size();
		LLColor4 col;

		LLVertexBuffer::unbind();
		LLGLEnable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
		
		gGL.pushMatrix();
		gGL.loadMatrix(gGLModelView);
		gGLLastMatrix = NULL;

		for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ2.begin(); iter != mGroupQ2.end(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			if (group->isDead())
			{
				continue;
			}

			LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();

			if (bridge && (!bridge->mDrawable || bridge->mDrawable->isDead()))
			{
				continue;
			}

			if (bridge)
			{
				gGL.pushMatrix();
				gGL.multMatrix((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
			}

			F32 alpha = llclamp((F32) (size-count)/size, 0.f, 1.f);

			
			LLVector2 c(1.f-alpha, alpha);
			c.normVec();

			
			++count;
			col.set(c.mV[0], c.mV[1], 0, alpha*0.5f+0.5f);
			group->drawObjectBox(col);

			if (bridge)
			{
				gGL.popMatrix();
			}
		}

		gGL.popMatrix();
	}

	gGL.flush();
	gUIProgram.unbind();
}

void LLPipeline::rebuildPools()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

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
}

void LLPipeline::addToQuickLookup( LLDrawPool* new_poolp )
{
	assertInitialized();

	switch( new_poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		if (mSimplePool)
		{
			llassert(0);
			LL_WARNS() << "Ignoring duplicate simple pool." << LL_ENDL;
		}
		else
		{
			mSimplePool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_ALPHA_MASK:
		if (mAlphaMaskPool)
		{
			llassert(0);
			LL_WARNS() << "Ignoring duplicate alpha mask pool." << LL_ENDL;
			break;
		}
		else
		{
			mAlphaMaskPool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
		if (mFullbrightAlphaMaskPool)
		{
			llassert(0);
			LL_WARNS() << "Ignoring duplicate alpha mask pool." << LL_ENDL;
			break;
		}
		else
		{
			mFullbrightAlphaMaskPool = (LLRenderPass*) new_poolp;
		}
		break;
		
	case LLDrawPool::POOL_GRASS:
		if (mGrassPool)
		{
			llassert(0);
			LL_WARNS() << "Ignoring duplicate grass pool." << LL_ENDL;
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
			LL_WARNS() << "Ignoring duplicate simple pool." << LL_ENDL;
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
			LL_WARNS() << "Ignoring duplicate simple pool." << LL_ENDL;
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
			LL_WARNS() << "Ignoring duplicate glow pool." << LL_ENDL;
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
			LL_WARNS() << "Ignoring duplicate bump pool." << LL_ENDL;
		}
		else
		{
			mBumpPool = new_poolp;
		}
		break;
	case LLDrawPool::POOL_MATERIALS:
		if (mMaterialsPool)
		{
			llassert(0);
			LL_WARNS() << "Ignorning duplicate materials pool." << LL_ENDL;
		}
		else
		{
			mMaterialsPool = new_poolp;
		}
		break;
	case LLDrawPool::POOL_ALPHA:
		if( mAlphaPool )
		{
			llassert(0);
			LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Alpha pool" << LL_ENDL;
		}
		else
		{
			mAlphaPool = (LLDrawPoolAlpha*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_AVATAR:
	case LLDrawPool::POOL_CONTROL_AV:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		if( mSkyPool )
		{
			llassert(0);
			LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Sky pool" << LL_ENDL;
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
			LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Water pool" << LL_ENDL;
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
			LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Ground Pool" << LL_ENDL;
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
			LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate WLSky Pool" << LL_ENDL;
		}
		else
		{ 
			mWLSkyPool = new_poolp;
		}
		break;

	default:
		llassert(0);
		LL_WARNS() << "Invalid Pool Type in  LLPipeline::addPool()" << LL_ENDL;
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
	switch( poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		llassert(mSimplePool == poolp);
		mSimplePool = NULL;
		break;

	case LLDrawPool::POOL_ALPHA_MASK:
		llassert(mAlphaMaskPool == poolp);
		mAlphaMaskPool = NULL;
		break;

	case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
		llassert(mFullbrightAlphaMaskPool == poolp);
		mFullbrightAlphaMaskPool = NULL;
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
				bool found = mTreePools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTreePools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_TERRAIN:
		#ifdef _DEBUG
			{
				bool found = mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
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
	
	case LLDrawPool::POOL_MATERIALS:
		llassert(poolp == mMaterialsPool);
		mMaterialsPool = NULL;
		break;
			
	case LLDrawPool::POOL_ALPHA:
		llassert( poolp == mAlphaPool );
		mAlphaPool = NULL;
		break;

	case LLDrawPool::POOL_AVATAR:
	case LLDrawPool::POOL_CONTROL_AV:
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
		LL_WARNS() << "Invalid Pool Type in  LLPipeline::removeFromQuickLookup() type=" << poolp->getType() << LL_ENDL;
		break;
	}
}

void LLPipeline::resetDrawOrders()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
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

void LLPipeline::setupAvatarLights(bool for_edit)
{
	assertInitialized();

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    bool sun_up = environment.getIsSunUp();


	if (for_edit)
	{
		LLColor4 diffuse(1.f, 1.f, 1.f, 0.f);
		LLVector4 light_pos_cam(-8.f, 0.25f, 10.f, 0.f);  // w==0 => directional light
		LLMatrix4 camera_mat = LLViewerCamera::getInstance()->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();
		LLVector4 light_pos = light_pos_cam * camera_rot;
		
		light_pos.normalize();

		LLLightState* light = gGL.getLight(1);

		mHWLightColors[1] = diffuse;

		light->setDiffuse(diffuse);
		light->setAmbient(LLColor4::black);
		light->setSpecular(LLColor4::black);
		light->setPosition(light_pos);
		light->setConstantAttenuation(1.f);
		light->setLinearAttenuation(0.f);
		light->setQuadraticAttenuation(0.f);
		light->setSpotExponent(0.f);
		light->setSpotCutoff(180.f);
	}
	else if (gAvatarBacklight) // Always true (unless overridden in a devs .ini)
	{
        LLVector3 light_dir = sun_up ? LLVector3(mSunDir) : LLVector3(mMoonDir);
		LLVector3 opposite_pos = -light_dir;
		LLVector3 orthog_light_pos = light_dir % LLVector3::z_axis;
		LLVector4 backlight_pos = LLVector4(lerp(opposite_pos, orthog_light_pos, 0.3f), 0.0f);
		backlight_pos.normalize();
			
		LLColor4 light_diffuse = sun_up ? mSunDiffuse : mMoonDiffuse;

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
		if (LLEnvironment::instance().getIsSunUp())
		{
			backlight_mag = BACKLIGHT_DAY_MAGNITUDE_OBJECT;
		}
		else
		{
			backlight_mag = BACKLIGHT_NIGHT_MAGNITUDE_OBJECT;
		}
		backlight_diffuse *= backlight_mag / max_component;

		mHWLightColors[1] = backlight_diffuse;

		LLLightState* light = gGL.getLight(1);

		light->setPosition(backlight_pos);
		light->setDiffuse(backlight_diffuse);
		light->setAmbient(LLColor4::black);
		light->setSpecular(LLColor4::black);
		light->setConstantAttenuation(1.f);
		light->setLinearAttenuation(0.f);
		light->setQuadraticAttenuation(0.f);
		light->setSpotExponent(0.f);
		light->setSpotCutoff(180.f);
	}
	else
	{
		LLLightState* light = gGL.getLight(1);

		mHWLightColors[1] = LLColor4::black;

		light->setDiffuse(LLColor4::black);
		light->setAmbient(LLColor4::black);
		light->setSpecular(LLColor4::black);
	}
}

static F32 calc_light_dist(LLVOVolume* light, const LLVector3& cam_pos, F32 max_dist)
{
	F32 inten = light->getLightIntensity();
	if (inten < .001f)
	{
		return max_dist;
	}
	bool selected = light->isSelected();
	if (selected)
	{
        return 0.f; // selected lights get highest priority
	}
    F32 radius = light->getLightRadius();
    F32 dist = dist_vec(light->getRenderPosition(), cam_pos);
    dist = llmax(dist - radius, 0.f);
	if (light->mDrawable.notNull() && light->mDrawable->isState(LLDrawable::ACTIVE))
	{
		// moving lights get a little higher priority (too much causes artifacts)
        dist = llmax(dist - light->getLightRadius()*0.25f, 0.f);
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
        LLVector3 cam_pos = camera.getOrigin();
		
        F32 max_dist;
        if (LLPipeline::sRenderDeferred)
        {
            max_dist = RenderFarClip;
        }
        else
        {
            max_dist = llmin(RenderFarClip, LIGHT_MAX_RADIUS * 4.f);
        }

		// UPDATE THE EXISTING NEARBY LIGHTS
		light_set_t cur_nearby_lights;
		for (light_set_t::iterator iter = mNearbyLights.begin();
			iter != mNearbyLights.end(); iter++)
		{
			const Light* light = &(*iter);
			LLDrawable* drawable = light->drawable;
            const LLViewerObject *vobj = light->drawable->getVObj();
            if(vobj && vobj->getAvatar() 
               && (vobj->getAvatar()->isTooComplex() || vobj->getAvatar()->isInMuteList())
               )
            {
                drawable->clearState(LLDrawable::NEARBY_LIGHT);
                continue;
            }

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
            F32 fade = light->fade;
            // actual fade gets decreased/increased by setupHWLights
            // light->fade value is 'time'.
            // >=0 and light will become visible as value increases
            // <0 and light will fade out
            if (dist < max_dist)
            {
                if (fade < 0)
                {
                    // mark light to fade in
                    // if fade was -LIGHT_FADE_TIME - it was fully invisible
                    // if fade -0 - it was fully visible
                    // visibility goes up from 0 to LIGHT_FADE_TIME.
                    fade += LIGHT_FADE_TIME;
                }
            }
            else
            {
                // mark light to fade out
                // visibility goes down from -0 to -LIGHT_FADE_TIME.
                if (fade >= LIGHT_FADE_TIME)
                {
                    fade = -0.0001f; // was fully visible
                }
                else if (fade >= 0)
                {
                    // 0.75 visible light should stay 0.75 visible, but should reverse direction
                    fade -= LIGHT_FADE_TIME;
                }
            }
            cur_nearby_lights.insert(Light(drawable, dist, fade));
		}
		mNearbyLights = cur_nearby_lights;
				
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
            if (!sRenderAttachedLights && light && light->isAttachment())
			{
				continue;
			}
            LLVOAvatar * av = light->getAvatar();
            if (av && (av->isTooComplex() || av->isInMuteList()))
            {
                // avatars that are already in the list will be removed by removeMutedAVsLights
                continue;
            }
            F32 dist = calc_light_dist(light, cam_pos, max_dist);
            if (dist >= max_dist)
			{
				continue;
			}
			new_nearby_lights.insert(Light(drawable, dist, 0.f));
            if (!LLPipeline::sRenderDeferred && new_nearby_lights.size() > (U32)MAX_LOCAL_LIGHTS)
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
            if (LLPipeline::sRenderDeferred || mNearbyLights.size() < (U32)MAX_LOCAL_LIGHTS)
			{
				mNearbyLights.insert(*light);
				((LLDrawable*) light->drawable)->setState(LLDrawable::NEARBY_LIGHT);
			}
			else
			{
				// crazy cast so that we can overwrite the fade value
				// even though gcc enforces sets as const
				// (fade value doesn't affect sort so this is safe)
				Light* farthest_light = (const_cast<Light*>(&(*(mNearbyLights.rbegin()))));
				if (light->dist < farthest_light->dist)
				{
                    // mark light to fade out
                    // visibility goes down from -0 to -LIGHT_FADE_TIME.
                    //
                    // This is a mess, but for now it needs to be in sync
                    // with fade code above. Ex: code above detects distance < max,
                    // sets fade time to positive, this code then detects closer
                    // lights and sets fade time negative, fully compensating
                    // for the code above
                    if (farthest_light->fade >= LIGHT_FADE_TIME)
                    {
                        farthest_light->fade = -0.0001f; // was fully visible
                    }
                    else if (farthest_light->fade >= 0)
                    {
                        farthest_light->fade -= LIGHT_FADE_TIME;
                    }
				}
				else
				{
					break; // none of the other lights are closer
				}
			}
		}
		
		//mark nearby lights not-removable.
		for (light_set_t::iterator iter = mNearbyLights.begin();
			 iter != mNearbyLights.end(); iter++)
		{
			const Light* light = &(*iter);
			((LLViewerOctreeEntryData*) light->drawable)->setVisible();
		}
	}
}

void LLPipeline::setupHWLights(LLDrawPool* pool)
{
	assertInitialized();
	
    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    // Ambient
    LLColor4 ambient = psky->getTotalAmbient();
		gGL.setAmbientLightColor(ambient);

    bool sun_up  = environment.getIsSunUp();
    bool moon_up = environment.getIsMoonUp();

	// Light 0 = Sun or Moon (All objects)
	{
        LLVector4 sun_dir(environment.getSunDirection(), 0.0f);
        LLVector4 moon_dir(environment.getMoonDirection(), 0.0f);

        mSunDir.setVec(sun_dir);
        mMoonDir.setVec(moon_dir);

        mSunDiffuse.setVec(psky->getSunlightColor());
        mMoonDiffuse.setVec(psky->getMoonlightColor());

		F32 max_color = llmax(mSunDiffuse.mV[0], mSunDiffuse.mV[1], mSunDiffuse.mV[2]);
		if (max_color > 1.f)
		{
			mSunDiffuse *= 1.f/max_color;
		}
		mSunDiffuse.clamp();

        max_color = llmax(mMoonDiffuse.mV[0], mMoonDiffuse.mV[1], mMoonDiffuse.mV[2]);
        if (max_color > 1.f)
        {
            mMoonDiffuse *= 1.f/max_color;
        }
        mMoonDiffuse.clamp();

        // prevent underlighting from having neither lightsource facing us
        if (!sun_up && !moon_up)
		{
            mSunDiffuse.setVec(LLColor4(0.0, 0.0, 0.0, 1.0));
            mMoonDiffuse.setVec(LLColor4(0.0, 0.0, 0.0, 1.0));
            mSunDir.setVec(LLVector4(0.0, 1.0, 0.0, 0.0));
            mMoonDir.setVec(LLVector4(0.0, 1.0, 0.0, 0.0));
		}

        LLVector4 light_dir = sun_up ? mSunDir : mMoonDir;

        mHWLightColors[0] = sun_up ? mSunDiffuse : mMoonDiffuse;

		LLLightState* light = gGL.getLight(0);
        light->setPosition(light_dir);

        light->setSunPrimary(sun_up);
        light->setDiffuse(mHWLightColors[0]);
        light->setDiffuseB(mMoonDiffuse);
        light->setAmbient(psky->getTotalAmbient());
		light->setSpecular(LLColor4::black);
		light->setConstantAttenuation(1.f);
		light->setLinearAttenuation(0.f);
		light->setQuadraticAttenuation(0.f);
		light->setSpotExponent(0.f);
		light->setSpotCutoff(180.f);
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

            if (light->isAttachment())
            {
                if (!sRenderAttachedLights)
                {
                    continue;
                }
            }

			if (drawable->isState(LLDrawable::ACTIVE))
			{
				mLightMovingMask |= (1<<cur_light);
			}
			
            //send linear light color to shader
			LLColor4  light_color = light->getLightLinearColor();
			light_color.mV[3] = 0.0f;

			F32 fade = iter->fade;
			if (fade < LIGHT_FADE_TIME)
			{
				// fade in/out light
				if (fade >= 0.f)
				{
					fade = fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade += gFrameIntervalSeconds.value();
				}
				else
				{
					fade = 1.f + fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade -= gFrameIntervalSeconds.value();
				}
				fade = llclamp(fade,0.f,1.f);
				light_color *= fade;
			}

            if (light_color.magVecSquared() < 0.001f)
            {
                continue;
            }

            LLVector3 light_pos(light->getRenderPosition());
            LLVector4 light_pos_gl(light_pos, 1.0f);

            F32 adjusted_radius = light->getLightRadius() * (sRenderDeferred ? 1.5f : 1.0f);
            if (adjusted_radius <= 0.001f)
            {
                continue;
            }

            F32 x = (3.f * (1.f + (light->getLightFalloff() * 2.0f)));  // why this magic?  probably trying to match a historic behavior.
            F32 linatten = x / adjusted_radius;                         // % of brightness at radius

            mHWLightColors[cur_light] = light_color;
			LLLightState* light_state = gGL.getLight(cur_light);
			
			light_state->setPosition(light_pos_gl);
			light_state->setDiffuse(light_color);
			light_state->setAmbient(LLColor4::black);
			light_state->setConstantAttenuation(0.f);
			if (sRenderDeferred)
			{
				light_state->setLinearAttenuation(linatten);
				light_state->setQuadraticAttenuation(light->getLightFalloff(DEFERRED_LIGHT_FALLOFF) + 1.f); // get falloff to match for forward deferred rendering lights
			}
			else
			{
				light_state->setLinearAttenuation(linatten);
				light_state->setQuadraticAttenuation(0.f);
			}
			

			if (light->isLightSpotlight() // directional (spot-)light
			    && (LLPipeline::sRenderDeferred || RenderSpotLightsInNondeferred)) // these are only rendered as GL spotlights if we're in deferred rendering mode *or* the setting forces them on
			{
				LLQuaternion quat = light->getRenderRotation();
				LLVector3 at_axis(0,0,-1); // this matches deferred rendering's object light direction
				at_axis *= quat;

				light_state->setSpotDirection(at_axis);
				light_state->setSpotCutoff(90.f);
				light_state->setSpotExponent(2.f);
	
				LLVector3 spotParams = light->getSpotLightParams();

				const LLColor4 specular(0.f, 0.f, 0.f, spotParams[2]);
				light_state->setSpecular(specular);
			}
			else // omnidirectional (point) light
			{
				light_state->setSpotExponent(0.f);
				light_state->setSpotCutoff(180.f);
				
				// we use specular.z = 1.0 as a cheap hack for the shaders to know that this is omnidirectional rather than a spotlight
				const LLColor4 specular(0.f, 0.f, 1.f, 0.f);
				light_state->setSpecular(specular);				
			}
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
		LLLightState* light = gGL.getLight(cur_light);
        light->setSunPrimary(true);
		light->setDiffuse(LLColor4::black);
		light->setAmbient(LLColor4::black);
		light->setSpecular(LLColor4::black);
	}

    // Bookmark comment to allow searching for mSpecialRenderMode == 3 (avatar edit mode),
    // prev site of forward (non-deferred) character light injection, removed by SL-13522 09/20

	// Init GL state
	for (S32 i = 0; i < 8; ++i)
	{
		gGL.getLight(i)->disable();
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
		if (mask)
		{
			stop_glerror();
			for (S32 i=0; i<8; i++)
			{
				LLLightState* light = gGL.getLight(i);
				if (mask & (1<<i))
				{
					light->enable();
					light->setDiffuse(mHWLightColors[i]);
				}
				else
				{
					light->disable();
					light->setDiffuse(LLColor4::black);
				}
			}
			stop_glerror();
		}
		mLightMask = mask;
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
	
	if (isAgentAvatarValid() && getLightingDetail() <= 0)
	{
		if (gAgentAvatarp->mSpecialRenderMode == 0) // normal
		{
			gPipeline.enableLightsAvatar();
		}
		else if (gAgentAvatarp->mSpecialRenderMode == 2)  // anim preview
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

void LLPipeline::enableLightsPreview()
{
	disableLights();

	LLColor4 ambient = PreviewAmbientColor;
	gGL.setAmbientLightColor(ambient);

	LLColor4 diffuse0 = PreviewDiffuse0;
	LLColor4 specular0 = PreviewSpecular0;
	LLColor4 diffuse1 = PreviewDiffuse1;
	LLColor4 specular1 = PreviewSpecular1;
	LLColor4 diffuse2 = PreviewDiffuse2;
	LLColor4 specular2 = PreviewSpecular2;

	LLVector3 dir0 = PreviewDirection0;
	LLVector3 dir1 = PreviewDirection1;
	LLVector3 dir2 = PreviewDirection2;

	dir0.normVec();
	dir1.normVec();
	dir2.normVec();
	
	LLVector4 light_pos(dir0, 0.0f);

	LLLightState* light = gGL.getLight(1);

	light->enable();
	light->setPosition(light_pos);
	light->setDiffuse(diffuse0);
	light->setAmbient(ambient);
	light->setSpecular(specular0);
	light->setSpotExponent(0.f);
	light->setSpotCutoff(180.f);

	light_pos = LLVector4(dir1, 0.f);

	light = gGL.getLight(2);
	light->enable();
	light->setPosition(light_pos);
	light->setDiffuse(diffuse1);
	light->setAmbient(ambient);
	light->setSpecular(specular1);
	light->setSpotExponent(0.f);
	light->setSpotCutoff(180.f);

	light_pos = LLVector4(dir2, 0.f);
	light = gGL.getLight(3);
	light->enable();
	light->setPosition(light_pos);
	light->setDiffuse(diffuse2);
	light->setAmbient(ambient);
	light->setSpecular(specular2);
	light->setSpotExponent(0.f);
	light->setSpotCutoff(180.f);
}


void LLPipeline::enableLightsAvatarEdit(const LLColor4& color)
{
	U32 mask = 0x2002; // Avatar backlight only, set ambient
	setupAvatarLights(TRUE);
	enableLights(mask);

	gGL.setAmbientLightColor(color);
}

void LLPipeline::enableLightsFullbright()
{
	assertInitialized();
	U32 mask = 0x1000; // Non-0 mask, set ambient
	enableLights(mask);
}

void LLPipeline::disableLights()
{
	enableLights(0); // no lighting (full bright)
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
		LL_INFOS() << "In mLights" << LL_ENDL;
	}
	if (std::find(mMovedList.begin(), mMovedList.end(), drawablep) != mMovedList.end())
	{
		LL_INFOS() << "In mMovedList" << LL_ENDL;
	}
	if (std::find(mShiftList.begin(), mShiftList.end(), drawablep) != mShiftList.end())
	{
		LL_INFOS() << "In mShiftList" << LL_ENDL;
	}
	if (mRetexturedList.find(drawablep) != mRetexturedList.end())
	{
		LL_INFOS() << "In mRetexturedList" << LL_ENDL;
	}
	
	if (std::find(mBuildQ1.begin(), mBuildQ1.end(), drawablep) != mBuildQ1.end())
	{
		LL_INFOS() << "In mBuildQ1" << LL_ENDL;
	}
	if (std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep) != mBuildQ2.end())
	{
		LL_INFOS() << "In mBuildQ2" << LL_ENDL;
	}

	S32 count;
	
	count = gObjectList.findReferences(drawablep);
	if (count)
	{
		LL_INFOS() << "In other drawables: " << count << " references" << LL_ENDL;
	}
}

bool LLPipeline::verify()
{
	bool ok = assertInitialized();
	if (ok) 
	{
		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			if (!poolp->verify())
			{
				ok = false;
			}
		}
	}

	if (!ok)
	{
		LL_WARNS() << "Pipeline verify failed!" << LL_ENDL;
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
	bool Inside = true;
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
			Inside		= false;

			// Calculate T distances to candidate planes
			if(IR(dir.mV[i]))	MaxT.mV[i] = (MinB.mV[i] - origin.mV[i]) / dir.mV[i];
		}
		else if(origin.mV[i] > MaxB.mV[i])
		{
			coord.mV[i]	= MaxB.mV[i];
			Inside		= false;

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

void LLPipeline::setLight(LLDrawable *drawablep, bool is_light)
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

//static
void LLPipeline::toggleRenderType(U32 type)
{
	gPipeline.mRenderTypeEnabled[type] = !gPipeline.mRenderTypeEnabled[type];
	if (type == LLPipeline::RENDER_TYPE_WATER)
	{
		gPipeline.mRenderTypeEnabled[LLPipeline::RENDER_TYPE_VOIDWATER] = !gPipeline.mRenderTypeEnabled[LLPipeline::RENDER_TYPE_VOIDWATER];
	}
}

//static
void LLPipeline::toggleRenderTypeControl(U32 type)
{
	U32 bit = (1<<type);
	if (gPipeline.hasRenderType(type))
	{
		LL_INFOS() << "Toggling render type mask " << std::hex << bit << " off" << std::dec << LL_ENDL;
	}
	else
	{
		LL_INFOS() << "Toggling render type mask " << std::hex << bit << " on" << std::dec << LL_ENDL;
	}
	gPipeline.toggleRenderType(type);
}

//static
bool LLPipeline::hasRenderTypeControl(U32 type)
{
	return gPipeline.hasRenderType(type);
}

// Allows UI items labeled "Hide foo" instead of "Show foo"
//static
bool LLPipeline::toggleRenderTypeControlNegated(S32 type)
{
	return !gPipeline.hasRenderType(type);
}

//static
void LLPipeline::toggleRenderDebug(U64 bit)
{
	if (gPipeline.hasRenderDebugMask(bit))
	{
		LL_INFOS() << "Toggling render debug mask " << std::hex << bit << " off" << std::dec << LL_ENDL;
	}
	else
	{
		LL_INFOS() << "Toggling render debug mask " << std::hex << bit << " on" << std::dec << LL_ENDL;
	}
	gPipeline.mRenderDebugMask ^= bit;
}


//static
bool LLPipeline::toggleRenderDebugControl(U64 bit)
{
	return gPipeline.hasRenderDebugMask(bit);
}

//static
void LLPipeline::toggleRenderDebugFeature(U32 bit)
{
	gPipeline.mRenderDebugFeatureMask ^= bit;
}


//static
bool LLPipeline::toggleRenderDebugFeatureControl(U32 bit)
{
	return gPipeline.hasRenderDebugFeatureMask(bit);
}

void LLPipeline::setRenderDebugFeatureControl(U32 bit, bool value)
{
	if (value)
	{
		gPipeline.mRenderDebugFeatureMask |= bit;
	}
	else
	{
		gPipeline.mRenderDebugFeatureMask &= !bit;
	}
}

void LLPipeline::pushRenderDebugFeatureMask()
{
	mRenderDebugFeatureStack.push(mRenderDebugFeatureMask);
}

void LLPipeline::popRenderDebugFeatureMask()
{
	if (mRenderDebugFeatureStack.empty())
	{
		LL_ERRS() << "Depleted render feature stack." << LL_ENDL;
	}

	mRenderDebugFeatureMask = mRenderDebugFeatureStack.top();
	mRenderDebugFeatureStack.pop();
}

// static
void LLPipeline::setRenderScriptedBeacons(bool val)
{
	sRenderScriptedBeacons = val;
}

// static
void LLPipeline::toggleRenderScriptedBeacons()
{
	sRenderScriptedBeacons = !sRenderScriptedBeacons;
}

// static
bool LLPipeline::getRenderScriptedBeacons()
{
	return sRenderScriptedBeacons;
}

// static
void LLPipeline::setRenderScriptedTouchBeacons(bool val)
{
	sRenderScriptedTouchBeacons = val;
}

// static
void LLPipeline::toggleRenderScriptedTouchBeacons()
{
	sRenderScriptedTouchBeacons = !sRenderScriptedTouchBeacons;
}

// static
bool LLPipeline::getRenderScriptedTouchBeacons()
{
	return sRenderScriptedTouchBeacons;
}

// static
void LLPipeline::setRenderMOAPBeacons(bool val)
{
	sRenderMOAPBeacons = val;
}

// static
void LLPipeline::toggleRenderMOAPBeacons()
{
	sRenderMOAPBeacons = !sRenderMOAPBeacons;
}

// static
bool LLPipeline::getRenderMOAPBeacons()
{
	return sRenderMOAPBeacons;
}

// static
void LLPipeline::setRenderPhysicalBeacons(bool val)
{
	sRenderPhysicalBeacons = val;
}

// static
void LLPipeline::toggleRenderPhysicalBeacons()
{
	sRenderPhysicalBeacons = !sRenderPhysicalBeacons;
}

// static
bool LLPipeline::getRenderPhysicalBeacons()
{
	return sRenderPhysicalBeacons;
}

// static
void LLPipeline::setRenderParticleBeacons(bool val)
{
	sRenderParticleBeacons = val;
}

// static
void LLPipeline::toggleRenderParticleBeacons()
{
	sRenderParticleBeacons = !sRenderParticleBeacons;
}

// static
bool LLPipeline::getRenderParticleBeacons()
{
	return sRenderParticleBeacons;
}

// static
void LLPipeline::setRenderSoundBeacons(bool val)
{
	sRenderSoundBeacons = val;
}

// static
void LLPipeline::toggleRenderSoundBeacons()
{
	sRenderSoundBeacons = !sRenderSoundBeacons;
}

// static
bool LLPipeline::getRenderSoundBeacons()
{
	return sRenderSoundBeacons;
}

// static
void LLPipeline::setRenderBeacons(bool val)
{
	sRenderBeacons = val;
}

// static
void LLPipeline::toggleRenderBeacons()
{
	sRenderBeacons = !sRenderBeacons;
}

// static
bool LLPipeline::getRenderBeacons()
{
	return sRenderBeacons;
}

// static
void LLPipeline::setRenderHighlights(bool val)
{
	sRenderHighlight = val;
}

// static
void LLPipeline::toggleRenderHighlights()
{
	sRenderHighlight = !sRenderHighlight;
}

// static
bool LLPipeline::getRenderHighlights()
{
	return sRenderHighlight;
}

// static
void LLPipeline::setRenderHighlightTextureChannel(LLRender::eTexIndex channel)
{
	sRenderHighlightTextureChannel = channel;
}

LLVOPartGroup* LLPipeline::lineSegmentIntersectParticle(const LLVector4a& start, const LLVector4a& end, LLVector4a* intersection,
														S32* face_hit)
{
	LLVector4a local_end = end;

	LLVector4a position;

	LLDrawable* drawable = NULL;

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_PARTICLE);
		if (part && hasRenderType(part->mDrawableType))
		{
			LLDrawable* hit = part->lineSegmentIntersect(start, local_end, TRUE, FALSE, face_hit, &position, NULL, NULL, NULL);
			if (hit)
			{
				drawable = hit;
				local_end = position;						
			}
		}
	}

	LLVOPartGroup* ret = NULL;
	if (drawable)
	{
		//make sure we're returning an LLVOPartGroup
		llassert(drawable->getVObj()->getPCode() == LLViewerObject::LL_VO_PART_GROUP);
		ret = (LLVOPartGroup*) drawable->getVObj().get();
	}
		
	if (intersection)
	{
		*intersection = position;
	}

	return ret;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInWorld(const LLVector4a& start, const LLVector4a& end,
														bool pick_transparent,
														bool pick_rigged,
														S32* face_hit,
														LLVector4a* intersection,         // return the intersection point
														LLVector2* tex_coord,            // return the texture coordinates of the intersection point
														LLVector4a* normal,               // return the surface normal at the intersection point
														LLVector4a* tangent             // return the surface tangent at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	LLVector4a local_end = end;

	LLVector4a position;

	sPickAvatar = false; //! LLToolMgr::getInstance()->inBuildMode();
	
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
		{
			if ((j == LLViewerRegion::PARTITION_VOLUME) || 
				(j == LLViewerRegion::PARTITION_BRIDGE) ||
                (j == LLViewerRegion::PARTITION_AVATAR) || // for attachments
				(j == LLViewerRegion::PARTITION_CONTROL_AV) ||
				(j == LLViewerRegion::PARTITION_TERRAIN) ||
				(j == LLViewerRegion::PARTITION_TREE) ||
				(j == LLViewerRegion::PARTITION_GRASS))  // only check these partitions for now
			{
				LLSpatialPartition* part = region->getSpatialPartition(j);
				if (part && hasRenderType(part->mDrawableType))
				{
					LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, pick_rigged, face_hit, &position, tex_coord, normal, tangent);
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
		LLVector4a local_normal;
		LLVector4a local_tangent;
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
		if (tangent)
		{
			local_tangent = *tangent;
		}
		else
		{
			local_tangent.clear();
		}
		if (normal)
		{
			local_normal = *normal;
		}
		else
		{
			local_normal.clear();
		}
				
		const F32 ATTACHMENT_OVERRIDE_DIST = 0.1f;

		//check against avatars
		sPickAvatar = true;
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
				iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* region = *iter;

			LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_AVATAR);
			if (part && hasRenderType(part->mDrawableType))
			{
				LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, pick_rigged, face_hit, &position, tex_coord, normal, tangent);
				if (hit)
				{
					LLVector4a delta;
					delta.setSub(position, local_end);

					if (!drawable || 
						!drawable->getVObj()->isAttachment() ||
						delta.getLength3().getF32() > ATTACHMENT_OVERRIDE_DIST)
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
						if (tangent)
						{
							*tangent = local_tangent;
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
		if (av->mNameText.notNull()
			&& av->mNameText->lineSegmentIntersect(start, local_end, position))
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

LLViewerObject* LLPipeline::lineSegmentIntersectInHUD(const LLVector4a& start, const LLVector4a& end,
													  bool pick_transparent,													
													  S32* face_hit,
													  LLVector4a* intersection,         // return the intersection point
													  LLVector2* tex_coord,            // return the texture coordinates of the intersection point
													  LLVector4a* normal,               // return the surface normal at the intersection point
													  LLVector4a* tangent				// return the surface tangent at the intersection point
	)
{
	LLDrawable* drawable = NULL;

	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* region = *iter;

		bool toggle = false;
		if (!hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{
			toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
			toggle = true;
		}

		LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_HUD);
		if (part)
		{
			LLDrawable* hit = part->lineSegmentIntersect(start, end, pick_transparent, FALSE, face_hit, intersection, tex_coord, normal, tangent);
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
	if (!drawable)
	{
		return;
	}

	for (S32 i = 0; i < drawable->getNumFaces(); i++)
	{
		LLFace* facep = drawable->getFace(i);
		if (facep)
		{
			facep->clearVertexBuffer();
		}
	}
}

void LLPipeline::resetVertexBuffers()
{	
	mResetVertexBuffers = true;
}

void LLPipeline::doResetVertexBuffers(bool forced)
{
	if (!mResetVertexBuffers)
	{
		return;
	}
	if(!forced && LLSpatialPartition::sTeleportRequested)
	{
		if(gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
		{
			return; //wait for teleporting to finish
		}
		else
		{
			//teleporting aborted
			LLSpatialPartition::sTeleportRequested = FALSE;
			mResetVertexBuffers = false;
			return;
		}
	}

    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	mResetVertexBuffers = false;

	mCubeVB = NULL;
    mDeferredVB = NULL;

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
	if(LLSpatialPartition::sTeleportRequested)
	{
		LLSpatialPartition::sTeleportRequested = FALSE;

		LLWorld::getInstance()->clearAllVisibleObjects();
		clearRebuildDrawables();
	}

	resetDrawOrders();

	gSky.resetVertexBuffers();

	LLVOPartGroup::destroyGL();

	if ( LLPathingLib::getInstance() )
	{
		LLPathingLib::getInstance()->cleanupVBOManager();
	}
	LLVOPartGroup::destroyGL();
    gGL.resetVertexBuffer();

	SUBSYSTEM_CLEANUP(LLVertexBuffer);
	
	if (LLVertexBuffer::sGLCount != 0)
	{
		LL_WARNS() << "VBO wipe failed -- " << LLVertexBuffer::sGLCount << " buffers remaining." << LL_ENDL;
	}

	LLVertexBuffer::unbind();	
	
	updateRenderBump();
	updateRenderDeferred();

	sUseTriStrips = gSavedSettings.getBOOL("RenderUseTriStrips");
	LLVertexBuffer::sUseStreamDraw = gSavedSettings.getBOOL("RenderUseStreamVBO");
	LLVertexBuffer::sUseVAO = gSavedSettings.getBOOL("RenderUseVAO");
	LLVertexBuffer::sPreferStreamDraw = gSavedSettings.getBOOL("RenderPreferStreamDraw");
	LLVertexBuffer::sEnableVBOs = gSavedSettings.getBOOL("RenderVBOEnable");
	LLVertexBuffer::sDisableVBOMapping = LLVertexBuffer::sEnableVBOs && gSavedSettings.getBOOL("RenderVBOMappingDisable") ;
	sBakeSunlight = gSavedSettings.getBOOL("RenderBakeSunlight");
	sNoAlpha = gSavedSettings.getBOOL("RenderNoAlpha");
	LLPipeline::sTextureBindTest = gSavedSettings.getBOOL("RenderDebugTextureBind");

	LLVertexBuffer::initClass(LLVertexBuffer::sEnableVBOs, LLVertexBuffer::sDisableVBOMapping);
    gGL.initVertexBuffer();

    mDeferredVB = new LLVertexBuffer(DEFERRED_VB_MASK, 0);
    mDeferredVB->allocateBuffer(8, 0, true);

	LLVOPartGroup::restoreGL();
}

void LLPipeline::renderObjects(U32 type, U32 mask, bool texture, bool batch_texture, bool rigged)
{
	assertInitialized();
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;
    if (rigged)
    {
        mSimplePool->pushRiggedBatches(type + 1, mask, texture, batch_texture);
    }
    else
    {
        mSimplePool->pushBatches(type, mask, texture, batch_texture);
    }
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;		
}

void LLPipeline::renderAlphaObjects(U32 mask, bool texture, bool batch_texture, bool rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    U32 type = LLRenderPass::PASS_ALPHA;
    LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    for (LLCullResult::drawinfo_iterator i = gPipeline.beginRenderMap(type); i != gPipeline.endRenderMap(type); ++i)
    {
        LLDrawInfo* pparams = *i;
        if (pparams)
        {
            if (rigged)
            {
                if (pparams->mAvatar != nullptr)
                {
                    if (lastAvatar != pparams->mAvatar || lastMeshId != pparams->mSkinInfo->mHash)
                    {
                        mSimplePool->uploadMatrixPalette(*pparams);
                        lastAvatar = pparams->mAvatar;
                        lastMeshId = pparams->mSkinInfo->mHash;
                    }

                    mSimplePool->pushBatch(*pparams, mask | LLVertexBuffer::MAP_WEIGHT4, texture, batch_texture);
                }
            }
            else if (pparams->mAvatar == nullptr)
            {
                mSimplePool->pushBatch(*pparams, mask, texture, batch_texture);
            }
        }
    }
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void LLPipeline::renderMaskedObjects(U32 type, U32 mask, bool texture, bool batch_texture, bool rigged)
{
	assertInitialized();
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;
    if (rigged)
    {
        mAlphaMaskPool->pushRiggedMaskBatches(type+1, mask, texture, batch_texture);
    }
    else
    {
        mAlphaMaskPool->pushMaskBatches(type, mask, texture, batch_texture);
    }
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;		
}

void LLPipeline::renderFullbrightMaskedObjects(U32 type, U32 mask, bool texture, bool batch_texture, bool rigged)
{
	assertInitialized();
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;
    if (rigged)
    {
        mFullbrightAlphaMaskPool->pushRiggedMaskBatches(type+1, mask, texture, batch_texture);
    }
    else
    {
        mFullbrightAlphaMaskPool->pushMaskBatches(type, mask, texture, batch_texture);
    }
	gGL.loadMatrix(gGLModelView);
	gGLLastMatrix = NULL;		
}

void apply_cube_face_rotation(U32 face)
{
	switch (face)
	{
		case 0: 
			gGL.rotatef(90.f, 0, 1, 0);
			gGL.rotatef(180.f, 1, 0, 0);
		break;
		case 2: 
			gGL.rotatef(-90.f, 1, 0, 0);
		break;
		case 4:
			gGL.rotatef(180.f, 0, 1, 0);
			gGL.rotatef(180.f, 0, 0, 1);
		break;
		case 1: 
			gGL.rotatef(-90.f, 0, 1, 0);
			gGL.rotatef(180.f, 1, 0, 0);
		break;
		case 3:
			gGL.rotatef(90, 1, 0, 0);
		break;
		case 5: 
			gGL.rotatef(180, 0, 0, 1);
		break;
	}
}

void validate_framebuffer_object()
{                                                           
	GLenum status;                                            
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT); 
	switch(status) 
	{                                          
		case GL_FRAMEBUFFER_COMPLETE:                       
			//framebuffer OK, no error.
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			// frame buffer not OK: probably means unsupported depth buffer format
			LL_ERRS() << "Framebuffer Incomplete Missing Attachment." << LL_ENDL;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			// frame buffer not OK: probably means unsupported depth buffer format
			LL_ERRS() << "Framebuffer Incomplete Attachment." << LL_ENDL;
			break; 
		case GL_FRAMEBUFFER_UNSUPPORTED:                    
			/* choose different formats */                        
			LL_ERRS() << "Framebuffer unsupported." << LL_ENDL;
			break;                                                
		default:                                                
			LL_ERRS() << "Unknown framebuffer status." << LL_ENDL;
			break;
	}
}

void LLPipeline::bindScreenToTexture() 
{
	
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_BLOOM("Bloom");

void LLPipeline::renderFinalize()
{
    LLVertexBuffer::unbind();
    LLGLState::checkStates();
    LLGLState::checkTextureChannels();

    assertInitialized();

    if (gUseWireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    LLVector2 tc1(0, 0);
    LLVector2 tc2((F32) mScreen.getWidth() * 2, (F32) mScreen.getHeight() * 2);

    LL_RECORD_BLOCK_TIME(FTM_RENDER_BLOOM);
    gGL.color4f(1, 1, 1, 1);
    LLGLDepthTest depth(GL_FALSE);
    LLGLDisable blend(GL_BLEND);
    LLGLDisable cull(GL_CULL_FACE);

    enableLightsFullbright();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();

    LLGLDisable test(GL_ALPHA_TEST);

    gGL.setColorMask(true, true);
    glClearColor(0, 0, 0, 0);

    if (sRenderGlow)
    {
        mGlow[2].bindTarget();
        mGlow[2].clear();

        gGlowExtractProgram.bind();
        F32 minLum = llmax((F32) RenderGlowMinLuminance, 0.0f);
        F32 maxAlpha = RenderGlowMaxExtractAlpha;
        F32 warmthAmount = RenderGlowWarmthAmount;
        LLVector3 lumWeights = RenderGlowLumWeights;
        LLVector3 warmthWeights = RenderGlowWarmthWeights;

        gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, minLum);
        gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MAX_EXTRACT_ALPHA, maxAlpha);
        gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_LUM_WEIGHTS, lumWeights.mV[0], lumWeights.mV[1],
                                      lumWeights.mV[2]);
        gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_WARMTH_WEIGHTS, warmthWeights.mV[0], warmthWeights.mV[1],
                                      warmthWeights.mV[2]);
        gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_WARMTH_AMOUNT, warmthAmount);
        
        {
            LLGLEnable blend_on(GL_BLEND);
            LLGLEnable test(GL_ALPHA_TEST);

            gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);

            mScreen.bindTexture(0, 0, LLTexUnit::TFO_POINT);

            gGL.color4f(1, 1, 1, 1);
            gPipeline.enableLightsFullbright();
            gGL.begin(LLRender::TRIANGLE_STRIP);
            gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
            gGL.vertex2f(-1, -1);

            gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
            gGL.vertex2f(-1, 3);

            gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
            gGL.vertex2f(3, -1);

            gGL.end();

            gGL.getTexUnit(0)->unbind(mScreen.getUsage());

            mGlow[2].flush();

            tc1.setVec(0, 0);
            tc2.setVec(2, 2); 
        }

        // power of two between 1 and 1024
        U32 glowResPow = RenderGlowResolutionPow;
        const U32 glow_res = llmax(1, llmin(1024, 1 << glowResPow));

        S32 kernel = RenderGlowIterations * 2;
        F32 delta = RenderGlowWidth / glow_res;
        // Use half the glow width if we have the res set to less than 9 so that it looks
        // almost the same in either case.
        if (glowResPow < 9)
        {
            delta *= 0.5f;
        }
        F32 strength = RenderGlowStrength;

        gGlowProgram.bind();
        gGlowProgram.uniform1f(LLShaderMgr::GLOW_STRENGTH, strength);

        for (S32 i = 0; i < kernel; i++)
        {
            mGlow[i % 2].bindTarget();
            mGlow[i % 2].clear();

            if (i == 0)
            {
                gGL.getTexUnit(0)->bind(&mGlow[2]);
            }
            else
            {
                gGL.getTexUnit(0)->bind(&mGlow[(i - 1) % 2]);
            }

            if (i % 2 == 0)
            {
                gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, delta, 0);
            }
            else
            {
                gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, 0, delta);
            }

            gGL.begin(LLRender::TRIANGLE_STRIP);
            gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
            gGL.vertex2f(-1, -1);

            gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
            gGL.vertex2f(-1, 3);

            gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
            gGL.vertex2f(3, -1);

            gGL.end();

            mGlow[i % 2].flush();
        }

        gGlowProgram.unbind();
    }
    else // !sRenderGlow, skip the glow ping-pong and just clear the result target
    {
        mGlow[1].bindTarget();
        mGlow[1].clear();
        mGlow[1].flush();
    }

    gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
    gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
    gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
    gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();
    glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);

    tc2.setVec((F32) mScreen.getWidth(), (F32) mScreen.getHeight());

    gGL.flush();

    LLVertexBuffer::unbind();

    if (LLPipeline::sRenderDeferred)
    {

        bool dof_enabled = !LLViewerCamera::getInstance()->cameraUnderWater() &&
                           (RenderDepthOfFieldInEditMode || !LLToolMgr::getInstance()->inBuildMode()) &&
                           RenderDepthOfField;

        bool multisample = RenderFSAASamples > 1 && mFXAABuffer.isComplete();

        gViewerWindow->setup3DViewport();

        if (dof_enabled)
        {
            LLGLSLShader *shader = &gDeferredPostProgram;
            LLGLDisable blend(GL_BLEND);

            // depth of field focal plane calculations
            static F32 current_distance = 16.f;
            static F32 start_distance = 16.f;
            static F32 transition_time = 1.f;

            LLVector3 focus_point;

            LLViewerObject *obj = LLViewerMediaFocus::getInstance()->getFocusedObject();
            if (obj && obj->mDrawable && obj->isSelected())
            { // focus on selected media object
                S32 face_idx = LLViewerMediaFocus::getInstance()->getFocusedFace();
                if (obj && obj->mDrawable)
                {
                    LLFace *face = obj->mDrawable->getFace(face_idx);
                    if (face)
                    {
                        focus_point = face->getPositionAgent();
                    }
                }
            }

            if (focus_point.isExactlyZero())
            {
                if (LLViewerJoystick::getInstance()->getOverrideCamera())
                { // focus on point under cursor
                    focus_point.set(gDebugRaycastIntersection.getF32ptr());
                }
                else if (gAgentCamera.cameraMouselook())
                { // focus on point under mouselook crosshairs
                    LLVector4a result;
                    result.clear();

                    gViewerWindow->cursorIntersect(-1, -1, 512.f, NULL, -1, FALSE, FALSE, NULL, &result);

                    focus_point.set(result.getF32ptr());
                }
                else
                {
                    // focus on alt-zoom target
                    LLViewerRegion *region = gAgent.getRegion();
                    if (region)
                    {
                        focus_point = LLVector3(gAgentCamera.getFocusGlobal() - region->getOriginGlobal());
                    }
                }
            }

            LLVector3 eye = LLViewerCamera::getInstance()->getOrigin();
            F32 target_distance = 16.f;
            if (!focus_point.isExactlyZero())
            {
                target_distance = LLViewerCamera::getInstance()->getAtAxis() * (focus_point - eye);
            }

            if (transition_time >= 1.f && fabsf(current_distance - target_distance) / current_distance > 0.01f)
            { // large shift happened, interpolate smoothly to new target distance
                transition_time = 0.f;
                start_distance = current_distance;
            }
            else if (transition_time < 1.f)
            { // currently in a transition, continue interpolating
                transition_time += 1.f / CameraFocusTransitionTime * gFrameIntervalSeconds.value();
                transition_time = llmin(transition_time, 1.f);

                F32 t = cosf(transition_time * F_PI + F_PI) * 0.5f + 0.5f;
                current_distance = start_distance + (target_distance - start_distance) * t;
            }
            else
            { // small or no change, just snap to target distance
                current_distance = target_distance;
            }

            // convert to mm
            F32 subject_distance = current_distance * 1000.f;
            F32 fnumber = CameraFNumber;
            F32 default_focal_length = CameraFocalLength;

            F32 fov = LLViewerCamera::getInstance()->getView();

            const F32 default_fov = CameraFieldOfView * F_PI / 180.f;

            // F32 aspect_ratio = (F32) mScreen.getWidth()/(F32)mScreen.getHeight();

            F32 dv = 2.f * default_focal_length * tanf(default_fov / 2.f);

            F32 focal_length = dv / (2 * tanf(fov / 2.f));

            // F32 tan_pixel_angle = tanf(LLDrawable::sCurPixelAngle);

            // from wikipedia -- c = |s2-s1|/s2 * f^2/(N(S1-f))
            // where	 N = fnumber
            //			 s2 = dot distance
            //			 s1 = subject distance
            //			 f = focal length
            //

            F32 blur_constant = focal_length * focal_length / (fnumber * (subject_distance - focal_length));
            blur_constant /= 1000.f; // convert to meters for shader
            F32 magnification = focal_length / (subject_distance - focal_length);

            { // build diffuse+bloom+CoF
                mDeferredLight.bindTarget();
                shader = &gDeferredCoFProgram;

                bindDeferredShader(*shader);

                S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mScreen.getUsage());
                if (channel > -1)
                {
                    mScreen.bindTexture(0, channel);
                }

                shader->uniform1f(LLShaderMgr::DOF_FOCAL_DISTANCE, -subject_distance / 1000.f);
                shader->uniform1f(LLShaderMgr::DOF_BLUR_CONSTANT, blur_constant);
                shader->uniform1f(LLShaderMgr::DOF_TAN_PIXEL_ANGLE, tanf(1.f / LLDrawable::sCurPixelAngle));
                shader->uniform1f(LLShaderMgr::DOF_MAGNIFICATION, magnification);
                shader->uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                shader->uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale);

                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
                gGL.vertex2f(-1, -1);

                gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
                gGL.vertex2f(-1, 3);

                gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
                gGL.vertex2f(3, -1);

                gGL.end();

                unbindDeferredShader(*shader);
                mDeferredLight.flush();
            }

            U32 dof_width = (U32)(mScreen.getWidth() * CameraDoFResScale);
            U32 dof_height = (U32)(mScreen.getHeight() * CameraDoFResScale);

            { // perform DoF sampling at half-res (preserve alpha channel)
                mScreen.bindTarget();
                glViewport(0, 0, dof_width, dof_height);
                gGL.setColorMask(true, false);

                shader = &gDeferredPostProgram;
                bindDeferredShader(*shader);
                S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mDeferredLight.getUsage());
                if (channel > -1)
                {
                    mDeferredLight.bindTexture(0, channel);
                }

                shader->uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                shader->uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale);

                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
                gGL.vertex2f(-1, -1);

                gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
                gGL.vertex2f(-1, 3);

                gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
                gGL.vertex2f(3, -1);

                gGL.end();

                unbindDeferredShader(*shader);
                mScreen.flush();
                gGL.setColorMask(true, true);
            }

            { // combine result based on alpha
                if (multisample)
                {
                    mDeferredLight.bindTarget();
                    glViewport(0, 0, mDeferredScreen.getWidth(), mDeferredScreen.getHeight());
                }
                else
                {
                    gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
                    gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
                    gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
                    gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();
                    glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
                }

                shader = &gDeferredDoFCombineProgram;
                bindDeferredShader(*shader);

                S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mScreen.getUsage());
                if (channel > -1)
                {
                    mScreen.bindTexture(0, channel);
                }

                shader->uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                shader->uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale);
                shader->uniform1f(LLShaderMgr::DOF_WIDTH, dof_width - 1);
                shader->uniform1f(LLShaderMgr::DOF_HEIGHT, dof_height - 1);

                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
                gGL.vertex2f(-1, -1);

                gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
                gGL.vertex2f(-1, 3);

                gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
                gGL.vertex2f(3, -1);

                gGL.end();

                unbindDeferredShader(*shader);

                if (multisample)
                {
                    mDeferredLight.flush();
                }
            }
        }
        else
        {
            if (multisample)
            {
                mDeferredLight.bindTarget();
            }
            LLGLSLShader *shader = &gDeferredPostNoDoFProgram;

            bindDeferredShader(*shader);

            S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mScreen.getUsage());
            if (channel > -1)
            {
                mScreen.bindTexture(0, channel);
            }

            gGL.begin(LLRender::TRIANGLE_STRIP);
            gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
            gGL.vertex2f(-1, -1);

            gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
            gGL.vertex2f(-1, 3);

            gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
            gGL.vertex2f(3, -1);

            gGL.end();

            unbindDeferredShader(*shader);

            if (multisample)
            {
                mDeferredLight.flush();
            }
        }

        if (multisample)
        {
            // bake out texture2D with RGBL for FXAA shader
            mFXAABuffer.bindTarget();

            S32 width = mScreen.getWidth();
            S32 height = mScreen.getHeight();
            glViewport(0, 0, width, height);

            LLGLSLShader *shader = &gGlowCombineFXAAProgram;

            shader->bind();
            shader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, width, height);

            S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mDeferredLight.getUsage());
            if (channel > -1)
            {
                mDeferredLight.bindTexture(0, channel);
            }

            gGL.begin(LLRender::TRIANGLE_STRIP);
            gGL.vertex2f(-1, -1);
            gGL.vertex2f(-1, 3);
            gGL.vertex2f(3, -1);
            gGL.end();

            gGL.flush();

            shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, mDeferredLight.getUsage());
            shader->unbind();

            mFXAABuffer.flush();

            shader = &gFXAAProgram;
            shader->bind();

            channel = shader->enableTexture(LLShaderMgr::DIFFUSE_MAP, mFXAABuffer.getUsage());
            if (channel > -1)
            {
                mFXAABuffer.bindTexture(0, channel, LLTexUnit::TFO_BILINEAR);
            }

            gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
            gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
            gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
            gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();
            glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);

            F32 scale_x = (F32) width / mFXAABuffer.getWidth();
            F32 scale_y = (F32) height / mFXAABuffer.getHeight();
            shader->uniform2f(LLShaderMgr::FXAA_TC_SCALE, scale_x, scale_y);
            shader->uniform2f(LLShaderMgr::FXAA_RCP_SCREEN_RES, 1.f / width * scale_x, 1.f / height * scale_y);
            shader->uniform4f(LLShaderMgr::FXAA_RCP_FRAME_OPT, -0.5f / width * scale_x, -0.5f / height * scale_y,
                              0.5f / width * scale_x, 0.5f / height * scale_y);
            shader->uniform4f(LLShaderMgr::FXAA_RCP_FRAME_OPT2, -2.f / width * scale_x, -2.f / height * scale_y,
                              2.f / width * scale_x, 2.f / height * scale_y);

            gGL.begin(LLRender::TRIANGLE_STRIP);
            gGL.vertex2f(-1, -1);
            gGL.vertex2f(-1, 3);
            gGL.vertex2f(3, -1);
            gGL.end();

            gGL.flush();
            shader->unbind();
        }
    }
    else // not deferred
    {
        U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;
        LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(mask, 0);
        buff->allocateBuffer(3, 0, TRUE);

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
        uv2[1] = LLVector2(0, tc2.mV[1] * 2.f);
        uv2[2] = LLVector2(tc2.mV[0] * 2.f, 0);

        v[0] = LLVector3(-1, -1, 0);
        v[1] = LLVector3(-1, 3, 0);
        v[2] = LLVector3(3, -1, 0);

        buff->flush();

        LLGLDisable blend(GL_BLEND);

        gGlowCombineProgram.bind();

        gGL.getTexUnit(0)->bind(&mGlow[1]);
        gGL.getTexUnit(1)->bind(&mScreen);

        LLGLEnable multisample(RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);

        buff->setBuffer(mask);
        buff->drawArrays(LLRender::TRIANGLE_STRIP, 0, 3);

        gGlowCombineProgram.unbind();
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PHYSICS_SHAPES))
    {
        gSplatTextureRectProgram.bind();

        gGL.setColorMask(true, false);

        LLVector2 tc1(0, 0);
        LLVector2 tc2((F32) gViewerWindow->getWorldViewWidthRaw() * 2,
                      (F32) gViewerWindow->getWorldViewHeightRaw() * 2);

        LLGLEnable blend(GL_BLEND);
        gGL.color4f(1, 1, 1, 0.75f);

        gGL.getTexUnit(0)->bind(&mPhysicsDisplay);

        gGL.begin(LLRender::TRIANGLES);
        gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
        gGL.vertex2f(-1, -1);

        gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
        gGL.vertex2f(-1, 3);

        gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
        gGL.vertex2f(3, -1);

        gGL.end();
        gGL.flush();

        gSplatTextureRectProgram.unbind();
    }

    if (LLRenderTarget::sUseFBO)
    { // copy depth buffer from mScreen to framebuffer
        LLRenderTarget::copyContentsToFramebuffer(mScreen, 0, 0, mScreen.getWidth(), mScreen.getHeight(), 0, 0,
                                                  mScreen.getWidth(), mScreen.getHeight(),
                                                  GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();

    LLVertexBuffer::unbind();

    LLGLState::checkStates();
    LLGLState::checkTextureChannels();
}

void LLPipeline::bindDeferredShader(LLGLSLShader& shader, LLRenderTarget* light_target)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    LLRenderTarget* deferred_target       = &mDeferredScreen;
    LLRenderTarget* deferred_depth_target = &mDeferredDepth;
    LLRenderTarget* deferred_light_target = &mDeferredLight;

	shader.bind();
	S32 channel = 0;
    channel = shader.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, deferred_target->getUsage());
	if (channel > -1)
	{
        deferred_target->bindTexture(0,channel, LLTexUnit::TFO_POINT);
	}

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_SPECULAR, deferred_target->getUsage());
	if (channel > -1)
	{
        deferred_target->bindTexture(1, channel, LLTexUnit::TFO_POINT);
	}

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_NORMAL, deferred_target->getUsage());
	if (channel > -1)
	{
        deferred_target->bindTexture(2, channel, LLTexUnit::TFO_POINT);
	}

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_depth_target->getUsage());
	if (channel > -1)
	{
        gGL.getTexUnit(channel)->bind(deferred_depth_target, TRUE);
		stop_glerror();
    }
		
    glh::matrix4f projection = get_current_projection();
		glh::matrix4f inv_proj = projection.inverse();
		
    if (shader.getUniformLocation(LLShaderMgr::INVERSE_PROJECTION_MATRIX) != -1)
    {
		shader.uniformMatrix4fv(LLShaderMgr::INVERSE_PROJECTION_MATRIX, 1, FALSE, inv_proj.m);
    }

    if (shader.getUniformLocation(LLShaderMgr::VIEWPORT) != -1)
    {
		shader.uniform4f(LLShaderMgr::VIEWPORT, (F32) gGLViewport[0],
									(F32) gGLViewport[1],
									(F32) gGLViewport[2],
									(F32) gGLViewport[3]);
	}

    if (sReflectionRender && !shader.getUniformLocation(LLShaderMgr::MODELVIEW_MATRIX))
    {
        shader.uniformMatrix4fv(LLShaderMgr::MODELVIEW_MATRIX, 1, FALSE, mReflectionModelView.m);  
    }

	channel = shader.enableTexture(LLShaderMgr::DEFERRED_NOISE);
	if (channel > -1)
	{
        gGL.getTexUnit(channel)->bindManual(LLTexUnit::TT_TEXTURE, mNoiseMap);
		gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
	}

	channel = shader.enableTexture(LLShaderMgr::DEFERRED_LIGHTFUNC);
	if (channel > -1)
	{
		gGL.getTexUnit(channel)->bindManual(LLTexUnit::TT_TEXTURE, mLightFunc);
	}

	stop_glerror();

    light_target = light_target ? light_target : deferred_light_target;
    channel = shader.enableTexture(LLShaderMgr::DEFERRED_LIGHT, light_target->getUsage());
	if (channel > -1)
	{
        light_target->bindTexture(0, channel, LLTexUnit::TFO_POINT);
	}

	channel = shader.enableTexture(LLShaderMgr::DEFERRED_BLOOM);
	if (channel > -1)
	{
		mGlow[1].bindTexture(0, channel);
	}

	stop_glerror();

	for (U32 i = 0; i < 4; i++)
	{
        LLRenderTarget* shadow_target = getShadowTarget(i);
        if (shadow_target)
        {
		channel = shader.enableTexture(LLShaderMgr::DEFERRED_SHADOW0+i, LLTexUnit::TT_TEXTURE);
		stop_glerror();
		if (channel > -1)
		{
			stop_glerror();
                gGL.getTexUnit(channel)->bind(getShadowTarget(i), TRUE);
                gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
			gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
			stop_glerror();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			stop_glerror();
		}
	}
    }

	for (U32 i = 4; i < 6; i++)
	{
		channel = shader.enableTexture(LLShaderMgr::DEFERRED_SHADOW0+i);
		stop_glerror();
		if (channel > -1)
		{
			stop_glerror();
			LLRenderTarget* shadow_target = getShadowTarget(i);
			if (shadow_target)
			{
				gGL.getTexUnit(channel)->bind(shadow_target, TRUE);
				gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
			gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
			stop_glerror();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			stop_glerror();
		}
	}
	}

	stop_glerror();

	F32 mat[16*6];
	for (U32 i = 0; i < 16; i++)
	{
		mat[i] = mSunShadowMatrix[0].m[i];
		mat[i+16] = mSunShadowMatrix[1].m[i];
		mat[i+32] = mSunShadowMatrix[2].m[i];
		mat[i+48] = mSunShadowMatrix[3].m[i];
		mat[i+64] = mSunShadowMatrix[4].m[i];
		mat[i+80] = mSunShadowMatrix[5].m[i];
	}

	shader.uniformMatrix4fv(LLShaderMgr::DEFERRED_SHADOW_MATRIX, 6, FALSE, mat);

	stop_glerror();

	channel = shader.enableTexture(LLShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
	if (channel > -1)
	{
		LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
		if (cube_map)
		{
			cube_map->enable(channel);
			cube_map->bind();
			F32* m = gGLModelView;
						
			F32 mat[] = { m[0], m[1], m[2],
						  m[4], m[5], m[6],
						  m[8], m[9], m[10] };
		
			shader.uniformMatrix3fv(LLShaderMgr::DEFERRED_ENV_MAT, 1, TRUE, mat);
		}
	}

    if (gAtmosphere)
    {
        // bind precomputed textures necessary for calculating sun and sky luminance
        channel = shader.enableTexture(LLShaderMgr::TRANSMITTANCE_TEX, LLTexUnit::TT_TEXTURE);
        if (channel > -1)
        {
            shader.bindTexture(LLShaderMgr::TRANSMITTANCE_TEX, gAtmosphere->getTransmittance());
        }

        channel = shader.enableTexture(LLShaderMgr::SCATTER_TEX, LLTexUnit::TT_TEXTURE_3D);
        if (channel > -1)
        {
            shader.bindTexture(LLShaderMgr::SCATTER_TEX, gAtmosphere->getScattering());
        }

        channel = shader.enableTexture(LLShaderMgr::SINGLE_MIE_SCATTER_TEX, LLTexUnit::TT_TEXTURE_3D);
        if (channel > -1)
        {
            shader.bindTexture(LLShaderMgr::SINGLE_MIE_SCATTER_TEX, gAtmosphere->getMieScattering());
        }

        channel = shader.enableTexture(LLShaderMgr::ILLUMINANCE_TEX, LLTexUnit::TT_TEXTURE);
        if (channel > -1)
        {
            shader.bindTexture(LLShaderMgr::ILLUMINANCE_TEX, gAtmosphere->getIlluminance());
        }
    }

	shader.uniform4fv(LLShaderMgr::DEFERRED_SHADOW_CLIP, 1, mSunClipPlanes.mV);
	shader.uniform1f(LLShaderMgr::DEFERRED_SUN_WASH, RenderDeferredSunWash);
	shader.uniform1f(LLShaderMgr::DEFERRED_SHADOW_NOISE, RenderShadowNoise);
	shader.uniform1f(LLShaderMgr::DEFERRED_BLUR_SIZE, RenderShadowBlurSize);

	shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_RADIUS, RenderSSAOScale);
	shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_MAX_RADIUS, RenderSSAOMaxScale);

	F32 ssao_factor = RenderSSAOFactor;
	shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_FACTOR, ssao_factor);
	shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_FACTOR_INV, 1.0/ssao_factor);

	LLVector3 ssao_effect = RenderSSAOEffect;
	F32 matrix_diag = (ssao_effect[0] + 2.0*ssao_effect[1])/3.0;
	F32 matrix_nondiag = (ssao_effect[0] - ssao_effect[1])/3.0;
	// This matrix scales (proj of color onto <1/rt(3),1/rt(3),1/rt(3)>) by
	// value factor, and scales remainder by saturation factor
	F32 ssao_effect_mat[] = {	matrix_diag, matrix_nondiag, matrix_nondiag,
								matrix_nondiag, matrix_diag, matrix_nondiag,
								matrix_nondiag, matrix_nondiag, matrix_diag};
	shader.uniformMatrix3fv(LLShaderMgr::DEFERRED_SSAO_EFFECT_MAT, 1, GL_FALSE, ssao_effect_mat);

	//F32 shadow_offset_error = 1.f + RenderShadowOffsetError * fabsf(LLViewerCamera::getInstance()->getOrigin().mV[2]);
	F32 shadow_bias_error = RenderShadowBiasError * fabsf(LLViewerCamera::getInstance()->getOrigin().mV[2])/3000.f;
    F32 shadow_bias       = RenderShadowBias + shadow_bias_error;

    shader.uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, deferred_target->getWidth(), deferred_target->getHeight());
	shader.uniform1f(LLShaderMgr::DEFERRED_NEAR_CLIP, LLViewerCamera::getInstance()->getNear()*2.f);
	shader.uniform1f (LLShaderMgr::DEFERRED_SHADOW_OFFSET, RenderShadowOffset); //*shadow_offset_error);
    shader.uniform1f(LLShaderMgr::DEFERRED_SHADOW_BIAS, shadow_bias);
	shader.uniform1f(LLShaderMgr::DEFERRED_SPOT_SHADOW_OFFSET, RenderSpotShadowOffset);
	shader.uniform1f(LLShaderMgr::DEFERRED_SPOT_SHADOW_BIAS, RenderSpotShadowBias);	

	shader.uniform3fv(LLShaderMgr::DEFERRED_SUN_DIR, 1, mTransformedSunDir.mV);
    shader.uniform3fv(LLShaderMgr::DEFERRED_MOON_DIR, 1, mTransformedMoonDir.mV);
	shader.uniform2f(LLShaderMgr::DEFERRED_SHADOW_RES, mShadow[0].getWidth(), mShadow[0].getHeight());
	shader.uniform2f(LLShaderMgr::DEFERRED_PROJ_SHADOW_RES, mShadow[4].getWidth(), mShadow[4].getHeight());
	shader.uniform1f(LLShaderMgr::DEFERRED_DEPTH_CUTOFF, RenderEdgeDepthCutoff);
	shader.uniform1f(LLShaderMgr::DEFERRED_NORM_CUTOFF, RenderEdgeNormCutoff);
	
	if (shader.getUniformLocation(LLShaderMgr::DEFERRED_NORM_MATRIX) >= 0)
	{
        glh::matrix4f norm_mat = get_current_modelview().inverse().transpose();
		shader.uniformMatrix4fv(LLShaderMgr::DEFERRED_NORM_MATRIX, 1, FALSE, norm_mat.m);
	}

    shader.uniform4fv(LLShaderMgr::SUNLIGHT_COLOR, 1, mSunDiffuse.mV);
    shader.uniform4fv(LLShaderMgr::MOONLIGHT_COLOR, 1, mMoonDiffuse.mV);

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t sky = environment.getCurrentSky();
}

LLColor3 pow3f(LLColor3 v, F32 f)
{
	v.mV[0] = powf(v.mV[0], f);
	v.mV[1] = powf(v.mV[1], f);
	v.mV[2] = powf(v.mV[2], f);
	return v;
}

LLVector4 pow4fsrgb(LLVector4 v, F32 f)
{
	v.mV[0] = powf(v.mV[0], f);
	v.mV[1] = powf(v.mV[1], f);
	v.mV[2] = powf(v.mV[2], f);
	return v;
}

void LLPipeline::renderDeferredLighting(LLRenderTarget *screen_target)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (!sCull)
    {
        return;
    }

    LLRenderTarget *deferred_target       = &mDeferredScreen;
    LLRenderTarget *deferred_depth_target = &mDeferredDepth;
    LLRenderTarget *deferred_light_target = &mDeferredLight;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("deferred"); //LL_RECORD_BLOCK_TIME(FTM_RENDER_DEFERRED);
        LLViewerCamera *camera = LLViewerCamera::getInstance();
        {
            LLGLDepthTest depth(GL_TRUE);
            deferred_depth_target->copyContents(*deferred_target,
                                                0,
                                                0,
                                                deferred_target->getWidth(),
                                                deferred_target->getHeight(),
                                                0,
                                                0,
                                                deferred_depth_target->getWidth(),
                                                deferred_depth_target->getHeight(),
                                                GL_DEPTH_BUFFER_BIT,
                                                GL_NEAREST);
        }

        LLGLEnable multisample(RenderFSAASamples > 0 ? GL_MULTISAMPLE_ARB : 0);

        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
        {
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        }

        // ati doesn't seem to love actually using the stencil buffer on FBO's
        LLGLDisable stencil(GL_STENCIL_TEST);
        // glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
        // glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        gGL.setColorMask(true, true);

        // draw a cube around every light
        LLVertexBuffer::unbind();

        LLGLEnable cull(GL_CULL_FACE);
        LLGLEnable blend(GL_BLEND);

        glh::matrix4f mat = copy_matrix(gGLModelView);

        LLStrider<LLVector3> vert;
        mDeferredVB->getVertexStrider(vert);

        vert[0].set(-1, 1, 0);
        vert[1].set(-1, -3, 0);
        vert[2].set(3, 1, 0);

        setupHWLights(NULL);  // to set mSun/MoonDir;

        glh::vec4f tc(mSunDir.mV);
        mat.mult_matrix_vec(tc);
        mTransformedSunDir.set(tc.v);

        glh::vec4f tc_moon(mMoonDir.mV);
        mat.mult_matrix_vec(tc_moon);
        mTransformedMoonDir.set(tc_moon.v);

        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();

        if (RenderDeferredSSAO || RenderShadowDetail > 0)
        {
            deferred_light_target->bindTarget();
            {  // paint shadow/SSAO light map (direct lighting lightmap)
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - sun shadow");
                bindDeferredShader(gDeferredSunProgram, deferred_light_target);
                mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
                glClearColor(1, 1, 1, 1);
                deferred_light_target->clear(GL_COLOR_BUFFER_BIT);
                glClearColor(0, 0, 0, 0);

                glh::matrix4f inv_trans = get_current_modelview().inverse().transpose();

                const U32 slice = 32;
                F32       offset[slice * 3];
                for (U32 i = 0; i < 4; i++)
                {
                    for (U32 j = 0; j < 8; j++)
                    {
                        glh::vec3f v;
                        v.set_value(sinf(6.284f / 8 * j), cosf(6.284f / 8 * j), -(F32) i);
                        v.normalize();
                        inv_trans.mult_matrix_vec(v);
                        v.normalize();
                        offset[(i * 8 + j) * 3 + 0] = v.v[0];
                        offset[(i * 8 + j) * 3 + 1] = v.v[2];
                        offset[(i * 8 + j) * 3 + 2] = v.v[1];
                    }
                }

                gDeferredSunProgram.uniform3fv(sOffset, slice, offset);
                gDeferredSunProgram.uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES,
                                              deferred_light_target->getWidth(),
                                              deferred_light_target->getHeight());

                {
                    LLGLDisable   blend(GL_BLEND);
                    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                    stop_glerror();
                    mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                    stop_glerror();
                }

                unbindDeferredShader(gDeferredSunProgram);
            }
            deferred_light_target->flush();
        }

        if (RenderDeferredSSAO)
        {  // soften direct lighting lightmap
            LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - soften shadow");
            // blur lightmap
            screen_target->bindTarget();
            glClearColor(1, 1, 1, 1);
            screen_target->clear(GL_COLOR_BUFFER_BIT);
            glClearColor(0, 0, 0, 0);

            bindDeferredShader(gDeferredBlurLightProgram);
            mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
            LLVector3 go          = RenderShadowGaussian;
            const U32 kern_length = 4;
            F32       blur_size   = RenderShadowBlurSize;
            F32       dist_factor = RenderShadowBlurDistFactor;

            // sample symmetrically with the middle sample falling exactly on 0.0
            F32 x = 0.f;

            LLVector3 gauss[32];  // xweight, yweight, offset

            for (U32 i = 0; i < kern_length; i++)
            {
                gauss[i].mV[0] = llgaussian(x, go.mV[0]);
                gauss[i].mV[1] = llgaussian(x, go.mV[1]);
                gauss[i].mV[2] = x;
                x += 1.f;
            }

            gDeferredBlurLightProgram.uniform2f(sDelta, 1.f, 0.f);
            gDeferredBlurLightProgram.uniform1f(sDistFactor, dist_factor);
            gDeferredBlurLightProgram.uniform3fv(sKern, kern_length, gauss[0].mV);
            gDeferredBlurLightProgram.uniform1f(sKernScale, blur_size * (kern_length / 2.f - 0.5f));

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                stop_glerror();
                mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                stop_glerror();
            }

            screen_target->flush();
            unbindDeferredShader(gDeferredBlurLightProgram);

            bindDeferredShader(gDeferredBlurLightProgram, screen_target);

            mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
            deferred_light_target->bindTarget();

            gDeferredBlurLightProgram.uniform2f(sDelta, 0.f, 1.f);

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                stop_glerror();
                mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                stop_glerror();
            }
            deferred_light_target->flush();
            unbindDeferredShader(gDeferredBlurLightProgram);
        }

        stop_glerror();
        gGL.popMatrix();
        stop_glerror();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        stop_glerror();
        gGL.popMatrix();
        stop_glerror();

        screen_target->bindTarget();
        // clear color buffer here - zeroing alpha (glow) is important or it will accumulate against sky
        glClearColor(0, 0, 0, 0);
        screen_target->clear(GL_COLOR_BUFFER_BIT);

        if (RenderDeferredAtmospheric)
        {  // apply sunlight contribution
            LLGLSLShader &soften_shader = LLPipeline::sUnderWaterRender ? gDeferredSoftenWaterProgram : gDeferredSoftenProgram;

            LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - atmospherics");
            bindDeferredShader(soften_shader);

            LLEnvironment &environment = LLEnvironment::instance();
            soften_shader.uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
            soften_shader.uniform4fv(LLShaderMgr::LIGHTNORM, 1, environment.getClampedLightNorm().mV);

            {
                LLGLDepthTest depth(GL_FALSE);
                LLGLDisable   blend(GL_BLEND);
                LLGLDisable   test(GL_ALPHA_TEST);

                // full screen blit
                gGL.pushMatrix();
                gGL.loadIdentity();
                gGL.matrixMode(LLRender::MM_PROJECTION);
                gGL.pushMatrix();
                gGL.loadIdentity();

                mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);

                mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                gGL.popMatrix();
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.popMatrix();
            }

            unbindDeferredShader(LLPipeline::sUnderWaterRender ? gDeferredSoftenWaterProgram : gDeferredSoftenProgram);
        }

        {  // render non-deferred geometry (fullbright, alpha, etc)
            LLGLDisable blend(GL_BLEND);
            LLGLDisable stencil(GL_STENCIL_TEST);
            gGL.setSceneBlendType(LLRender::BT_ALPHA);

            gPipeline.pushRenderTypeMask();

            gPipeline.andRenderTypeMask(LLPipeline::RENDER_TYPE_SKY,
                                        LLPipeline::RENDER_TYPE_CLOUDS,
                                        LLPipeline::RENDER_TYPE_WL_SKY,
                                        LLPipeline::END_RENDER_TYPES);

            renderGeomPostDeferred(*LLViewerCamera::getInstance(), false);
            gPipeline.popRenderTypeMask();
        }

        bool render_local = RenderLocalLights;

        if (render_local)
        {
            gGL.setSceneBlendType(LLRender::BT_ADD);
            std::list<LLVector4>        fullscreen_lights;
            LLDrawable::drawable_list_t spot_lights;
            LLDrawable::drawable_list_t fullscreen_spot_lights;

            for (U32 i = 0; i < 2; i++)
            {
                mTargetShadowSpotLight[i] = NULL;
            }

            std::list<LLVector4> light_colors;

            LLVertexBuffer::unbind();

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - local lights");
                bindDeferredShader(gDeferredLightProgram);

                if (mCubeVB.isNull())
                {
                    mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX, GL_STATIC_DRAW_ARB);
                }

                mCubeVB->setBuffer(LLVertexBuffer::MAP_VERTEX);

                LLGLDepthTest depth(GL_TRUE, GL_FALSE);
                // mNearbyLights already includes distance calculation and excludes muted avatars.
                // It is calculated from mLights
                // mNearbyLights also provides fade value to gracefully fade-out out of range lights
                for (light_set_t::iterator iter = mNearbyLights.begin(); iter != mNearbyLights.end(); ++iter)
                {
                    LLDrawable * drawablep = iter->drawable;
                    LLVOVolume * volume = drawablep->getVOVolume();
                    if (!volume)
                    {
                        continue;
                    }

                    if (volume->isAttachment())
                    {
                        if (!sRenderAttachedLights)
                        {
                            continue;
                        }
                    }

                    LLVector4a center;
                    center.load3(drawablep->getPositionAgent().mV);
                    const F32 *c = center.getF32ptr();
                    F32        s = volume->getLightRadius() * 1.5f;

                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor();

                    if (col.magVecSquared() < 0.001f)
                    {
                        continue;
                    }

                    if (s <= 0.001f)
                    {
                        continue;
                    }

                    LLVector4a sa;
                    sa.splat(s);
                    if (camera->AABBInFrustumNoFarClip(center, sa) == 0)
                    {
                        continue;
                    }

                    sVisibleLightCount++;

                    if (camera->getOrigin().mV[0] > c[0] + s + 0.2f || camera->getOrigin().mV[0] < c[0] - s - 0.2f ||
                        camera->getOrigin().mV[1] > c[1] + s + 0.2f || camera->getOrigin().mV[1] < c[1] - s - 0.2f ||
                        camera->getOrigin().mV[2] > c[2] + s + 0.2f || camera->getOrigin().mV[2] < c[2] - s - 0.2f)
                    {  // draw box if camera is outside box
                        if (render_local)
                        {
                            if (volume->isLightSpotlight())
                            {
                                drawablep->getVOVolume()->updateSpotLightPriority();
                                spot_lights.push_back(drawablep);
                                continue;
                            }

                            gDeferredLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, c);
                            gDeferredLightProgram.uniform1f(LLShaderMgr::LIGHT_SIZE, s);
                            gDeferredLightProgram.uniform3fv(LLShaderMgr::DIFFUSE_COLOR, 1, col.mV);
                            gDeferredLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF));
                            gGL.syncMatrices();

                            mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, center));
                            stop_glerror();
                        }
                    }
                    else
                    {
                        if (volume->isLightSpotlight())
                        {
                            drawablep->getVOVolume()->updateSpotLightPriority();
                            fullscreen_spot_lights.push_back(drawablep);
                            continue;
                        }

                        glh::vec3f tc(c);
                        mat.mult_matrix_vec(tc);

                        fullscreen_lights.push_back(LLVector4(tc.v[0], tc.v[1], tc.v[2], s));
                        light_colors.push_back(LLVector4(col.mV[0], col.mV[1], col.mV[2], volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF)));
                    }
                }

                // Bookmark comment to allow searching for mSpecialRenderMode == 3 (avatar edit mode),
                // prev site of appended deferred character light, removed by SL-13522 09/20

                unbindDeferredShader(gDeferredLightProgram);
            }

            if (!spot_lights.empty())
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - projectors");
                LLGLDepthTest depth(GL_TRUE, GL_FALSE);
                bindDeferredShader(gDeferredSpotLightProgram);

                mCubeVB->setBuffer(LLVertexBuffer::MAP_VERTEX);

                gDeferredSpotLightProgram.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

                for (LLDrawable::drawable_list_t::iterator iter = spot_lights.begin(); iter != spot_lights.end(); ++iter)
                {
                    LLDrawable *drawablep = *iter;

                    LLVOVolume *volume = drawablep->getVOVolume();

                    LLVector4a center;
                    center.load3(drawablep->getPositionAgent().mV);
                    const F32 *c = center.getF32ptr();
                    F32        s = volume->getLightRadius() * 1.5f;

                    sVisibleLightCount++;

                    setupSpotLight(gDeferredSpotLightProgram, drawablep);

                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor();

                    gDeferredSpotLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, c);
                    gDeferredSpotLightProgram.uniform1f(LLShaderMgr::LIGHT_SIZE, s);
                    gDeferredSpotLightProgram.uniform3fv(LLShaderMgr::DIFFUSE_COLOR, 1, col.mV);
                    gDeferredSpotLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF));
                    gGL.syncMatrices();

                    mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, center));
                }
                gDeferredSpotLightProgram.disableTexture(LLShaderMgr::DEFERRED_PROJECTION);
                unbindDeferredShader(gDeferredSpotLightProgram);
            }

            // reset mDeferredVB to fullscreen triangle
            mDeferredVB->getVertexStrider(vert);
            vert[0].set(-1, 1, 0);
            vert[1].set(-1, -3, 0);
            vert[2].set(3, 1, 0);

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - fullscreen lights");
                LLGLDepthTest depth(GL_FALSE);

                // full screen blit
                gGL.pushMatrix();
                gGL.loadIdentity();
                gGL.matrixMode(LLRender::MM_PROJECTION);
                gGL.pushMatrix();
                gGL.loadIdentity();

                U32 count = 0;

                const U32 max_count = LL_DEFERRED_MULTI_LIGHT_COUNT;
                LLVector4 light[max_count];
                LLVector4 col[max_count];

                F32 far_z = 0.f;

                while (!fullscreen_lights.empty())
                {
                    light[count] = fullscreen_lights.front();
                    fullscreen_lights.pop_front();
                    col[count] = light_colors.front();
                    light_colors.pop_front();

                    far_z = llmin(light[count].mV[2] - light[count].mV[3], far_z);
                    count++;
                    if (count == max_count || fullscreen_lights.empty())
                    {
                        U32 idx = count - 1;
                        bindDeferredShader(gDeferredMultiLightProgram[idx]);
                        gDeferredMultiLightProgram[idx].uniform1i(LLShaderMgr::MULTI_LIGHT_COUNT, count);
                        gDeferredMultiLightProgram[idx].uniform4fv(LLShaderMgr::MULTI_LIGHT, count, (GLfloat *) light);
                        gDeferredMultiLightProgram[idx].uniform4fv(LLShaderMgr::MULTI_LIGHT_COL, count, (GLfloat *) col);
                        gDeferredMultiLightProgram[idx].uniform1f(LLShaderMgr::MULTI_LIGHT_FAR_Z, far_z);
                        far_z = 0.f;
                        count = 0;
                        mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
                        mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                        unbindDeferredShader(gDeferredMultiLightProgram[idx]);
                    }
                }

                bindDeferredShader(gDeferredMultiSpotLightProgram);

                gDeferredMultiSpotLightProgram.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

                mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);

                for (LLDrawable::drawable_list_t::iterator iter = fullscreen_spot_lights.begin(); iter != fullscreen_spot_lights.end(); ++iter)
                {
                    LLDrawable *drawablep           = *iter;
                    LLVOVolume *volume              = drawablep->getVOVolume();
                    LLVector3   center              = drawablep->getPositionAgent();
                    F32 *       c                   = center.mV;
                    F32         light_size_final    = volume->getLightRadius() * 1.5f;
                    F32         light_falloff_final = volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF);

                    sVisibleLightCount++;

                    glh::vec3f tc(c);
                    mat.mult_matrix_vec(tc);

                    setupSpotLight(gDeferredMultiSpotLightProgram, drawablep);

                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor();

                    gDeferredMultiSpotLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, tc.v);
                    gDeferredMultiSpotLightProgram.uniform1f(LLShaderMgr::LIGHT_SIZE, light_size_final);
                    gDeferredMultiSpotLightProgram.uniform3fv(LLShaderMgr::DIFFUSE_COLOR, 1, col.mV);
                    gDeferredMultiSpotLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, light_falloff_final);
                    mDeferredVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                }

                gDeferredMultiSpotLightProgram.disableTexture(LLShaderMgr::DEFERRED_PROJECTION);
                unbindDeferredShader(gDeferredMultiSpotLightProgram);

                gGL.popMatrix();
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.popMatrix();
            }
        }

        gGL.setColorMask(true, true);
    }

    screen_target->flush();

    // gamma correct lighting
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();

    {
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);

        LLVector2 tc1(0, 0);
        LLVector2 tc2((F32) screen_target->getWidth() * 2, (F32) screen_target->getHeight() * 2);

        screen_target->bindTarget();
        // Apply gamma correction to the frame here.
        gDeferredPostGammaCorrectProgram.bind();
        // mDeferredVB->setBuffer(LLVertexBuffer::MAP_VERTEX);
        S32 channel = 0;
        channel     = gDeferredPostGammaCorrectProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, screen_target->getUsage());
        if (channel > -1)
        {
            screen_target->bindTexture(0, channel, LLTexUnit::TFO_POINT);
        }

        gDeferredPostGammaCorrectProgram.uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, screen_target->getWidth(), screen_target->getHeight());

        F32 gamma = gSavedSettings.getF32("RenderDeferredDisplayGamma");

        gDeferredPostGammaCorrectProgram.uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f / 2.2f));

        gGL.begin(LLRender::TRIANGLE_STRIP);
        gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
        gGL.vertex2f(-1, -1);

        gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
        gGL.vertex2f(-1, 3);

        gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
        gGL.vertex2f(3, -1);

        gGL.end();

        gGL.getTexUnit(channel)->unbind(screen_target->getUsage());
        gDeferredPostGammaCorrectProgram.unbind();
        screen_target->flush();
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();

    screen_target->bindTarget();

    {  // render non-deferred geometry (alpha, fullbright, glow)
        LLGLDisable blend(GL_BLEND);
        LLGLDisable stencil(GL_STENCIL_TEST);

        pushRenderTypeMask();
        andRenderTypeMask(LLPipeline::RENDER_TYPE_ALPHA,
                          LLPipeline::RENDER_TYPE_FULLBRIGHT,
                          LLPipeline::RENDER_TYPE_VOLUME,
                          LLPipeline::RENDER_TYPE_GLOW,
                          LLPipeline::RENDER_TYPE_BUMP,
                          LLPipeline::RENDER_TYPE_PASS_SIMPLE,
                          LLPipeline::RENDER_TYPE_PASS_ALPHA,
                          LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_PASS_BUMP,
                          LLPipeline::RENDER_TYPE_PASS_POST_BUMP,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY,
                          LLPipeline::RENDER_TYPE_PASS_GLOW,
                          LLPipeline::RENDER_TYPE_PASS_GRASS,
                          LLPipeline::RENDER_TYPE_PASS_SHINY,
                          LLPipeline::RENDER_TYPE_PASS_INVISIBLE,
                          LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY,
                          LLPipeline::RENDER_TYPE_AVATAR,
                          LLPipeline::RENDER_TYPE_CONTROL_AV,
                          LLPipeline::RENDER_TYPE_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK,
                          END_RENDER_TYPES);

        renderGeomPostDeferred(*LLViewerCamera::getInstance());
        popRenderTypeMask();
    }

    {
        // render highlights, etc.
        renderHighlights();
        mHighlightFaces.clear();

        renderDebug();

        LLVertexBuffer::unbind();

        if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
        {
            // Render debugging beacons.
            gObjectList.renderObjectBeacons();
            gObjectList.resetObjectBeacons();
            gSky.addSunMoonBeacons();
        }
    }

    screen_target->flush();
}

void LLPipeline::setupSpotLight(LLGLSLShader& shader, LLDrawable* drawablep)
{
	//construct frustum
	LLVOVolume* volume = drawablep->getVOVolume();
	LLVector3 params = volume->getSpotLightParams();

	F32 fov = params.mV[0];
	F32 focus = params.mV[1];

	LLVector3 pos = drawablep->getPositionAgent();
	LLQuaternion quat = volume->getRenderRotation();
	LLVector3 scale = volume->getScale();
	
	//get near clip plane
	LLVector3 at_axis(0,0,-scale.mV[2]*0.5f);
	at_axis *= quat;

	LLVector3 np = pos+at_axis;
	at_axis.normVec();

	//get origin that has given fov for plane np, at_axis, and given scale
	F32 dist = (scale.mV[1]*0.5f)/tanf(fov*0.5f);

	LLVector3 origin = np - at_axis*dist;

	//matrix from volume space to agent space
	LLMatrix4 light_mat(quat, LLVector4(origin,1.f));

	glh::matrix4f light_to_agent((F32*) light_mat.mMatrix);
	glh::matrix4f light_to_screen = get_current_modelview() * light_to_agent;

	glh::matrix4f screen_to_light = light_to_screen.inverse();

	F32 s = volume->getLightRadius()*1.5f;
	F32 near_clip = dist;
	F32 width = scale.mV[VX];
	F32 height = scale.mV[VY];
	F32 far_clip = s+dist-scale.mV[VZ];

	F32 fovy = fov * RAD_TO_DEG;
	F32 aspect = width/height;

	glh::matrix4f trans(0.5f, 0.f, 0.f, 0.5f,
				0.f, 0.5f, 0.f, 0.5f,
				0.f, 0.f, 0.5f, 0.5f,
				0.f, 0.f, 0.f, 1.f);

	glh::vec3f p1(0, 0, -(near_clip+0.01f));
	glh::vec3f p2(0, 0, -(near_clip+1.f));

	glh::vec3f screen_origin(0, 0, 0);

	light_to_screen.mult_matrix_vec(p1);
	light_to_screen.mult_matrix_vec(p2);
	light_to_screen.mult_matrix_vec(screen_origin);

	glh::vec3f n = p2-p1;
	n.normalize();
	
	F32 proj_range = far_clip - near_clip;
	glh::matrix4f light_proj = gl_perspective(fovy, aspect, near_clip, far_clip);
	screen_to_light = trans * light_proj * screen_to_light;
	shader.uniformMatrix4fv(LLShaderMgr::PROJECTOR_MATRIX, 1, FALSE, screen_to_light.m);
	shader.uniform1f(LLShaderMgr::PROJECTOR_NEAR, near_clip);
	shader.uniform3fv(LLShaderMgr::PROJECTOR_P, 1, p1.v);
	shader.uniform3fv(LLShaderMgr::PROJECTOR_N, 1, n.v);
	shader.uniform3fv(LLShaderMgr::PROJECTOR_ORIGIN, 1, screen_origin.v);
	shader.uniform1f(LLShaderMgr::PROJECTOR_RANGE, proj_range);
	shader.uniform1f(LLShaderMgr::PROJECTOR_AMBIANCE, params.mV[2]);
	S32 s_idx = -1;

	for (U32 i = 0; i < 2; i++)
	{
		if (mShadowSpotLight[i] == drawablep)
		{
			s_idx = i;
		}
	}

	shader.uniform1i(LLShaderMgr::PROJECTOR_SHADOW_INDEX, s_idx);

	if (s_idx >= 0)
	{
		shader.uniform1f(LLShaderMgr::PROJECTOR_SHADOW_FADE, 1.f-mSpotLightFade[s_idx]);
	}
	else
	{
		shader.uniform1f(LLShaderMgr::PROJECTOR_SHADOW_FADE, 1.f);
	}

	{
		LLDrawable* potential = drawablep;
		//determine if this is a good light for casting shadows
		F32 m_pri = volume->getSpotLightPriority();

		for (U32 i = 0; i < 2; i++)
		{
			F32 pri = 0.f;

			if (mTargetShadowSpotLight[i].notNull())
			{
				pri = mTargetShadowSpotLight[i]->getVOVolume()->getSpotLightPriority();			
			}

			if (m_pri > pri)
			{
				LLDrawable* temp = mTargetShadowSpotLight[i];
				mTargetShadowSpotLight[i] = potential;
				potential = temp;
				m_pri = pri;
			}
		}
	}

	LLViewerTexture* img = volume->getLightTexture();

	if (img == NULL)
	{
		img = LLViewerFetchedTexture::sWhiteImagep;
	}

	S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

	if (channel > -1)
	{
		if (img)
		{
			gGL.getTexUnit(channel)->bind(img);

			F32 lod_range = logf(img->getWidth())/logf(2.f);

			shader.uniform1f(LLShaderMgr::PROJECTOR_FOCUS, focus);
			shader.uniform1f(LLShaderMgr::PROJECTOR_LOD, lod_range);
			shader.uniform1f(LLShaderMgr::PROJECTOR_AMBIENT_LOD, llclamp((proj_range-focus)/proj_range*lod_range, 0.f, 1.f));
		}
	}
		
}

void LLPipeline::unbindDeferredShader(LLGLSLShader &shader)
{
    LLRenderTarget* deferred_target       = &mDeferredScreen;
    LLRenderTarget* deferred_depth_target = &mDeferredDepth;
    LLRenderTarget* deferred_light_target = &mDeferredLight;

	stop_glerror();
    shader.disableTexture(LLShaderMgr::DEFERRED_NORMAL, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_SPECULAR, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_depth_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_LIGHT, deferred_light_target->getUsage());
	shader.disableTexture(LLShaderMgr::DIFFUSE_MAP);
	shader.disableTexture(LLShaderMgr::DEFERRED_BLOOM);

	for (U32 i = 0; i < 4; i++)
	{
		if (shader.disableTexture(LLShaderMgr::DEFERRED_SHADOW0+i) > -1)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		}
	}

	for (U32 i = 4; i < 6; i++)
	{
		if (shader.disableTexture(LLShaderMgr::DEFERRED_SHADOW0+i) > -1)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		}
	}

	shader.disableTexture(LLShaderMgr::DEFERRED_NOISE);
	shader.disableTexture(LLShaderMgr::DEFERRED_LIGHTFUNC);

	S32 channel = shader.disableTexture(LLShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (!assertInitialized())
    {
        return;
    }

    if (LLPipeline::sWaterReflections && LLDrawPoolWater::sNeedsReflectionUpdate)
    {
        //disable occlusion culling for reflection/refraction passes (save setting to restore later)
        S32 occlude = LLPipeline::sUseOcclusion;
        LLPipeline::sUseOcclusion = 0;

        bool skip_avatar_update = false;
        if (!isAgentAvatarValid() || gAgentCamera.getCameraAnimating() || gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK || !LLVOAvatar::sVisibleInFirstPerson)
        {
            skip_avatar_update = true;
        }

        LLCamera camera = camera_in;
        camera.setFar(camera_in.getFar() * 0.75f);

        bool camera_is_underwater = LLViewerCamera::getInstance()->cameraUnderWater();

        LLPipeline::sReflectionRender = true;

        gPipeline.pushRenderTypeMask();

        glh::matrix4f saved_modelview  = get_current_modelview();
        glh::matrix4f saved_projection = get_current_projection();
        glh::matrix4f mat;

        S32 reflection_detail  = RenderReflectionDetail;

        F32 water_height      = gAgent.getRegion()->getWaterHeight(); 
        F32 camera_height     = camera_in.getOrigin().mV[VZ];
        F32 distance_to_water = (water_height < camera_height) ? (camera_height - water_height) : (water_height - camera_height);

        LLVector3 reflection_offset      = LLVector3(0, 0, distance_to_water * 2.0f);
        LLVector3 camera_look_at         = camera_in.getAtAxis();
        LLVector3 reflection_look_at     = LLVector3(camera_look_at.mV[VX], camera_look_at.mV[VY], -camera_look_at.mV[VZ]);
        LLVector3 reflect_origin         = camera_in.getOrigin() - reflection_offset;
        LLVector3 reflect_interest_point = reflect_origin + (reflection_look_at * 5.0f);

        camera.setOriginAndLookAt(reflect_origin, LLVector3::z_axis, reflect_interest_point);

        //plane params
        LLPlane plane;
        LLVector3 pnorm;

        if (camera_is_underwater)
        {
            //camera is below water, cull above water
            pnorm.setVec(0, 0, 1);
        }
        else
        {
            //camera is above water, cull below water
            pnorm = LLVector3(0, 0, -1);
        }

        plane.setVec(LLVector3(0, 0, water_height), pnorm);

        if (!camera_is_underwater)
        {
            //generate planar reflection map
            LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WATER0;

            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.pushMatrix();

            mat.set_scale(glh::vec3f(1, 1, -1));
            mat.set_translate(glh::vec3f(0,0,water_height*2.f));
            mat = saved_modelview * mat;


            mReflectionModelView = mat;

            set_current_modelview(mat);
            gGL.loadMatrix(mat.m);

            LLViewerCamera::updateFrustumPlanes(camera, FALSE, TRUE);

            glh::vec3f    origin(0, 0, 0);
            glh::matrix4f inv_mat = mat.inverse();
            inv_mat.mult_matrix_vec(origin);

            camera.setOrigin(origin.v);

            glCullFace(GL_FRONT);

            if (LLDrawPoolWater::sNeedsReflectionUpdate)
            {
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                glClearColor(0,0,0,0);
                mWaterRef.bindTarget();

                gGL.setColorMask(true, true);
                mWaterRef.clear();
                gGL.setColorMask(true, false);
                mWaterRef.getViewport(gGLViewport);

                //initial sky pass (no user clip plane)
                //mask out everything but the sky
                gPipeline.pushRenderTypeMask();
                {
                    if (reflection_detail >= WATER_REFLECT_MINIMAL)
                    {
                        gPipeline.andRenderTypeMask(
                            LLPipeline::RENDER_TYPE_SKY,
                            LLPipeline::RENDER_TYPE_WL_SKY,
                            LLPipeline::RENDER_TYPE_CLOUDS,
                            LLPipeline::END_RENDER_TYPES);
                    }
                    else
                    {
                        gPipeline.andRenderTypeMask(
                            LLPipeline::RENDER_TYPE_SKY,
                            LLPipeline::RENDER_TYPE_WL_SKY,
                            LLPipeline::END_RENDER_TYPES);
                    }

                    updateCull(camera, mSky);
                    stateSort(camera, mSky);
                    renderGeom(camera, TRUE);
                }
                gPipeline.popRenderTypeMask();

                if (reflection_detail >= WATER_REFLECT_NONE_WATER_TRANSPARENT)
                {
                    gPipeline.pushRenderTypeMask();
                    {
                        clearRenderTypeMask(
                            LLPipeline::RENDER_TYPE_WATER,
                            LLPipeline::RENDER_TYPE_VOIDWATER,
                            LLPipeline::RENDER_TYPE_GROUND,
                            LLPipeline::RENDER_TYPE_SKY,
                            LLPipeline::RENDER_TYPE_CLOUDS,
                            LLPipeline::END_RENDER_TYPES);

                        if (reflection_detail > WATER_REFLECT_MINIMAL)
                        { //mask out selected geometry based on reflection detail
                            if (reflection_detail < WATER_REFLECT_EVERYTHING)
                            {
                                clearRenderTypeMask(LLPipeline::RENDER_TYPE_PARTICLES, END_RENDER_TYPES);
                                if (reflection_detail < WATER_REFLECT_AVATARS)
                                {
                                    clearRenderTypeMask(
                                        LLPipeline::RENDER_TYPE_AVATAR,
                                        LLPipeline::RENDER_TYPE_CONTROL_AV,
                                        END_RENDER_TYPES);
                                    if (reflection_detail < WATER_REFLECT_STATIC_OBJECTS)
                                    {
                                        clearRenderTypeMask(LLPipeline::RENDER_TYPE_VOLUME, END_RENDER_TYPES);
                                    }
                                }
                            }

                            LLGLUserClipPlane clip_plane(plane, mReflectionModelView, saved_projection);
                            LLGLDisable cull(GL_CULL_FACE);
                            updateCull(camera, mReflectedObjects, &plane);
                            stateSort(camera, mReflectedObjects);
                            renderGeom(camera);
                        }
                    }
                    gPipeline.popRenderTypeMask();
                }

                mWaterRef.flush();
            }

            glCullFace(GL_BACK);
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.popMatrix();

            set_current_modelview(saved_modelview);
        }

        camera.setOrigin(camera_in.getOrigin());
        //render distortion map
        static bool last_update = true;
        if (last_update)
        {
            gPipeline.pushRenderTypeMask();

            camera.setFar(camera_in.getFar());
            clearRenderTypeMask(
                LLPipeline::RENDER_TYPE_WATER,
                LLPipeline::RENDER_TYPE_VOIDWATER,
                LLPipeline::RENDER_TYPE_GROUND,
                END_RENDER_TYPES);

            // intentionally inverted so that distortion map contents (objects under the water when we're above it)
            // will properly include water fog effects
            LLPipeline::sUnderWaterRender = !camera_is_underwater;

            if (LLPipeline::sUnderWaterRender)
            {
                clearRenderTypeMask(
                    LLPipeline::RENDER_TYPE_GROUND,
                    LLPipeline::RENDER_TYPE_SKY,
                    LLPipeline::RENDER_TYPE_CLOUDS,
                    LLPipeline::RENDER_TYPE_WL_SKY,
                    END_RENDER_TYPES);
            }
            LLViewerCamera::updateFrustumPlanes(camera);

            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            if (LLPipeline::sUnderWaterRender || LLDrawPoolWater::sNeedsDistortionUpdate)
            {
                LLPipeline::sDistortionRender = true;

                LLColor3 col = LLEnvironment::instance().getCurrentWater()->getWaterFogColor();
                glClearColor(col.mV[0], col.mV[1], col.mV[2], 0.f);

                // HACK FIX -- pretend underwater camera is the world camera to fix weird visibility artifacts
                // during distortion render (doesn't break main render because the camera is the same perspective
                // as world camera and occlusion culling is disabled for this pass)
                //LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WATER1;
                LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

                mWaterDis.bindTarget();
                mWaterDis.getViewport(gGLViewport);

                gGL.setColorMask(true, true);
                mWaterDis.clear();
                gGL.setColorMask(true, false);

                F32 water_dist = water_height;

                //clip out geometry on the same side of water as the camera w/ enough margin to not include the water geo itself,
                // but not so much as to clip out parts of avatars that should be seen under the water in the distortion map
                LLPlane plane;

                if (camera_is_underwater)
                {
                    //nudge clip plane below water to avoid visible holes in objects intersecting water surface
                    water_dist /= LLPipeline::sDistortionWaterClipPlaneMargin;
                    //camera is below water, clip plane points up
                    pnorm.setVec(0, 0, -1);
                }
                else
                {
                    //nudge clip plane above water to avoid visible holes in objects intersecting water surface
                    water_dist *= LLPipeline::sDistortionWaterClipPlaneMargin;
                    //camera is above water, clip plane points down
                    pnorm = LLVector3(0, 0, 1);
                }

                plane.setVec(LLVector3(0, 0, water_dist), pnorm);

                LLGLUserClipPlane clip_plane(plane, saved_modelview, saved_projection);

                gGL.setColorMask(true, true);
                mWaterDis.clear();
                gGL.setColorMask(true, false);

                if (reflection_detail >= WATER_REFLECT_NONE_WATER_TRANSPARENT)
                {
                    updateCull(camera, mRefractedObjects, &plane);
                    stateSort(camera, mRefractedObjects);
                    renderGeom(camera);
                }

                gUIProgram.bind();

                LLWorld::getInstance()->renderPropertyLines();

                gUIProgram.unbind();

                mWaterDis.flush();
            }

            LLPipeline::sDistortionRender = false;

            gPipeline.popRenderTypeMask();
        }
        last_update = LLDrawPoolWater::sNeedsReflectionUpdate && LLDrawPoolWater::sNeedsDistortionUpdate;

        gPipeline.popRenderTypeMask();

        LLPipeline::sUnderWaterRender = false;
        LLPipeline::sReflectionRender = false;

        LLDrawPoolWater::sNeedsReflectionUpdate = FALSE;
        LLDrawPoolWater::sNeedsDistortionUpdate = FALSE;

        if (!LLRenderTarget::sUseFBO)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        glClearColor(0.f, 0.f, 0.f, 0.f);
        gViewerWindow->setup3DViewport();

        LLGLState::checkStates();

        if (!skip_avatar_update)
        {
            gAgentAvatarp->updateAttachmentVisibility(gAgentCamera.getCameraMode());
        }

        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

        // restore occlusion culling
        LLPipeline::sUseOcclusion = occlude;
    }
    else
    {
        // Initial sky pass is still needed even if water reflection is not rendering
        bool camera_is_underwater = LLViewerCamera::getInstance()->cameraUnderWater();
        if (!camera_is_underwater)
        {
            gPipeline.pushRenderTypeMask();
            {
                gPipeline.andRenderTypeMask(
                    LLPipeline::RENDER_TYPE_SKY,
                    LLPipeline::RENDER_TYPE_WL_SKY,
                    LLPipeline::END_RENDER_TYPES);

                LLCamera camera = camera_in;
                camera.setFar(camera_in.getFar() * 0.75f);

                updateCull(camera, mSky);
                stateSort(camera, mSky);
                renderGeom(camera, TRUE);
            }
            gPipeline.popRenderTypeMask();
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

static LLTrace::BlockTimerStatHandle FTM_SHADOW_RENDER("Render Shadows");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA("Alpha Shadow");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_SIMPLE("Simple Shadow");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_GEOM("Shadow Geom");

static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_MASKED("Alpha Masked");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_BLEND("Alpha Blend");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_TREE("Alpha Tree");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_GRASS("Alpha Grass");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_FULLBRIGHT_ALPHA_MASKED("Fullbright Alpha Masked");

void LLPipeline::renderShadow(glh::matrix4f& view, glh::matrix4f& proj, LLCamera& shadow_cam, LLCullResult& result, bool use_shader, bool use_occlusion, U32 target_width)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_RENDER);

    //disable occlusion culling for shadow passes (save setting to restore later)
    S32 occlude = LLPipeline::sUseOcclusion;
    if (!use_occlusion)
    {
        LLPipeline::sUseOcclusion = 0;
    }
    LLPipeline::sShadowRender = true;

    static const U32 types[] = {
        LLRenderPass::PASS_SIMPLE,
        LLRenderPass::PASS_FULLBRIGHT,
        LLRenderPass::PASS_SHINY,
        LLRenderPass::PASS_BUMP,
        LLRenderPass::PASS_FULLBRIGHT_SHINY ,
        LLRenderPass::PASS_MATERIAL,
        LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
        LLRenderPass::PASS_SPECMAP,
        LLRenderPass::PASS_SPECMAP_EMISSIVE,
        LLRenderPass::PASS_NORMMAP,
        LLRenderPass::PASS_NORMMAP_EMISSIVE,
        LLRenderPass::PASS_NORMSPEC,
        LLRenderPass::PASS_NORMSPEC_EMISSIVE,
    };

    LLGLEnable cull(GL_CULL_FACE);

    //enable depth clamping if available
    LLGLEnable depth_clamp(gGLManager.mHasDepthClamp ? GL_DEPTH_CLAMP : 0);

    if (use_shader)
    {
        gDeferredShadowCubeProgram.bind();
    }

    LLRenderTarget& occlusion_target = mShadowOcclusion[LLViewerCamera::sCurCameraID - 1];

    occlusion_target.bindTarget();
    updateCull(shadow_cam, result);
    occlusion_target.flush();

    stateSort(shadow_cam, result);


    //generate shadow map
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadMatrix(proj.m);
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadMatrix(view.m);

    stop_glerror();
    gGLLastMatrix = NULL;

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    stop_glerror();

    LLEnvironment& environment = LLEnvironment::instance();

    LLVertexBuffer::unbind();

    for (int j = 0; j < 2; ++j) // 0 -- static, 1 -- rigged
    {
        bool rigged = j == 1;
        if (!use_shader)
        { //occlusion program is general purpose depth-only no-textures
            gOcclusionProgram.bind(rigged);
        }
        else
        {
            gDeferredShadowProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
        }

        gGL.diffuseColor4f(1, 1, 1, 1);

        S32 shadow_detail = gSavedSettings.getS32("RenderShadowDetail");

        // if not using VSM, disable color writes
        if (shadow_detail <= 2)
        {
            gGL.setColorMask(false, false);
        }

        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow simple"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_SIMPLE);

        gGL.getTexUnit(0)->disable();
        for (U32 i = 0; i < sizeof(types) / sizeof(U32); ++i)
        {
            renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE, FALSE, rigged);
        }
        gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
        if (!use_shader)
        {
            gOcclusionProgram.unbind();
        }


    }

    if (use_shader)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow geom"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_GEOM);

        gDeferredShadowProgram.unbind();
        renderGeomShadow(shadow_cam);
        gDeferredShadowProgram.bind();
        gDeferredShadowProgram.uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
    }
    else
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow geom"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_GEOM);

        renderGeomShadow(shadow_cam);
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_ALPHA);

        for (int i = 0; i < 2; ++i)
        {
            bool rigged = i == 1;

            gDeferredShadowAlphaMaskProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width);
            LLGLSLShader::sCurBoundShaderPtr->uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);

            U32 mask = LLVertexBuffer::MAP_VERTEX |
                LLVertexBuffer::MAP_TEXCOORD0 |
                LLVertexBuffer::MAP_COLOR |
                LLVertexBuffer::MAP_TEXTURE_INDEX;

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha masked"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_ALPHA_MASKED);
                renderMaskedObjects(LLRenderPass::PASS_ALPHA_MASK, mask, TRUE, TRUE, rigged);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha blend"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_ALPHA_BLEND);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(0.598f);
                renderAlphaObjects(mask, TRUE, TRUE, rigged);
            }


            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow fullbright alpha masked"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_FULLBRIGHT_ALPHA_MASKED);
                gDeferredShadowFullbrightAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width);
                LLGLSLShader::sCurBoundShaderPtr->uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
                renderFullbrightMaskedObjects(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, mask, TRUE, TRUE, rigged);
            }


            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha grass"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_ALPHA_GRASS);
                gDeferredTreeShadowProgram.bind(rigged);
                if (i == 0)
                {
                    LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(0.598f);
                    renderObjects(LLRenderPass::PASS_GRASS, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, TRUE);
                }

                U32 no_idx_mask = mask & ~LLVertexBuffer::MAP_TEXTURE_INDEX;
                renderMaskedObjects(LLRenderPass::PASS_NORMSPEC_MASK, no_idx_mask, true, false, rigged);
                renderMaskedObjects(LLRenderPass::PASS_MATERIAL_ALPHA_MASK, no_idx_mask, true, false, rigged);
                renderMaskedObjects(LLRenderPass::PASS_SPECMAP_MASK, no_idx_mask, true, false, rigged);
                renderMaskedObjects(LLRenderPass::PASS_NORMMAP_MASK, no_idx_mask, true, false, rigged);
            }
        }
    }

    //glCullFace(GL_BACK);

    gDeferredShadowCubeProgram.bind();
    gGLLastMatrix = NULL;
    gGL.loadMatrix(gGLModelView);

    LLRenderTarget& occlusion_source = mShadow[LLViewerCamera::sCurCameraID - 1];

    doOcclusion(shadow_cam, occlusion_source, occlusion_target);

    if (use_shader)
    {
        gDeferredShadowProgram.unbind();
    }

    gGL.setColorMask(true, true);

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
    gGLLastMatrix = NULL;

    LLPipeline::sUseOcclusion = occlude;
    LLPipeline::sShadowRender = false;
}

bool LLPipeline::getVisiblePointCloud(LLCamera& camera, LLVector3& min, LLVector3& max, std::vector<LLVector3>& fp, LLVector3 light_dir)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	//get point cloud of intersection of frust and min, max

	if (getVisibleExtents(camera, min, max))
	{
		return false;
	}

	//get set of planes on bounding box
	LLPlane bp[] = { 
		LLPlane(min, LLVector3(-1,0,0)),
		LLPlane(min, LLVector3(0,-1,0)),
		LLPlane(min, LLVector3(0,0,-1)),
		LLPlane(max, LLVector3(1,0,0)),
		LLPlane(max, LLVector3(0,1,0)),
		LLPlane(max, LLVector3(0,0,1))};
	
	//potential points
	std::vector<LLVector3> pp;

	//add corners of AABB
	pp.push_back(LLVector3(min.mV[0], min.mV[1], min.mV[2]));
	pp.push_back(LLVector3(max.mV[0], min.mV[1], min.mV[2]));
	pp.push_back(LLVector3(min.mV[0], max.mV[1], min.mV[2]));
	pp.push_back(LLVector3(max.mV[0], max.mV[1], min.mV[2]));
	pp.push_back(LLVector3(min.mV[0], min.mV[1], max.mV[2]));
	pp.push_back(LLVector3(max.mV[0], min.mV[1], max.mV[2]));
	pp.push_back(LLVector3(min.mV[0], max.mV[1], max.mV[2]));
	pp.push_back(LLVector3(max.mV[0], max.mV[1], max.mV[2]));

	//add corners of camera frustum
	for (U32 i = 0; i < LLCamera::AGENT_FRUSTRUM_NUM; i++)
	{
		pp.push_back(camera.mAgentFrustum[i]);
	}


	//bounding box line segments
	U32 bs[] = 
			{
		0,1,
		1,3,
		3,2,
		2,0,

		4,5,
		5,7,
		7,6,
		6,4,

		0,4,
		1,5,
		3,7,
		2,6
	};

	for (U32 i = 0; i < 12; i++)
	{ //for each line segment in bounding box
		for (U32 j = 0; j < LLCamera::AGENT_PLANE_NO_USER_CLIP_NUM; j++) 
		{ //for each plane in camera frustum
			const LLPlane& cp = camera.getAgentPlane(j);
			const LLVector3& v1 = pp[bs[i*2+0]];
			const LLVector3& v2 = pp[bs[i*2+1]];
			LLVector3 n;
			cp.getVector3(n);

			LLVector3 line = v1-v2;

			F32 d1 = line*n;
			F32 d2 = -cp.dist(v2);

			F32 t = d2/d1;

			if (t > 0.f && t < 1.f)
			{
				LLVector3 intersect = v2+line*t;
				pp.push_back(intersect);
			}
		}
	}
			
	//camera frustum line segments
	const U32 fs[] =
	{
		0,1,
		1,2,
		2,3,
		3,0,

		4,5,
		5,6,
		6,7,
		7,4,
	
		0,4,
		1,5,
		2,6,
		3,7	
	};

	for (U32 i = 0; i < 12; i++)
	{
		for (U32 j = 0; j < 6; ++j)
		{
			const LLVector3& v1 = pp[fs[i*2+0]+8];
			const LLVector3& v2 = pp[fs[i*2+1]+8];
			const LLPlane& cp = bp[j];
			LLVector3 n;
			cp.getVector3(n);

			LLVector3 line = v1-v2;

			F32 d1 = line*n;
			F32 d2 = -cp.dist(v2);

			F32 t = d2/d1;

			if (t > 0.f && t < 1.f)
			{
				LLVector3 intersect = v2+line*t;
				pp.push_back(intersect);
			}	
		}
	}

	LLVector3 ext[] = { min-LLVector3(0.05f,0.05f,0.05f),
		max+LLVector3(0.05f,0.05f,0.05f) };

	for (U32 i = 0; i < pp.size(); ++i)
	{
		bool found = true;

		const F32* p = pp[i].mV;
			
		for (U32 j = 0; j < 3; ++j)
		{
			if (p[j] < ext[0].mV[j] ||
				p[j] > ext[1].mV[j])
			{
				found = false;
				break;
			}
		}
				
		for (U32 j = 0; j < LLCamera::AGENT_PLANE_NO_USER_CLIP_NUM; ++j)
		{
			const LLPlane& cp = camera.getAgentPlane(j);
			F32 dist = cp.dist(pp[i]);
			if (dist > 0.05f) //point is above some plane, not contained
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			fp.push_back(pp[i]);
		}
	}
	
	if (fp.empty())
	{
		return false;
	}
	
	return true;
}

void LLPipeline::renderHighlight(const LLViewerObject* obj, F32 fade)
{
	if (obj && obj->getVolume())
	{
		for (LLViewerObject::child_list_t::const_iterator iter = obj->getChildren().begin(); iter != obj->getChildren().end(); ++iter)
		{
			renderHighlight(*iter, fade);
		}

		LLDrawable* drawable = obj->mDrawable;
		if (drawable)
		{
			for (S32 i = 0; i < drawable->getNumFaces(); ++i)
			{
				LLFace* face = drawable->getFace(i);
				if (face)
				{
					face->renderSelected(LLViewerTexture::sNullImagep, LLColor4(1,1,1,fade));
				}
			}
		}
	}
}

void LLPipeline::generateHighlight(LLCamera& camera)
{
	//render highlighted object as white into offscreen render target
	if (mHighlightObject.notNull())
	{
		mHighlightSet.insert(HighlightItem(mHighlightObject));
	}
	
	if (!mHighlightSet.empty())
	{
		F32 transition = gFrameIntervalSeconds.value()/RenderHighlightFadeTime;

		LLGLDisable test(GL_ALPHA_TEST);
		LLGLDepthTest depth(GL_FALSE);
		mHighlight.bindTarget();
		disableLights();
		gGL.setColorMask(true, true);
		mHighlight.clear();

        gHighlightProgram.bind();

		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
		for (std::set<HighlightItem>::iterator iter = mHighlightSet.begin(); iter != mHighlightSet.end(); )
		{
			std::set<HighlightItem>::iterator cur_iter = iter++;

			if (cur_iter->mItem.isNull())
			{
				mHighlightSet.erase(cur_iter);
				continue;
			}

			if (cur_iter->mItem == mHighlightObject)
			{
				cur_iter->incrFade(transition); 
			}
			else
			{
				cur_iter->incrFade(-transition);
				if (cur_iter->mFade <= 0.f)
				{
					mHighlightSet.erase(cur_iter);
					continue;
				}
			}

			renderHighlight(cur_iter->mItem->getVObj(), cur_iter->mFade);
		}

		mHighlight.flush();
		gGL.setColorMask(true, false);
		gViewerWindow->setup3DViewport();
	}
}

LLRenderTarget* LLPipeline::getShadowTarget(U32 i)
{
    return &mShadow[i];
}

static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW("Gen Sun Shadow");
static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW_SPOT_RENDER("Spot Shadow Render");

void LLPipeline::generateSunShadow(LLCamera& camera)
{
	if (!sRenderDeferred || RenderShadowDetail <= 0)
	{
		return;
	}

	LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_GEN_SUN_SHADOW);

	bool skip_avatar_update = false;
	if (!isAgentAvatarValid() || gAgentCamera.getCameraAnimating() || gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK || !LLVOAvatar::sVisibleInFirstPerson)
	{

		skip_avatar_update = true;
	}

	if (!skip_avatar_update)
	{
		gAgentAvatarp->updateAttachmentVisibility(CAMERA_MODE_THIRD_PERSON);
	}

	F64 last_modelview[16];
	F64 last_projection[16];
	for (U32 i = 0; i < 16; i++)
	{ //store last_modelview of world camera
		last_modelview[i] = gGLLastModelView[i];
		last_projection[i] = gGLLastProjection[i];
	}

	pushRenderTypeMask();
	andRenderTypeMask(LLPipeline::RENDER_TYPE_SIMPLE,
					LLPipeline::RENDER_TYPE_ALPHA,
					LLPipeline::RENDER_TYPE_GRASS,
					LLPipeline::RENDER_TYPE_FULLBRIGHT,
					LLPipeline::RENDER_TYPE_BUMP,
					LLPipeline::RENDER_TYPE_VOLUME,
					LLPipeline::RENDER_TYPE_AVATAR,
					LLPipeline::RENDER_TYPE_CONTROL_AV,
					LLPipeline::RENDER_TYPE_TREE, 
					LLPipeline::RENDER_TYPE_TERRAIN,
					LLPipeline::RENDER_TYPE_WATER,
					LLPipeline::RENDER_TYPE_VOIDWATER,
					LLPipeline::RENDER_TYPE_PASS_ALPHA,
					LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK,
					LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK,
					LLPipeline::RENDER_TYPE_PASS_GRASS,
					LLPipeline::RENDER_TYPE_PASS_SIMPLE,
					LLPipeline::RENDER_TYPE_PASS_BUMP,
					LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT,
					LLPipeline::RENDER_TYPE_PASS_SHINY,
					LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY,
					LLPipeline::RENDER_TYPE_PASS_MATERIAL,
					LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA,
					LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_MASK,
					LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_EMISSIVE,
					LLPipeline::RENDER_TYPE_PASS_SPECMAP,
					LLPipeline::RENDER_TYPE_PASS_SPECMAP_BLEND,
					LLPipeline::RENDER_TYPE_PASS_SPECMAP_MASK,
					LLPipeline::RENDER_TYPE_PASS_SPECMAP_EMISSIVE,
					LLPipeline::RENDER_TYPE_PASS_NORMMAP,
					LLPipeline::RENDER_TYPE_PASS_NORMMAP_BLEND,
					LLPipeline::RENDER_TYPE_PASS_NORMMAP_MASK,
					LLPipeline::RENDER_TYPE_PASS_NORMMAP_EMISSIVE,
					LLPipeline::RENDER_TYPE_PASS_NORMSPEC,
					LLPipeline::RENDER_TYPE_PASS_NORMSPEC_BLEND,
					LLPipeline::RENDER_TYPE_PASS_NORMSPEC_MASK,
					LLPipeline::RENDER_TYPE_PASS_NORMSPEC_EMISSIVE,
                    LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SIMPLE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_BUMP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SHINY_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_EMISSIVE_RIGGED,
					END_RENDER_TYPES);

	gGL.setColorMask(false, false);

    LLEnvironment& environment = LLEnvironment::instance();

	//get sun view matrix
	
	//store current projection/modelview matrix
	glh::matrix4f saved_proj = get_current_projection();
	glh::matrix4f saved_view = get_current_modelview();
	glh::matrix4f inv_view = saved_view.inverse();

	glh::matrix4f view[6];
	glh::matrix4f proj[6];
	
	//clip contains parallel split distances for 3 splits
	LLVector3 clip = RenderShadowClipPlanes;

    LLVector3 caster_dir(environment.getIsSunUp() ? mSunDir : mMoonDir);

	//F32 slope_threshold = gSavedSettings.getF32("RenderShadowSlopeThreshold");

	//far clip on last split is minimum of camera view distance and 128
	mSunClipPlanes = LLVector4(clip, clip.mV[2] * clip.mV[2]/clip.mV[1]);

	clip = RenderShadowOrthoClipPlanes;
	mSunOrthoClipPlanes = LLVector4(clip, clip.mV[2]*clip.mV[2]/clip.mV[1]);

	//currently used for amount to extrude frusta corners for constructing shadow frusta
	//LLVector3 n = RenderShadowNearDist;
	//F32 nearDist[] = { n.mV[0], n.mV[1], n.mV[2], n.mV[2] };

	//put together a universal "near clip" plane for shadow frusta
	LLPlane shadow_near_clip;
	{
		LLVector3 p = gAgent.getPositionAgent();
		p += caster_dir * RenderFarClip*2.f;
		shadow_near_clip.setVec(p, caster_dir);
	}

	LLVector3 lightDir = -caster_dir;
	lightDir.normVec();

	glh::vec3f light_dir(lightDir.mV);

	//create light space camera matrix
	
	LLVector3 at = lightDir;

	LLVector3 up = camera.getAtAxis();

	if (fabsf(up*lightDir) > 0.75f)
	{
		up = camera.getUpAxis();
	}

	/*LLVector3 left = up%at;
	up = at%left;*/

	up.normVec();
	at.normVec();
	
	
	LLCamera main_camera = camera;
	
	F32 near_clip = 0.f;
	{
		//get visible point cloud
		std::vector<LLVector3> fp;

		main_camera.calcAgentFrustumPlanes(main_camera.mAgentFrustum);
		
		LLVector3 min,max;
		getVisiblePointCloud(main_camera,min,max,fp);

		if (fp.empty())
		{
			if (!hasRenderDebugMask(RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowCamera[0] = main_camera;
				mShadowExtents[0][0] = min;
				mShadowExtents[0][1] = max;

				mShadowFrustPoints[0].clear();
				mShadowFrustPoints[1].clear();
				mShadowFrustPoints[2].clear();
				mShadowFrustPoints[3].clear();
			}
			popRenderTypeMask();

			if (!skip_avatar_update)
			{
				gAgentAvatarp->updateAttachmentVisibility(gAgentCamera.getCameraMode());
			}

			return;
		}

		//get good split distances for frustum
		for (U32 i = 0; i < fp.size(); ++i)
		{
			glh::vec3f v(fp[i].mV);
			saved_view.mult_matrix_vec(v);
			fp[i].setVec(v.v);
		}

		min = fp[0];
		max = fp[0];

		//get camera space bounding box
		for (U32 i = 1; i < fp.size(); ++i)
		{
			update_min_max(min, max, fp[i]);
		}

		near_clip    = llclamp(-max.mV[2], 0.01f, 4.0f);
		F32 far_clip = llclamp(-min.mV[2]*2.f, 16.0f, 512.0f);

		//far_clip = llmin(far_clip, 128.f);
		far_clip = llmin(far_clip, camera.getFar());

		F32 range = far_clip-near_clip;

		LLVector3 split_exp = RenderShadowSplitExponent;

		F32 da = 1.f-llmax( fabsf(lightDir*up), fabsf(lightDir*camera.getLeftAxis()) );
		
		da = powf(da, split_exp.mV[2]);

		F32 sxp = split_exp.mV[1] + (split_exp.mV[0]-split_exp.mV[1])*da;
		
		for (U32 i = 0; i < 4; ++i)
		{
			F32 x = (F32)(i+1)/4.f;
			x = powf(x, sxp);
			mSunClipPlanes.mV[i] = near_clip+range*x;
		}

		mSunClipPlanes.mV[0] *= 1.25f; //bump back first split for transition padding
	}

	// convenience array of 4 near clip plane distances
	F32 dist[] = { near_clip, mSunClipPlanes.mV[0], mSunClipPlanes.mV[1], mSunClipPlanes.mV[2], mSunClipPlanes.mV[3] };
	

	if (mSunDiffuse == LLColor4::black)
	{ //sun diffuse is totally black, shadows don't matter
		LLGLDepthTest depth(GL_TRUE);

		for (S32 j = 0; j < 4; j++)
		{
			mShadow[j].bindTarget();
			mShadow[j].clear();
			mShadow[j].flush();
		}
	}
	else
	{
		for (S32 j = 0; j < 4; j++)
		{
			if (!hasRenderDebugMask(RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowFrustPoints[j].clear();
			}

			LLViewerCamera::sCurCameraID = (LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SHADOW0+j);

			//restore render matrices
			set_current_modelview(saved_view);
			set_current_projection(saved_proj);

			LLVector3 eye = camera.getOrigin();

			//camera used for shadow cull/render
			LLCamera shadow_cam;
		
			//create world space camera frustum for this split
			shadow_cam = camera;
			shadow_cam.setFar(16.f);
	
			LLViewerCamera::updateFrustumPlanes(shadow_cam, FALSE, FALSE, TRUE);

			LLVector3* frust = shadow_cam.mAgentFrustum;

			LLVector3 pn = shadow_cam.getAtAxis();
		
			LLVector3 min, max;

			//construct 8 corners of split frustum section
			for (U32 i = 0; i < 4; i++)
			{
				LLVector3 delta = frust[i+4]-eye;
				delta += (frust[i+4]-frust[(i+2)%4+4])*0.05f;
				delta.normVec();
				F32 dp = delta*pn;
				frust[i] = eye + (delta*dist[j]*0.75f)/dp;
				frust[i+4] = eye + (delta*dist[j+1]*1.25f)/dp;
			}
						
			shadow_cam.calcAgentFrustumPlanes(frust);
			shadow_cam.mFrustumCornerDist = 0.f;
		
			if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowCamera[j] = shadow_cam;
			}

			std::vector<LLVector3> fp;

			if (!gPipeline.getVisiblePointCloud(shadow_cam, min, max, fp, lightDir))
			{
				//no possible shadow receivers
				if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
				{
					mShadowExtents[j][0] = LLVector3();
					mShadowExtents[j][1] = LLVector3();
					mShadowCamera[j+4] = shadow_cam;
				}

				mShadow[j].bindTarget();
				{
					LLGLDepthTest depth(GL_TRUE);
					mShadow[j].clear();
				}
				mShadow[j].flush();

				mShadowError.mV[j] = 0.f;
				mShadowFOV.mV[j] = 0.f;

				continue;
			}

			if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowExtents[j][0] = min;
				mShadowExtents[j][1] = max;
				mShadowFrustPoints[j] = fp;
			}
				

			//find a good origin for shadow projection
			LLVector3 origin;

			//get a temporary view projection
			view[j] = look(camera.getOrigin(), lightDir, -up);

			std::vector<LLVector3> wpf;

			for (U32 i = 0; i < fp.size(); i++)
			{
				glh::vec3f p = glh::vec3f(fp[i].mV);
				view[j].mult_matrix_vec(p);
				wpf.push_back(LLVector3(p.v));
			}

			min = wpf[0];
			max = wpf[0];

			for (U32 i = 0; i < fp.size(); ++i)
			{ //get AABB in camera space
				update_min_max(min, max, wpf[i]);
			}

			// Construct a perspective transform with perspective along y-axis that contains
			// points in wpf
			//Known:
			// - far clip plane
			// - near clip plane
			// - points in frustum
			//Find:
			// - origin

			//get some "interesting" points of reference
			LLVector3 center = (min+max)*0.5f;
			LLVector3 size = (max-min)*0.5f;
			LLVector3 near_center = center;
			near_center.mV[1] += size.mV[1]*2.f;
		
		
			//put all points in wpf in quadrant 0, reletive to center of min/max
			//get the best fit line using least squares
			F32 bfm = 0.f;
			F32 bfb = 0.f;

			for (U32 i = 0; i < wpf.size(); ++i)
			{
				wpf[i] -= center;
				wpf[i].mV[0] = fabsf(wpf[i].mV[0]);
				wpf[i].mV[2] = fabsf(wpf[i].mV[2]);
			}

			if (!wpf.empty())
			{ 
				F32 sx = 0.f;
				F32 sx2 = 0.f;
				F32 sy = 0.f;
				F32 sxy = 0.f;
			
				for (U32 i = 0; i < wpf.size(); ++i)
				{		
					sx += wpf[i].mV[0];
					sx2 += wpf[i].mV[0]*wpf[i].mV[0];
					sy += wpf[i].mV[1];
					sxy += wpf[i].mV[0]*wpf[i].mV[1]; 
				}

				bfm = (sy*sx-wpf.size()*sxy)/(sx*sx-wpf.size()*sx2);
				bfb = (sx*sxy-sy*sx2)/(sx*sx-bfm*sx2);
			}
		
			{
				// best fit line is y=bfm*x+bfb
		
				//find point that is furthest to the right of line
				F32 off_x = -1.f;
				LLVector3 lp;

				for (U32 i = 0; i < wpf.size(); ++i)
				{
					//y = bfm*x+bfb
					//x = (y-bfb)/bfm
					F32 lx = (wpf[i].mV[1]-bfb)/bfm;

					lx = wpf[i].mV[0]-lx;
				
					if (off_x < lx)
					{
						off_x = lx;
						lp = wpf[i];
					}
				}

				//get line with slope bfm through lp
				// bfb = y-bfm*x
				bfb = lp.mV[1]-bfm*lp.mV[0];

				//calculate error
				mShadowError.mV[j] = 0.f;

				for (U32 i = 0; i < wpf.size(); ++i)
				{
					F32 lx = (wpf[i].mV[1]-bfb)/bfm;
					mShadowError.mV[j] += fabsf(wpf[i].mV[0]-lx);
				}

				mShadowError.mV[j] /= wpf.size();
				mShadowError.mV[j] /= size.mV[0];

				if (mShadowError.mV[j] > RenderShadowErrorCutoff)
				{ //just use ortho projection
					mShadowFOV.mV[j] = -1.f;
					origin.clearVec();
					proj[j] = gl_ortho(min.mV[0], max.mV[0],
										min.mV[1], max.mV[1],
										-max.mV[2], -min.mV[2]);
				}
				else
				{
					//origin is where line x = 0;
					origin.setVec(0,bfb,0);

					F32 fovz = 1.f;
					F32 fovx = 1.f;
				
					LLVector3 zp;
					LLVector3 xp;

					for (U32 i = 0; i < wpf.size(); ++i)
					{
						LLVector3 atz = wpf[i]-origin;
						atz.mV[0] = 0.f;
						atz.normVec();
						if (fovz > -atz.mV[1])
						{
							zp = wpf[i];
							fovz = -atz.mV[1];
						}
					
						LLVector3 atx = wpf[i]-origin;
						atx.mV[2] = 0.f;
						atx.normVec();
						if (fovx > -atx.mV[1])
						{
							fovx = -atx.mV[1];
							xp = wpf[i];
						}
					}

					fovx = acos(fovx);
					fovz = acos(fovz);

					F32 cutoff = llmin((F32) RenderShadowFOVCutoff, 1.4f);
				
					mShadowFOV.mV[j] = fovx;
				
					if (fovx < cutoff && fovz > cutoff)
					{
						//x is a good fit, but z is too big, move away from zp enough so that fovz matches cutoff
						F32 d = zp.mV[2]/tan(cutoff);
						F32 ny = zp.mV[1] + fabsf(d);

						origin.mV[1] = ny;

						fovz = 1.f;
						fovx = 1.f;

						for (U32 i = 0; i < wpf.size(); ++i)
						{
							LLVector3 atz = wpf[i]-origin;
							atz.mV[0] = 0.f;
							atz.normVec();
							fovz = llmin(fovz, -atz.mV[1]);

							LLVector3 atx = wpf[i]-origin;
							atx.mV[2] = 0.f;
							atx.normVec();
							fovx = llmin(fovx, -atx.mV[1]);
						}

						fovx = acos(fovx);
						fovz = acos(fovz);

						mShadowFOV.mV[j] = cutoff;
					}

				
					origin += center;
			
					F32 ynear = -(max.mV[1]-origin.mV[1]);
					F32 yfar = -(min.mV[1]-origin.mV[1]);
				
					if (ynear < 0.1f) //keep a sensible near clip plane
					{
						F32 diff = 0.1f-ynear;
						origin.mV[1] += diff;
						ynear += diff;
						yfar += diff;
					}
								
					if (fovx > cutoff)
					{ //just use ortho projection
						origin.clearVec();
						mShadowError.mV[j] = -1.f;
						proj[j] = gl_ortho(min.mV[0], max.mV[0],
								min.mV[1], max.mV[1],
								-max.mV[2], -min.mV[2]);
					}
					else
					{
						//get perspective projection
						view[j] = view[j].inverse();

						glh::vec3f origin_agent(origin.mV);
					
						//translate view to origin
						view[j].mult_matrix_vec(origin_agent);

						eye = LLVector3(origin_agent.v);

						if (!hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
						{
							mShadowFrustOrigin[j] = eye;
						}
				
						view[j] = look(LLVector3(origin_agent.v), lightDir, -up);

						F32 fx = 1.f/tanf(fovx);
						F32 fz = 1.f/tanf(fovz);

						proj[j] = glh::matrix4f(-fx, 0, 0, 0,
												0, (yfar+ynear)/(ynear-yfar), 0, (2.f*yfar*ynear)/(ynear-yfar),
												0, 0, -fz, 0,
												0, -1.f, 0, 0);
					}
				}
			}

			//shadow_cam.setFar(128.f);
			shadow_cam.setOriginAndLookAt(eye, up, center);

			shadow_cam.setOrigin(0,0,0);

			set_current_modelview(view[j]);
			set_current_projection(proj[j]);

			LLViewerCamera::updateFrustumPlanes(shadow_cam, FALSE, FALSE, TRUE);

			//shadow_cam.ignoreAgentFrustumPlane(LLCamera::AGENT_PLANE_NEAR);
			shadow_cam.getAgentPlane(LLCamera::AGENT_PLANE_NEAR).set(shadow_near_clip);

			//translate and scale to from [-1, 1] to [0, 1]
			glh::matrix4f trans(0.5f, 0.f, 0.f, 0.5f,
							0.f, 0.5f, 0.f, 0.5f,
							0.f, 0.f, 0.5f, 0.5f,
							0.f, 0.f, 0.f, 1.f);

			set_current_modelview(view[j]);
			set_current_projection(proj[j]);

			for (U32 i = 0; i < 16; i++)
			{
				gGLLastModelView[i] = mShadowModelview[j].m[i];
				gGLLastProjection[i] = mShadowProjection[j].m[i];
			}

			mShadowModelview[j] = view[j];
			mShadowProjection[j] = proj[j];
			mSunShadowMatrix[j] = trans*proj[j]*view[j]*inv_view;
		
			stop_glerror();

			mShadow[j].bindTarget();
			mShadow[j].getViewport(gGLViewport);
			mShadow[j].clear();
		
			U32 target_width = mShadow[j].getWidth();

			{
				static LLCullResult result[4];
				renderShadow(view[j], proj[j], shadow_cam, result[j], TRUE, FALSE, target_width);
			}

			mShadow[j].flush();
 
			if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA))
			{
				mShadowCamera[j+4] = shadow_cam;
			}
		}
	}

	
	//hack to disable projector shadows 
	bool gen_shadow = RenderShadowDetail > 1;

	if (gen_shadow)
	{
		LLTrace::CountStatHandle<>* velocity_stat = LLViewerCamera::getVelocityStat();
		F32 fade_amt = gFrameIntervalSeconds.value() 
			* llmax(LLTrace::get_frame_recording().getLastRecording().getSum(*velocity_stat) / LLTrace::get_frame_recording().getLastRecording().getDuration().value(), 1.0);

		//update shadow targets
		for (U32 i = 0; i < 2; i++)
		{ //for each current shadow
			LLViewerCamera::sCurCameraID = (LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SHADOW4+i);

			if (mShadowSpotLight[i].notNull() && 
				(mShadowSpotLight[i] == mTargetShadowSpotLight[0] ||
				mShadowSpotLight[i] == mTargetShadowSpotLight[1]))
			{ //keep this spotlight
				mSpotLightFade[i] = llmin(mSpotLightFade[i]+fade_amt, 1.f);
			}
			else
			{ //fade out this light
				mSpotLightFade[i] = llmax(mSpotLightFade[i]-fade_amt, 0.f);
				
				if (mSpotLightFade[i] == 0.f || mShadowSpotLight[i].isNull())
				{ //faded out, grab one of the pending spots (whichever one isn't already taken)
					if (mTargetShadowSpotLight[0] != mShadowSpotLight[(i+1)%2])
					{
						mShadowSpotLight[i] = mTargetShadowSpotLight[0];
					}
					else
					{
						mShadowSpotLight[i] = mTargetShadowSpotLight[1];
					}
				}
			}
		}

		for (S32 i = 0; i < 2; i++)
		{
			set_current_modelview(saved_view);
			set_current_projection(saved_proj);

			if (mShadowSpotLight[i].isNull())
			{
				continue;
			}

			LLVOVolume* volume = mShadowSpotLight[i]->getVOVolume();

			if (!volume)
			{
				mShadowSpotLight[i] = NULL;
				continue;
			}

			LLDrawable* drawable = mShadowSpotLight[i];

			LLVector3 params = volume->getSpotLightParams();
			F32 fov = params.mV[0];

			//get agent->light space matrix (modelview)
			LLVector3 center = drawable->getPositionAgent();
			LLQuaternion quat = volume->getRenderRotation();

			//get near clip plane
			LLVector3 scale = volume->getScale();
			LLVector3 at_axis(0,0,-scale.mV[2]*0.5f);
			at_axis *= quat;

			LLVector3 np = center+at_axis;
			at_axis.normVec();

			//get origin that has given fov for plane np, at_axis, and given scale
			F32 dist = (scale.mV[1]*0.5f)/tanf(fov*0.5f);

			LLVector3 origin = np - at_axis*dist;

			LLMatrix4 mat(quat, LLVector4(origin, 1.f));

			view[i+4] = glh::matrix4f((F32*) mat.mMatrix);

			view[i+4] = view[i+4].inverse();

			//get perspective matrix
			F32 near_clip = dist+0.01f;
			F32 width = scale.mV[VX];
			F32 height = scale.mV[VY];
			F32 far_clip = dist+volume->getLightRadius()*1.5f;

			F32 fovy = fov * RAD_TO_DEG;
			F32 aspect = width/height;
			
			proj[i+4] = gl_perspective(fovy, aspect, near_clip, far_clip);

			//translate and scale to from [-1, 1] to [0, 1]
			glh::matrix4f trans(0.5f, 0.f, 0.f, 0.5f,
							0.f, 0.5f, 0.f, 0.5f,
							0.f, 0.f, 0.5f, 0.5f,
							0.f, 0.f, 0.f, 1.f);

			set_current_modelview(view[i+4]);
			set_current_projection(proj[i+4]);

			mSunShadowMatrix[i+4] = trans*proj[i+4]*view[i+4]*inv_view;
			
			for (U32 j = 0; j < 16; j++)
			{
				gGLLastModelView[j] = mShadowModelview[i+4].m[j];
				gGLLastProjection[j] = mShadowProjection[i+4].m[j];
			}

			mShadowModelview[i+4] = view[i+4];
			mShadowProjection[i+4] = proj[i+4];

			LLCamera shadow_cam = camera;
			shadow_cam.setFar(far_clip);
			shadow_cam.setOrigin(origin);

			LLViewerCamera::updateFrustumPlanes(shadow_cam, FALSE, FALSE, TRUE);

			stop_glerror();

			mShadow[i+4].bindTarget();
			mShadow[i+4].getViewport(gGLViewport);
			mShadow[i+4].clear();

			U32 target_width = mShadow[i+4].getWidth();

			static LLCullResult result[2];

			LLViewerCamera::sCurCameraID = (LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SHADOW0 + i + 4);

            RenderSpotLight = drawable;            

			renderShadow(view[i+4], proj[i+4], shadow_cam, result[i], FALSE, FALSE, target_width);

            RenderSpotLight = nullptr;

			mShadow[i+4].flush();
 		}
	}
	else
	{ //no spotlight shadows
		mShadowSpotLight[0] = mShadowSpotLight[1] = NULL;
	}


	if (!CameraOffset)
	{
		set_current_modelview(saved_view);
		set_current_projection(saved_proj);
	}
	else
	{
		set_current_modelview(view[1]);
		set_current_projection(proj[1]);
		gGL.loadMatrix(view[1].m);
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.loadMatrix(proj[1].m);
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
	gGL.setColorMask(true, false);

	for (U32 i = 0; i < 16; i++)
	{
		gGLLastModelView[i] = last_modelview[i];
		gGLLastProjection[i] = last_projection[i];
	}

	popRenderTypeMask();

	if (!skip_avatar_update)
	{
		gAgentAvatarp->updateAttachmentVisibility(gAgentCamera.getCameraMode());
	}
}

void LLPipeline::renderGroups(LLRenderPass* pass, U32 type, U32 mask, bool texture)
{
	for (LLCullResult::sg_iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() &&
			(!sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) &&
			gPipeline.hasRenderType(group->getSpatialPartition()->mDrawableType) &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			pass->renderGroup(group,type,mask,texture);
		}
	}
}

void LLPipeline::renderRiggedGroups(LLRenderPass* pass, U32 type, U32 mask, bool texture)
{
    for (LLCullResult::sg_iterator i = sCull->beginVisibleGroups(); i != sCull->endVisibleGroups(); ++i)
    {
        LLSpatialGroup* group = *i;
        if (!group->isDead() &&
            (!sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) &&
            gPipeline.hasRenderType(group->getSpatialPartition()->mDrawableType) &&
            group->mDrawMap.find(type) != group->mDrawMap.end())
        {
            pass->renderRiggedGroup(group, type, mask, texture);
        }
    }
}

static LLTrace::BlockTimerStatHandle FTM_GENERATE_IMPOSTOR("Generate Impostor");

void LLPipeline::generateImpostor(LLVOAvatar* avatar, bool preview_avatar)
{
    LL_RECORD_BLOCK_TIME(FTM_GENERATE_IMPOSTOR);
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();

	static LLCullResult result;
	result.clear();
	grabReferences(result);
	
	if (!avatar || !avatar->mDrawable)
	{
        LL_WARNS_ONCE("AvatarRenderPipeline") << "Avatar is " << (avatar ? "not drawable" : "null") << LL_ENDL;
		return;
	}
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " is drawable" << LL_ENDL;

	assertInitialized();

    // previews can't be muted or impostered
	bool visually_muted = !preview_avatar && avatar->isVisuallyMuted();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( visually_muted ? "" : "not ") << "visually muted"
                              << LL_ENDL;
	bool too_complex = !preview_avatar && avatar->isTooComplex();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( too_complex ? "" : "not ") << "too complex"
                              << LL_ENDL;

	pushRenderTypeMask();
	
	if (visually_muted || too_complex)
	{
        // only show jelly doll geometry
		andRenderTypeMask(LLPipeline::RENDER_TYPE_AVATAR,
							LLPipeline::RENDER_TYPE_CONTROL_AV,
							END_RENDER_TYPES);
	}
	else
	{
        //hide world geometry
        clearRenderTypeMask(
            RENDER_TYPE_SKY,
            RENDER_TYPE_WL_SKY,
            RENDER_TYPE_GROUND,
            RENDER_TYPE_TERRAIN,
            RENDER_TYPE_GRASS,
            RENDER_TYPE_CONTROL_AV, // Animesh
            RENDER_TYPE_TREE,
            RENDER_TYPE_VOIDWATER,
            RENDER_TYPE_WATER,
            RENDER_TYPE_PASS_GRASS,
            RENDER_TYPE_HUD,
            RENDER_TYPE_PARTICLES,
            RENDER_TYPE_CLOUDS,
            RENDER_TYPE_HUD_PARTICLES,
            END_RENDER_TYPES
         );
	}
	
	S32 occlusion = sUseOcclusion;
	sUseOcclusion = 0;

	sReflectionRender = ! sRenderDeferred;

	sShadowRender = true;
	sImpostorRender = true;

	LLViewerCamera* viewer_camera = LLViewerCamera::getInstance();

	{
		markVisible(avatar->mDrawable, *viewer_camera);

        if (preview_avatar)
        {
            // Only show rigged attachments for preview
            // For the sake of performance and so that static
            // objects won't obstruct previewing changes
            LLVOAvatar::attachment_map_t::iterator iter;
            for (iter = avatar->mAttachmentPoints.begin();
                iter != avatar->mAttachmentPoints.end();
                ++iter)
            {
                LLViewerJointAttachment *attachment = iter->second;
                for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                    attachment_iter != attachment->mAttachedObjects.end();
                    ++attachment_iter)
                {
                    LLViewerObject* attached_object = attachment_iter->get();
                    if (attached_object)
                    {
                        if (attached_object->isRiggedMesh())
                        {
                            markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                        }
                        else
                        {
                            // sometimes object is a linkset and rigged mesh is a child
                            LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
                            for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
                                iter != child_list.end(); iter++)
                            {
                                LLViewerObject* child = *iter;
                                if (child->isRiggedMesh())
                                {
                                    markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            LLVOAvatar::attachment_map_t::iterator iter;
            for (iter = avatar->mAttachmentPoints.begin();
                iter != avatar->mAttachmentPoints.end();
                ++iter)
            {
                LLViewerJointAttachment *attachment = iter->second;
                for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                    attachment_iter != attachment->mAttachedObjects.end();
                    ++attachment_iter)
                {
                    LLViewerObject* attached_object = attachment_iter->get();
                    if (attached_object)
                    {
                        markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    }
                }
            }
        }
	}

	stateSort(*LLViewerCamera::getInstance(), result);
	
	LLCamera camera = *viewer_camera;
	LLVector2 tdim;
	U32 resY = 0;
	U32 resX = 0;

    if (!preview_avatar)
	{
		const LLVector4a* ext = avatar->mDrawable->getSpatialExtents();
		LLVector3 pos(avatar->getRenderPosition()+avatar->getImpostorOffset());

		camera.lookAt(viewer_camera->getOrigin(), pos, viewer_camera->getUpAxis());
	
		LLVector4a half_height;
		half_height.setSub(ext[1], ext[0]);
		half_height.mul(0.5f);

		LLVector4a left;
		left.load3(camera.getLeftAxis().mV);
		left.mul(left);
		llassert(left.dot3(left).getF32() > F_APPROXIMATELY_ZERO);
		left.normalize3fast();

		LLVector4a up;
		up.load3(camera.getUpAxis().mV);
		up.mul(up);
		llassert(up.dot3(up).getF32() > F_APPROXIMATELY_ZERO);
		up.normalize3fast();

		tdim.mV[0] = fabsf(half_height.dot3(left).getF32());
		tdim.mV[1] = fabsf(half_height.dot3(up).getF32());

		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.pushMatrix();
	
		F32 distance = (pos-camera.getOrigin()).length();
		F32 fov = atanf(tdim.mV[1]/distance)*2.f*RAD_TO_DEG;
		F32 aspect = tdim.mV[0]/tdim.mV[1];
		glh::matrix4f persp = gl_perspective(fov, aspect, 1.f, 256.f);
		set_current_projection(persp);
		gGL.loadMatrix(persp.m);

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		glh::matrix4f mat;
		camera.getOpenGLTransform(mat.m);

		mat = glh::matrix4f((GLfloat*) OGL_TO_CFR_ROTATION) * mat;

		gGL.loadMatrix(mat.m);
		set_current_modelview(mat);

		glClearColor(0.0f,0.0f,0.0f,0.0f);
		gGL.setColorMask(true, true);
	
		// get the number of pixels per angle
		F32 pa = gViewerWindow->getWindowHeightRaw() / (RAD_TO_DEG * viewer_camera->getView());

		//get resolution based on angle width and height of impostor (double desired resolution to prevent aliasing)
		resY = llmin(nhpo2((U32) (fov*pa)), (U32) 512);
		resX = llmin(nhpo2((U32) (atanf(tdim.mV[0]/distance)*2.f*RAD_TO_DEG*pa)), (U32) 512);

		if (!avatar->mImpostor.isComplete())
		{
            avatar->mImpostor.allocate(resX, resY, GL_RGBA, TRUE, FALSE);

			if (LLPipeline::sRenderDeferred)
			{
				addDeferredAttachments(avatar->mImpostor, true);
			}
		
			gGL.getTexUnit(0)->bind(&avatar->mImpostor);
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
		else if(resX != avatar->mImpostor.getWidth() || resY != avatar->mImpostor.getHeight())
		{
			avatar->mImpostor.resize(resX,resY);
		}

		avatar->mImpostor.bindTarget();
	}

	F32 old_alpha = LLDrawPoolAvatar::sMinimumAlpha;

	if (visually_muted || too_complex)
	{ //disable alpha masking for muted avatars (get whole skin silhouette)
		LLDrawPoolAvatar::sMinimumAlpha = 0.f;
	}

    if (preview_avatar)
    {
        // previews don't care about imposters
        if (LLPipeline::sRenderDeferred)
        {
            renderGeomDeferred(camera);
            renderGeomPostDeferred(camera);
        }
        else
        {
            renderGeom(camera);
        }
    }
    else if (LLPipeline::sRenderDeferred)
	{
		avatar->mImpostor.clear();
		renderGeomDeferred(camera);

		renderGeomPostDeferred(camera);		

		// Shameless hack time: render it all again,
		// this time writing the depth
		// values we need to generate the alpha mask below
		// while preserving the alpha-sorted color rendering
		// from the previous pass
		//
		sImpostorRenderAlphaDepthPass = true;
		// depth-only here...
		//
		gGL.setColorMask(false,false);
		renderGeomPostDeferred(camera);

		sImpostorRenderAlphaDepthPass = false;

	}
	else
	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, resX, resY);
		avatar->mImpostor.clear();
		renderGeom(camera);

		// Shameless hack time: render it all again,
		// this time writing the depth
		// values we need to generate the alpha mask below
		// while preserving the alpha-sorted color rendering
		// from the previous pass
		//
		sImpostorRenderAlphaDepthPass = true;

		// depth-only here...
		//
		gGL.setColorMask(false,false);
		renderGeom(camera);

		sImpostorRenderAlphaDepthPass = false;
	}

	LLDrawPoolAvatar::sMinimumAlpha = old_alpha;

	{ //create alpha mask based on depth buffer (grey out if muted)
		if (LLPipeline::sRenderDeferred)
		{
			GLuint buff = GL_COLOR_ATTACHMENT0;
			LL_PROFILER_GPU_ZONEC( "gl.DrawBuffersARB", 0x8000FF );
			glDrawBuffersARB(1, &buff);
		}

		LLGLDisable blend(GL_BLEND);

		if (visually_muted || too_complex)
		{
			gGL.setColorMask(true, true);
		}
		else
		{
			gGL.setColorMask(false, true);
		}
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);

		gGL.flush();

		gGL.pushMatrix();
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.pushMatrix();
		gGL.loadIdentity();

		static const F32 clip_plane = 0.99999f;

		gDebugProgram.bind();

		if (visually_muted)
		{	// Visually muted avatar
            LLColor4 muted_color(avatar->getMutedAVColor());
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set solid color " << muted_color << LL_ENDL;
			gGL.diffuseColor4fv( muted_color.mV );
		}
		else if (!preview_avatar)
		{ //grey muted avatar
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set grey" << LL_ENDL;
			gGL.diffuseColor4fv(LLColor4::pink.mV );
		}

		gGL.begin(LLRender::QUADS);
		gGL.vertex3f(-1, -1, clip_plane);
		gGL.vertex3f(1, -1, clip_plane);
		gGL.vertex3f(1, 1, clip_plane);
		gGL.vertex3f(-1, 1, clip_plane);
		gGL.end();
		gGL.flush();

		gDebugProgram.unbind();

		gGL.popMatrix();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.popMatrix();
	}

    if (!preview_avatar)
    {
        avatar->mImpostor.flush();
        avatar->setImpostorDim(tdim);
    }

	sUseOcclusion = occlusion;
	sReflectionRender = false;
	sImpostorRender = false;
	sShadowRender = false;
	popRenderTypeMask();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

    if (!preview_avatar)
    {
        avatar->mNeedsImpostorUpdate = FALSE;
        avatar->cacheImpostorValues();
        avatar->mLastImpostorUpdateFrameTime = gFrameTimeSeconds;
    }

	LLVertexBuffer::unbind();
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
}

bool LLPipeline::hasRenderBatches(const U32 type) const
{
	return sCull->getRenderMapSize(type) > 0;
}

LLCullResult::drawinfo_iterator LLPipeline::beginRenderMap(U32 type)
{
	return sCull->beginRenderMap(type);
}

LLCullResult::drawinfo_iterator LLPipeline::endRenderMap(U32 type)
{
	return sCull->endRenderMap(type);
}

LLCullResult::sg_iterator LLPipeline::beginAlphaGroups()
{
	return sCull->beginAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::endAlphaGroups()
{
	return sCull->endAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::beginRiggedAlphaGroups()
{
    return sCull->beginRiggedAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::endRiggedAlphaGroups()
{
    return sCull->endRiggedAlphaGroups();
}

bool LLPipeline::hasRenderType(const U32 type) const
{
    // STORM-365 : LLViewerJointAttachment::setAttachmentVisibility() is setting type to 0 to actually mean "do not render"
    // We then need to test that value here and return false to prevent attachment to render (in mouselook for instance)
    // TODO: reintroduce RENDER_TYPE_NONE in LLRenderTypeMask and initialize its mRenderTypeEnabled[RENDER_TYPE_NONE] to false explicitely
	return (type == 0 ? false : mRenderTypeEnabled[type]);
}

void LLPipeline::setRenderTypeMask(U32 type, ...)
{
	va_list args;

	va_start(args, type);
	while (type < END_RENDER_TYPES)
	{
		mRenderTypeEnabled[type] = true;
		type = va_arg(args, U32);
	}
	va_end(args);

	if (type > END_RENDER_TYPES)
	{
		LL_ERRS() << "Invalid render type." << LL_ENDL;
	}
}

bool LLPipeline::hasAnyRenderType(U32 type, ...) const
{
	va_list args;

	va_start(args, type);
	while (type < END_RENDER_TYPES)
	{
		if (mRenderTypeEnabled[type])
		{
			return true;
		}
		type = va_arg(args, U32);
	}
	va_end(args);

	if (type > END_RENDER_TYPES)
	{
		LL_ERRS() << "Invalid render type." << LL_ENDL;
	}

	return false;
}

void LLPipeline::pushRenderTypeMask()
{
	std::string cur_mask;
	cur_mask.assign((const char*) mRenderTypeEnabled, sizeof(mRenderTypeEnabled));
	mRenderTypeEnableStack.push(cur_mask);
}

void LLPipeline::popRenderTypeMask()
{
	if (mRenderTypeEnableStack.empty())
	{
		LL_ERRS() << "Depleted render type stack." << LL_ENDL;
	}

	memcpy(mRenderTypeEnabled, mRenderTypeEnableStack.top().data(), sizeof(mRenderTypeEnabled));
	mRenderTypeEnableStack.pop();
}

void LLPipeline::andRenderTypeMask(U32 type, ...)
{
	va_list args;

	bool tmp[NUM_RENDER_TYPES];
	for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
	{
		tmp[i] = false;
	}

	va_start(args, type);
	while (type < END_RENDER_TYPES)
	{
		if (mRenderTypeEnabled[type]) 
		{
			tmp[type] = true;
		}

		type = va_arg(args, U32);
	}
	va_end(args);

	if (type > END_RENDER_TYPES)
	{
		LL_ERRS() << "Invalid render type." << LL_ENDL;
	}

	for (U32 i = 0; i < LLPipeline::NUM_RENDER_TYPES; ++i)
	{
		mRenderTypeEnabled[i] = tmp[i];
	}

}

void LLPipeline::clearRenderTypeMask(U32 type, ...)
{
	va_list args;

	va_start(args, type);
	while (type < END_RENDER_TYPES)
	{
		mRenderTypeEnabled[type] = false;
		
		type = va_arg(args, U32);
	}
	va_end(args);

	if (type > END_RENDER_TYPES)
	{
		LL_ERRS() << "Invalid render type." << LL_ENDL;
	}
}

void LLPipeline::setAllRenderTypes()
{
	for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
	{
		mRenderTypeEnabled[i] = true;
	}
}

void LLPipeline::clearAllRenderTypes()
{
	for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
	{
		mRenderTypeEnabled[i] = false;
	}
}

void LLPipeline::addDebugBlip(const LLVector3& position, const LLColor4& color)
{
	DebugBlip blip(position, color);
	mDebugBlips.push_back(blip);
}

void LLPipeline::hidePermanentObjects( std::vector<U32>& restoreList )
{
	//This method is used to hide any vo's from the object list that may have
	//the permanent flag set.
	
	U32 objCnt = gObjectList.getNumObjects();
	for (U32 i = 0; i < objCnt; ++i)
	{
		LLViewerObject* pObject = gObjectList.getObject(i);
		if ( pObject && pObject->flagObjectPermanent() )
		{
			LLDrawable *pDrawable = pObject->mDrawable;
		
			if ( pDrawable )
			{
				restoreList.push_back( i );
				hideDrawable( pDrawable );			
			}
		}
	}

	skipRenderingOfTerrain( true );
}

void LLPipeline::restorePermanentObjects( const std::vector<U32>& restoreList )
{
	//This method is used to restore(unhide) any vo's from the object list that may have
	//been hidden because their permanency flag was set.

	std::vector<U32>::const_iterator itCurrent	= restoreList.begin();
	std::vector<U32>::const_iterator itEnd		= restoreList.end();
	
	U32 objCnt = gObjectList.getNumObjects();

	while ( itCurrent != itEnd )
	{
		U32 index = *itCurrent;
		LLViewerObject* pObject = NULL;
		if ( index < objCnt ) 
		{
			pObject = gObjectList.getObject( index );
		}
		if ( pObject )
		{
			LLDrawable *pDrawable = pObject->mDrawable;
			if ( pDrawable )
			{
				pDrawable->clearState( LLDrawable::FORCE_INVISIBLE );
				unhideDrawable( pDrawable );				
			}
		}
		++itCurrent;
	}
	
	skipRenderingOfTerrain( false );
}

void LLPipeline::skipRenderingOfTerrain( bool flag )
{
	pool_set_t::iterator iter = mPools.begin();
	while ( iter != mPools.end() )
	{
		LLDrawPool* pPool = *iter;		
		U32 poolType = pPool->getType();					
		if ( hasRenderType( pPool->getType() ) && poolType == LLDrawPool::POOL_TERRAIN )
		{
			pPool->setSkipRenderFlag( flag );			
		}
		++iter;
	}
}

void LLPipeline::hideObject( const LLUUID& id )
{
	LLViewerObject *pVO = gObjectList.findObject( id );
	
	if ( pVO )
	{
		LLDrawable *pDrawable = pVO->mDrawable;
		
		if ( pDrawable )
		{
			hideDrawable( pDrawable );		
		}		
	}
}

void LLPipeline::hideDrawable( LLDrawable *pDrawable )
{
	pDrawable->setState( LLDrawable::FORCE_INVISIBLE );
	markRebuild( pDrawable, LLDrawable::REBUILD_ALL, TRUE );
	//hide the children
	LLViewerObject::const_child_list_t& child_list = pDrawable->getVObj()->getChildren();
	for ( LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		  iter != child_list.end(); iter++ )
	{
		LLViewerObject* child = *iter;
		LLDrawable* drawable = child->mDrawable;					
		if ( drawable )
		{
			drawable->setState( LLDrawable::FORCE_INVISIBLE );
			markRebuild( drawable, LLDrawable::REBUILD_ALL, TRUE );
		}
	}
}
void LLPipeline::unhideDrawable( LLDrawable *pDrawable )
{
	pDrawable->clearState( LLDrawable::FORCE_INVISIBLE );
	markRebuild( pDrawable, LLDrawable::REBUILD_ALL, TRUE );
	//restore children
	LLViewerObject::const_child_list_t& child_list = pDrawable->getVObj()->getChildren();
	for ( LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		  iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;
		LLDrawable* drawable = child->mDrawable;					
		if ( drawable )
		{
			drawable->clearState( LLDrawable::FORCE_INVISIBLE );
			markRebuild( drawable, LLDrawable::REBUILD_ALL, TRUE );
		}
	}
}
void LLPipeline::restoreHiddenObject( const LLUUID& id )
{
	LLViewerObject *pVO = gObjectList.findObject( id );
	
	if ( pVO )
	{
		LLDrawable *pDrawable = pVO->mDrawable;
		if ( pDrawable )
		{
			unhideDrawable( pDrawable );			
		}
	}
}

