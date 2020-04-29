/** 
 * @file llviewershadermgr.cpp
 * @brief Viewer shader manager implementation.
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

#include <boost/lexical_cast.hpp>

#include "llfeaturemanager.h"
#include "llviewershadermgr.h"
#include "llviewercontrol.h"

#include "llrender.h"
#include "llenvironment.h"
#include "llatmosphere.h"
#include "llworld.h"
#include "llsky.h"
#include "llvosky.h"

#include "pipeline.h"

#include "llfile.h"
#include "llviewerwindow.h"
#include "llwindow.h"

#include "lljoint.h"
#include "llskinningutil.h"

static LLStaticHashedString sTexture0("texture0");
static LLStaticHashedString sTexture1("texture1");
static LLStaticHashedString sTex0("tex0");
static LLStaticHashedString sTex1("tex1");
static LLStaticHashedString sDitherTex("dither_tex");
static LLStaticHashedString sGlowMap("glowMap");
static LLStaticHashedString sScreenMap("screenMap");

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

BOOL				LLViewerShaderMgr::sInitialized = FALSE;
bool				LLViewerShaderMgr::sSkipReload = false;

LLVector4			gShinyOrigin;

//transform shaders
LLGLSLShader			gTransformPositionProgram;
LLGLSLShader			gTransformTexCoordProgram;
LLGLSLShader			gTransformNormalProgram;
LLGLSLShader			gTransformColorProgram;
LLGLSLShader			gTransformTangentProgram;

//utility shaders
LLGLSLShader	gOcclusionProgram;
LLGLSLShader	gOcclusionCubeProgram;
LLGLSLShader	gCustomAlphaProgram;
LLGLSLShader	gGlowCombineProgram;
LLGLSLShader	gSplatTextureRectProgram;
LLGLSLShader	gGlowCombineFXAAProgram;
LLGLSLShader	gTwoTextureAddProgram;
LLGLSLShader	gTwoTextureCompareProgram;
LLGLSLShader	gOneTextureFilterProgram;
LLGLSLShader	gOneTextureNoColorProgram;
LLGLSLShader	gDebugProgram;
LLGLSLShader	gClipProgram;
LLGLSLShader	gDownsampleDepthProgram;
LLGLSLShader	gDownsampleDepthRectProgram;
LLGLSLShader	gAlphaMaskProgram;
LLGLSLShader	gBenchmarkProgram;


//object shaders
LLGLSLShader		gObjectSimpleProgram;
LLGLSLShader		gObjectSimpleImpostorProgram;
LLGLSLShader		gObjectPreviewProgram;
LLGLSLShader		gObjectSimpleWaterProgram;
LLGLSLShader		gObjectSimpleAlphaMaskProgram;
LLGLSLShader		gObjectSimpleWaterAlphaMaskProgram;
LLGLSLShader		gObjectFullbrightProgram;
LLGLSLShader		gObjectFullbrightWaterProgram;
LLGLSLShader		gObjectEmissiveProgram;
LLGLSLShader		gObjectEmissiveWaterProgram;
LLGLSLShader		gObjectFullbrightAlphaMaskProgram;
LLGLSLShader		gObjectFullbrightWaterAlphaMaskProgram;
LLGLSLShader		gObjectFullbrightShinyProgram;
LLGLSLShader		gObjectFullbrightShinyWaterProgram;
LLGLSLShader		gObjectShinyProgram;
LLGLSLShader		gObjectShinyWaterProgram;
LLGLSLShader		gObjectBumpProgram;
LLGLSLShader		gTreeProgram;
LLGLSLShader		gTreeWaterProgram;
LLGLSLShader		gObjectFullbrightNoColorProgram;
LLGLSLShader		gObjectFullbrightNoColorWaterProgram;

LLGLSLShader		gObjectSimpleNonIndexedProgram;
LLGLSLShader		gObjectSimpleNonIndexedTexGenProgram;
LLGLSLShader		gObjectSimpleNonIndexedTexGenWaterProgram;
LLGLSLShader		gObjectSimpleNonIndexedWaterProgram;
LLGLSLShader		gObjectAlphaMaskNonIndexedProgram;
LLGLSLShader		gObjectAlphaMaskNonIndexedWaterProgram;
LLGLSLShader		gObjectAlphaMaskNoColorProgram;
LLGLSLShader		gObjectAlphaMaskNoColorWaterProgram;
LLGLSLShader		gObjectFullbrightNonIndexedProgram;
LLGLSLShader		gObjectFullbrightNonIndexedWaterProgram;
LLGLSLShader		gObjectEmissiveNonIndexedProgram;
LLGLSLShader		gObjectEmissiveNonIndexedWaterProgram;
LLGLSLShader		gObjectFullbrightShinyNonIndexedProgram;
LLGLSLShader		gObjectFullbrightShinyNonIndexedWaterProgram;
LLGLSLShader		gObjectShinyNonIndexedProgram;
LLGLSLShader		gObjectShinyNonIndexedWaterProgram;

//object hardware skinning shaders
LLGLSLShader		gSkinnedObjectSimpleProgram;
LLGLSLShader		gSkinnedObjectFullbrightProgram;
LLGLSLShader		gSkinnedObjectEmissiveProgram;
LLGLSLShader		gSkinnedObjectFullbrightShinyProgram;
LLGLSLShader		gSkinnedObjectShinySimpleProgram;

LLGLSLShader		gSkinnedObjectSimpleWaterProgram;
LLGLSLShader		gSkinnedObjectFullbrightWaterProgram;
LLGLSLShader		gSkinnedObjectEmissiveWaterProgram;
LLGLSLShader		gSkinnedObjectFullbrightShinyWaterProgram;
LLGLSLShader		gSkinnedObjectShinySimpleWaterProgram;

//environment shaders
LLGLSLShader		gTerrainProgram;
LLGLSLShader		gTerrainWaterProgram;
LLGLSLShader		gWaterProgram;
LLGLSLShader        gWaterEdgeProgram;
LLGLSLShader		gUnderWaterProgram;

//interface shaders
LLGLSLShader		gHighlightProgram;
LLGLSLShader		gHighlightNormalProgram;
LLGLSLShader		gHighlightSpecularProgram;

LLGLSLShader		gDeferredHighlightProgram;
LLGLSLShader		gDeferredHighlightNormalProgram;
LLGLSLShader		gDeferredHighlightSpecularProgram;

LLGLSLShader		gPathfindingProgram;
LLGLSLShader		gPathfindingNoNormalsProgram;

//avatar shader handles
LLGLSLShader		gAvatarProgram;
LLGLSLShader		gAvatarWaterProgram;
LLGLSLShader		gAvatarEyeballProgram;
LLGLSLShader		gAvatarPickProgram;
LLGLSLShader		gImpostorProgram;

// WindLight shader handles
LLGLSLShader			gWLSkyProgram;
LLGLSLShader			gWLCloudProgram;
LLGLSLShader            gWLSunProgram;
LLGLSLShader            gWLMoonProgram;

// Effects Shaders
LLGLSLShader			gGlowProgram;
LLGLSLShader			gGlowExtractProgram;
LLGLSLShader			gPostColorFilterProgram;
LLGLSLShader			gPostNightVisionProgram;

// Deferred rendering shaders
LLGLSLShader			gDeferredImpostorProgram;
LLGLSLShader			gDeferredWaterProgram;
LLGLSLShader			gDeferredUnderWaterProgram;
LLGLSLShader			gDeferredDiffuseProgram;
LLGLSLShader			gDeferredDiffuseAlphaMaskProgram;
LLGLSLShader			gDeferredNonIndexedDiffuseProgram;
LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskProgram;
LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
LLGLSLShader			gDeferredSkinnedDiffuseProgram;
LLGLSLShader			gDeferredSkinnedBumpProgram;
LLGLSLShader			gDeferredSkinnedAlphaProgram;
LLGLSLShader			gDeferredBumpProgram;
LLGLSLShader			gDeferredTerrainProgram;
LLGLSLShader            gDeferredTerrainWaterProgram;
LLGLSLShader			gDeferredTreeProgram;
LLGLSLShader			gDeferredTreeShadowProgram;
LLGLSLShader			gDeferredAvatarProgram;
LLGLSLShader			gDeferredAvatarAlphaProgram;
LLGLSLShader			gDeferredLightProgram;
LLGLSLShader			gDeferredMultiLightProgram[16];
LLGLSLShader			gDeferredSpotLightProgram;
LLGLSLShader			gDeferredMultiSpotLightProgram;
LLGLSLShader			gDeferredSunProgram;
LLGLSLShader			gDeferredBlurLightProgram;
LLGLSLShader			gDeferredSoftenProgram;
LLGLSLShader			gDeferredSoftenWaterProgram;
LLGLSLShader			gDeferredShadowProgram;
LLGLSLShader			gDeferredShadowCubeProgram;
LLGLSLShader			gDeferredShadowAlphaMaskProgram;
LLGLSLShader			gDeferredShadowFullbrightAlphaMaskProgram;
LLGLSLShader			gDeferredAvatarShadowProgram;
LLGLSLShader			gDeferredAvatarAlphaShadowProgram;
LLGLSLShader			gDeferredAvatarAlphaMaskShadowProgram;
LLGLSLShader			gDeferredAttachmentShadowProgram;
LLGLSLShader			gDeferredAttachmentAlphaShadowProgram;
LLGLSLShader			gDeferredAttachmentAlphaMaskShadowProgram;
LLGLSLShader			gDeferredAlphaProgram;
LLGLSLShader			gDeferredAlphaImpostorProgram;
LLGLSLShader			gDeferredAlphaWaterProgram;
LLGLSLShader			gDeferredAvatarEyesProgram;
LLGLSLShader			gDeferredFullbrightProgram;
LLGLSLShader			gDeferredFullbrightAlphaMaskProgram;
LLGLSLShader			gDeferredFullbrightWaterProgram;
LLGLSLShader			gDeferredFullbrightAlphaMaskWaterProgram;
LLGLSLShader			gDeferredEmissiveProgram;
LLGLSLShader			gDeferredPostProgram;
LLGLSLShader			gDeferredCoFProgram;
LLGLSLShader			gDeferredDoFCombineProgram;
LLGLSLShader			gDeferredPostGammaCorrectProgram;
LLGLSLShader			gFXAAProgram;
LLGLSLShader			gDeferredPostNoDoFProgram;
LLGLSLShader			gDeferredWLSkyProgram;
LLGLSLShader			gDeferredWLCloudProgram;
LLGLSLShader			gDeferredWLSunProgram;
LLGLSLShader			gDeferredWLMoonProgram;
LLGLSLShader			gDeferredStarProgram;
LLGLSLShader			gDeferredFullbrightShinyProgram;
LLGLSLShader			gDeferredSkinnedFullbrightShinyProgram;
LLGLSLShader			gDeferredSkinnedFullbrightProgram;
LLGLSLShader			gNormalMapGenProgram;

// Deferred materials shaders
LLGLSLShader			gDeferredMaterialProgram[LLMaterial::SHADER_COUNT*2];
LLGLSLShader			gDeferredMaterialWaterProgram[LLMaterial::SHADER_COUNT*2];

LLViewerShaderMgr::LLViewerShaderMgr() :
	mShaderLevel(SHADER_COUNT, 0),
	mMaxAvatarShaderLevel(0)
{   
    /// Make sure WL Sky is the first program
    //ONLY shaders that need WL Param management should be added here
	mShaderList.push_back(&gWLSkyProgram);
	mShaderList.push_back(&gWLCloudProgram);
	mShaderList.push_back(&gWLSunProgram);
	mShaderList.push_back(&gWLMoonProgram);
	mShaderList.push_back(&gAvatarProgram);
	mShaderList.push_back(&gObjectShinyProgram);
	mShaderList.push_back(&gObjectShinyNonIndexedProgram);
	mShaderList.push_back(&gWaterProgram);
	mShaderList.push_back(&gWaterEdgeProgram);
	mShaderList.push_back(&gAvatarEyeballProgram); 
	mShaderList.push_back(&gObjectSimpleProgram);
	mShaderList.push_back(&gObjectSimpleImpostorProgram);
	mShaderList.push_back(&gObjectPreviewProgram);
	mShaderList.push_back(&gImpostorProgram);
	mShaderList.push_back(&gObjectFullbrightNoColorProgram);
	mShaderList.push_back(&gObjectFullbrightNoColorWaterProgram);
	mShaderList.push_back(&gObjectSimpleAlphaMaskProgram);
	mShaderList.push_back(&gObjectBumpProgram);
	mShaderList.push_back(&gObjectEmissiveProgram);
	mShaderList.push_back(&gObjectEmissiveWaterProgram);
	mShaderList.push_back(&gObjectFullbrightProgram);
	mShaderList.push_back(&gObjectFullbrightAlphaMaskProgram);
	mShaderList.push_back(&gObjectFullbrightShinyProgram);
	mShaderList.push_back(&gObjectFullbrightShinyWaterProgram);
	mShaderList.push_back(&gObjectSimpleNonIndexedProgram);
	mShaderList.push_back(&gObjectSimpleNonIndexedTexGenProgram);
	mShaderList.push_back(&gObjectSimpleNonIndexedTexGenWaterProgram);
	mShaderList.push_back(&gObjectSimpleNonIndexedWaterProgram);
	mShaderList.push_back(&gObjectAlphaMaskNonIndexedProgram);
	mShaderList.push_back(&gObjectAlphaMaskNonIndexedWaterProgram);
	mShaderList.push_back(&gObjectAlphaMaskNoColorProgram);
	mShaderList.push_back(&gObjectAlphaMaskNoColorWaterProgram);
	mShaderList.push_back(&gTreeProgram);
	mShaderList.push_back(&gTreeWaterProgram);
	mShaderList.push_back(&gObjectFullbrightNonIndexedProgram);
	mShaderList.push_back(&gObjectFullbrightNonIndexedWaterProgram);
	mShaderList.push_back(&gObjectEmissiveNonIndexedProgram);
	mShaderList.push_back(&gObjectEmissiveNonIndexedWaterProgram);
	mShaderList.push_back(&gObjectFullbrightShinyNonIndexedProgram);
	mShaderList.push_back(&gObjectFullbrightShinyNonIndexedWaterProgram);
	mShaderList.push_back(&gSkinnedObjectSimpleProgram);
	mShaderList.push_back(&gSkinnedObjectFullbrightProgram);
	mShaderList.push_back(&gSkinnedObjectEmissiveProgram);
	mShaderList.push_back(&gSkinnedObjectFullbrightShinyProgram);
	mShaderList.push_back(&gSkinnedObjectShinySimpleProgram);
	mShaderList.push_back(&gSkinnedObjectSimpleWaterProgram);
	mShaderList.push_back(&gSkinnedObjectFullbrightWaterProgram);
	mShaderList.push_back(&gSkinnedObjectEmissiveWaterProgram);
	mShaderList.push_back(&gSkinnedObjectFullbrightShinyWaterProgram);
	mShaderList.push_back(&gSkinnedObjectShinySimpleWaterProgram);
	mShaderList.push_back(&gTerrainProgram);
	mShaderList.push_back(&gTerrainWaterProgram);
	mShaderList.push_back(&gObjectSimpleWaterProgram);
	mShaderList.push_back(&gObjectFullbrightWaterProgram);
	mShaderList.push_back(&gObjectSimpleWaterAlphaMaskProgram);
	mShaderList.push_back(&gObjectFullbrightWaterAlphaMaskProgram);
	mShaderList.push_back(&gAvatarWaterProgram);
	mShaderList.push_back(&gObjectShinyWaterProgram);
	mShaderList.push_back(&gObjectShinyNonIndexedWaterProgram);
	mShaderList.push_back(&gUnderWaterProgram);
	mShaderList.push_back(&gDeferredSunProgram);
	mShaderList.push_back(&gDeferredSoftenProgram);
	mShaderList.push_back(&gDeferredSoftenWaterProgram);
	mShaderList.push_back(&gDeferredAlphaProgram);
	mShaderList.push_back(&gDeferredAlphaImpostorProgram);
	mShaderList.push_back(&gDeferredAlphaWaterProgram);
	mShaderList.push_back(&gDeferredSkinnedAlphaProgram);
	mShaderList.push_back(&gDeferredFullbrightProgram);
	mShaderList.push_back(&gDeferredFullbrightAlphaMaskProgram);
	mShaderList.push_back(&gDeferredFullbrightWaterProgram);
	mShaderList.push_back(&gDeferredFullbrightAlphaMaskWaterProgram);	
	mShaderList.push_back(&gDeferredFullbrightShinyProgram);
	mShaderList.push_back(&gDeferredSkinnedFullbrightShinyProgram);
	mShaderList.push_back(&gDeferredSkinnedFullbrightProgram);
	mShaderList.push_back(&gDeferredEmissiveProgram);
	mShaderList.push_back(&gDeferredAvatarEyesProgram);
	mShaderList.push_back(&gDeferredWaterProgram);
	mShaderList.push_back(&gDeferredUnderWaterProgram);	
    mShaderList.push_back(&gDeferredTerrainWaterProgram);
	mShaderList.push_back(&gDeferredAvatarAlphaProgram);
	mShaderList.push_back(&gDeferredWLSkyProgram);
	mShaderList.push_back(&gDeferredWLCloudProgram);
    mShaderList.push_back(&gDeferredWLMoonProgram);
    mShaderList.push_back(&gDeferredWLSunProgram);    
}

LLViewerShaderMgr::~LLViewerShaderMgr()
{
	mShaderLevel.clear();
	mShaderList.clear();
}

// static
LLViewerShaderMgr * LLViewerShaderMgr::instance()
{
	if(NULL == sInstance)
	{
		sInstance = new LLViewerShaderMgr();
	}

	return static_cast<LLViewerShaderMgr*>(sInstance);
}

// static
void LLViewerShaderMgr::releaseInstance()
{
	if (sInstance != NULL)
	{
		delete sInstance;
		sInstance = NULL;
	}
}

void LLViewerShaderMgr::initAttribsAndUniforms(void)
{
	if (mReservedAttribs.empty())
	{
		LLShaderMgr::initAttribsAndUniforms();
	}	
}
	

//============================================================================
// Set Levels

S32 LLViewerShaderMgr::getShaderLevel(S32 type)
{
	return LLPipeline::sDisableShaders ? 0 : mShaderLevel[type];
}

//============================================================================
// Shader Management

void LLViewerShaderMgr::setShaders()
{
    //setShaders might be called redundantly by gSavedSettings, so return on reentrance
    static bool reentrance = false;
    
    if (!gPipeline.mInitialized || !sInitialized || reentrance || sSkipReload)
    {
        return;
    }

    static LLCachedControl<U32> max_texture_index(gSavedSettings, "RenderMaxTextureIndex", 16);
    LLGLSLShader::sIndexedTextureChannels = llmax(llmin(gGLManager.mNumTextureImageUnits, (S32) max_texture_index), 1);

    //NEVER use more than 16 texture channels (work around for prevalent driver bug)
    LLGLSLShader::sIndexedTextureChannels = llmin(LLGLSLShader::sIndexedTextureChannels, 16);

    if (gGLManager.mGLSLVersionMajor < 1 ||
        (gGLManager.mGLSLVersionMajor == 1 && gGLManager.mGLSLVersionMinor <= 20))
    { //NEVER use indexed texture rendering when GLSL version is 1.20 or earlier
        LLGLSLShader::sIndexedTextureChannels = 1;
    }

    reentrance = true;

    //setup preprocessor definitions
    LLShaderMgr::instance()->mDefinitions["NUM_TEX_UNITS"] = llformat("%d", gGLManager.mNumTextureImageUnits);
    
    // Make sure the compiled shader map is cleared before we recompile shaders.
    mVertexShaderObjects.clear();
    mFragmentShaderObjects.clear();
    
    initAttribsAndUniforms();
    gPipeline.releaseGLBuffers();

    LLPipeline::sWaterReflections = gGLManager.mHasCubeMap;
    LLPipeline::sRenderGlow = gSavedSettings.getBOOL("RenderGlow"); 
    LLPipeline::updateRenderDeferred();
    
    //hack to reset buffers that change behavior with shaders
    gPipeline.resetVertexBuffers();

    if (gViewerWindow)
    {
        gViewerWindow->setCursor(UI_CURSOR_WAIT);
    }

    // Lighting
    gPipeline.setLightingDetail(-1);

    // Shaders
    LL_INFOS("ShaderLoading") << "\n~~~~~~~~~~~~~~~~~~\n Loading Shaders:\n~~~~~~~~~~~~~~~~~~" << LL_ENDL;
    LL_INFOS("ShaderLoading") << llformat("Using GLSL %d.%d", gGLManager.mGLSLVersionMajor, gGLManager.mGLSLVersionMinor) << LL_ENDL;

    for (S32 i = 0; i < SHADER_COUNT; i++)
    {
        mShaderLevel[i] = 0;
    }
    mMaxAvatarShaderLevel = 0;

    LLGLSLShader::sNoFixedFunction = false;
    LLVertexBuffer::unbind();

    llassert((gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 10));

    bool canRenderDeferred       = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred");
    bool hasWindLightShaders     = LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders");
    S32 shadow_detail            = gSavedSettings.getS32("RenderShadowDetail");
    bool useRenderDeferred       = canRenderDeferred && gSavedSettings.getBOOL("RenderDeferred") && gSavedSettings.getBOOL("RenderAvatarVP");
    bool doingWindLight          = hasWindLightShaders && gSavedSettings.getBOOL("WindLightUseAtmosShaders");

    //using shaders, disable fixed function
    LLGLSLShader::sNoFixedFunction = true;

    S32 light_class = 3;
    S32 interface_class = 2;
    S32 env_class = 2;
    S32 obj_class = 2;
    S32 effect_class = 2;
    S32 wl_class = 1;
    S32 water_class = 2;
    S32 deferred_class = 0;
    S32 transform_class = gGLManager.mHasTransformFeedback ? 1 : 0;

    static LLCachedControl<bool> use_transform_feedback(gSavedSettings, "RenderUseTransformFeedback", false);
    if (!use_transform_feedback)
    {
        transform_class = 0;
    }

    if (useRenderDeferred)
    {
        //shadows
        switch (shadow_detail)
        {                
            case 1:
                deferred_class = 2; // PCF shadows
            break; 

            case 2:
                deferred_class = 2; // PCF shadows
            break; 

            case 0: 
            default:
                deferred_class = 1; // no shadows
            break; 
        }
    }

    if (doingWindLight)
    {
        // user has disabled WindLight in their settings, downgrade
        // windlight shaders to stub versions.
        wl_class = 2;
    }
    else
    {
        light_class = 2;
    }

    // Trigger a full rebuild of the fallback skybox / cubemap if we've toggled windlight shaders
    if (!wl_class || (mShaderLevel[SHADER_WINDLIGHT] != wl_class && gSky.mVOSkyp.notNull()))
    {
        gSky.mVOSkyp->forceSkyUpdate();
    }

    // Load lighting shaders
    mShaderLevel[SHADER_LIGHTING] = light_class;
    mShaderLevel[SHADER_INTERFACE] = interface_class;
    mShaderLevel[SHADER_ENVIRONMENT] = env_class;
    mShaderLevel[SHADER_WATER] = water_class;
    mShaderLevel[SHADER_OBJECT] = obj_class;
    mShaderLevel[SHADER_EFFECT] = effect_class;
    mShaderLevel[SHADER_WINDLIGHT] = wl_class;
    mShaderLevel[SHADER_DEFERRED] = deferred_class;
    mShaderLevel[SHADER_TRANSFORM] = transform_class;

    BOOL loaded = loadBasicShaders();
    if (loaded)
    {
        LL_INFOS() << "Loaded basic shaders." << LL_ENDL;
    }
    else
    {
        LL_WARNS() << "Failed to load basic shaders." << LL_ENDL;
        llassert(loaded);
    }

    if (loaded)
    {
        gPipeline.mVertexShadersEnabled = TRUE;
        gPipeline.mVertexShadersLoaded = 1;

        // Load all shaders to set max levels
        loaded = loadShadersEnvironment();

        if (loaded)
        {
            LL_INFOS() << "Loaded environment shaders." << LL_ENDL;
        }
        else
        {
            LL_WARNS() << "Failed to load environment shaders." << LL_ENDL;
            llassert(loaded);
        }

        if (loaded)
        {
            loaded = loadShadersWater();
            if (loaded)
            {
                LL_INFOS() << "Loaded water shaders." << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "Failed to load water shaders." << LL_ENDL;
                llassert(loaded);
            }
        }

        if (loaded)
        {
            loaded = loadShadersWindLight();
            if (loaded)
            {
                LL_INFOS() << "Loaded windlight shaders." << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "Failed to load windlight shaders." << LL_ENDL;
                llassert(loaded);
            }
        }

        if (loaded)
        {
            loaded = loadShadersEffects();
            if (loaded)
            {
                LL_INFOS() << "Loaded effects shaders." << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "Failed to load effects shaders." << LL_ENDL;
                llassert(loaded);
            }
        }

        if (loaded)
        {
            loaded = loadShadersInterface();
            if (loaded)
            {
                LL_INFOS() << "Loaded interface shaders." << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "Failed to load interface shaders." << LL_ENDL;
                llassert(loaded);
            }
        }

        if (loaded)

        {
            loaded = loadTransformShaders();
            if (loaded)
            {
                LL_INFOS() << "Loaded transform shaders." << LL_ENDL;
            }
            else
            {
                LL_WARNS() << "Failed to load transform shaders." << LL_ENDL;
                llassert(loaded);
            }
        }

        if (loaded)
        {
            // Load max avatar shaders to set the max level
            mShaderLevel[SHADER_AVATAR] = 3;
            mMaxAvatarShaderLevel = 3;
                
            if (gSavedSettings.getBOOL("RenderAvatarVP") && loadShadersObject())
            { //hardware skinning is enabled and rigged attachment shaders loaded correctly
                BOOL avatar_cloth = gSavedSettings.getBOOL("RenderAvatarCloth");

                // cloth is a class3 shader
                S32 avatar_class = avatar_cloth ? 3 : 1;
                
                // Set the actual level
                mShaderLevel[SHADER_AVATAR] = avatar_class;

                loaded = loadShadersAvatar();
                llassert(loaded);

                if (mShaderLevel[SHADER_AVATAR] != avatar_class)
                {
                    if (mShaderLevel[SHADER_AVATAR] == 0)
                    {
                        gSavedSettings.setBOOL("RenderAvatarVP", FALSE);
                    }
                    if(llmax(mShaderLevel[SHADER_AVATAR]-1,0) >= 3)
                    {
                        avatar_cloth = true;
                    }
                    else
                    {
                        avatar_cloth = false;
                    }
                    gSavedSettings.setBOOL("RenderAvatarCloth", avatar_cloth);
                }
            }
            else
            { //hardware skinning not possible, neither is deferred rendering
                mShaderLevel[SHADER_AVATAR] = 0;
                mShaderLevel[SHADER_DEFERRED] = 0;

                if (gSavedSettings.getBOOL("RenderAvatarVP"))
                {
                    gSavedSettings.setBOOL("RenderDeferred", FALSE);
                    gSavedSettings.setBOOL("RenderAvatarCloth", FALSE);
                    gSavedSettings.setBOOL("RenderAvatarVP", FALSE);
                }

                loadShadersAvatar(); // unloads

                loaded = loadShadersObject();
                llassert(loaded);
            }
        }

        if (!loaded)
        { //some shader absolutely could not load, try to fall back to a simpler setting
            if (gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
            { //disable windlight and try again
                gSavedSettings.setBOOL("WindLightUseAtmosShaders", FALSE);
                LL_WARNS() << "Falling back to no windlight shaders." << LL_ENDL;
                reentrance = false;
                setShaders();
                return;
            }
        }       

        llassert(loaded);

        if (loaded && !loadShadersDeferred())
        { //everything else succeeded but deferred failed, disable deferred and try again
            gSavedSettings.setBOOL("RenderDeferred", FALSE);
            LL_WARNS() << "Falling back to no deferred shaders." << LL_ENDL;
            reentrance = false;
            setShaders();
            return;
        }
    }
    else
    {
        LLGLSLShader::sNoFixedFunction = false;
        gPipeline.mVertexShadersEnabled = FALSE;
        gPipeline.mVertexShadersLoaded = 0;
        mShaderLevel[SHADER_LIGHTING] = 0;
        mShaderLevel[SHADER_INTERFACE] = 0;
        mShaderLevel[SHADER_ENVIRONMENT] = 0;
        mShaderLevel[SHADER_WATER] = 0;
        mShaderLevel[SHADER_OBJECT] = 0;
        mShaderLevel[SHADER_EFFECT] = 0;
        mShaderLevel[SHADER_WINDLIGHT] = 0;
        mShaderLevel[SHADER_AVATAR] = 0;
    }
    
    if (gViewerWindow)
    {
        gViewerWindow->setCursor(UI_CURSOR_ARROW);
    }
    gPipeline.createGLBuffers();

    reentrance = false;
}

void LLViewerShaderMgr::unloadShaders()
{
	gOcclusionProgram.unload();
	gOcclusionCubeProgram.unload();
	gDebugProgram.unload();
	gClipProgram.unload();
	gDownsampleDepthProgram.unload();
	gDownsampleDepthRectProgram.unload();
	gBenchmarkProgram.unload();
	gAlphaMaskProgram.unload();
	gUIProgram.unload();
	gPathfindingProgram.unload();
	gPathfindingNoNormalsProgram.unload();
	gCustomAlphaProgram.unload();
	gGlowCombineProgram.unload();
	gSplatTextureRectProgram.unload();
	gGlowCombineFXAAProgram.unload();
	gTwoTextureAddProgram.unload();
	gTwoTextureCompareProgram.unload();
	gOneTextureFilterProgram.unload();
	gOneTextureNoColorProgram.unload();
	gSolidColorProgram.unload();

	gObjectFullbrightNoColorProgram.unload();
	gObjectFullbrightNoColorWaterProgram.unload();
	gObjectSimpleProgram.unload();
	gObjectSimpleImpostorProgram.unload();
	gObjectPreviewProgram.unload();
	gImpostorProgram.unload();
	gObjectSimpleAlphaMaskProgram.unload();
	gObjectBumpProgram.unload();
	gObjectSimpleWaterProgram.unload();
	gObjectSimpleWaterAlphaMaskProgram.unload();
	gObjectFullbrightProgram.unload();
	gObjectFullbrightWaterProgram.unload();
	gObjectEmissiveProgram.unload();
	gObjectEmissiveWaterProgram.unload();
	gObjectFullbrightAlphaMaskProgram.unload();
	gObjectFullbrightWaterAlphaMaskProgram.unload();

	gObjectShinyProgram.unload();
	gObjectFullbrightShinyProgram.unload();
	gObjectFullbrightShinyWaterProgram.unload();
	gObjectShinyWaterProgram.unload();

	gObjectSimpleNonIndexedProgram.unload();
	gObjectSimpleNonIndexedTexGenProgram.unload();
	gObjectSimpleNonIndexedTexGenWaterProgram.unload();
	gObjectSimpleNonIndexedWaterProgram.unload();
	gObjectAlphaMaskNonIndexedProgram.unload();
	gObjectAlphaMaskNonIndexedWaterProgram.unload();
	gObjectAlphaMaskNoColorProgram.unload();
	gObjectAlphaMaskNoColorWaterProgram.unload();
	gObjectFullbrightNonIndexedProgram.unload();
	gObjectFullbrightNonIndexedWaterProgram.unload();
	gObjectEmissiveNonIndexedProgram.unload();
	gObjectEmissiveNonIndexedWaterProgram.unload();
	gTreeProgram.unload();
	gTreeWaterProgram.unload();

	gObjectShinyNonIndexedProgram.unload();
	gObjectFullbrightShinyNonIndexedProgram.unload();
	gObjectFullbrightShinyNonIndexedWaterProgram.unload();
	gObjectShinyNonIndexedWaterProgram.unload();

	gSkinnedObjectSimpleProgram.unload();
	gSkinnedObjectFullbrightProgram.unload();
	gSkinnedObjectEmissiveProgram.unload();
	gSkinnedObjectFullbrightShinyProgram.unload();
	gSkinnedObjectShinySimpleProgram.unload();
	
	gSkinnedObjectSimpleWaterProgram.unload();
	gSkinnedObjectFullbrightWaterProgram.unload();
	gSkinnedObjectEmissiveWaterProgram.unload();
	gSkinnedObjectFullbrightShinyWaterProgram.unload();
	gSkinnedObjectShinySimpleWaterProgram.unload();
	

	gWaterProgram.unload();
    gWaterEdgeProgram.unload();
	gUnderWaterProgram.unload();
	gTerrainProgram.unload();
	gTerrainWaterProgram.unload();
	gGlowProgram.unload();
	gGlowExtractProgram.unload();
	gAvatarProgram.unload();
	gAvatarWaterProgram.unload();
	gAvatarEyeballProgram.unload();
	gAvatarPickProgram.unload();
	gHighlightProgram.unload();
	gHighlightNormalProgram.unload();
	gHighlightSpecularProgram.unload();

	gWLSkyProgram.unload();
	gWLCloudProgram.unload();
    gWLSunProgram.unload();
    gWLMoonProgram.unload();

	gPostColorFilterProgram.unload();
	gPostNightVisionProgram.unload();

	gDeferredDiffuseProgram.unload();
	gDeferredDiffuseAlphaMaskProgram.unload();
	gDeferredNonIndexedDiffuseAlphaMaskProgram.unload();
	gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.unload();
	gDeferredNonIndexedDiffuseProgram.unload();
	gDeferredSkinnedDiffuseProgram.unload();
	gDeferredSkinnedBumpProgram.unload();
	gDeferredSkinnedAlphaProgram.unload();

	gTransformPositionProgram.unload();
	gTransformTexCoordProgram.unload();
	gTransformNormalProgram.unload();
	gTransformColorProgram.unload();
	gTransformTangentProgram.unload();

	mShaderLevel[SHADER_LIGHTING] = 0;
	mShaderLevel[SHADER_OBJECT] = 0;
	mShaderLevel[SHADER_AVATAR] = 0;
	mShaderLevel[SHADER_ENVIRONMENT] = 0;
	mShaderLevel[SHADER_WATER] = 0;
	mShaderLevel[SHADER_INTERFACE] = 0;
	mShaderLevel[SHADER_EFFECT] = 0;
	mShaderLevel[SHADER_WINDLIGHT] = 0;
	mShaderLevel[SHADER_TRANSFORM] = 0;

	gPipeline.mVertexShadersLoaded = 0;
}

BOOL LLViewerShaderMgr::loadBasicShaders()
{
	// Load basic dependency shaders first
	// All of these have to load for any shaders to function
	
	S32 sum_lights_class = 3;

	// class one cards will get the lower sum lights
	// class zero we're not going to think about
	// since a class zero card COULD be a ridiculous new card
	// and old cards should have the features masked
	if(LLFeatureManager::getInstance()->getGPUClass() == GPU_CLASS_1)
	{
		sum_lights_class = 2;
	}

	// If we have sun and moon only checked, then only sum those lights.
	if (gPipeline.getLightingDetail() == 0)
	{
		sum_lights_class = 1;
	}

#if LL_DARWIN
	// Work around driver crashes on older Macs when using deferred rendering
	// NORSPEC-59
	//
	if (gGLManager.mIsMobileGF)
		sum_lights_class = 3;
#endif

	// Use the feature table to mask out the max light level to use.  Also make sure it's at least 1.
	S32 max_light_class = gSavedSettings.getS32("RenderShaderLightingMaxLevel");
	sum_lights_class = llclamp(sum_lights_class, 1, max_light_class);

	// Load the Basic Vertex Shaders at the appropriate level. 
	// (in order of shader function call depth for reference purposes, deepest level first)

	vector< pair<string, S32> > shaders;
	shaders.push_back( make_pair( "windlight/atmosphericsVarsV.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsVarsWaterV.glsl",  mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsHelpersV.glsl",    mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "lighting/lightFuncV.glsl",               mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsV.glsl",               sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightV.glsl",                   mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/lightFuncSpecularV.glsl",       mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsSpecularV.glsl",       sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightSpecularV.glsl",           mShaderLevel[SHADER_LIGHTING] ) );
    shaders.push_back( make_pair( "windlight/atmosphericsFuncs.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsV.glsl",           mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "avatar/avatarSkinV.glsl",                1 ) );
	shaders.push_back( make_pair( "avatar/objectSkinV.glsl",                1 ) );
	if (gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)
	{
		shaders.push_back( make_pair( "objects/indexedTextureV.glsl",           1 ) );
	}
	shaders.push_back( make_pair( "objects/nonindexedTextureV.glsl",        1 ) );

	boost::unordered_map<std::string, std::string> attribs;
	attribs["MAX_JOINTS_PER_MESH_OBJECT"] = 
		boost::lexical_cast<std::string>(LLSkinningUtil::getMaxJointCount());

    BOOL ambient_kill = gSavedSettings.getBOOL("AmbientDisable");
	BOOL sunlight_kill = gSavedSettings.getBOOL("SunlightDisable");
    BOOL local_light_kill = gSavedSettings.getBOOL("LocalLightDisable");

    if (ambient_kill)
    {
        attribs["AMBIENT_KILL"] = "1";
    }

    if (sunlight_kill)
    {
        attribs["SUNLIGHT_KILL"] = "1";
    }

    if (local_light_kill)
    {
       attribs["LOCAL_LIGHT_KILL"] = "1";
    }

	// We no longer have to bind the shaders to global glhandles, they are automatically added to a map now.
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_VERTEX_SHADER_ARB
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_VERTEX_SHADER_ARB, &attribs) == 0)
		{
			LL_SHADER_LOADING_WARNS() << "Failed to load vertex shader " << shaders[i].first << LL_ENDL;
			return FALSE;
		}
	}

	// Load the Basic Fragment Shaders at the appropriate level. 
	// (in order of shader function call depth for reference purposes, deepest level first)

	shaders.clear();
	S32 ch = 1;

	if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 30)
	{ //use indexed texture rendering for GLSL >= 1.30
		ch = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
	}

	std::vector<S32> index_channels;    
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsVarsF.glsl",      mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsVarsWaterF.glsl",     mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsHelpersF.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/gammaF.glsl",                 mShaderLevel[SHADER_WINDLIGHT]) );
    index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsFuncs.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsF.glsl",          mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/transportF.glsl",             mShaderLevel[SHADER_WINDLIGHT] ) ); 
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/waterFogF.glsl",                mShaderLevel[SHADER_WATER] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/encodeNormF.glsl",	mShaderLevel[SHADER_ENVIRONMENT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/srgbF.glsl",                    mShaderLevel[SHADER_ENVIRONMENT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/deferredUtil.glsl",                    1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/shadowUtil.glsl",                      1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/aoUtil.glsl",                          1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightNonIndexedF.glsl",                    mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightAlphaMaskNonIndexedF.glsl",                   mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightNonIndexedF.glsl",          mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightNonIndexedAlphaMaskF.glsl",         mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightWaterNonIndexedF.glsl",               mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightWaterAlphaMaskNonIndexedF.glsl",              mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightWaterNonIndexedF.glsl", mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightWaterNonIndexedAlphaMaskF.glsl",    mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightShinyNonIndexedF.glsl",               mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightShinyNonIndexedF.glsl", mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightShinyWaterNonIndexedF.glsl",          mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightFullbrightShinyWaterNonIndexedF.glsl", mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightF.glsl",                  mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightAlphaMaskF.glsl",                 mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightF.glsl",            mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightAlphaMaskF.glsl",           mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightWaterF.glsl",             mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightWaterAlphaMaskF.glsl",    mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightWaterF.glsl",   mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightWaterAlphaMaskF.glsl",  mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightShinyF.glsl",             mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightShinyF.glsl",   mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightShinyWaterF.glsl",            mShaderLevel[SHADER_LIGHTING] ) );
    index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightFullbrightShinyWaterF.glsl", mShaderLevel[SHADER_LIGHTING] ) );
    
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_FRAGMENT_SHADER_ARB
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_FRAGMENT_SHADER_ARB, &attribs, index_channels[i]) == 0)
		{
			LL_SHADER_LOADING_WARNS() << "Failed to load fragment shader " << shaders[i].first << LL_ENDL;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersEnvironment()
{
	BOOL success = TRUE;

	if (mShaderLevel[SHADER_ENVIRONMENT] == 0)
	{
		gTerrainProgram.unload();
		return TRUE;
	}

	if (success)
	{
		gTerrainProgram.mName = "Terrain Shader";
		gTerrainProgram.mFeatures.calculatesLighting = true;
		gTerrainProgram.mFeatures.calculatesAtmospherics = true;
		gTerrainProgram.mFeatures.hasAtmospherics = true;
		gTerrainProgram.mFeatures.hasTransport = true;
		gTerrainProgram.mFeatures.hasGamma = true;
		gTerrainProgram.mFeatures.hasSrgb = true;
		gTerrainProgram.mFeatures.mIndexedTextureChannels = 0;
		gTerrainProgram.mFeatures.disableTextureIndex = true;
		gTerrainProgram.mFeatures.hasGamma = true;
        gTerrainProgram.mShaderFiles.clear();
        gTerrainProgram.mShaderFiles.push_back(make_pair("environment/terrainV.glsl", GL_VERTEX_SHADER_ARB));
        gTerrainProgram.mShaderFiles.push_back(make_pair("environment/terrainF.glsl", GL_FRAGMENT_SHADER_ARB));
        gTerrainProgram.mShaderLevel = mShaderLevel[SHADER_ENVIRONMENT];
        success = gTerrainProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (!success)
	{
		mShaderLevel[SHADER_ENVIRONMENT] = 0;
		return FALSE;
	}
	
	LLWorld::getInstance()->updateWaterObjects();
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersWater()
{
	BOOL success = TRUE;
	BOOL terrainWaterSuccess = TRUE;

	if (mShaderLevel[SHADER_WATER] == 0)
	{
		gWaterProgram.unload();
		gWaterEdgeProgram.unload();
		gUnderWaterProgram.unload();
		gTerrainWaterProgram.unload();
		return TRUE;
	}

	if (success)
	{
		// load water shader
		gWaterProgram.mName = "Water Shader";
		gWaterProgram.mFeatures.calculatesAtmospherics = true;
		gWaterProgram.mFeatures.hasGamma = true;
		gWaterProgram.mFeatures.hasTransport = true;
        gWaterProgram.mFeatures.hasSrgb = true;
		gWaterProgram.mShaderFiles.clear();
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gWaterProgram.mShaderLevel = mShaderLevel[SHADER_WATER];
		success = gWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
	// load water shader
		gWaterEdgeProgram.mName = "Water Edge Shader";
		gWaterEdgeProgram.mFeatures.calculatesAtmospherics = true;
		gWaterEdgeProgram.mFeatures.hasGamma = true;
		gWaterEdgeProgram.mFeatures.hasTransport = true;
        gWaterEdgeProgram.mFeatures.hasSrgb = true;
		gWaterEdgeProgram.mShaderFiles.clear();
		gWaterEdgeProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gWaterEdgeProgram.mShaderFiles.push_back(make_pair("environment/waterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gWaterEdgeProgram.addPermutation("WATER_EDGE", "1");
		gWaterEdgeProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gWaterEdgeProgram.mShaderLevel = mShaderLevel[SHADER_WATER];
		success = gWaterEdgeProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		//load under water vertex shader
		gUnderWaterProgram.mName = "Underwater Shader";
		gUnderWaterProgram.mFeatures.calculatesAtmospherics = true;
		gUnderWaterProgram.mFeatures.hasWaterFog = true;
		gUnderWaterProgram.mShaderFiles.clear();
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/underWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gUnderWaterProgram.mShaderLevel = mShaderLevel[SHADER_WATER];        
		gUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;       
		success = gUnderWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		//load terrain water shader
		gTerrainWaterProgram.mName = "Terrain Water Shader";
		gTerrainWaterProgram.mFeatures.calculatesLighting = true;
		gTerrainWaterProgram.mFeatures.calculatesAtmospherics = true;
		gTerrainWaterProgram.mFeatures.hasAtmospherics = true;
		gTerrainWaterProgram.mFeatures.hasWaterFog = true;
		gTerrainWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gTerrainWaterProgram.mFeatures.disableTextureIndex = true;
		gTerrainWaterProgram.mShaderFiles.clear();
		gTerrainWaterProgram.mShaderFiles.push_back(make_pair("environment/terrainWaterV.glsl", GL_VERTEX_SHADER_ARB));
		gTerrainWaterProgram.mShaderFiles.push_back(make_pair("environment/terrainWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTerrainWaterProgram.mShaderLevel = mShaderLevel[SHADER_ENVIRONMENT];
		gTerrainWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;

        gTerrainWaterProgram.clearPermutations();

        if (LLPipeline::RenderDeferred)
        {
            gTerrainWaterProgram.addPermutation("ALM", "1");
        }

		terrainWaterSuccess = gTerrainWaterProgram.createShader(NULL, NULL);
		llassert(terrainWaterSuccess);
	}	

	/// Keep track of water shader levels
	if (gWaterProgram.mShaderLevel != mShaderLevel[SHADER_WATER]
		|| gUnderWaterProgram.mShaderLevel != mShaderLevel[SHADER_WATER])
	{
		mShaderLevel[SHADER_WATER] = llmin(gWaterProgram.mShaderLevel, gUnderWaterProgram.mShaderLevel);
	}

	if (!success)
	{
		mShaderLevel[SHADER_WATER] = 0;
		return FALSE;
	}

	// if we failed to load the terrain water shaders and we need them (using class2 water),
	// then drop down to class1 water.
	if (mShaderLevel[SHADER_WATER] > 1 && !terrainWaterSuccess)
	{
		mShaderLevel[SHADER_WATER]--;
		return loadShadersWater();
	}
	
	LLWorld::getInstance()->updateWaterObjects();

	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersEffects()
{
	BOOL success = TRUE;

	if (mShaderLevel[SHADER_EFFECT] == 0)
	{
		gGlowProgram.unload();
		gGlowExtractProgram.unload();
		gPostColorFilterProgram.unload();	
		gPostNightVisionProgram.unload();
		return TRUE;
	}

	if (success)
	{
		gGlowProgram.mName = "Glow Shader (Post)";
		gGlowProgram.mShaderFiles.clear();
		gGlowProgram.mShaderFiles.push_back(make_pair("effects/glowV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowProgram.mShaderFiles.push_back(make_pair("effects/glowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowProgram.mShaderLevel = mShaderLevel[SHADER_EFFECT];
		success = gGlowProgram.createShader(NULL, NULL);
		if (!success)
		{
			LLPipeline::sRenderGlow = FALSE;
		}
	}
	
	if (success)
	{
		gGlowExtractProgram.mName = "Glow Extract Shader (Post)";
		gGlowExtractProgram.mShaderFiles.clear();
		gGlowExtractProgram.mShaderFiles.push_back(make_pair("effects/glowExtractV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowExtractProgram.mShaderFiles.push_back(make_pair("effects/glowExtractF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowExtractProgram.mShaderLevel = mShaderLevel[SHADER_EFFECT];
		success = gGlowExtractProgram.createShader(NULL, NULL);
		if (!success)
		{
			LLPipeline::sRenderGlow = FALSE;
		}
	}
	
	return success;

}

BOOL LLViewerShaderMgr::loadShadersDeferred()
{
    bool use_sun_shadow = mShaderLevel[SHADER_DEFERRED] > 1;

    BOOL ambient_kill = gSavedSettings.getBOOL("AmbientDisable");
	BOOL sunlight_kill = gSavedSettings.getBOOL("SunlightDisable");
    BOOL local_light_kill = gSavedSettings.getBOOL("LocalLightDisable");

	if (mShaderLevel[SHADER_DEFERRED] == 0)
	{
		gDeferredTreeProgram.unload();
		gDeferredTreeShadowProgram.unload();
		gDeferredDiffuseProgram.unload();
		gDeferredDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.unload();
		gDeferredNonIndexedDiffuseProgram.unload();
		gDeferredSkinnedDiffuseProgram.unload();
		gDeferredSkinnedBumpProgram.unload();
		gDeferredSkinnedAlphaProgram.unload();
		gDeferredBumpProgram.unload();
		gDeferredImpostorProgram.unload();
		gDeferredTerrainProgram.unload();
		gDeferredTerrainWaterProgram.unload();
		gDeferredLightProgram.unload();
		for (U32 i = 0; i < LL_DEFERRED_MULTI_LIGHT_COUNT; ++i)
		{
			gDeferredMultiLightProgram[i].unload();
		}
		gDeferredSpotLightProgram.unload();
		gDeferredMultiSpotLightProgram.unload();
		gDeferredSunProgram.unload();
		gDeferredBlurLightProgram.unload();
		gDeferredSoftenProgram.unload();
		gDeferredSoftenWaterProgram.unload();
		gDeferredShadowProgram.unload();
		gDeferredShadowCubeProgram.unload();
        gDeferredShadowAlphaMaskProgram.unload();
        gDeferredShadowFullbrightAlphaMaskProgram.unload();
		gDeferredAvatarShadowProgram.unload();
        gDeferredAvatarAlphaShadowProgram.unload();
        gDeferredAvatarAlphaMaskShadowProgram.unload();
		gDeferredAttachmentShadowProgram.unload();
        gDeferredAttachmentAlphaShadowProgram.unload();
        gDeferredAttachmentAlphaMaskShadowProgram.unload();
		gDeferredAvatarProgram.unload();
		gDeferredAvatarAlphaProgram.unload();
		gDeferredAlphaProgram.unload();
		gDeferredAlphaWaterProgram.unload();
		gDeferredFullbrightProgram.unload();
		gDeferredFullbrightAlphaMaskProgram.unload();
		gDeferredFullbrightWaterProgram.unload();
		gDeferredFullbrightAlphaMaskWaterProgram.unload();
		gDeferredEmissiveProgram.unload();
		gDeferredAvatarEyesProgram.unload();
		gDeferredPostProgram.unload();		
		gDeferredCoFProgram.unload();		
		gDeferredDoFCombineProgram.unload();
		gDeferredPostGammaCorrectProgram.unload();
		gFXAAProgram.unload();
		gDeferredWaterProgram.unload();
		gDeferredUnderWaterProgram.unload();
		gDeferredWLSkyProgram.unload();
		gDeferredWLCloudProgram.unload();
        gDeferredWLSunProgram.unload();
        gDeferredWLMoonProgram.unload();
		gDeferredStarProgram.unload();
		gDeferredFullbrightShinyProgram.unload();
		gDeferredSkinnedFullbrightShinyProgram.unload();
		gDeferredSkinnedFullbrightProgram.unload();

        gDeferredHighlightProgram.unload();
        gDeferredHighlightNormalProgram.unload();
        gDeferredHighlightSpecularProgram.unload();

		gNormalMapGenProgram.unload();
		for (U32 i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
		{
			gDeferredMaterialProgram[i].unload();
			gDeferredMaterialWaterProgram[i].unload();
		}
		return TRUE;
	}

	BOOL success = TRUE;

    if (success)
	{
		gDeferredHighlightProgram.mName = "Deferred Highlight Shader";
		gDeferredHighlightProgram.mShaderFiles.clear();
		gDeferredHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredHighlightProgram.mShaderFiles.push_back(make_pair("deferred/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredHighlightProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];		
		success = gDeferredHighlightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredHighlightNormalProgram.mName = "Deferred Highlight Normals Shader";
		gDeferredHighlightNormalProgram.mShaderFiles.clear();
		gDeferredHighlightNormalProgram.mShaderFiles.push_back(make_pair("interface/highlightNormV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredHighlightNormalProgram.mShaderFiles.push_back(make_pair("deferred/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredHighlightNormalProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];		
		success = gHighlightNormalProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredHighlightSpecularProgram.mName = "Deferred Highlight Spec Shader";
		gDeferredHighlightSpecularProgram.mShaderFiles.clear();
		gDeferredHighlightSpecularProgram.mShaderFiles.push_back(make_pair("interface/highlightSpecV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredHighlightSpecularProgram.mShaderFiles.push_back(make_pair("deferred/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredHighlightSpecularProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];		
		success = gDeferredHighlightSpecularProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDiffuseProgram.mName = "Deferred Diffuse Shader";
        gDeferredDiffuseProgram.mFeatures.encodesNormal = true;
        gDeferredDiffuseProgram.mFeatures.hasSrgb = true;
		gDeferredDiffuseProgram.mShaderFiles.clear();
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseIndexedF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDiffuseProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredDiffuseProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Alpha Mask Shader";
        gDeferredDiffuseAlphaMaskProgram.mFeatures.encodesNormal = true;
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskIndexedF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDiffuseAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredDiffuseAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask Shader";
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mFeatures.encodesNormal = true;
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskProgram.createShader(NULL, NULL);
        llassert(success);
	}
    
	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask Shader";
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mFeatures.encodesNormal = true;
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskNoColorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredNonIndexedDiffuseProgram.mName = "Non Indexed Deferred Diffuse Shader";
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.clear();
        gDeferredNonIndexedDiffuseProgram.mFeatures.encodesNormal = true;
        gDeferredNonIndexedDiffuseProgram.mFeatures.hasSrgb = true;
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredSkinnedDiffuseProgram.mName = "Deferred Skinned Diffuse Shader";
		gDeferredSkinnedDiffuseProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedDiffuseProgram.mFeatures.encodesNormal = true;
        gDeferredSkinnedDiffuseProgram.mFeatures.hasSrgb = true;
		gDeferredSkinnedDiffuseProgram.mShaderFiles.clear();
		gDeferredSkinnedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedDiffuseProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredSkinnedDiffuseProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSkinnedBumpProgram.mName = "Deferred Skinned Bump Shader";
		gDeferredSkinnedBumpProgram.mFeatures.hasObjectSkinning = true;
        gDeferredSkinnedBumpProgram.mFeatures.encodesNormal = true;
		gDeferredSkinnedBumpProgram.mShaderFiles.clear();
		gDeferredSkinnedBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedBumpProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredSkinnedBumpProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredSkinnedAlphaProgram.mName = "Deferred Skinned Alpha Shader";
		gDeferredSkinnedAlphaProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredSkinnedAlphaProgram.mFeatures.hasLighting = false;
		gDeferredSkinnedAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredSkinnedAlphaProgram.mFeatures.disableTextureIndex = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasSrgb = true;
		gDeferredSkinnedAlphaProgram.mFeatures.encodesNormal = true;
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasAtmospherics = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasTransport = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasGamma = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasShadows = true;
        
		gDeferredSkinnedAlphaProgram.mShaderFiles.clear();
		gDeferredSkinnedAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		gDeferredSkinnedAlphaProgram.clearPermutations();
		gDeferredSkinnedAlphaProgram.addPermutation("USE_DIFFUSE_TEX", "1");
		gDeferredSkinnedAlphaProgram.addPermutation("HAS_SKIN", "1");
        gDeferredSkinnedAlphaProgram.addPermutation("USE_VERTEX_COLOR", "1");

		if (use_sun_shadow)
		{
			gDeferredSkinnedAlphaProgram.addPermutation("HAS_SHADOW", "1");
		}

        if (ambient_kill)
        {
            gDeferredSkinnedAlphaProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredSkinnedAlphaProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredSkinnedAlphaProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		success = gDeferredSkinnedAlphaProgram.createShader(NULL, NULL);
		llassert(success);

		// Hack to include uniforms for lighting without linking in lighting file
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gDeferredBumpProgram.mName = "Deferred Bump Shader";
		gDeferredBumpProgram.mFeatures.encodesNormal = true;
		gDeferredBumpProgram.mShaderFiles.clear();
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredBumpProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredBumpProgram.createShader(NULL, NULL);
		llassert(success);
	}

	gDeferredMaterialProgram[1].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[5].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[9].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[13].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[1+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[5+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[9+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialProgram[13+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;

	gDeferredMaterialWaterProgram[1].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[5].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[9].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[13].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[1+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[5+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[9+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;
	gDeferredMaterialWaterProgram[13+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = false;

	for (U32 i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
	{
		if (success)
		{
            mShaderList.push_back(&gDeferredMaterialProgram[i]);

			gDeferredMaterialProgram[i].mName = llformat("Deferred Material Shader %d", i);
			
			U32 alpha_mode = i & 0x3;

			gDeferredMaterialProgram[i].mShaderFiles.clear();
			gDeferredMaterialProgram[i].mShaderFiles.push_back(make_pair("deferred/materialV.glsl", GL_VERTEX_SHADER_ARB));
			gDeferredMaterialProgram[i].mShaderFiles.push_back(make_pair("deferred/materialF.glsl", GL_FRAGMENT_SHADER_ARB));
			gDeferredMaterialProgram[i].mShaderLevel = mShaderLevel[SHADER_DEFERRED];

			gDeferredMaterialProgram[i].clearPermutations();

			bool has_normal_map   = (i & 0x8) > 0;
			bool has_specular_map = (i & 0x4) > 0;

			if (has_normal_map)
			{
				gDeferredMaterialProgram[i].addPermutation("HAS_NORMAL_MAP", "1");
			}

			if (has_specular_map)
			{
				gDeferredMaterialProgram[i].addPermutation("HAS_SPECULAR_MAP", "1");
			}

            if (ambient_kill)
            {
                gDeferredMaterialProgram[i].addPermutation("AMBIENT_KILL", "1");
            }

            if (sunlight_kill)
            {
                gDeferredMaterialProgram[i].addPermutation("SUNLIGHT_KILL", "1");
            }

            if (local_light_kill)
            {
                gDeferredMaterialProgram[i].addPermutation("LOCAL_LIGHT_KILL", "1");
            }

            gDeferredMaterialProgram[i].addPermutation("DIFFUSE_ALPHA_MODE", llformat("%d", alpha_mode));

            if (use_sun_shadow)
            {
                gDeferredMaterialProgram[i].addPermutation("HAS_SUN_SHADOW", "1");
            }

            bool has_skin = i & 0x10;
            gDeferredMaterialProgram[i].mFeatures.hasSrgb = true;
            gDeferredMaterialProgram[i].mFeatures.hasTransport = true;
            gDeferredMaterialProgram[i].mFeatures.encodesNormal = true;
            gDeferredMaterialProgram[i].mFeatures.calculatesAtmospherics = true;
            gDeferredMaterialProgram[i].mFeatures.hasAtmospherics = true;
            gDeferredMaterialProgram[i].mFeatures.hasGamma = true;
            gDeferredMaterialProgram[i].mFeatures.hasShadows = use_sun_shadow;
            
            if (has_skin)
            {
                gDeferredMaterialProgram[i].addPermutation("HAS_SKIN", "1");
                gDeferredMaterialProgram[i].mFeatures.hasObjectSkinning = true;
            }

            success = gDeferredMaterialProgram[i].createShader(NULL, NULL);
            llassert(success);
		}

		if (success)
		{
            mShaderList.push_back(&gDeferredMaterialWaterProgram[i]);

            gDeferredMaterialWaterProgram[i].mName = llformat("Deferred Underwater Material Shader %d", i);

            U32 alpha_mode = i & 0x3;

            gDeferredMaterialWaterProgram[i].mShaderFiles.clear();
            gDeferredMaterialWaterProgram[i].mShaderFiles.push_back(make_pair("deferred/materialV.glsl", GL_VERTEX_SHADER_ARB));
            gDeferredMaterialWaterProgram[i].mShaderFiles.push_back(make_pair("deferred/materialF.glsl", GL_FRAGMENT_SHADER_ARB));
            gDeferredMaterialWaterProgram[i].mShaderLevel = mShaderLevel[SHADER_DEFERRED];
            gDeferredMaterialWaterProgram[i].mShaderGroup = LLGLSLShader::SG_WATER;

            gDeferredMaterialWaterProgram[i].clearPermutations();

            bool has_normal_map   = (i & 0x8) > 0;
            bool has_specular_map = (i & 0x4) > 0;

            if (has_normal_map)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("HAS_NORMAL_MAP", "1");
            }

            if (has_specular_map)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("HAS_SPECULAR_MAP", "1");
            }

            gDeferredMaterialWaterProgram[i].addPermutation("DIFFUSE_ALPHA_MODE", llformat("%d", alpha_mode));
            if (use_sun_shadow)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("HAS_SUN_SHADOW", "1");
            }

            bool has_skin = i & 0x10;
            if (has_skin)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("HAS_SKIN", "1");
            }
            gDeferredMaterialWaterProgram[i].addPermutation("WATER_FOG","1");

            if (ambient_kill)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("AMBIENT_KILL", "1");
            }

            if (sunlight_kill)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("SUNLIGHT_KILL", "1");
            }

            if (local_light_kill)
            {
                gDeferredMaterialWaterProgram[i].addPermutation("LOCAL_LIGHT_KILL", "1");
            }

            gDeferredMaterialWaterProgram[i].mFeatures.hasWaterFog = true;
            gDeferredMaterialWaterProgram[i].mFeatures.hasSrgb = true;
            gDeferredMaterialWaterProgram[i].mFeatures.encodesNormal = true;
            gDeferredMaterialWaterProgram[i].mFeatures.calculatesAtmospherics = true;
            gDeferredMaterialWaterProgram[i].mFeatures.hasAtmospherics = true;
            gDeferredMaterialWaterProgram[i].mFeatures.hasGamma = true;

            gDeferredMaterialWaterProgram[i].mFeatures.hasTransport = true;
            gDeferredMaterialWaterProgram[i].mFeatures.hasShadows = use_sun_shadow;
            
            if (has_skin)
            {
                gDeferredMaterialWaterProgram[i].mFeatures.hasObjectSkinning = true;
            }

            success = gDeferredMaterialWaterProgram[i].createShader(NULL, NULL);//&mWLUniforms);
            llassert(success);
		}
	}

	gDeferredMaterialProgram[1].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[5].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[9].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[13].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[1+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[5+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[9+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialProgram[13+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;

	gDeferredMaterialWaterProgram[1].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[5].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[9].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[13].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[1+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[5+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[9+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;
	gDeferredMaterialWaterProgram[13+LLMaterial::SHADER_COUNT].mFeatures.hasLighting = true;

	
	if (success)
	{
		gDeferredTreeProgram.mName = "Deferred Tree Shader";
		gDeferredTreeProgram.mShaderFiles.clear();
        gDeferredTreeProgram.mFeatures.encodesNormal = true;
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTreeProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredTreeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredTreeShadowProgram.mName = "Deferred Tree Shadow Shader";
		gDeferredTreeShadowProgram.mShaderFiles.clear();
		gDeferredTreeShadowProgram.mFeatures.isDeferred = true;
		gDeferredTreeShadowProgram.mFeatures.hasShadows = true;
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTreeShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredTreeShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredImpostorProgram.mName = "Deferred Impostor Shader";
		gDeferredImpostorProgram.mFeatures.hasSrgb = true;
		gDeferredImpostorProgram.mFeatures.encodesNormal = true;
		//gDeferredImpostorProgram.mFeatures.isDeferred = true;
		gDeferredImpostorProgram.mShaderFiles.clear();
		gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorF.glsl", GL_FRAGMENT_SHADER_ARB));
        gDeferredImpostorProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gDeferredImpostorProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{       
		gDeferredLightProgram.mName = "Deferred Light Shader";
		gDeferredLightProgram.mFeatures.isDeferred = true;
		gDeferredLightProgram.mFeatures.hasShadows = true;
        gDeferredLightProgram.mFeatures.hasSrgb = true;

		gDeferredLightProgram.mShaderFiles.clear();
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        gDeferredLightProgram.clearPermutations();

        if (ambient_kill)
        {
            gDeferredLightProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredLightProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredLightProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		success = gDeferredLightProgram.createShader(NULL, NULL);
        llassert(success);
	}

	for (U32 i = 0; i < LL_DEFERRED_MULTI_LIGHT_COUNT; i++)
	{
		if (success)
		{
			gDeferredMultiLightProgram[i].mName = llformat("Deferred MultiLight Shader %d", i);
			gDeferredMultiLightProgram[i].mFeatures.isDeferred = true;
			gDeferredMultiLightProgram[i].mFeatures.hasShadows = true;
            gDeferredMultiLightProgram[i].mFeatures.hasSrgb = true;

            gDeferredMultiLightProgram[i].clearPermutations();
			gDeferredMultiLightProgram[i].mShaderFiles.clear();
			gDeferredMultiLightProgram[i].mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER_ARB));
			gDeferredMultiLightProgram[i].mShaderFiles.push_back(make_pair("deferred/multiPointLightF.glsl", GL_FRAGMENT_SHADER_ARB));
			gDeferredMultiLightProgram[i].mShaderLevel = mShaderLevel[SHADER_DEFERRED];
			gDeferredMultiLightProgram[i].addPermutation("LIGHT_COUNT", llformat("%d", i+1));

            if (ambient_kill)
            {
                gDeferredMultiLightProgram[i].addPermutation("AMBIENT_KILL", "1");
            }

            if (sunlight_kill)
            {
                gDeferredMultiLightProgram[i].addPermutation("SUNLIGHT_KILL", "1");
            }

            if (local_light_kill)
            {
                gDeferredMultiLightProgram[i].addPermutation("LOCAL_LIGHT_KILL", "1");
            }

			success = gDeferredMultiLightProgram[i].createShader(NULL, NULL);
            llassert(success);
		}
	}

	if (success)
	{
		gDeferredSpotLightProgram.mName = "Deferred SpotLight Shader";
		gDeferredSpotLightProgram.mShaderFiles.clear();
		gDeferredSpotLightProgram.mFeatures.hasSrgb = true;
		gDeferredSpotLightProgram.mFeatures.isDeferred = true;
		gDeferredSpotLightProgram.mFeatures.hasShadows = true;

        gDeferredSpotLightProgram.clearPermutations();
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/spotLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSpotLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        if (ambient_kill)
        {
            gDeferredSpotLightProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredSpotLightProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredSpotLightProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		success = gDeferredSpotLightProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredMultiSpotLightProgram.mName = "Deferred MultiSpotLight Shader";
		gDeferredMultiSpotLightProgram.mFeatures.hasSrgb = true;
		gDeferredMultiSpotLightProgram.mFeatures.isDeferred = true;
		gDeferredMultiSpotLightProgram.mFeatures.hasShadows = true;

        gDeferredMultiSpotLightProgram.clearPermutations();
		gDeferredMultiSpotLightProgram.mShaderFiles.clear();
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/multiSpotLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredMultiSpotLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        if (local_light_kill)
        {
            gDeferredMultiSpotLightProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		success = gDeferredMultiSpotLightProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		std::string fragment;
		std::string vertex = "deferred/sunLightV.glsl";

        bool use_ao = gSavedSettings.getBOOL("RenderDeferredSSAO");

        if (use_ao)
        {
            fragment = "deferred/sunLightSSAOF.glsl";
        }
        else
        {
            fragment = "deferred/sunLightF.glsl";
            if (mShaderLevel[SHADER_DEFERRED] == 1)
            { //no shadows, no SSAO, no frag coord
                vertex = "deferred/sunLightNoFragCoordV.glsl";
            }
        }

        gDeferredSunProgram.mName = "Deferred Sun Shader";
        gDeferredSunProgram.mFeatures.isDeferred    = true;
        gDeferredSunProgram.mFeatures.hasShadows    = true;
        gDeferredSunProgram.mFeatures.hasAmbientOcclusion = use_ao;

        gDeferredSunProgram.mName = "Deferred Sun Shader";
		gDeferredSunProgram.mShaderFiles.clear();
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(vertex, GL_VERTEX_SHADER_ARB));
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(fragment, GL_FRAGMENT_SHADER_ARB));
		gDeferredSunProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gDeferredSunProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredBlurLightProgram.mName = "Deferred Blur Light Shader";
		gDeferredBlurLightProgram.mFeatures.isDeferred = true;

		gDeferredBlurLightProgram.mShaderFiles.clear();
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredBlurLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		success = gDeferredBlurLightProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredAlphaProgram.mName = "Deferred Alpha Shader";

		gDeferredAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredAlphaProgram.mFeatures.hasLighting = false;
		gDeferredAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredAlphaProgram.mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
		gDeferredAlphaProgram.mFeatures.hasSrgb = true;
		gDeferredAlphaProgram.mFeatures.encodesNormal = true;
		gDeferredAlphaProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredAlphaProgram.mFeatures.hasAtmospherics = true;
        gDeferredAlphaProgram.mFeatures.hasGamma = true;
        gDeferredAlphaProgram.mFeatures.hasTransport = true;
        gDeferredAlphaProgram.mFeatures.hasShadows = use_sun_shadow;

        if (mShaderLevel[SHADER_DEFERRED] < 1)
        {
            gDeferredAlphaProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        }
        else
        { //shave off some texture units for shadow maps
            gDeferredAlphaProgram.mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels - 6, 1);
        }
            
        gDeferredAlphaProgram.mShaderFiles.clear();
        gDeferredAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER_ARB));

        gDeferredAlphaProgram.clearPermutations();
        gDeferredAlphaProgram.addPermutation("USE_VERTEX_COLOR", "1");
        gDeferredAlphaProgram.addPermutation("USE_INDEXED_TEX", "1");
        if (use_sun_shadow)
        {
            gDeferredAlphaProgram.addPermutation("HAS_SHADOW", "1");
        }

        if (ambient_kill)
        {
            gDeferredAlphaProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredAlphaProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredAlphaProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

        gDeferredAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gDeferredAlphaProgram.createShader(NULL, NULL);
        llassert(success);

        // Hack
        gDeferredAlphaProgram.mFeatures.calculatesLighting = true;
        gDeferredAlphaProgram.mFeatures.hasLighting = true;
    }

    if (success)
    {
        gDeferredAlphaImpostorProgram.mName = "Deferred Alpha Impostor Shader";

// Begin Hack
		gDeferredAlphaImpostorProgram.mFeatures.calculatesLighting = false;
		gDeferredAlphaImpostorProgram.mFeatures.hasLighting = false;

        gDeferredAlphaImpostorProgram.mFeatures.hasSrgb = true;
		gDeferredAlphaImpostorProgram.mFeatures.isAlphaLighting = true;
        gDeferredAlphaImpostorProgram.mFeatures.encodesNormal = true;
        gDeferredAlphaImpostorProgram.mFeatures.hasShadows = use_sun_shadow;

        if (mShaderLevel[SHADER_DEFERRED] < 1)
        {
            gDeferredAlphaImpostorProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        }
        else
        { //shave off some texture units for shadow maps
            gDeferredAlphaImpostorProgram.mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels - 6, 1);
        }

        gDeferredAlphaImpostorProgram.mShaderFiles.clear();
        gDeferredAlphaImpostorProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredAlphaImpostorProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER_ARB));

        gDeferredAlphaImpostorProgram.clearPermutations();
        gDeferredAlphaImpostorProgram.addPermutation("USE_INDEXED_TEX", "1");
        gDeferredAlphaImpostorProgram.addPermutation("FOR_IMPOSTOR", "1");
        gDeferredAlphaImpostorProgram.addPermutation("USE_VERTEX_COLOR", "1");

        if (use_sun_shadow)
        {
            gDeferredAlphaImpostorProgram.addPermutation("HAS_SHADOW", "1");
        }

        gDeferredAlphaImpostorProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gDeferredAlphaImpostorProgram.createShader(NULL, NULL);
        llassert(success);

// End Hack
        gDeferredAlphaImpostorProgram.mFeatures.calculatesLighting = true;
        gDeferredAlphaImpostorProgram.mFeatures.hasLighting = true;
    }

	if (success)
	{
		gDeferredAlphaWaterProgram.mName = "Deferred Alpha Underwater Shader";
		gDeferredAlphaWaterProgram.mFeatures.calculatesLighting = false;
		gDeferredAlphaWaterProgram.mFeatures.hasLighting = false;
		gDeferredAlphaWaterProgram.mFeatures.isAlphaLighting = true;
		gDeferredAlphaWaterProgram.mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
		gDeferredAlphaWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredAlphaWaterProgram.mFeatures.hasSrgb = true;
		gDeferredAlphaWaterProgram.mFeatures.encodesNormal = true;
		gDeferredAlphaWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAlphaWaterProgram.mFeatures.hasAtmospherics = true;
		gDeferredAlphaWaterProgram.mFeatures.hasGamma = true;
		gDeferredAlphaWaterProgram.mFeatures.hasTransport = true;
		gDeferredAlphaWaterProgram.mFeatures.hasShadows = use_sun_shadow;

		if (mShaderLevel[SHADER_DEFERRED] < 1)
		{
			gDeferredAlphaWaterProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		}
		else
		{ //shave off some texture units for shadow maps
			gDeferredAlphaWaterProgram.mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels - 6, 1);
		}
		gDeferredAlphaWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gDeferredAlphaWaterProgram.mShaderFiles.clear();
		gDeferredAlphaWaterProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAlphaWaterProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER_ARB));

		gDeferredAlphaWaterProgram.clearPermutations();
		gDeferredAlphaWaterProgram.addPermutation("USE_INDEXED_TEX", "1");
		gDeferredAlphaWaterProgram.addPermutation("WATER_FOG", "1");
        gDeferredAlphaWaterProgram.addPermutation("USE_VERTEX_COLOR", "1");
		if (use_sun_shadow)
		{
			gDeferredAlphaWaterProgram.addPermutation("HAS_SHADOW", "1");
		}

        if (ambient_kill)
        {
            gDeferredAlphaWaterProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredAlphaWaterProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredAlphaWaterProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }
        gDeferredAlphaWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		success = gDeferredAlphaWaterProgram.createShader(NULL, NULL);
		llassert(success);

		// Hack
		gDeferredAlphaWaterProgram.mFeatures.calculatesLighting = true;
		gDeferredAlphaWaterProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gDeferredAvatarEyesProgram.mName = "Deferred Avatar Eyes Shader";
		gDeferredAvatarEyesProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarEyesProgram.mFeatures.hasGamma = true;
		gDeferredAvatarEyesProgram.mFeatures.hasTransport = true;
		gDeferredAvatarEyesProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarEyesProgram.mFeatures.hasSrgb = true;
		gDeferredAvatarEyesProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarEyesProgram.mFeatures.hasShadows = true;

		gDeferredAvatarEyesProgram.mShaderFiles.clear();
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/avatarEyesV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarEyesProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarEyesProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightProgram.mName = "Deferred Fullbright Shader";
		gDeferredFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightProgram.mFeatures.hasSrgb = true;		
		gDeferredFullbrightProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightProgram.mShaderFiles.clear();
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredFullbrightProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightAlphaMaskProgram.mName = "Deferred Fullbright Alpha Masking Shader";
		gDeferredFullbrightAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasSrgb = true;		
		gDeferredFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightAlphaMaskProgram.addPermutation("HAS_ALPHA_MASK","1");
		gDeferredFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredFullbrightAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightWaterProgram.mName = "Deferred Fullbright Underwater Shader";
		gDeferredFullbrightWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightWaterProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightWaterProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredFullbrightWaterProgram.mFeatures.hasSrgb = true;
		gDeferredFullbrightWaterProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightWaterProgram.mShaderFiles.clear();
		gDeferredFullbrightWaterProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightWaterProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredFullbrightWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gDeferredFullbrightWaterProgram.addPermutation("WATER_FOG","1");
		success = gDeferredFullbrightWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightAlphaMaskWaterProgram.mName = "Deferred Fullbright Underwater Alpha Masking Shader";
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.hasSrgb = true;
		gDeferredFullbrightAlphaMaskWaterProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightAlphaMaskWaterProgram.mShaderFiles.clear();
		gDeferredFullbrightAlphaMaskWaterProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightAlphaMaskWaterProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightAlphaMaskWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredFullbrightAlphaMaskWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gDeferredFullbrightAlphaMaskWaterProgram.addPermutation("HAS_ALPHA_MASK","1");
		gDeferredFullbrightAlphaMaskWaterProgram.addPermutation("WATER_FOG","1");
		success = gDeferredFullbrightAlphaMaskWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightShinyProgram.mName = "Deferred FullbrightShiny Shader";
		gDeferredFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasAtmospherics = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasSrgb = true;
		gDeferredFullbrightShinyProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels-1;
		gDeferredFullbrightShinyProgram.mShaderFiles.clear();
		gDeferredFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredFullbrightShinyProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSkinnedFullbrightProgram.mName = "Skinned Fullbright Shader";
		gDeferredSkinnedFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.hasAtmospherics = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.hasGamma = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.hasTransport = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.disableTextureIndex = true;
		gDeferredSkinnedFullbrightProgram.mFeatures.hasSrgb = true;
		gDeferredSkinnedFullbrightProgram.mShaderFiles.clear();
		gDeferredSkinnedFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gDeferredSkinnedFullbrightProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSkinnedFullbrightShinyProgram.mName = "Skinned Fullbright Shiny Shader";
		gDeferredSkinnedFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredSkinnedFullbrightShinyProgram.mFeatures.hasAtmospherics = true;
		gDeferredSkinnedFullbrightShinyProgram.mFeatures.hasGamma = true;
		gDeferredSkinnedFullbrightShinyProgram.mFeatures.hasTransport = true;
		gDeferredSkinnedFullbrightShinyProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedFullbrightShinyProgram.mFeatures.disableTextureIndex = true;
        gDeferredSkinnedFullbrightShinyProgram.mFeatures.hasSrgb = true;
		gDeferredSkinnedFullbrightShinyProgram.mShaderFiles.clear();
		gDeferredSkinnedFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinySkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gDeferredSkinnedFullbrightShinyProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredEmissiveProgram.mName = "Deferred Emissive Shader";
		gDeferredEmissiveProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredEmissiveProgram.mFeatures.hasGamma = true;
		gDeferredEmissiveProgram.mFeatures.hasTransport = true;
		gDeferredEmissiveProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredEmissiveProgram.mShaderFiles.clear();
		gDeferredEmissiveProgram.mShaderFiles.push_back(make_pair("deferred/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredEmissiveProgram.mShaderFiles.push_back(make_pair("deferred/emissiveF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredEmissiveProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredEmissiveProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		// load water shader
		gDeferredWaterProgram.mName = "Deferred Water Shader";
		gDeferredWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWaterProgram.mFeatures.hasGamma = true;
		gDeferredWaterProgram.mFeatures.hasTransport = true;
		gDeferredWaterProgram.mFeatures.encodesNormal = true;
        gDeferredWaterProgram.mFeatures.hasSrgb = true;

		gDeferredWaterProgram.mShaderFiles.clear();
		gDeferredWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gDeferredWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		// load water shader
		gDeferredUnderWaterProgram.mName = "Deferred Under Water Shader";
		gDeferredUnderWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredUnderWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredUnderWaterProgram.mFeatures.hasGamma = true;
		gDeferredUnderWaterProgram.mFeatures.hasTransport = true;
		gDeferredUnderWaterProgram.mFeatures.hasSrgb = true;
		gDeferredUnderWaterProgram.mFeatures.encodesNormal = true;
		//gDeferredUnderWaterProgram.mFeatures.hasShadows = true;

		gDeferredUnderWaterProgram.mShaderFiles.clear();
		gDeferredUnderWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredUnderWaterProgram.mShaderFiles.push_back(make_pair("deferred/underWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredUnderWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gDeferredUnderWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSoftenProgram.mName = "Deferred Soften Shader";
		gDeferredSoftenProgram.mShaderFiles.clear();
		gDeferredSoftenProgram.mFeatures.hasSrgb = true;
		gDeferredSoftenProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSoftenProgram.mFeatures.hasAtmospherics = true;
		gDeferredSoftenProgram.mFeatures.hasTransport = true;
		gDeferredSoftenProgram.mFeatures.hasGamma = true;
		gDeferredSoftenProgram.mFeatures.isDeferred = true;
		gDeferredSoftenProgram.mFeatures.hasShadows = use_sun_shadow;

        gDeferredSoftenProgram.clearPermutations();
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightF.glsl", GL_FRAGMENT_SHADER_ARB));

		gDeferredSoftenProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        if (ambient_kill)
        {
            gDeferredSoftenProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredSoftenProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredSoftenProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		if (gSavedSettings.getBOOL("RenderDeferredSSAO"))
		{ //if using SSAO, take screen space light map into account as if shadows are enabled
			gDeferredSoftenProgram.mShaderLevel = llmax(gDeferredSoftenProgram.mShaderLevel, 2);
		}
				
		success = gDeferredSoftenProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSoftenWaterProgram.mName = "Deferred Soften Underwater Shader";
		gDeferredSoftenWaterProgram.mShaderFiles.clear();
		gDeferredSoftenWaterProgram.mShaderFiles.push_back(make_pair("deferred/softenLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSoftenWaterProgram.mShaderFiles.push_back(make_pair("deferred/softenLightF.glsl", GL_FRAGMENT_SHADER_ARB));

        gDeferredSoftenWaterProgram.clearPermutations();
		gDeferredSoftenWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredSoftenWaterProgram.addPermutation("WATER_FOG", "1");
		gDeferredSoftenWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gDeferredSoftenWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredSoftenWaterProgram.mFeatures.hasSrgb = true;
		gDeferredSoftenWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSoftenWaterProgram.mFeatures.hasAtmospherics = true;
		gDeferredSoftenWaterProgram.mFeatures.hasTransport = true;
		gDeferredSoftenWaterProgram.mFeatures.hasGamma = true;
        gDeferredSoftenWaterProgram.mFeatures.isDeferred = true;
        gDeferredSoftenWaterProgram.mFeatures.hasShadows = use_sun_shadow;

        if (ambient_kill)
        {
            gDeferredSoftenWaterProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredSoftenWaterProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredSoftenWaterProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }

		if (gSavedSettings.getBOOL("RenderDeferredSSAO"))
		{ //if using SSAO, take screen space light map into account as if shadows are enabled
			gDeferredSoftenWaterProgram.mShaderLevel = llmax(gDeferredSoftenWaterProgram.mShaderLevel, 2);
		}

		success = gDeferredSoftenWaterProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredShadowProgram.mName = "Deferred Shadow Shader";
		gDeferredShadowProgram.mFeatures.isDeferred = true;
		gDeferredShadowProgram.mFeatures.hasShadows = true;
		gDeferredShadowProgram.mShaderFiles.clear();
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredShadowProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		success = gDeferredShadowProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredShadowCubeProgram.mName = "Deferred Shadow Cube Shader";
		gDeferredShadowCubeProgram.mFeatures.isDeferred = true;
		gDeferredShadowCubeProgram.mFeatures.hasShadows = true;
		gDeferredShadowCubeProgram.mShaderFiles.clear();
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowCubeV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredShadowCubeProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		gDeferredShadowCubeProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowCubeProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredShadowFullbrightAlphaMaskProgram.mName = "Deferred Shadow Fullbright Alpha Mask Shader";
		gDeferredShadowFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER_ARB));

        gDeferredShadowFullbrightAlphaMaskProgram.clearPermutations();
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredShadowFullbrightAlphaMaskProgram.addPermutation("DEPTH_CLAMP", "1");
		}
        gDeferredShadowFullbrightAlphaMaskProgram.addPermutation("IS_FULLBRIGHT", "1");
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowFullbrightAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}
    
    if (success)
	{
		gDeferredShadowAlphaMaskProgram.mName = "Deferred Shadow Alpha Mask Shader";
		gDeferredShadowAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

		gDeferredShadowAlphaMaskProgram.mShaderFiles.clear();
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredShadowAlphaMaskProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		gDeferredShadowAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarShadowProgram.mName = "Deferred Avatar Shadow Shader";
		gDeferredAvatarShadowProgram.mFeatures.hasSkinning = true;

		gDeferredAvatarShadowProgram.mShaderFiles.clear();
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredAvatarShadowProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		gDeferredAvatarShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarShadowProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarAlphaShadowProgram.mName = "Deferred Avatar Alpha Shadow Shader";
		gDeferredAvatarAlphaShadowProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.clear();
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarAlphaShadowProgram.addPermutation("DEPTH_CLAMP", gGLManager.mHasDepthClamp ? "1" : "0");
		gDeferredAvatarAlphaShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarAlphaShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

    if (success)
	{
		gDeferredAvatarAlphaMaskShadowProgram.mName = "Deferred Avatar Alpha Mask Shadow Shader";
		gDeferredAvatarAlphaMaskShadowProgram.mFeatures.hasSkinning  = true;
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.clear();
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaMaskShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarAlphaMaskShadowProgram.addPermutation("DEPTH_CLAMP", gGLManager.mHasDepthClamp ? "1" : "0");
		gDeferredAvatarAlphaMaskShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarAlphaMaskShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredAttachmentShadowProgram.mName = "Deferred Attachment Shadow Shader";
		gDeferredAttachmentShadowProgram.mFeatures.hasObjectSkinning = true;

		gDeferredAttachmentShadowProgram.mShaderFiles.clear();
		gDeferredAttachmentShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAttachmentShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		if (gGLManager.mHasDepthClamp)
		{
			gDeferredAttachmentShadowProgram.addPermutation("DEPTH_CLAMP", "1");
		}
		gDeferredAttachmentShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAttachmentShadowProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAttachmentAlphaShadowProgram.mName = "Deferred Attachment Alpha Shadow Shader";
		gDeferredAttachmentAlphaShadowProgram.mFeatures.hasObjectSkinning = true;
		gDeferredAttachmentAlphaShadowProgram.mShaderFiles.clear();
		gDeferredAttachmentAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentAlphaShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAttachmentAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentAlphaShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAttachmentAlphaShadowProgram.addPermutation("DEPTH_CLAMP", gGLManager.mHasDepthClamp ? "1" : "0");
		gDeferredAttachmentAlphaShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAttachmentAlphaShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

    if (success)
	{
		gDeferredAttachmentAlphaMaskShadowProgram.mName = "Deferred Attachment Alpha Mask Shadow Shader";
		gDeferredAttachmentAlphaMaskShadowProgram.mFeatures.hasObjectSkinning = true;
		gDeferredAttachmentAlphaMaskShadowProgram.mShaderFiles.clear();
		gDeferredAttachmentAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentAlphaShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAttachmentAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentAlphaMaskShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAttachmentAlphaMaskShadowProgram.addPermutation("DEPTH_CLAMP", gGLManager.mHasDepthClamp ? "1" : "0");
		gDeferredAttachmentAlphaMaskShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAttachmentAlphaMaskShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredTerrainProgram.mName = "Deferred Terrain Shader";
		gDeferredTerrainProgram.mFeatures.encodesNormal = true;
		gDeferredTerrainProgram.mFeatures.hasSrgb = true;
		gDeferredTerrainProgram.mFeatures.calculatesLighting = false;
		gDeferredTerrainProgram.mFeatures.hasLighting = false;
		gDeferredTerrainProgram.mFeatures.isAlphaLighting = true;
		gDeferredTerrainProgram.mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
		gDeferredTerrainProgram.mFeatures.hasWaterFog = true;
		gDeferredTerrainProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasGamma = true;
		gDeferredTerrainProgram.mFeatures.hasTransport = true;

		gDeferredTerrainProgram.mShaderFiles.clear();
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTerrainProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gDeferredTerrainProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredTerrainWaterProgram.mName = "Deferred Terrain Underwater Shader";
		gDeferredTerrainWaterProgram.mFeatures.encodesNormal = true;
		gDeferredTerrainWaterProgram.mFeatures.hasSrgb = true;
		gDeferredTerrainWaterProgram.mFeatures.calculatesLighting = false;
		gDeferredTerrainWaterProgram.mFeatures.hasLighting = false;
		gDeferredTerrainWaterProgram.mFeatures.isAlphaLighting = true;
		gDeferredTerrainWaterProgram.mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
		gDeferredTerrainWaterProgram.mFeatures.hasWaterFog = true;
		gDeferredTerrainWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredTerrainWaterProgram.mFeatures.hasAtmospherics = true;
		gDeferredTerrainWaterProgram.mFeatures.hasGamma = true;
		gDeferredTerrainWaterProgram.mFeatures.hasTransport = true;

		gDeferredTerrainWaterProgram.mShaderFiles.clear();
		gDeferredTerrainWaterProgram.mShaderFiles.push_back(make_pair("deferred/terrainV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTerrainWaterProgram.mShaderFiles.push_back(make_pair("deferred/terrainF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTerrainWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredTerrainWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		gDeferredTerrainWaterProgram.addPermutation("WATER_FOG", "1");
		success = gDeferredTerrainWaterProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredAvatarProgram.mName = "Avatar Shader";
		gDeferredAvatarProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarProgram.mShaderFiles.clear();
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarAlphaProgram.mName = "Avatar Alpha Shader";
		gDeferredAvatarAlphaProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasSrgb = true;
		gDeferredAvatarAlphaProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasAtmospherics = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasTransport = true;
        gDeferredAvatarAlphaProgram.mFeatures.hasGamma = true;
        gDeferredAvatarAlphaProgram.mFeatures.isDeferred = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasShadows = true;

		gDeferredAvatarAlphaProgram.mShaderFiles.clear();
        gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER_ARB));

		gDeferredAvatarAlphaProgram.clearPermutations();
		gDeferredAvatarAlphaProgram.addPermutation("USE_DIFFUSE_TEX", "1");
		gDeferredAvatarAlphaProgram.addPermutation("IS_AVATAR_SKIN", "1");
		if (use_sun_shadow)
		{
			gDeferredAvatarAlphaProgram.addPermutation("HAS_SHADOW", "1");
		}

        if (ambient_kill)
        {
            gDeferredAvatarAlphaProgram.addPermutation("AMBIENT_KILL", "1");
        }

        if (sunlight_kill)
        {
            gDeferredAvatarAlphaProgram.addPermutation("SUNLIGHT_KILL", "1");
        }

        if (local_light_kill)
        {
            gDeferredAvatarAlphaProgram.addPermutation("LOCAL_LIGHT_KILL", "1");
        }
		gDeferredAvatarAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		success = gDeferredAvatarAlphaProgram.createShader(NULL, NULL);
		llassert(success);

		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gDeferredPostGammaCorrectProgram.mName = "Deferred Gamma Correction Post Process";
		gDeferredPostGammaCorrectProgram.mFeatures.hasSrgb = true;
		gDeferredPostGammaCorrectProgram.mFeatures.isDeferred = true;
		gDeferredPostGammaCorrectProgram.mShaderFiles.clear();
		gDeferredPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredGammaCorrect.glsl", GL_FRAGMENT_SHADER_ARB));
        gDeferredPostGammaCorrectProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostGammaCorrectProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gFXAAProgram.mName = "FXAA Shader";
		gFXAAProgram.mFeatures.isDeferred = true;
		gFXAAProgram.mShaderFiles.clear();
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredV.glsl", GL_VERTEX_SHADER_ARB));
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/fxaaF.glsl", GL_FRAGMENT_SHADER_ARB));
		gFXAAProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gFXAAProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredPostProgram.mName = "Deferred Post Shader";
		gFXAAProgram.mFeatures.isDeferred = true;
		gDeferredPostProgram.mShaderFiles.clear();
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredPostProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredCoFProgram.mName = "Deferred CoF Shader";
		gDeferredCoFProgram.mShaderFiles.clear();
		gDeferredCoFProgram.mFeatures.isDeferred = true;
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/cofF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredCoFProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredCoFProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredDoFCombineProgram.mName = "Deferred DoFCombine Shader";
		gDeferredDoFCombineProgram.mFeatures.isDeferred = true;
		gDeferredDoFCombineProgram.mShaderFiles.clear();
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/dofCombineF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDoFCombineProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredDoFCombineProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredPostNoDoFProgram.mName = "Deferred Post Shader";
		gDeferredPostNoDoFProgram.mFeatures.isDeferred = true;
		gDeferredPostNoDoFProgram.mShaderFiles.clear();
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoDoFF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredPostNoDoFProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostNoDoFProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredWLSkyProgram.mName = "Deferred Windlight Sky Shader";
		gDeferredWLSkyProgram.mShaderFiles.clear();
		gDeferredWLSkyProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWLSkyProgram.mFeatures.hasTransport = true;
		gDeferredWLSkyProgram.mFeatures.hasGamma = true;
		gDeferredWLSkyProgram.mFeatures.hasSrgb = true;

		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWLSkyProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;

		success = gDeferredWLSkyProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredWLCloudProgram.mName = "Deferred Windlight Cloud Program";
		gDeferredWLCloudProgram.mShaderFiles.clear();
		gDeferredWLCloudProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWLCloudProgram.mFeatures.hasTransport = true;
        gDeferredWLCloudProgram.mFeatures.hasGamma = true;
        gDeferredWLCloudProgram.mFeatures.hasSrgb = true;
        
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWLCloudProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gDeferredWLCloudProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
	    gDeferredWLSunProgram.mName = "Deferred Windlight Sun Program";
        gDeferredWLSunProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredWLSunProgram.mFeatures.hasTransport = true;
        gDeferredWLSunProgram.mFeatures.hasGamma = true;
        gDeferredWLSunProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLSunProgram.mFeatures.isFullbright = true;
        gDeferredWLSunProgram.mFeatures.disableTextureIndex = true;
        gDeferredWLSunProgram.mFeatures.hasSrgb = true;
        gDeferredWLSunProgram.mShaderFiles.clear();
        gDeferredWLSunProgram.mShaderFiles.push_back(make_pair("deferred/sunDiscV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredWLSunProgram.mShaderFiles.push_back(make_pair("deferred/sunDiscF.glsl", GL_FRAGMENT_SHADER_ARB));
        gDeferredWLSunProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gDeferredWLSunProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gDeferredWLMoonProgram.mName = "Deferred Windlight Moon Program";
        gDeferredWLMoonProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredWLMoonProgram.mFeatures.hasTransport = true;
        gDeferredWLMoonProgram.mFeatures.hasGamma = true;
        gDeferredWLMoonProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLMoonProgram.mFeatures.hasSrgb = true;
        gDeferredWLMoonProgram.mFeatures.isFullbright = true;
        gDeferredWLMoonProgram.mFeatures.disableTextureIndex = true;
        
        gDeferredWLMoonProgram.mShaderFiles.clear();
        gDeferredWLMoonProgram.mShaderFiles.push_back(make_pair("deferred/moonV.glsl", GL_VERTEX_SHADER_ARB));
        gDeferredWLMoonProgram.mShaderFiles.push_back(make_pair("deferred/moonF.glsl", GL_FRAGMENT_SHADER_ARB));
        gDeferredWLMoonProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredWLMoonProgram.mShaderGroup = LLGLSLShader::SG_SKY;
 	 	success = gDeferredWLMoonProgram.createShader(NULL, NULL);
        llassert(success);
 	}

 	if (success)
	{
		gDeferredStarProgram.mName = "Deferred Star Program";
		gDeferredStarProgram.mShaderFiles.clear();
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredStarProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredStarProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gDeferredStarProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gNormalMapGenProgram.mName = "Normal Map Generation Program";
		gNormalMapGenProgram.mShaderFiles.clear();
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenV.glsl", GL_VERTEX_SHADER_ARB));
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenF.glsl", GL_FRAGMENT_SHADER_ARB));
		gNormalMapGenProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gNormalMapGenProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gNormalMapGenProgram.createShader(NULL, NULL);
	}

	return success;
}

BOOL LLViewerShaderMgr::loadShadersObject()
{
	BOOL success = TRUE;
	
	if (mShaderLevel[SHADER_OBJECT] == 0)
	{
		gObjectShinyProgram.unload();
		gObjectFullbrightShinyProgram.unload();
		gObjectFullbrightShinyWaterProgram.unload();
		gObjectShinyWaterProgram.unload();
		gObjectFullbrightNoColorProgram.unload();
		gObjectFullbrightNoColorWaterProgram.unload();
		gObjectSimpleProgram.unload();
		gObjectSimpleImpostorProgram.unload();
		gObjectPreviewProgram.unload();
		gImpostorProgram.unload();
		gObjectSimpleAlphaMaskProgram.unload();
		gObjectBumpProgram.unload();
		gObjectSimpleWaterProgram.unload();
		gObjectSimpleWaterAlphaMaskProgram.unload();
		gObjectEmissiveProgram.unload();
		gObjectEmissiveWaterProgram.unload();
		gObjectFullbrightProgram.unload();
		gObjectFullbrightAlphaMaskProgram.unload();
		gObjectFullbrightWaterProgram.unload();
		gObjectFullbrightWaterAlphaMaskProgram.unload();
		gObjectShinyNonIndexedProgram.unload();
		gObjectFullbrightShinyNonIndexedProgram.unload();
		gObjectFullbrightShinyNonIndexedWaterProgram.unload();
		gObjectShinyNonIndexedWaterProgram.unload();
		gObjectSimpleNonIndexedTexGenProgram.unload();
		gObjectSimpleNonIndexedTexGenWaterProgram.unload();
		gObjectSimpleNonIndexedWaterProgram.unload();
		gObjectAlphaMaskNonIndexedProgram.unload();
		gObjectAlphaMaskNonIndexedWaterProgram.unload();
		gObjectAlphaMaskNoColorProgram.unload();
		gObjectAlphaMaskNoColorWaterProgram.unload();
		gObjectFullbrightNonIndexedProgram.unload();
		gObjectFullbrightNonIndexedWaterProgram.unload();
		gObjectEmissiveNonIndexedProgram.unload();
		gObjectEmissiveNonIndexedWaterProgram.unload();
		gSkinnedObjectSimpleProgram.unload();
		gSkinnedObjectFullbrightProgram.unload();
		gSkinnedObjectEmissiveProgram.unload();
		gSkinnedObjectFullbrightShinyProgram.unload();
		gSkinnedObjectShinySimpleProgram.unload();
		gSkinnedObjectSimpleWaterProgram.unload();
		gSkinnedObjectFullbrightWaterProgram.unload();
		gSkinnedObjectEmissiveWaterProgram.unload();
		gSkinnedObjectFullbrightShinyWaterProgram.unload();
		gSkinnedObjectShinySimpleWaterProgram.unload();
		gTreeProgram.unload();
		gTreeWaterProgram.unload();
	
		return TRUE;
	}

	if (success)
	{
		gObjectSimpleNonIndexedProgram.mName = "Non indexed Shader";
		gObjectSimpleNonIndexedProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectSimpleNonIndexedProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleNonIndexedProgram.mFeatures.hasLighting = true;
        gObjectSimpleNonIndexedProgram.mFeatures.hasAlphaMask = true; // Fix for MAINT-8836
		gObjectSimpleNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectSimpleNonIndexedProgram.mShaderFiles.clear();
		gObjectSimpleNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectSimpleNonIndexedProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gObjectSimpleNonIndexedTexGenProgram.mName = "Non indexed tex-gen Shader";
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.hasGamma = true;
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.hasLighting = true;
		gObjectSimpleNonIndexedTexGenProgram.mFeatures.disableTextureIndex = true;
		gObjectSimpleNonIndexedTexGenProgram.mShaderFiles.clear();
		gObjectSimpleNonIndexedTexGenProgram.mShaderFiles.push_back(make_pair("objects/simpleTexGenV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleNonIndexedTexGenProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleNonIndexedTexGenProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectSimpleNonIndexedTexGenProgram.createShader(NULL, NULL);
	}
	

	if (success)
	{
		gObjectSimpleNonIndexedWaterProgram.mName = "Non indexed Water Shader";
		gObjectSimpleNonIndexedWaterProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleNonIndexedWaterProgram.mFeatures.hasWaterFog = true;
		gObjectSimpleNonIndexedWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleNonIndexedWaterProgram.mFeatures.hasLighting = true;
		gObjectSimpleNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectSimpleNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectSimpleNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectSimpleNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectSimpleNonIndexedWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectSimpleNonIndexedTexGenWaterProgram.mName = "Non indexed tex-gen Water Shader";
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.hasWaterFog = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.hasLighting = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderFiles.clear();
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleTexGenV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectSimpleNonIndexedTexGenWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectAlphaMaskNonIndexedProgram.mName = "Non indexed alpha mask Shader";
		gObjectAlphaMaskNonIndexedProgram.mFeatures.calculatesLighting = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.hasAtmospherics = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.hasLighting = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectAlphaMaskNonIndexedProgram.mFeatures.hasAlphaMask = true;
		gObjectAlphaMaskNonIndexedProgram.mShaderFiles.clear();
		gObjectAlphaMaskNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleNonIndexedV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectAlphaMaskNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectAlphaMaskNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectAlphaMaskNonIndexedProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gObjectAlphaMaskNonIndexedWaterProgram.mName = "Non indexed alpha mask Water Shader";
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.calculatesLighting = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.hasWaterFog = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.hasLighting = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mFeatures.hasAlphaMask = true;
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleNonIndexedV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectAlphaMaskNonIndexedWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectAlphaMaskNoColorProgram.mName = "No color alpha mask Shader";
		gObjectAlphaMaskNoColorProgram.mFeatures.calculatesLighting = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.calculatesAtmospherics = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasGamma = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasAtmospherics = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasLighting = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.disableTextureIndex = true;
		gObjectAlphaMaskNoColorProgram.mFeatures.hasAlphaMask = true;
		gObjectAlphaMaskNoColorProgram.mShaderFiles.clear();
		gObjectAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("objects/simpleNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectAlphaMaskNoColorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectAlphaMaskNoColorProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gObjectAlphaMaskNoColorWaterProgram.mName = "No color alpha mask Water Shader";
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.calculatesLighting = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.hasWaterFog = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.hasLighting = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectAlphaMaskNoColorWaterProgram.mFeatures.hasAlphaMask = true;
		gObjectAlphaMaskNoColorWaterProgram.mShaderFiles.clear();
		gObjectAlphaMaskNoColorWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectAlphaMaskNoColorWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectAlphaMaskNoColorWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectAlphaMaskNoColorWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectAlphaMaskNoColorWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gTreeProgram.mName = "Tree Shader";
		gTreeProgram.mFeatures.calculatesLighting = true;
		gTreeProgram.mFeatures.calculatesAtmospherics = true;
		gTreeProgram.mFeatures.hasGamma = true;
		gTreeProgram.mFeatures.hasAtmospherics = true;
		gTreeProgram.mFeatures.hasLighting = true;
		gTreeProgram.mFeatures.disableTextureIndex = true;
		gTreeProgram.mFeatures.hasAlphaMask = true;
		gTreeProgram.mShaderFiles.clear();
		gTreeProgram.mShaderFiles.push_back(make_pair("objects/treeV.glsl", GL_VERTEX_SHADER_ARB));
		gTreeProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTreeProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gTreeProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gTreeWaterProgram.mName = "Tree Water Shader";
		gTreeWaterProgram.mFeatures.calculatesLighting = true;
		gTreeWaterProgram.mFeatures.calculatesAtmospherics = true;
		gTreeWaterProgram.mFeatures.hasWaterFog = true;
		gTreeWaterProgram.mFeatures.hasAtmospherics = true;
		gTreeWaterProgram.mFeatures.hasLighting = true;
		gTreeWaterProgram.mFeatures.disableTextureIndex = true;
		gTreeWaterProgram.mFeatures.hasAlphaMask = true;
		gTreeWaterProgram.mShaderFiles.clear();
		gTreeWaterProgram.mShaderFiles.push_back(make_pair("objects/treeV.glsl", GL_VERTEX_SHADER_ARB));
		gTreeWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTreeWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gTreeWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gTreeWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightNonIndexedProgram.mName = "Non Indexed Fullbright Shader";
		gObjectFullbrightNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectFullbrightNonIndexedProgram.mFeatures.hasTransport = true;
		gObjectFullbrightNonIndexedProgram.mFeatures.isFullbright = true;
		gObjectFullbrightNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightNonIndexedProgram.mShaderFiles.clear();
		gObjectFullbrightNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightNonIndexedProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightNonIndexedWaterProgram.mName = "Non Indexed Fullbright Water Shader";
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.hasWaterFog = true;		
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightNonIndexedWaterProgram.mFeatures.hasSrgb = true;
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightNonIndexedWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectEmissiveNonIndexedProgram.mName = "Non Indexed Emissive Shader";
		gObjectEmissiveNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectEmissiveNonIndexedProgram.mFeatures.hasTransport = true;
		gObjectEmissiveNonIndexedProgram.mFeatures.isFullbright = true;
		gObjectEmissiveNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectEmissiveNonIndexedProgram.mFeatures.hasSrgb = true;
		gObjectEmissiveNonIndexedProgram.mShaderFiles.clear();
		gObjectEmissiveNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectEmissiveNonIndexedProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectEmissiveNonIndexedWaterProgram.mName = "Non Indexed Emissive Water Shader";
		gObjectEmissiveNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveNonIndexedWaterProgram.mFeatures.isFullbright = true;
		gObjectEmissiveNonIndexedWaterProgram.mFeatures.hasWaterFog = true;		
		gObjectEmissiveNonIndexedWaterProgram.mFeatures.hasTransport = true;
		gObjectEmissiveNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectEmissiveNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectEmissiveNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectEmissiveNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectEmissiveNonIndexedWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightNoColorProgram.mName = "Non Indexed no color Fullbright Shader";
		gObjectFullbrightNoColorProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightNoColorProgram.mFeatures.hasGamma = true;
		gObjectFullbrightNoColorProgram.mFeatures.hasTransport = true;
		gObjectFullbrightNoColorProgram.mFeatures.isFullbright = true;
		gObjectFullbrightNoColorProgram.mFeatures.hasSrgb = true;
		gObjectFullbrightNoColorProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightNoColorProgram.mShaderFiles.clear();
		gObjectFullbrightNoColorProgram.mShaderFiles.push_back(make_pair("objects/fullbrightNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNoColorProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNoColorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightNoColorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightNoColorWaterProgram.mName = "Non Indexed no color Fullbright Water Shader";
		gObjectFullbrightNoColorWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightNoColorWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightNoColorWaterProgram.mFeatures.hasWaterFog = true;		
		gObjectFullbrightNoColorWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightNoColorWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightNoColorWaterProgram.mShaderFiles.clear();
		gObjectFullbrightNoColorWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNoColorWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNoColorWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightNoColorWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightNoColorWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectShinyNonIndexedProgram.mName = "Non Indexed Shiny Shader";
		gObjectShinyNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyNonIndexedProgram.mFeatures.calculatesLighting = true;
		gObjectShinyNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectShinyNonIndexedProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyNonIndexedProgram.mFeatures.isShiny = true;
		gObjectShinyNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectShinyNonIndexedProgram.mShaderFiles.clear();
		gObjectShinyNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/shinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectShinyNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/shinyF.glsl", GL_FRAGMENT_SHADER_ARB));		
		gObjectShinyNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectShinyNonIndexedProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectShinyNonIndexedWaterProgram.mName = "Non Indexed Shiny Water Shader";
		gObjectShinyNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyNonIndexedWaterProgram.mFeatures.calculatesLighting = true;
		gObjectShinyNonIndexedWaterProgram.mFeatures.isShiny = true;
		gObjectShinyNonIndexedWaterProgram.mFeatures.hasWaterFog = true;
		gObjectShinyNonIndexedWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectShinyNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectShinyNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectShinyNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectShinyNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectShinyNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectShinyNonIndexedWaterProgram.createShader(NULL, NULL);
	}
	
    if (success)
	{
		gObjectFullbrightShinyNonIndexedProgram.mName = "Non Indexed Fullbright Shiny Shader";
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.isFullbright = true;
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.isShiny = true;
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.hasGamma = true;
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.hasTransport = true;
		gObjectFullbrightShinyNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightShinyNonIndexedProgram.mShaderFiles.clear();
		gObjectFullbrightShinyNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightShinyNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightShinyNonIndexedProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightShinyNonIndexedProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightShinyNonIndexedWaterProgram.mName = "Non Indexed Fullbright Shiny Water Shader";
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.isShiny = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.hasGamma = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.hasWaterFog = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightShinyNonIndexedWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gImpostorProgram.mName = "Impostor Shader";
		gImpostorProgram.mFeatures.disableTextureIndex = true;
		gImpostorProgram.mFeatures.hasSrgb = true;
		gImpostorProgram.mShaderFiles.clear();
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorV.glsl", GL_VERTEX_SHADER_ARB));
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gImpostorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gImpostorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectPreviewProgram.mName = "Simple Shader";
		gObjectPreviewProgram.mFeatures.calculatesLighting = false;
		gObjectPreviewProgram.mFeatures.calculatesAtmospherics = false;
		gObjectPreviewProgram.mFeatures.hasGamma = false;
		gObjectPreviewProgram.mFeatures.hasAtmospherics = false;
		gObjectPreviewProgram.mFeatures.hasLighting = false;
		gObjectPreviewProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectPreviewProgram.mFeatures.disableTextureIndex = true;
		gObjectPreviewProgram.mShaderFiles.clear();
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectPreviewProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectPreviewProgram.createShader(NULL, NULL);
		gObjectPreviewProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gObjectSimpleProgram.mName = "Simple Shader";
		gObjectSimpleProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleProgram.mFeatures.hasGamma = true;
		gObjectSimpleProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleProgram.mFeatures.hasLighting = true;
		gObjectSimpleProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectSimpleProgram.mShaderFiles.clear();
		gObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectSimpleProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectSimpleImpostorProgram.mName = "Simple Impostor Shader";
		gObjectSimpleImpostorProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleImpostorProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleImpostorProgram.mFeatures.hasGamma = true;
		gObjectSimpleImpostorProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleImpostorProgram.mFeatures.hasLighting = true;
		gObjectSimpleImpostorProgram.mFeatures.mIndexedTextureChannels = 0;
		// force alpha mask version of lighting so we can weed out
		// transparent pixels from impostor temp buffer
		//
		gObjectSimpleImpostorProgram.mFeatures.hasAlphaMask = true; 
		gObjectSimpleImpostorProgram.mShaderFiles.clear();
		gObjectSimpleImpostorProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleImpostorProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleImpostorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		
		success = gObjectSimpleImpostorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectSimpleWaterProgram.mName = "Simple Water Shader";
		gObjectSimpleWaterProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleWaterProgram.mFeatures.hasWaterFog = true;
		gObjectSimpleWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleWaterProgram.mFeatures.hasLighting = true;
		gObjectSimpleWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectSimpleWaterProgram.mShaderFiles.clear();
		gObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectSimpleWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectSimpleWaterProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gObjectBumpProgram.mName = "Bump Shader";
		/*gObjectBumpProgram.mFeatures.calculatesLighting = true;
		gObjectBumpProgram.mFeatures.calculatesAtmospherics = true;
		gObjectBumpProgram.mFeatures.hasGamma = true;
		gObjectBumpProgram.mFeatures.hasAtmospherics = true;
		gObjectBumpProgram.mFeatures.hasLighting = true;
		gObjectBumpProgram.mFeatures.mIndexedTextureChannels = 0;*/
		gObjectBumpProgram.mFeatures.encodesNormal = true;
		gObjectBumpProgram.mShaderFiles.clear();
		gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectBumpProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectBumpProgram.createShader(NULL, NULL);
		if (success)
		{ //lldrawpoolbump assumes "texture0" has channel 0 and "texture1" has channel 1
			gObjectBumpProgram.bind();
			gObjectBumpProgram.uniform1i(sTexture0, 0);
			gObjectBumpProgram.uniform1i(sTexture1, 1);
			gObjectBumpProgram.unbind();
		}
	}
	
	
	if (success)
	{
		gObjectSimpleAlphaMaskProgram.mName = "Simple Alpha Mask Shader";
		gObjectSimpleAlphaMaskProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasGamma = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasLighting = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.hasAlphaMask = true;
		gObjectSimpleAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectSimpleAlphaMaskProgram.mShaderFiles.clear();
		gObjectSimpleAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectSimpleAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectSimpleWaterAlphaMaskProgram.mName = "Simple Water Alpha Mask Shader";
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.calculatesLighting = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.hasWaterFog = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.hasAtmospherics = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.hasLighting = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.hasAlphaMask = true;
		gObjectSimpleWaterAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectSimpleWaterAlphaMaskProgram.mShaderFiles.clear();
		gObjectSimpleWaterAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleWaterAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleWaterAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectSimpleWaterAlphaMaskProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectSimpleWaterAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightProgram.mName = "Fullbright Shader";
		gObjectFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightProgram.mFeatures.hasGamma = true;
		gObjectFullbrightProgram.mFeatures.hasTransport = true;
		gObjectFullbrightProgram.mFeatures.isFullbright = true;
		gObjectFullbrightProgram.mFeatures.hasSrgb = true;
		gObjectFullbrightProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightProgram.mShaderFiles.clear();
		gObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightWaterProgram.mName = "Fullbright Water Shader";
		gObjectFullbrightWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightWaterProgram.mFeatures.hasWaterFog = true;		
		gObjectFullbrightWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightWaterProgram.mShaderFiles.clear();
		gObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectEmissiveProgram.mName = "Emissive Shader";
		gObjectEmissiveProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveProgram.mFeatures.hasGamma = true;
		gObjectEmissiveProgram.mFeatures.hasTransport = true;
		gObjectEmissiveProgram.mFeatures.isFullbright = true;
		gObjectEmissiveProgram.mFeatures.hasSrgb = true;
		gObjectEmissiveProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectEmissiveProgram.mShaderFiles.clear();
		gObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectEmissiveProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectEmissiveWaterProgram.mName = "Emissive Water Shader";
		gObjectEmissiveWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectEmissiveWaterProgram.mFeatures.isFullbright = true;
		gObjectEmissiveWaterProgram.mFeatures.hasWaterFog = true;		
		gObjectEmissiveWaterProgram.mFeatures.hasTransport = true;
		gObjectEmissiveWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectEmissiveWaterProgram.mShaderFiles.clear();
		gObjectEmissiveWaterProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectEmissiveWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectEmissiveWaterProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightAlphaMaskProgram.mName = "Fullbright Alpha Mask Shader";
		gObjectFullbrightAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.hasGamma = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.hasTransport = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.isFullbright = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.hasAlphaMask = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.hasSrgb = true;
		gObjectFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightWaterAlphaMaskProgram.mName = "Fullbright Water Shader";
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.isFullbright = true;
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.hasWaterFog = true;		
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.hasTransport = true;
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.hasAlphaMask = true;
		gObjectFullbrightWaterAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightWaterAlphaMaskProgram.mShaderFiles.clear();
		gObjectFullbrightWaterAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightWaterAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightWaterAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightWaterAlphaMaskProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightWaterAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectShinyProgram.mName = "Shiny Shader";
		gObjectShinyProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyProgram.mFeatures.calculatesLighting = true;
		gObjectShinyProgram.mFeatures.hasGamma = true;
		gObjectShinyProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyProgram.mFeatures.isShiny = true;
		gObjectShinyProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectShinyProgram.mShaderFiles.clear();
		gObjectShinyProgram.mShaderFiles.push_back(make_pair("objects/shinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectShinyProgram.mShaderFiles.push_back(make_pair("objects/shinyF.glsl", GL_FRAGMENT_SHADER_ARB));		
		gObjectShinyProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectShinyProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectShinyWaterProgram.mName = "Shiny Water Shader";
		gObjectShinyWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectShinyWaterProgram.mFeatures.calculatesLighting = true;
		gObjectShinyWaterProgram.mFeatures.isShiny = true;
		gObjectShinyWaterProgram.mFeatures.hasWaterFog = true;
		gObjectShinyWaterProgram.mFeatures.hasAtmospherics = true;
		gObjectShinyWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectShinyWaterProgram.mShaderFiles.clear();
		gObjectShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectShinyWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectShinyWaterProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gObjectFullbrightShinyProgram.mName = "Fullbright Shiny Shader";
		gObjectFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightShinyProgram.mFeatures.isFullbright = true;
		gObjectFullbrightShinyProgram.mFeatures.isShiny = true;
		gObjectFullbrightShinyProgram.mFeatures.hasGamma = true;
		gObjectFullbrightShinyProgram.mFeatures.hasTransport = true;
		gObjectFullbrightShinyProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightShinyProgram.mShaderFiles.clear();
		gObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightShinyProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectFullbrightShinyWaterProgram.mName = "Fullbright Shiny Water Shader";
		gObjectFullbrightShinyWaterProgram.mFeatures.calculatesAtmospherics = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.isFullbright = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.isShiny = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.hasGamma = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.hasTransport = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.hasWaterFog = true;
		gObjectFullbrightShinyWaterProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightShinyWaterProgram.mShaderFiles.clear();
		gObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightShinyWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		gObjectFullbrightShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightShinyWaterProgram.createShader(NULL, NULL);
	}

	if (mShaderLevel[SHADER_AVATAR] > 0)
	{ //load hardware skinned attachment shaders
		if (success)
		{
			gSkinnedObjectSimpleProgram.mName = "Skinned Simple Shader";
			gSkinnedObjectSimpleProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectSimpleProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectSimpleProgram.mFeatures.hasGamma = true;
			gSkinnedObjectSimpleProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectSimpleProgram.mFeatures.hasLighting = true;
			gSkinnedObjectSimpleProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectSimpleProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectSimpleProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectSimpleProgram.mShaderFiles.clear();
			gSkinnedObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectSimpleProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectSimpleProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectFullbrightProgram.mName = "Skinned Fullbright Shader";
			gSkinnedObjectFullbrightProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectFullbrightProgram.mFeatures.hasGamma = true;
			gSkinnedObjectFullbrightProgram.mFeatures.hasTransport = true;
			gSkinnedObjectFullbrightProgram.mFeatures.isFullbright = true;
			gSkinnedObjectFullbrightProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectFullbrightProgram.mFeatures.hasAlphaMask = true;			
			gSkinnedObjectFullbrightProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightProgram.mFeatures.hasSrgb = true;
			gSkinnedObjectFullbrightProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectEmissiveProgram.mName = "Skinned Emissive Shader";
			gSkinnedObjectEmissiveProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectEmissiveProgram.mFeatures.hasGamma = true;
			gSkinnedObjectEmissiveProgram.mFeatures.hasTransport = true;
			gSkinnedObjectEmissiveProgram.mFeatures.isFullbright = true;
			gSkinnedObjectEmissiveProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectEmissiveProgram.mFeatures.disableTextureIndex = true;
            gSkinnedObjectEmissiveProgram.mFeatures.hasSrgb = true;
			gSkinnedObjectEmissiveProgram.mShaderFiles.clear();
			gSkinnedObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/emissiveSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectEmissiveProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectEmissiveProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectEmissiveWaterProgram.mName = "Skinned Emissive Water Shader";
			gSkinnedObjectEmissiveWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.hasTransport = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.isFullbright = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectEmissiveWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectEmissiveWaterProgram.mShaderFiles.clear();
			gSkinnedObjectEmissiveWaterProgram.mShaderFiles.push_back(make_pair("objects/emissiveSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectEmissiveWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectEmissiveWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectEmissiveWaterProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectFullbrightShinyProgram.mName = "Skinned Fullbright Shiny Shader";
			gSkinnedObjectFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.hasGamma = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.hasTransport = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.isShiny = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.isFullbright = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectFullbrightShinyProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinySkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightShinyProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectShinySimpleProgram.mName = "Skinned Shiny Simple Shader";
			gSkinnedObjectShinySimpleProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasGamma = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.isShiny = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectShinySimpleProgram.mShaderFiles.clear();
			gSkinnedObjectShinySimpleProgram.mShaderFiles.push_back(make_pair("objects/shinySimpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectShinySimpleProgram.mShaderFiles.push_back(make_pair("objects/shinyF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectShinySimpleProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectShinySimpleProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectSimpleWaterProgram.mName = "Skinned Simple Water Shader";
			gSkinnedObjectSimpleWaterProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasLighting = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectSimpleWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectSimpleWaterProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.clear();
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectSimpleWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectSimpleWaterProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectFullbrightWaterProgram.mName = "Skinned Fullbright Water Shader";
			gSkinnedObjectFullbrightWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasTransport = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.isFullbright = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightWaterProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectFullbrightShinyWaterProgram.mName = "Skinned Fullbright Shiny Water Shader";
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasTransport = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.isShiny = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.isFullbright = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinySkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightShinyWaterProgram.createShader(NULL, NULL);
		}

		if (success)
		{
			gSkinnedObjectShinySimpleWaterProgram.mName = "Skinned Shiny Simple Water Shader";
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasAlphaMask = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.isShiny = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectShinySimpleWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.clear();
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/shinySimpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectShinySimpleWaterProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectShinySimpleWaterProgram.createShader(NULL, NULL);
		}
	}

	if( !success )
	{
		mShaderLevel[SHADER_OBJECT] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersAvatar()
{
	BOOL success = TRUE;

	if (mShaderLevel[SHADER_AVATAR] == 0)
	{
		gAvatarProgram.unload();
		gAvatarWaterProgram.unload();
		gAvatarEyeballProgram.unload();
		gAvatarPickProgram.unload();
		return TRUE;
	}

	if (success)
	{
		gAvatarProgram.mName = "Avatar Shader";
		gAvatarProgram.mFeatures.hasSkinning = true;
		gAvatarProgram.mFeatures.calculatesAtmospherics = true;
		gAvatarProgram.mFeatures.calculatesLighting = true;
		gAvatarProgram.mFeatures.hasGamma = true;
		gAvatarProgram.mFeatures.hasAtmospherics = true;
		gAvatarProgram.mFeatures.hasLighting = true;
		gAvatarProgram.mFeatures.hasAlphaMask = true;
		gAvatarProgram.mFeatures.disableTextureIndex = true;
		gAvatarProgram.mShaderFiles.clear();
		gAvatarProgram.mShaderFiles.push_back(make_pair("avatar/avatarV.glsl", GL_VERTEX_SHADER_ARB));
		gAvatarProgram.mShaderFiles.push_back(make_pair("avatar/avatarF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAvatarProgram.mShaderLevel = mShaderLevel[SHADER_AVATAR];
		success = gAvatarProgram.createShader(NULL, NULL);
			
		if (success)
		{
			gAvatarWaterProgram.mName = "Avatar Water Shader";
			gAvatarWaterProgram.mFeatures.hasSkinning = true;
			gAvatarWaterProgram.mFeatures.calculatesAtmospherics = true;
			gAvatarWaterProgram.mFeatures.calculatesLighting = true;
			gAvatarWaterProgram.mFeatures.hasWaterFog = true;
			gAvatarWaterProgram.mFeatures.hasAtmospherics = true;
			gAvatarWaterProgram.mFeatures.hasLighting = true;
			gAvatarWaterProgram.mFeatures.hasAlphaMask = true;
			gAvatarWaterProgram.mFeatures.disableTextureIndex = true;
			gAvatarWaterProgram.mShaderFiles.clear();
			gAvatarWaterProgram.mShaderFiles.push_back(make_pair("avatar/avatarV.glsl", GL_VERTEX_SHADER_ARB));
			gAvatarWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			// Note: no cloth under water:
			gAvatarWaterProgram.mShaderLevel = llmin(mShaderLevel[SHADER_AVATAR], 1);	
			gAvatarWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;				
			success = gAvatarWaterProgram.createShader(NULL, NULL);
		}

		/// Keep track of avatar levels
		if (gAvatarProgram.mShaderLevel != mShaderLevel[SHADER_AVATAR])
		{
			mMaxAvatarShaderLevel = mShaderLevel[SHADER_AVATAR] = gAvatarProgram.mShaderLevel;
		}
	}

	if (success)
	{
		gAvatarPickProgram.mName = "Avatar Pick Shader";
		gAvatarPickProgram.mFeatures.hasSkinning = true;
		gAvatarPickProgram.mFeatures.disableTextureIndex = true;
		gAvatarPickProgram.mShaderFiles.clear();
		gAvatarPickProgram.mShaderFiles.push_back(make_pair("avatar/pickAvatarV.glsl", GL_VERTEX_SHADER_ARB));
		gAvatarPickProgram.mShaderFiles.push_back(make_pair("avatar/pickAvatarF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAvatarPickProgram.mShaderLevel = mShaderLevel[SHADER_AVATAR];
		success = gAvatarPickProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gAvatarEyeballProgram.mName = "Avatar Eyeball Program";
		gAvatarEyeballProgram.mFeatures.calculatesLighting = true;
		gAvatarEyeballProgram.mFeatures.isSpecular = true;
		gAvatarEyeballProgram.mFeatures.calculatesAtmospherics = true;
		gAvatarEyeballProgram.mFeatures.hasGamma = true;
		gAvatarEyeballProgram.mFeatures.hasAtmospherics = true;
		gAvatarEyeballProgram.mFeatures.hasLighting = true;
		gAvatarEyeballProgram.mFeatures.hasAlphaMask = true;
		gAvatarEyeballProgram.mFeatures.disableTextureIndex = true;
		gAvatarEyeballProgram.mShaderFiles.clear();
		gAvatarEyeballProgram.mShaderFiles.push_back(make_pair("avatar/eyeballV.glsl", GL_VERTEX_SHADER_ARB));
		gAvatarEyeballProgram.mShaderFiles.push_back(make_pair("avatar/eyeballF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAvatarEyeballProgram.mShaderLevel = mShaderLevel[SHADER_AVATAR];
		success = gAvatarEyeballProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mShaderLevel[SHADER_AVATAR] = 0;
		mMaxAvatarShaderLevel = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersInterface()
{
	BOOL success = TRUE;

	if (mShaderLevel[SHADER_INTERFACE] == 0)
	{
		gHighlightProgram.unload();
		return TRUE;
	}
	
	if (success)
	{
		gHighlightProgram.mName = "Highlight Shader";
		gHighlightProgram.mShaderFiles.clear();
		gHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightV.glsl", GL_VERTEX_SHADER_ARB));
		gHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gHighlightProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gHighlightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gHighlightNormalProgram.mName = "Highlight Normals Shader";
		gHighlightNormalProgram.mShaderFiles.clear();
		gHighlightNormalProgram.mShaderFiles.push_back(make_pair("interface/highlightNormV.glsl", GL_VERTEX_SHADER_ARB));
		gHighlightNormalProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gHighlightNormalProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gHighlightNormalProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gHighlightSpecularProgram.mName = "Highlight Spec Shader";
		gHighlightSpecularProgram.mShaderFiles.clear();
		gHighlightSpecularProgram.mShaderFiles.push_back(make_pair("interface/highlightSpecV.glsl", GL_VERTEX_SHADER_ARB));
		gHighlightSpecularProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gHighlightSpecularProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gHighlightSpecularProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gUIProgram.mName = "UI Shader";
		gUIProgram.mShaderFiles.clear();
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiV.glsl", GL_VERTEX_SHADER_ARB));
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiF.glsl", GL_FRAGMENT_SHADER_ARB));
		gUIProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gUIProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingProgram.mName = "Pathfinding Shader";
		gPathfindingProgram.mShaderFiles.clear();
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingV.glsl", GL_VERTEX_SHADER_ARB));
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER_ARB));
		gPathfindingProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gPathfindingProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingNoNormalsProgram.mName = "PathfindingNoNormals Shader";
		gPathfindingNoNormalsProgram.mShaderFiles.clear();
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingNoNormalV.glsl", GL_VERTEX_SHADER_ARB));
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER_ARB));
		gPathfindingNoNormalsProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gPathfindingNoNormalsProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gCustomAlphaProgram.mName = "Custom Alpha Shader";
		gCustomAlphaProgram.mShaderFiles.clear();
		gCustomAlphaProgram.mShaderFiles.push_back(make_pair("interface/customalphaV.glsl", GL_VERTEX_SHADER_ARB));
		gCustomAlphaProgram.mShaderFiles.push_back(make_pair("interface/customalphaF.glsl", GL_FRAGMENT_SHADER_ARB));
		gCustomAlphaProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gCustomAlphaProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gSplatTextureRectProgram.mName = "Splat Texture Rect Shader";
		gSplatTextureRectProgram.mShaderFiles.clear();
		gSplatTextureRectProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectV.glsl", GL_VERTEX_SHADER_ARB));
		gSplatTextureRectProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectF.glsl", GL_FRAGMENT_SHADER_ARB));
		gSplatTextureRectProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gSplatTextureRectProgram.createShader(NULL, NULL);
		if (success)
		{
			gSplatTextureRectProgram.bind();
			gSplatTextureRectProgram.uniform1i(sScreenMap, 0);
			gSplatTextureRectProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineProgram.mName = "Glow Combine Shader";
		gGlowCombineProgram.mShaderFiles.clear();
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowCombineProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gGlowCombineProgram.createShader(NULL, NULL);
		if (success)
		{
			gGlowCombineProgram.bind();
			gGlowCombineProgram.uniform1i(sGlowMap, 0);
			gGlowCombineProgram.uniform1i(sScreenMap, 1);
			gGlowCombineProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineFXAAProgram.mName = "Glow CombineFXAA Shader";
		gGlowCombineFXAAProgram.mShaderFiles.clear();
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowCombineFXAAProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gGlowCombineFXAAProgram.createShader(NULL, NULL);
		if (success)
		{
			gGlowCombineFXAAProgram.bind();
			gGlowCombineFXAAProgram.uniform1i(sGlowMap, 0);
			gGlowCombineFXAAProgram.uniform1i(sScreenMap, 1);
			gGlowCombineFXAAProgram.unbind();
		}
	}


	if (success)
	{
		gTwoTextureAddProgram.mName = "Two Texture Add Shader";
		gTwoTextureAddProgram.mShaderFiles.clear();
		gTwoTextureAddProgram.mShaderFiles.push_back(make_pair("interface/twotextureaddV.glsl", GL_VERTEX_SHADER_ARB));
		gTwoTextureAddProgram.mShaderFiles.push_back(make_pair("interface/twotextureaddF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTwoTextureAddProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gTwoTextureAddProgram.createShader(NULL, NULL);
		if (success)
		{
			gTwoTextureAddProgram.bind();
			gTwoTextureAddProgram.uniform1i(sTex0, 0);
			gTwoTextureAddProgram.uniform1i(sTex1, 1);
		}
	}

#ifdef LL_WINDOWS
	if (success)
	{
		gTwoTextureCompareProgram.mName = "Two Texture Compare Shader";
		gTwoTextureCompareProgram.mShaderFiles.clear();
		gTwoTextureCompareProgram.mShaderFiles.push_back(make_pair("interface/twotexturecompareV.glsl", GL_VERTEX_SHADER_ARB));
		gTwoTextureCompareProgram.mShaderFiles.push_back(make_pair("interface/twotexturecompareF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTwoTextureCompareProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gTwoTextureCompareProgram.createShader(NULL, NULL);
		if (success)
		{
			gTwoTextureCompareProgram.bind();
			gTwoTextureCompareProgram.uniform1i(sTex0, 0);
			gTwoTextureCompareProgram.uniform1i(sTex1, 1);
			gTwoTextureCompareProgram.uniform1i(sDitherTex, 2);
		}
	}

	if (success)
	{
		gOneTextureFilterProgram.mName = "One Texture Filter Shader";
		gOneTextureFilterProgram.mShaderFiles.clear();
		gOneTextureFilterProgram.mShaderFiles.push_back(make_pair("interface/onetexturefilterV.glsl", GL_VERTEX_SHADER_ARB));
		gOneTextureFilterProgram.mShaderFiles.push_back(make_pair("interface/onetexturefilterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOneTextureFilterProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gOneTextureFilterProgram.createShader(NULL, NULL);
		if (success)
		{
			gOneTextureFilterProgram.bind();
			gOneTextureFilterProgram.uniform1i(sTex0, 0);
		}
	}
#endif

	if (success)
	{
		gOneTextureNoColorProgram.mName = "One Texture No Color Shader";
		gOneTextureNoColorProgram.mShaderFiles.clear();
		gOneTextureNoColorProgram.mShaderFiles.push_back(make_pair("interface/onetexturenocolorV.glsl", GL_VERTEX_SHADER_ARB));
		gOneTextureNoColorProgram.mShaderFiles.push_back(make_pair("interface/onetexturenocolorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOneTextureNoColorProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gOneTextureNoColorProgram.createShader(NULL, NULL);
		if (success)
		{
			gOneTextureNoColorProgram.bind();
			gOneTextureNoColorProgram.uniform1i(sTex0, 0);
		}
	}

	if (success)
	{
		gSolidColorProgram.mName = "Solid Color Shader";
		gSolidColorProgram.mShaderFiles.clear();
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorV.glsl", GL_VERTEX_SHADER_ARB));
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gSolidColorProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gSolidColorProgram.createShader(NULL, NULL);
		if (success)
		{
			gSolidColorProgram.bind();
			gSolidColorProgram.uniform1i(sTex0, 0);
			gSolidColorProgram.unbind();
		}
	}

	if (success)
	{
		gOcclusionProgram.mName = "Occlusion Shader";
		gOcclusionProgram.mShaderFiles.clear();
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionV.glsl", GL_VERTEX_SHADER_ARB));
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOcclusionProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gOcclusionProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gOcclusionCubeProgram.mName = "Occlusion Cube Shader";
		gOcclusionCubeProgram.mShaderFiles.clear();
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionCubeV.glsl", GL_VERTEX_SHADER_ARB));
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOcclusionCubeProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gOcclusionCubeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDebugProgram.mName = "Debug Shader";
		gDebugProgram.mShaderFiles.clear();
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugV.glsl", GL_VERTEX_SHADER_ARB));
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDebugProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gDebugProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gClipProgram.mName = "Clip Shader";
		gClipProgram.mShaderFiles.clear();
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipV.glsl", GL_VERTEX_SHADER_ARB));
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipF.glsl", GL_FRAGMENT_SHADER_ARB));
		gClipProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gClipProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDownsampleDepthProgram.mName = "DownsampleDepth Shader";
		gDownsampleDepthProgram.mShaderFiles.clear();
		gDownsampleDepthProgram.mShaderFiles.push_back(make_pair("interface/downsampleDepthV.glsl", GL_VERTEX_SHADER_ARB));
		gDownsampleDepthProgram.mShaderFiles.push_back(make_pair("interface/downsampleDepthF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDownsampleDepthProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gDownsampleDepthProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gBenchmarkProgram.mName = "Benchmark Shader";
		gBenchmarkProgram.mShaderFiles.clear();
		gBenchmarkProgram.mShaderFiles.push_back(make_pair("interface/benchmarkV.glsl", GL_VERTEX_SHADER_ARB));
		gBenchmarkProgram.mShaderFiles.push_back(make_pair("interface/benchmarkF.glsl", GL_FRAGMENT_SHADER_ARB));
		gBenchmarkProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gBenchmarkProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDownsampleDepthRectProgram.mName = "DownsampleDepthRect Shader";
		gDownsampleDepthRectProgram.mShaderFiles.clear();
		gDownsampleDepthRectProgram.mShaderFiles.push_back(make_pair("interface/downsampleDepthV.glsl", GL_VERTEX_SHADER_ARB));
		gDownsampleDepthRectProgram.mShaderFiles.push_back(make_pair("interface/downsampleDepthRectF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDownsampleDepthRectProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gDownsampleDepthRectProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gAlphaMaskProgram.mName = "Alpha Mask Shader";
		gAlphaMaskProgram.mShaderFiles.clear();
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskV.glsl", GL_VERTEX_SHADER_ARB));
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gAlphaMaskProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mShaderLevel[SHADER_INTERFACE] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersWindLight()
{	
	BOOL success = TRUE;

	if (mShaderLevel[SHADER_WINDLIGHT] < 2)
	{
		gWLSkyProgram.unload();
		gWLCloudProgram.unload();
		gWLSunProgram.unload();
		gWLMoonProgram.unload();
		return TRUE;
	}

    if (success)
    {
        gWLSkyProgram.mName = "Windlight Sky Shader";
        gWLSkyProgram.mShaderFiles.clear();
        gWLSkyProgram.mFeatures.calculatesAtmospherics = true;
        gWLSkyProgram.mFeatures.hasTransport = true;
        gWLSkyProgram.mFeatures.hasGamma = true;
        gWLSkyProgram.mFeatures.hasSrgb = true;
        gWLSkyProgram.mShaderFiles.push_back(make_pair("windlight/skyV.glsl", GL_VERTEX_SHADER_ARB));
        gWLSkyProgram.mShaderFiles.push_back(make_pair("windlight/skyF.glsl", GL_FRAGMENT_SHADER_ARB));
        gWLSkyProgram.mShaderLevel = mShaderLevel[SHADER_WINDLIGHT];
        gWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gWLSkyProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gWLCloudProgram.mName = "Windlight Cloud Program";
        gWLCloudProgram.mShaderFiles.clear();
        gWLCloudProgram.mFeatures.calculatesAtmospherics = true;
        gWLCloudProgram.mFeatures.hasTransport = true;
        gWLCloudProgram.mFeatures.hasGamma = true;
        gWLCloudProgram.mFeatures.hasSrgb = true;
        gWLCloudProgram.mShaderFiles.push_back(make_pair("windlight/cloudsV.glsl", GL_VERTEX_SHADER_ARB));
        gWLCloudProgram.mShaderFiles.push_back(make_pair("windlight/cloudsF.glsl", GL_FRAGMENT_SHADER_ARB));
        gWLCloudProgram.mShaderLevel = mShaderLevel[SHADER_WINDLIGHT];
        gWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gWLCloudProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gWLSunProgram.mName = "Windlight Sun Program";
        gWLSunProgram.mShaderFiles.clear();
        gWLSunProgram.mFeatures.calculatesAtmospherics = true;
        gWLSunProgram.mFeatures.hasTransport = true;
        gWLSunProgram.mFeatures.hasGamma = true;
        gWLSunProgram.mFeatures.hasAtmospherics = true;
        gWLSunProgram.mFeatures.isFullbright = true;
        gWLSunProgram.mFeatures.disableTextureIndex = true;
        gWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        gWLSunProgram.mShaderFiles.push_back(make_pair("windlight/sunDiscV.glsl", GL_VERTEX_SHADER_ARB));
        gWLSunProgram.mShaderFiles.push_back(make_pair("windlight/sunDiscF.glsl", GL_FRAGMENT_SHADER_ARB));
        gWLSunProgram.mShaderLevel = mShaderLevel[SHADER_WINDLIGHT];
        gWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gWLSunProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gWLMoonProgram.mName = "Windlight Moon Program";
        gWLMoonProgram.mShaderFiles.clear();
        gWLMoonProgram.mFeatures.calculatesAtmospherics = true;
        gWLMoonProgram.mFeatures.hasTransport = true;
        gWLMoonProgram.mFeatures.hasGamma = true;
        gWLMoonProgram.mFeatures.hasAtmospherics = true;
        gWLMoonProgram.mFeatures.isFullbright = true;
        gWLMoonProgram.mFeatures.disableTextureIndex = true;
        gWLMoonProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        gWLMoonProgram.mShaderFiles.push_back(make_pair("windlight/moonV.glsl", GL_VERTEX_SHADER_ARB));
        gWLMoonProgram.mShaderFiles.push_back(make_pair("windlight/moonF.glsl", GL_FRAGMENT_SHADER_ARB));
        gWLMoonProgram.mShaderLevel = mShaderLevel[SHADER_WINDLIGHT];
        gWLMoonProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gWLMoonProgram.createShader(NULL, NULL);
    }

	return success;
}

BOOL LLViewerShaderMgr::loadTransformShaders()
{
	BOOL success = TRUE;
	
	if (mShaderLevel[SHADER_TRANSFORM] < 1)
	{
		gTransformPositionProgram.unload();
		gTransformTexCoordProgram.unload();
		gTransformNormalProgram.unload();
		gTransformColorProgram.unload();
		gTransformTangentProgram.unload();
		return TRUE;
	}

	if (success)
	{
        gTransformPositionProgram.mName = "Position Transform Shader";
		gTransformPositionProgram.mShaderFiles.clear();
		gTransformPositionProgram.mShaderFiles.push_back(make_pair("transform/positionV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformPositionProgram.mShaderLevel = mShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"position_out",
			"texture_index_out",
		};
	
		success = gTransformPositionProgram.createShader(NULL, NULL, 2, varyings);
	}

	if (success)
	{
		gTransformTexCoordProgram.mName = "TexCoord Transform Shader";
		gTransformTexCoordProgram.mShaderFiles.clear();
		gTransformTexCoordProgram.mShaderFiles.push_back(make_pair("transform/texcoordV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformTexCoordProgram.mShaderLevel = mShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"texcoord_out",
		};
	
		success = gTransformTexCoordProgram.createShader(NULL, NULL, 1, varyings);
	}

	if (success)
	{
		gTransformNormalProgram.mName = "Normal Transform Shader";
		gTransformNormalProgram.mShaderFiles.clear();
		gTransformNormalProgram.mShaderFiles.push_back(make_pair("transform/normalV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformNormalProgram.mShaderLevel = mShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"normal_out",
		};
	
		success = gTransformNormalProgram.createShader(NULL, NULL, 1, varyings);
	}

	if (success)
	{
		gTransformColorProgram.mName = "Color Transform Shader";
		gTransformColorProgram.mShaderFiles.clear();
		gTransformColorProgram.mShaderFiles.push_back(make_pair("transform/colorV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformColorProgram.mShaderLevel = mShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"color_out",
		};
	
		success = gTransformColorProgram.createShader(NULL, NULL, 1, varyings);
	}

	if (success)
	{
		gTransformTangentProgram.mName = "Binormal Transform Shader";
		gTransformTangentProgram.mShaderFiles.clear();
		gTransformTangentProgram.mShaderFiles.push_back(make_pair("transform/binormalV.glsl", GL_VERTEX_SHADER_ARB));
        gTransformTangentProgram.mShaderLevel = mShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"tangent_out",
		};
	
		success = gTransformTangentProgram.createShader(NULL, NULL, 1, varyings);
	}

	
	return success;
}

std::string LLViewerShaderMgr::getShaderDirPrefix(void)
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/class");
}

void LLViewerShaderMgr::updateShaderUniforms(LLGLSLShader * shader)
{
    LLEnvironment::instance().updateShaderUniforms(shader);
}

LLViewerShaderMgr::shader_iter LLViewerShaderMgr::beginShaders() const
{
	return mShaderList.begin();
}

LLViewerShaderMgr::shader_iter LLViewerShaderMgr::endShaders() const
{
	return mShaderList.end();
}

