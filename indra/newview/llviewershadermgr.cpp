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
#include "llversioninfo.h"

#include "llrender.h"
#include "llenvironment.h"
#include "llerrorcontrol.h"
#include "llatmosphere.h"
#include "llworld.h"
#include "llsky.h"

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

bool				LLViewerShaderMgr::sInitialized = false;
bool				LLViewerShaderMgr::sSkipReload = false;

LLVector4			gShinyOrigin;

//utility shaders
LLGLSLShader	gOcclusionProgram;
LLGLSLShader    gSkinnedOcclusionProgram;
LLGLSLShader	gOcclusionCubeProgram;
LLGLSLShader	gGlowCombineProgram;
LLGLSLShader	gReflectionMipProgram;
LLGLSLShader    gGaussianProgram;
LLGLSLShader	gRadianceGenProgram;
LLGLSLShader	gIrradianceGenProgram;
LLGLSLShader	gGlowCombineFXAAProgram;
LLGLSLShader	gTwoTextureCompareProgram;
LLGLSLShader	gOneTextureFilterProgram;
LLGLSLShader	gDebugProgram;
LLGLSLShader    gSkinnedDebugProgram;
LLGLSLShader	gClipProgram;
LLGLSLShader	gAlphaMaskProgram;
LLGLSLShader	gBenchmarkProgram;
LLGLSLShader    gReflectionProbeDisplayProgram;
LLGLSLShader    gCopyProgram;
LLGLSLShader    gCopyDepthProgram;

//object shaders
LLGLSLShader		gObjectPreviewProgram;
LLGLSLShader        gSkinnedObjectPreviewProgram;
LLGLSLShader        gPhysicsPreviewProgram;
LLGLSLShader		gObjectFullbrightAlphaMaskProgram;
LLGLSLShader        gSkinnedObjectFullbrightAlphaMaskProgram;
LLGLSLShader		gObjectBumpProgram;
LLGLSLShader        gSkinnedObjectBumpProgram;
LLGLSLShader		gObjectAlphaMaskNoColorProgram;

//environment shaders
LLGLSLShader		gWaterProgram;
LLGLSLShader        gWaterEdgeProgram;
LLGLSLShader		gUnderWaterProgram;

//interface shaders
LLGLSLShader		gHighlightProgram;
LLGLSLShader        gSkinnedHighlightProgram;
LLGLSLShader		gHighlightNormalProgram;
LLGLSLShader		gHighlightSpecularProgram;

LLGLSLShader		gDeferredHighlightProgram;

LLGLSLShader		gPathfindingProgram;
LLGLSLShader		gPathfindingNoNormalsProgram;

//avatar shader handles
LLGLSLShader		gAvatarProgram;
LLGLSLShader		gAvatarEyeballProgram;
LLGLSLShader		gImpostorProgram;

// Effects Shaders
LLGLSLShader			gGlowProgram;
LLGLSLShader			gGlowExtractProgram;
LLGLSLShader			gPostScreenSpaceReflectionProgram;

// Deferred rendering shaders
LLGLSLShader			gDeferredImpostorProgram;
LLGLSLShader			gDeferredDiffuseProgram;
LLGLSLShader			gDeferredDiffuseAlphaMaskProgram;
LLGLSLShader            gDeferredSkinnedDiffuseAlphaMaskProgram;
LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskProgram;
LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
LLGLSLShader			gDeferredSkinnedDiffuseProgram;
LLGLSLShader			gDeferredSkinnedBumpProgram;
LLGLSLShader			gDeferredBumpProgram;
LLGLSLShader			gDeferredTerrainProgram;
LLGLSLShader			gDeferredTreeProgram;
LLGLSLShader			gDeferredTreeShadowProgram;
LLGLSLShader            gDeferredSkinnedTreeShadowProgram;
LLGLSLShader			gDeferredAvatarProgram;
LLGLSLShader			gDeferredAvatarAlphaProgram;
LLGLSLShader			gDeferredLightProgram;
LLGLSLShader			gDeferredMultiLightProgram[16];
LLGLSLShader			gDeferredSpotLightProgram;
LLGLSLShader			gDeferredMultiSpotLightProgram;
LLGLSLShader			gDeferredSunProgram;
LLGLSLShader            gHazeProgram;
LLGLSLShader            gHazeWaterProgram;
LLGLSLShader			gDeferredBlurLightProgram;
LLGLSLShader			gDeferredSoftenProgram;
LLGLSLShader			gDeferredShadowProgram;
LLGLSLShader            gDeferredSkinnedShadowProgram;
LLGLSLShader			gDeferredShadowCubeProgram;
LLGLSLShader			gDeferredShadowAlphaMaskProgram;
LLGLSLShader            gDeferredSkinnedShadowAlphaMaskProgram;
LLGLSLShader			gDeferredShadowGLTFAlphaMaskProgram;
LLGLSLShader			gDeferredSkinnedShadowGLTFAlphaMaskProgram;
LLGLSLShader            gDeferredShadowGLTFAlphaBlendProgram;
LLGLSLShader            gDeferredSkinnedShadowGLTFAlphaBlendProgram;
LLGLSLShader			gDeferredShadowFullbrightAlphaMaskProgram;
LLGLSLShader            gDeferredSkinnedShadowFullbrightAlphaMaskProgram;
LLGLSLShader			gDeferredAvatarShadowProgram;
LLGLSLShader			gDeferredAvatarAlphaShadowProgram;
LLGLSLShader			gDeferredAvatarAlphaMaskShadowProgram;
LLGLSLShader			gDeferredAlphaProgram;
LLGLSLShader			gHUDAlphaProgram;
LLGLSLShader            gDeferredSkinnedAlphaProgram;
LLGLSLShader			gDeferredAlphaImpostorProgram;
LLGLSLShader            gDeferredSkinnedAlphaImpostorProgram;
LLGLSLShader			gDeferredAvatarEyesProgram;
LLGLSLShader			gDeferredFullbrightProgram;
LLGLSLShader            gHUDFullbrightProgram;
LLGLSLShader			gDeferredFullbrightAlphaMaskProgram;
LLGLSLShader			gHUDFullbrightAlphaMaskProgram;
LLGLSLShader			gDeferredFullbrightAlphaMaskAlphaProgram;
LLGLSLShader			gHUDFullbrightAlphaMaskAlphaProgram;
LLGLSLShader			gDeferredEmissiveProgram;
LLGLSLShader            gDeferredSkinnedEmissiveProgram;
LLGLSLShader			gDeferredPostProgram;
LLGLSLShader			gDeferredCoFProgram;
LLGLSLShader			gDeferredDoFCombineProgram;
LLGLSLShader			gDeferredPostGammaCorrectProgram;
LLGLSLShader            gNoPostGammaCorrectProgram;
LLGLSLShader            gLegacyPostGammaCorrectProgram;
LLGLSLShader			gExposureProgram;
LLGLSLShader			gLuminanceProgram;
LLGLSLShader			gFXAAProgram;
LLGLSLShader			gDeferredPostNoDoFProgram;
LLGLSLShader			gDeferredWLSkyProgram;
LLGLSLShader			gDeferredWLCloudProgram;
LLGLSLShader			gDeferredWLSunProgram;
LLGLSLShader			gDeferredWLMoonProgram;
LLGLSLShader			gDeferredStarProgram;
LLGLSLShader			gDeferredFullbrightShinyProgram;
LLGLSLShader            gHUDFullbrightShinyProgram;
LLGLSLShader            gDeferredSkinnedFullbrightShinyProgram;
LLGLSLShader			gDeferredSkinnedFullbrightProgram;
LLGLSLShader            gDeferredSkinnedFullbrightAlphaMaskProgram;
LLGLSLShader            gDeferredSkinnedFullbrightAlphaMaskAlphaProgram;
LLGLSLShader			gNormalMapGenProgram;
LLGLSLShader            gDeferredGenBrdfLutProgram;
LLGLSLShader            gDeferredBufferVisualProgram;

// Deferred materials shaders
LLGLSLShader			gDeferredMaterialProgram[LLMaterial::SHADER_COUNT*2];
LLGLSLShader			gHUDPBROpaqueProgram;
LLGLSLShader            gPBRGlowProgram;
LLGLSLShader            gPBRGlowSkinnedProgram;
LLGLSLShader			gDeferredPBROpaqueProgram;
LLGLSLShader            gDeferredSkinnedPBROpaqueProgram;
LLGLSLShader            gHUDPBRAlphaProgram;
LLGLSLShader            gDeferredPBRAlphaProgram;
LLGLSLShader            gDeferredSkinnedPBRAlphaProgram;

//helper for making a rigged variant of a given shader
bool make_rigged_variant(LLGLSLShader& shader, LLGLSLShader& riggedShader)
{
    riggedShader.mName = llformat("Skinned %s", shader.mName.c_str());
    riggedShader.mFeatures = shader.mFeatures;
    riggedShader.mFeatures.hasObjectSkinning = true;
    riggedShader.mDefines = shader.mDefines;    // NOTE: Must come before addPermutation
    riggedShader.addPermutation("HAS_SKIN", "1");
    riggedShader.mShaderFiles = shader.mShaderFiles;
    riggedShader.mShaderLevel = shader.mShaderLevel;
    riggedShader.mShaderGroup = shader.mShaderGroup;

    shader.mRiggedVariant = &riggedShader;
    return riggedShader.createShader(NULL, NULL);
}

LLViewerShaderMgr::LLViewerShaderMgr() :
	mShaderLevel(SHADER_COUNT, 0),
	mMaxAvatarShaderLevel(0)
{   
    //ONLY shaders that need WL Param management should be added here
	mShaderList.push_back(&gAvatarProgram);
	mShaderList.push_back(&gWaterProgram);
	mShaderList.push_back(&gWaterEdgeProgram);
	mShaderList.push_back(&gAvatarEyeballProgram); 
	mShaderList.push_back(&gImpostorProgram);
	mShaderList.push_back(&gObjectBumpProgram);
    mShaderList.push_back(&gSkinnedObjectBumpProgram);
	mShaderList.push_back(&gObjectFullbrightAlphaMaskProgram);
    mShaderList.push_back(&gSkinnedObjectFullbrightAlphaMaskProgram);
	mShaderList.push_back(&gObjectAlphaMaskNoColorProgram);
	mShaderList.push_back(&gUnderWaterProgram);
	mShaderList.push_back(&gDeferredSunProgram);
    mShaderList.push_back(&gHazeProgram);
    mShaderList.push_back(&gHazeWaterProgram);
	mShaderList.push_back(&gDeferredSoftenProgram);
	mShaderList.push_back(&gDeferredAlphaProgram);
    mShaderList.push_back(&gHUDAlphaProgram);
    mShaderList.push_back(&gDeferredSkinnedAlphaProgram);
	mShaderList.push_back(&gDeferredAlphaImpostorProgram);
    mShaderList.push_back(&gDeferredSkinnedAlphaImpostorProgram);
	mShaderList.push_back(&gDeferredFullbrightProgram);
    mShaderList.push_back(&gHUDFullbrightProgram);
	mShaderList.push_back(&gDeferredFullbrightAlphaMaskProgram);
    mShaderList.push_back(&gHUDFullbrightAlphaMaskProgram);
    mShaderList.push_back(&gDeferredFullbrightAlphaMaskAlphaProgram);
    mShaderList.push_back(&gHUDFullbrightAlphaMaskAlphaProgram);
	mShaderList.push_back(&gDeferredFullbrightShinyProgram);
    mShaderList.push_back(&gHUDFullbrightShinyProgram);
    mShaderList.push_back(&gDeferredSkinnedFullbrightShinyProgram);
	mShaderList.push_back(&gDeferredSkinnedFullbrightProgram);
    mShaderList.push_back(&gDeferredSkinnedFullbrightAlphaMaskProgram);
    mShaderList.push_back(&gDeferredSkinnedFullbrightAlphaMaskAlphaProgram);
	mShaderList.push_back(&gDeferredEmissiveProgram);
    mShaderList.push_back(&gDeferredSkinnedEmissiveProgram);
	mShaderList.push_back(&gDeferredAvatarEyesProgram);
    mShaderList.push_back(&gDeferredAvatarAlphaProgram);
	mShaderList.push_back(&gDeferredWLSkyProgram);
	mShaderList.push_back(&gDeferredWLCloudProgram);
    mShaderList.push_back(&gDeferredWLMoonProgram);
    mShaderList.push_back(&gDeferredWLSunProgram);
    mShaderList.push_back(&gDeferredPBRAlphaProgram);
    mShaderList.push_back(&gHUDPBRAlphaProgram);
    mShaderList.push_back(&gDeferredSkinnedPBRAlphaProgram);
    mShaderList.push_back(&gDeferredPostGammaCorrectProgram); // for gamma
    mShaderList.push_back(&gNoPostGammaCorrectProgram);
    mShaderList.push_back(&gLegacyPostGammaCorrectProgram);

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
	return mShaderLevel[type];
}

//============================================================================
// Shader Management

void LLViewerShaderMgr::setShaders()
{
    LL_PROFILE_ZONE_SCOPED;
    //setShaders might be called redundantly by gSavedSettings, so return on reentrance
    static bool reentrance = false;
    
    if (!gPipeline.mInitialized || !sInitialized || reentrance || sSkipReload)
    {
        return;
    }

    if (!gGLManager.mHasRequirements)
    {
        // Viewer will show 'hardware requirements' warning later
        LL_INFOS("ShaderLoading") << "Not supported hardware/software" << LL_ENDL;
        return;
    }

	{
		static LLCachedControl<bool> shader_cache_enabled(gSavedSettings, "RenderShaderCacheEnabled", true);
		static LLUUID old_cache_version;
		static LLUUID current_cache_version;
		if (current_cache_version.isNull())
		{
			HBXXH128 hash_obj;
			hash_obj.update(LLVersionInfo::instance().getVersion());
			current_cache_version = hash_obj.digest();

			old_cache_version = LLUUID(gSavedSettings.getString("RenderShaderCacheVersion"));
			gSavedSettings.setString("RenderShaderCacheVersion", current_cache_version.asString());
		}

		initShaderCache(shader_cache_enabled, old_cache_version, current_cache_version);
	}

    static LLCachedControl<U32> max_texture_index(gSavedSettings, "RenderMaxTextureIndex", 16);
    
    // when using indexed texture rendering, leave some texture units available for shadow and reflection maps
    LLGLSLShader::sIndexedTextureChannels = llmax(llmin(gGLManager.mNumTextureImageUnits-12, (S32) max_texture_index), 1);

    reentrance = true;

    // Make sure the compiled shader map is cleared before we recompile shaders.
    mVertexShaderObjects.clear();
    mFragmentShaderObjects.clear();
    
    initAttribsAndUniforms();
    gPipeline.releaseGLBuffers();

	unloadShaders();

    LLPipeline::sRenderGlow = gSavedSettings.getBOOL("RenderGlow"); 
    
    if (gViewerWindow)
    {
        gViewerWindow->setCursor(UI_CURSOR_WAIT);
    }

    // Shaders
    LL_INFOS("ShaderLoading") << "\n~~~~~~~~~~~~~~~~~~\n Loading Shaders:\n~~~~~~~~~~~~~~~~~~" << LL_ENDL;
    LL_INFOS("ShaderLoading") << llformat("Using GLSL %d.%d", gGLManager.mGLSLVersionMajor, gGLManager.mGLSLVersionMinor) << LL_ENDL;

    for (S32 i = 0; i < SHADER_COUNT; i++)
    {
        mShaderLevel[i] = 0;
    }
    mMaxAvatarShaderLevel = 0;

    LLVertexBuffer::unbind();

    llassert((gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 10));

    
    S32 light_class = 3;
    S32 interface_class = 2;
    S32 env_class = 2;
    S32 obj_class = 2;
    S32 effect_class = 2;
    S32 wl_class = 2;
    S32 water_class = 3;
    S32 deferred_class = 3;

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

    std::string shader_name = loadBasicShaders();
    if (shader_name.empty())
    {
        LL_INFOS("Shader") << "Loaded basic shaders." << LL_ENDL;
    }
    else
    {
        // "ShaderLoading" and "Shader" need to be logged
        LLError::ELevel lvl = LLError::getDefaultLevel();
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        loadBasicShaders();
        LLError::setDefaultLevel(lvl);
        LL_ERRS() << "Unable to load basic shader " << shader_name << ", verify graphics driver installed and current." << LL_ENDL;
        reentrance = false; // For hygiene only, re-try probably helps nothing 
        return;
    }

    gPipeline.mShadersLoaded = true;

    bool loaded = loadShadersWater();

    if (loaded)
    {
        LL_INFOS() << "Loaded water shaders." << LL_ENDL;
    }
    else
    {
        LL_WARNS() << "Failed to load water shaders." << LL_ENDL;
        llassert(loaded);
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
        // Load max avatar shaders to set the max level
        mShaderLevel[SHADER_AVATAR] = 3;
        mMaxAvatarShaderLevel = 3;

        if (loadShadersObject())
        { //hardware skinning is enabled and rigged attachment shaders loaded correctly
            // cloth is a class3 shader
            S32 avatar_class = 1;
                
            // Set the actual level
            mShaderLevel[SHADER_AVATAR] = avatar_class;

            loaded = loadShadersAvatar();
            llassert(loaded);
        }
        else
        { //hardware skinning not possible, neither is deferred rendering
            llassert(false); // SHOULD NOT BE POSSIBLE
        }
    }

    llassert(loaded);
    loaded = loaded && loadShadersDeferred();
    llassert(loaded);

	persistShaderCacheMetadata();

    if (gViewerWindow)
    {
        gViewerWindow->setCursor(UI_CURSOR_ARROW);
    }
    gPipeline.createGLBuffers();

    reentrance = false;
}

void LLViewerShaderMgr::unloadShaders()
{
	while (!LLGLSLShader::sInstances.empty())
	{
		LLGLSLShader* shader = *(LLGLSLShader::sInstances.begin());
		shader->unload();
	}

	mShaderLevel[SHADER_LIGHTING] = 0;
	mShaderLevel[SHADER_OBJECT] = 0;
	mShaderLevel[SHADER_AVATAR] = 0;
	mShaderLevel[SHADER_ENVIRONMENT] = 0;
	mShaderLevel[SHADER_WATER] = 0;
	mShaderLevel[SHADER_INTERFACE] = 0;
	mShaderLevel[SHADER_EFFECT] = 0;
	mShaderLevel[SHADER_WINDLIGHT] = 0;

	gPipeline.mShadersLoaded = false;
}

std::string LLViewerShaderMgr::loadBasicShaders()
{
	// Load basic dependency shaders first
	// All of these have to load for any shaders to function
	
	S32 sum_lights_class = 3;

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
	shaders.push_back( make_pair( "windlight/atmosphericsHelpersV.glsl",    mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "lighting/lightFuncV.glsl",               mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsV.glsl",               sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightV.glsl",                   mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/lightFuncSpecularV.glsl",       mShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsSpecularV.glsl",       sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightSpecularV.glsl",           mShaderLevel[SHADER_LIGHTING] ) );
    shaders.push_back( make_pair( "windlight/atmosphericsFuncs.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsV.glsl",           mShaderLevel[SHADER_WINDLIGHT] ) );
    shaders.push_back( make_pair( "environment/srgbF.glsl",                 1 ) );
	shaders.push_back( make_pair( "avatar/avatarSkinV.glsl",                1 ) );
	shaders.push_back( make_pair( "avatar/objectSkinV.glsl",                1 ) );
    shaders.push_back( make_pair( "deferred/textureUtilV.glsl",             1 ) );
	if (gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)
	{
		shaders.push_back( make_pair( "objects/indexedTextureV.glsl",           1 ) );
	}
	shaders.push_back( make_pair( "objects/nonindexedTextureV.glsl",        1 ) );

	std::map<std::string, std::string> attribs;
	attribs["MAX_JOINTS_PER_MESH_OBJECT"] = 
		boost::lexical_cast<std::string>(LLSkinningUtil::getMaxJointCount());

    bool ssr = gSavedSettings.getBOOL("RenderScreenSpaceReflections");

	bool has_reflection_probes = gSavedSettings.getBOOL("RenderReflectionsEnabled") && gGLManager.mGLVersion > 3.99f;

	S32 probe_level = llclamp(gSavedSettings.getS32("RenderReflectionProbeLevel"), 0, 3);

    S32 shadow_detail            = gSavedSettings.getS32("RenderShadowDetail");

    if (shadow_detail >= 1)
    {
        attribs["SUN_SHADOW"] = "1";

        if (shadow_detail >= 2)
        {
            attribs["SPOT_SHADOW"] = "1";
        }
    }

    if (ssr)
    {
        attribs["SSR"] = "1";
    }

	if (has_reflection_probes)
	{
        attribs["REFMAP_LEVEL"] = std::to_string(probe_level);
		attribs["REF_SAMPLE_COUNT"] = "32";
	}

	LLGLSLShader::sGlobalDefines = attribs;

	// We no longer have to bind the shaders to global glhandles, they are automatically added to a map now.
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_VERTEX_SHADER
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_VERTEX_SHADER, &attribs) == 0)
		{
			LL_WARNS("Shader") << "Failed to load vertex shader " << shaders[i].first << LL_ENDL;
			return shaders[i].first;
		}
	}

	// Load the Basic Fragment Shaders at the appropriate level. 
	// (in order of shader function call depth for reference purposes, deepest level first)

	shaders.clear();
	S32 ch = 1;

	if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 30)
	{ //use indexed texture rendering for GLSL >= 1.30
		ch = llmax(LLGLSLShader::sIndexedTextureChannels, 1);
	}


	std::vector<S32> index_channels;    
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsVarsF.glsl",      mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsHelpersF.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/gammaF.glsl",                 mShaderLevel[SHADER_WINDLIGHT]) );
    index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsFuncs.glsl",       mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "windlight/atmosphericsF.glsl",          mShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/waterFogF.glsl",                mShaderLevel[SHADER_WATER] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/encodeNormF.glsl",	mShaderLevel[SHADER_ENVIRONMENT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "environment/srgbF.glsl",                    mShaderLevel[SHADER_ENVIRONMENT] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/deferredUtil.glsl",                    1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/shadowUtil.glsl",                      1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/aoUtil.glsl",                          1) );
    index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/reflectionProbeF.glsl",                has_reflection_probes ? 3 : 2) );
    index_channels.push_back(-1);    shaders.push_back( make_pair( "deferred/screenSpaceReflUtil.glsl",             ssr ? 3 : 1) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightNonIndexedF.glsl",                    mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);    shaders.push_back( make_pair( "lighting/lightAlphaMaskNonIndexedF.glsl",                   mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightF.glsl",                  mShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);    shaders.push_back( make_pair( "lighting/lightAlphaMaskF.glsl",                 mShaderLevel[SHADER_LIGHTING] ) );
	
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_FRAGMENT_SHADER
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_FRAGMENT_SHADER, &attribs, index_channels[i]) == 0)
		{
			LL_WARNS("Shader") << "Failed to load fragment shader " << shaders[i].first << LL_ENDL;
			return shaders[i].first;
		}
	}

	return std::string();
}

bool LLViewerShaderMgr::loadShadersWater()
{
    LL_PROFILE_ZONE_SCOPED;
	bool success = true;
	bool terrainWaterSuccess = true;
    
    bool use_sun_shadow = mShaderLevel[SHADER_DEFERRED] > 1 &&
        gSavedSettings.getS32("RenderShadowDetail") > 0;

	if (mShaderLevel[SHADER_WATER] == 0)
	{
		gWaterProgram.unload();
		gWaterEdgeProgram.unload();
		gUnderWaterProgram.unload();
		return true;
	}

	if (success)
	{
		// load water shader
		gWaterProgram.mName = "Water Shader";
		gWaterProgram.mFeatures.calculatesAtmospherics = true;
        gWaterProgram.mFeatures.hasAtmospherics = true;
		gWaterProgram.mFeatures.hasGamma = true;
		gWaterProgram.mFeatures.hasSrgb = true;
        gWaterProgram.mFeatures.hasReflectionProbes = true;
        gWaterProgram.mFeatures.hasShadows = use_sun_shadow;
		gWaterProgram.mShaderFiles.clear();
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER));
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterF.glsl", GL_FRAGMENT_SHADER));
        gWaterProgram.clearPermutations();
        if (LLPipeline::sRenderTransparentWater)
        {
            gWaterProgram.addPermutation("TRANSPARENT_WATER", "1");
        }

        if (use_sun_shadow)
        {
            gWaterProgram.addPermutation("HAS_SUN_SHADOW", "1");
        }

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
        gWaterEdgeProgram.mFeatures.hasAtmospherics = true;
		gWaterEdgeProgram.mFeatures.hasGamma = true;
		gWaterEdgeProgram.mFeatures.hasSrgb = true;
        gWaterEdgeProgram.mFeatures.hasReflectionProbes = true;
        gWaterEdgeProgram.mFeatures.hasShadows = use_sun_shadow;
		gWaterEdgeProgram.mShaderFiles.clear();
		gWaterEdgeProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER));
		gWaterEdgeProgram.mShaderFiles.push_back(make_pair("environment/waterF.glsl", GL_FRAGMENT_SHADER));
        gWaterEdgeProgram.clearPermutations();
		gWaterEdgeProgram.addPermutation("WATER_EDGE", "1");
        if (LLPipeline::sRenderTransparentWater)
        {
            gWaterEdgeProgram.addPermutation("TRANSPARENT_WATER", "1");
        }

        if (use_sun_shadow)
        {
            gWaterEdgeProgram.addPermutation("HAS_SUN_SHADOW", "1");
        }
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
		gUnderWaterProgram.mFeatures.hasAtmospherics = true;
		gUnderWaterProgram.mShaderFiles.clear();
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER));
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/underWaterF.glsl", GL_FRAGMENT_SHADER));
		gUnderWaterProgram.mShaderLevel = mShaderLevel[SHADER_WATER];        
		gUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;       
        gUnderWaterProgram.clearPermutations();
        if (LLPipeline::sRenderTransparentWater)
        {
            gUnderWaterProgram.addPermutation("TRANSPARENT_WATER", "1");
        }
		success = gUnderWaterProgram.createShader(NULL, NULL);
		llassert(success);
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
		return false;
	}

	// if we failed to load the terrain water shaders and we need them (using class2 water),
	// then drop down to class1 water.
	if (mShaderLevel[SHADER_WATER] > 1 && !terrainWaterSuccess)
	{
		mShaderLevel[SHADER_WATER]--;
		return loadShadersWater();
	}
	
	LLWorld::getInstance()->updateWaterObjects();

    return true;
}

bool LLViewerShaderMgr::loadShadersEffects()
{
    LL_PROFILE_ZONE_SCOPED;
	bool success = true;

	if (mShaderLevel[SHADER_EFFECT] == 0)
	{
		gGlowProgram.unload();
		gGlowExtractProgram.unload();
		return true;
	}

	if (success)
	{
		gGlowProgram.mName = "Glow Shader (Post)";
		gGlowProgram.mShaderFiles.clear();
		gGlowProgram.mShaderFiles.push_back(make_pair("effects/glowV.glsl", GL_VERTEX_SHADER));
		gGlowProgram.mShaderFiles.push_back(make_pair("effects/glowF.glsl", GL_FRAGMENT_SHADER));
		gGlowProgram.mShaderLevel = mShaderLevel[SHADER_EFFECT];
		success = gGlowProgram.createShader(NULL, NULL);
		if (!success)
		{
			LLPipeline::sRenderGlow = false;
		}
	}
	
	if (success)
	{
        const bool use_glow_noise = gSavedSettings.getBOOL("RenderGlowNoise");
        const std::string glow_noise_label = use_glow_noise ? " (+Noise)" : "";

		gGlowExtractProgram.mName = llformat("Glow Extract Shader (Post)%s", glow_noise_label.c_str());
		gGlowExtractProgram.mShaderFiles.clear();
		gGlowExtractProgram.mShaderFiles.push_back(make_pair("effects/glowExtractV.glsl", GL_VERTEX_SHADER));
		gGlowExtractProgram.mShaderFiles.push_back(make_pair("effects/glowExtractF.glsl", GL_FRAGMENT_SHADER));
		gGlowExtractProgram.mShaderLevel = mShaderLevel[SHADER_EFFECT];

        if (use_glow_noise)
        {
            gGlowExtractProgram.addPermutation("HAS_NOISE", "1");
        }

		success = gGlowExtractProgram.createShader(NULL, NULL);
		if (!success)
		{
			LLPipeline::sRenderGlow = false;
		}
	}
	
	return success;

}

bool LLViewerShaderMgr::loadShadersDeferred()
{
    LL_PROFILE_ZONE_SCOPED;
    bool use_sun_shadow = mShaderLevel[SHADER_DEFERRED] > 1 && 
        gSavedSettings.getS32("RenderShadowDetail") > 0;

	if (mShaderLevel[SHADER_DEFERRED] == 0)
	{
		gDeferredTreeProgram.unload();
		gDeferredTreeShadowProgram.unload();
        gDeferredSkinnedTreeShadowProgram.unload();
		gDeferredDiffuseProgram.unload();
        gDeferredSkinnedDiffuseProgram.unload();
		gDeferredDiffuseAlphaMaskProgram.unload();
        gDeferredSkinnedDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.unload();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.unload();
		gDeferredBumpProgram.unload();
        gDeferredSkinnedBumpProgram.unload();
		gDeferredImpostorProgram.unload();
		gDeferredTerrainProgram.unload();
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
		gDeferredShadowProgram.unload();
        gDeferredSkinnedShadowProgram.unload();
		gDeferredShadowCubeProgram.unload();
        gDeferredShadowAlphaMaskProgram.unload();
        gDeferredSkinnedShadowAlphaMaskProgram.unload();
        gDeferredShadowGLTFAlphaMaskProgram.unload();
        gDeferredSkinnedShadowGLTFAlphaMaskProgram.unload();
        gDeferredShadowFullbrightAlphaMaskProgram.unload();
        gDeferredSkinnedShadowFullbrightAlphaMaskProgram.unload();
		gDeferredAvatarShadowProgram.unload();
        gDeferredAvatarAlphaShadowProgram.unload();
        gDeferredAvatarAlphaMaskShadowProgram.unload();
		gDeferredAvatarProgram.unload();
		gDeferredAvatarAlphaProgram.unload();
		gDeferredAlphaProgram.unload();
        gHUDAlphaProgram.unload();
        gDeferredSkinnedAlphaProgram.unload();
		gDeferredFullbrightProgram.unload();
        gHUDFullbrightProgram.unload();
		gDeferredFullbrightAlphaMaskProgram.unload();
        gHUDFullbrightAlphaMaskProgram.unload();
        gDeferredFullbrightAlphaMaskAlphaProgram.unload();
        gHUDFullbrightAlphaMaskAlphaProgram.unload();
		gDeferredEmissiveProgram.unload();
        gDeferredSkinnedEmissiveProgram.unload();
		gDeferredAvatarEyesProgram.unload();
		gDeferredPostProgram.unload();		
		gDeferredCoFProgram.unload();		
		gDeferredDoFCombineProgram.unload();
        gExposureProgram.unload();
        gLuminanceProgram.unload();
		gDeferredPostGammaCorrectProgram.unload();
        gNoPostGammaCorrectProgram.unload();
        gLegacyPostGammaCorrectProgram.unload();
		gFXAAProgram.unload();
		gDeferredWLSkyProgram.unload();
		gDeferredWLCloudProgram.unload();
        gDeferredWLSunProgram.unload();
        gDeferredWLMoonProgram.unload();
		gDeferredStarProgram.unload();
		gDeferredFullbrightShinyProgram.unload();
        gHUDFullbrightShinyProgram.unload();
        gDeferredSkinnedFullbrightShinyProgram.unload();
		gDeferredSkinnedFullbrightProgram.unload();
        gDeferredSkinnedFullbrightAlphaMaskProgram.unload();
        gDeferredSkinnedFullbrightAlphaMaskAlphaProgram.unload();

        gDeferredHighlightProgram.unload();
        
		gNormalMapGenProgram.unload();
        gDeferredGenBrdfLutProgram.unload();
		gDeferredBufferVisualProgram.unload();

		for (U32 i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
		{
			gDeferredMaterialProgram[i].unload();
		}

        gHUDPBROpaqueProgram.unload();
        gPBRGlowProgram.unload();
        gDeferredPBROpaqueProgram.unload();
        gDeferredSkinnedPBROpaqueProgram.unload();
        gDeferredPBRAlphaProgram.unload();
        gDeferredSkinnedPBRAlphaProgram.unload();

		return true;
	}

	bool success = true;

    if (success)
	{
		gDeferredHighlightProgram.mName = "Deferred Highlight Shader";
		gDeferredHighlightProgram.mShaderFiles.clear();
		gDeferredHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightV.glsl", GL_VERTEX_SHADER));
		gDeferredHighlightProgram.mShaderFiles.push_back(make_pair("deferred/highlightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredHighlightProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];		
		success = gDeferredHighlightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDiffuseProgram.mName = "Deferred Diffuse Shader";
        gDeferredDiffuseProgram.mFeatures.encodesNormal = true;
        gDeferredDiffuseProgram.mFeatures.hasSrgb = true;
		gDeferredDiffuseProgram.mShaderFiles.clear();
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER));
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseIndexedF.glsl", GL_FRAGMENT_SHADER));
		gDeferredDiffuseProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredDiffuseProgram, gDeferredSkinnedDiffuseProgram);
		success = success && gDeferredDiffuseProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Alpha Mask Shader";
        gDeferredDiffuseAlphaMaskProgram.mFeatures.encodesNormal = true;
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER));
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskIndexedF.glsl", GL_FRAGMENT_SHADER));
		gDeferredDiffuseAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredDiffuseAlphaMaskProgram, gDeferredSkinnedDiffuseAlphaMaskProgram);
		success = success && gDeferredDiffuseAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask Shader";
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mFeatures.encodesNormal = true;
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskF.glsl", GL_FRAGMENT_SHADER));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskProgram.createShader(NULL, NULL);
        llassert(success);
	}
    
	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask No Color Shader";
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mFeatures.encodesNormal = true;
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseNoColorV.glsl", GL_VERTEX_SHADER));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskNoColorF.glsl", GL_FRAGMENT_SHADER));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredBumpProgram.mName = "Deferred Bump Shader";
		gDeferredBumpProgram.mFeatures.encodesNormal = true;
		gDeferredBumpProgram.mShaderFiles.clear();
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpV.glsl", GL_VERTEX_SHADER));
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpF.glsl", GL_FRAGMENT_SHADER));
		gDeferredBumpProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredBumpProgram, gDeferredSkinnedBumpProgram);
		success = success && gDeferredBumpProgram.createShader(NULL, NULL);
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

	for (U32 i = 0; i < LLMaterial::SHADER_COUNT*2; ++i)
	{
		if (success)
		{
            mShaderList.push_back(&gDeferredMaterialProgram[i]);

			gDeferredMaterialProgram[i].mName = llformat("Deferred Material Shader %d", i);
			
			U32 alpha_mode = i & 0x3;

			gDeferredMaterialProgram[i].mShaderFiles.clear();
			gDeferredMaterialProgram[i].mShaderFiles.push_back(make_pair("deferred/materialV.glsl", GL_VERTEX_SHADER));
			gDeferredMaterialProgram[i].mShaderFiles.push_back(make_pair("deferred/materialF.glsl", GL_FRAGMENT_SHADER));
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

            gDeferredMaterialProgram[i].addPermutation("DIFFUSE_ALPHA_MODE", llformat("%d", alpha_mode));

            if (alpha_mode != 0)
            {
                gDeferredMaterialProgram[i].mFeatures.hasAlphaMask = true;
                gDeferredMaterialProgram[i].addPermutation("HAS_ALPHA_MASK", "1");
            }

            if (use_sun_shadow)
            {
                gDeferredMaterialProgram[i].addPermutation("HAS_SUN_SHADOW", "1");
            }

            bool has_skin = i & 0x10;
            gDeferredMaterialProgram[i].mFeatures.hasSrgb = true;
            gDeferredMaterialProgram[i].mFeatures.encodesNormal = true;
            gDeferredMaterialProgram[i].mFeatures.calculatesAtmospherics = true;
            gDeferredMaterialProgram[i].mFeatures.hasAtmospherics = true;
            gDeferredMaterialProgram[i].mFeatures.hasGamma = true;
            gDeferredMaterialProgram[i].mFeatures.hasShadows = use_sun_shadow;
            gDeferredMaterialProgram[i].mFeatures.hasReflectionProbes = true;

            if (has_skin)
            {
                gDeferredMaterialProgram[i].addPermutation("HAS_SKIN", "1");
                gDeferredMaterialProgram[i].mFeatures.hasObjectSkinning = true;
            }
            else
            {
                gDeferredMaterialProgram[i].mRiggedVariant = &gDeferredMaterialProgram[i + 0x10];
            }

            success = gDeferredMaterialProgram[i].createShader(NULL, NULL);
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

    if (success)
    {
        gDeferredPBROpaqueProgram.mName = "Deferred PBR Opaque Shader";
        gDeferredPBROpaqueProgram.mFeatures.encodesNormal = true;
        gDeferredPBROpaqueProgram.mFeatures.hasSrgb = true;

        gDeferredPBROpaqueProgram.mShaderFiles.clear();
        gDeferredPBROpaqueProgram.mShaderFiles.push_back(make_pair("deferred/pbropaqueV.glsl", GL_VERTEX_SHADER));
        gDeferredPBROpaqueProgram.mShaderFiles.push_back(make_pair("deferred/pbropaqueF.glsl", GL_FRAGMENT_SHADER));
        gDeferredPBROpaqueProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredPBROpaqueProgram.clearPermutations();
        
        success = make_rigged_variant(gDeferredPBROpaqueProgram, gDeferredSkinnedPBROpaqueProgram);
        if (success)
        {
            success = gDeferredPBROpaqueProgram.createShader(NULL, NULL);
        }
        llassert(success);
    }

    if (success)
    {
        gPBRGlowProgram.mName = " PBR Glow Shader";
        gPBRGlowProgram.mFeatures.hasSrgb = true;
        gPBRGlowProgram.mShaderFiles.clear();
        gPBRGlowProgram.mShaderFiles.push_back(make_pair("deferred/pbrglowV.glsl", GL_VERTEX_SHADER));
        gPBRGlowProgram.mShaderFiles.push_back(make_pair("deferred/pbrglowF.glsl", GL_FRAGMENT_SHADER));
        gPBRGlowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = make_rigged_variant(gPBRGlowProgram, gPBRGlowSkinnedProgram);
        if (success)
        {
            success = gPBRGlowProgram.createShader(NULL, NULL);
        }
        llassert(success);
    }

    if (success)
    {
        gHUDPBROpaqueProgram.mName = "HUD PBR Opaque Shader";
        gHUDPBROpaqueProgram.mFeatures.hasSrgb = true;
        gHUDPBROpaqueProgram.mShaderFiles.clear();
        gHUDPBROpaqueProgram.mShaderFiles.push_back(make_pair("deferred/pbropaqueV.glsl", GL_VERTEX_SHADER));
        gHUDPBROpaqueProgram.mShaderFiles.push_back(make_pair("deferred/pbropaqueF.glsl", GL_FRAGMENT_SHADER));
        gHUDPBROpaqueProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gHUDPBROpaqueProgram.clearPermutations();
        gHUDPBROpaqueProgram.addPermutation("IS_HUD", "1");

        success = gHUDPBROpaqueProgram.createShader(NULL, NULL);
 
        llassert(success);
    }

    

	if (success)
	{
        LLGLSLShader* shader = &gDeferredPBRAlphaProgram;
        shader->mName = "Deferred PBR Alpha Shader";
                          
        shader->mFeatures.calculatesLighting = false;
        shader->mFeatures.hasLighting = false;
        shader->mFeatures.isAlphaLighting = true;
        shader->mFeatures.hasSrgb = true;
        shader->mFeatures.encodesNormal = true;
        shader->mFeatures.calculatesAtmospherics = true;
        shader->mFeatures.hasAtmospherics = true;
        shader->mFeatures.hasGamma = true;
        shader->mFeatures.hasShadows = use_sun_shadow;
        shader->mFeatures.isDeferred = true; // include deferredUtils
        shader->mFeatures.hasReflectionProbes = mShaderLevel[SHADER_DEFERRED];

        shader->mShaderFiles.clear();
        shader->mShaderFiles.push_back(make_pair("deferred/pbralphaV.glsl", GL_VERTEX_SHADER));
        shader->mShaderFiles.push_back(make_pair("deferred/pbralphaF.glsl", GL_FRAGMENT_SHADER));

        shader->clearPermutations();

        U32 alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
        shader->addPermutation("DIFFUSE_ALPHA_MODE", llformat("%d", alpha_mode));
        shader->addPermutation("HAS_NORMAL_MAP", "1");
        shader->addPermutation("HAS_SPECULAR_MAP", "1"); // PBR: Packed: Occlusion, Metal, Roughness
        shader->addPermutation("HAS_EMISSIVE_MAP", "1");
        shader->addPermutation("USE_VERTEX_COLOR", "1");

        if (use_sun_shadow)
        {
            shader->addPermutation("HAS_SUN_SHADOW", "1");
        }

        shader->mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(*shader, gDeferredSkinnedPBRAlphaProgram);
        if (success)
        {
            success = shader->createShader(NULL, NULL);
        }
        llassert(success);

        // Alpha Shader Hack
        // See: LLRender::syncMatrices()
        shader->mFeatures.calculatesLighting = true;
        shader->mFeatures.hasLighting = true;

        shader->mRiggedVariant->mFeatures.calculatesLighting = true;
        shader->mRiggedVariant->mFeatures.hasLighting = true;
    }

    if (success)
    {
        LLGLSLShader* shader = &gHUDPBRAlphaProgram;
        shader->mName = "HUD PBR Alpha Shader";

        shader->mFeatures.hasSrgb = true;
        
        shader->mShaderFiles.clear();
        shader->mShaderFiles.push_back(make_pair("deferred/pbralphaV.glsl", GL_VERTEX_SHADER));
        shader->mShaderFiles.push_back(make_pair("deferred/pbralphaF.glsl", GL_FRAGMENT_SHADER));

        shader->clearPermutations();

        shader->addPermutation("IS_HUD", "1");

        shader->mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = shader->createShader(NULL, NULL);
        llassert(success);
    }
	
	if (success)
	{
		gDeferredTreeProgram.mName = "Deferred Tree Shader";
		gDeferredTreeProgram.mShaderFiles.clear();
        gDeferredTreeProgram.mFeatures.encodesNormal = true;
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeV.glsl", GL_VERTEX_SHADER));
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeF.glsl", GL_FRAGMENT_SHADER));
		gDeferredTreeProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredTreeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredTreeShadowProgram.mName = "Deferred Tree Shadow Shader";
		gDeferredTreeShadowProgram.mShaderFiles.clear();
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowV.glsl", GL_VERTEX_SHADER));
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowF.glsl", GL_FRAGMENT_SHADER));
		gDeferredTreeShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredTreeShadowProgram.mRiggedVariant = &gDeferredSkinnedTreeShadowProgram;
		success = gDeferredTreeShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}

    if (success)
    {
        gDeferredSkinnedTreeShadowProgram.mName = "Deferred Skinned Tree Shadow Shader";
        gDeferredSkinnedTreeShadowProgram.mShaderFiles.clear();
        gDeferredSkinnedTreeShadowProgram.mFeatures.hasObjectSkinning = true;
        gDeferredSkinnedTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowSkinnedV.glsl", GL_VERTEX_SHADER));
        gDeferredSkinnedTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowF.glsl", GL_FRAGMENT_SHADER));
        gDeferredSkinnedTreeShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gDeferredSkinnedTreeShadowProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredImpostorProgram.mName = "Deferred Impostor Shader";
		gDeferredImpostorProgram.mFeatures.hasSrgb = true;
		gDeferredImpostorProgram.mFeatures.encodesNormal = true;
		//gDeferredImpostorProgram.mFeatures.isDeferred = true;
		gDeferredImpostorProgram.mShaderFiles.clear();
		gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorV.glsl", GL_VERTEX_SHADER));
        gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorF.glsl", GL_FRAGMENT_SHADER));
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
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER));
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        gDeferredLightProgram.clearPermutations();

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
			gDeferredMultiLightProgram[i].mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER));
			gDeferredMultiLightProgram[i].mShaderFiles.push_back(make_pair("deferred/multiPointLightF.glsl", GL_FRAGMENT_SHADER));
			gDeferredMultiLightProgram[i].mShaderLevel = mShaderLevel[SHADER_DEFERRED];
			gDeferredMultiLightProgram[i].addPermutation("LIGHT_COUNT", llformat("%d", i+1));

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
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER));
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/spotLightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredSpotLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

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
        gDeferredMultiSpotLightProgram.addPermutation("MULTI_SPOTLIGHT", "1");
		gDeferredMultiSpotLightProgram.mShaderFiles.clear();
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER));
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/spotLightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredMultiSpotLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

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

        gDeferredSunProgram.mShaderFiles.clear();
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(vertex, GL_VERTEX_SHADER));
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(fragment, GL_FRAGMENT_SHADER));
		gDeferredSunProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gDeferredSunProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gDeferredBlurLightProgram.mName = "Deferred Blur Light Shader";
		gDeferredBlurLightProgram.mFeatures.isDeferred = true;

		gDeferredBlurLightProgram.mShaderFiles.clear();
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightV.glsl", GL_VERTEX_SHADER));
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredBlurLightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		success = gDeferredBlurLightProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
        for (int i = 0; i < 3 && success; ++i)
        {
            LLGLSLShader* shader = nullptr;
            bool rigged = (i == 1);
            bool hud = (i == 2);

            if (hud)
            {
                shader = &gHUDAlphaProgram;
                shader->mName = "HUD Alpha Shader";
            }
            else if (!rigged)
            {
                shader = &gDeferredAlphaProgram;
                shader->mName = "Deferred Alpha Shader";
                shader->mRiggedVariant = &gDeferredSkinnedAlphaProgram;
            }
            else
            {
                shader = &gDeferredSkinnedAlphaProgram;
                shader->mName = "Skinned Deferred Alpha Shader";
                shader->mFeatures.hasObjectSkinning = true;
            }

            shader->mFeatures.calculatesLighting = false;
            shader->mFeatures.hasLighting = false;
            shader->mFeatures.isAlphaLighting = true;
            shader->mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
            shader->mFeatures.hasSrgb = true;
            shader->mFeatures.encodesNormal = true;
            shader->mFeatures.calculatesAtmospherics = true;
            shader->mFeatures.hasAtmospherics = true;
            shader->mFeatures.hasGamma = true;
            shader->mFeatures.hasShadows = use_sun_shadow;
            shader->mFeatures.hasReflectionProbes = true;
            shader->mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

            shader->mShaderFiles.clear();
            shader->mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER));
            shader->mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER));

            shader->clearPermutations();
            shader->addPermutation("USE_VERTEX_COLOR", "1");
            shader->addPermutation("HAS_ALPHA_MASK", "1");
            shader->addPermutation("USE_INDEXED_TEX", "1");
            if (use_sun_shadow)
            {
                shader->addPermutation("HAS_SUN_SHADOW", "1");
            }

            if (rigged)
            {
                shader->addPermutation("HAS_SKIN", "1");
            }

            if (hud)
            {
                shader->addPermutation("IS_HUD", "1");
            }

            shader->mShaderLevel = mShaderLevel[SHADER_DEFERRED];

            success = shader->createShader(NULL, NULL);
            llassert(success);

            // Hack
            shader->mFeatures.calculatesLighting = true;
            shader->mFeatures.hasLighting = true;
        }
    }

    if (success)
    {
        LLGLSLShader* shaders[] = { 
            &gDeferredAlphaImpostorProgram, 
            &gDeferredSkinnedAlphaImpostorProgram 
        };

        for (int i = 0; i < 2 && success; ++i)
        {
            bool rigged = i == 1;
            LLGLSLShader* shader = shaders[i];

            shader->mName = rigged ? "Skinned Deferred Alpha Impostor Shader" : "Deferred Alpha Impostor Shader";

            // Begin Hack
            shader->mFeatures.calculatesLighting = false;
            shader->mFeatures.hasLighting = false;

            shader->mFeatures.hasSrgb = true;
            shader->mFeatures.isAlphaLighting = true;
            shader->mFeatures.encodesNormal = true;
            shader->mFeatures.hasShadows = use_sun_shadow;
            shader->mFeatures.hasReflectionProbes = true;
            shader->mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

            shader->mShaderFiles.clear();
            shader->mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER));
            shader->mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER));

            shader->clearPermutations();
            shader->addPermutation("USE_INDEXED_TEX", "1");
            shader->addPermutation("FOR_IMPOSTOR", "1");
            shader->addPermutation("HAS_ALPHA_MASK", "1");
            shader->addPermutation("USE_VERTEX_COLOR", "1");
            if (rigged)
            {
                shader->mFeatures.hasObjectSkinning = true;
                shader->addPermutation("HAS_SKIN", "1");
            }

            if (use_sun_shadow)
            {
                shader->addPermutation("HAS_SUN_SHADOW", "1");
            }

            shader->mRiggedVariant = &gDeferredSkinnedAlphaImpostorProgram;
            shader->mShaderLevel = mShaderLevel[SHADER_DEFERRED];
            if (!rigged)
            {
                shader->mRiggedVariant = shaders[1];
            }
            success = shader->createShader(NULL, NULL);
            llassert(success);

            // End Hack
            shader->mFeatures.calculatesLighting = true;
            shader->mFeatures.hasLighting = true;
        }
    }

	if (success)
	{
		gDeferredAvatarEyesProgram.mName = "Deferred Avatar Eyes Shader";
		gDeferredAvatarEyesProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarEyesProgram.mFeatures.hasGamma = true;
		gDeferredAvatarEyesProgram.mFeatures.hasAtmospherics = true;
		gDeferredAvatarEyesProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarEyesProgram.mFeatures.hasSrgb = true;
		gDeferredAvatarEyesProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarEyesProgram.mFeatures.hasShadows = true;

		gDeferredAvatarEyesProgram.mShaderFiles.clear();
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/avatarEyesV.glsl", GL_VERTEX_SHADER));
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER));
		gDeferredAvatarEyesProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarEyesProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredFullbrightProgram.mName = "Deferred Fullbright Shader";
		gDeferredFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightProgram.mFeatures.hasAtmospherics = true;
		gDeferredFullbrightProgram.mFeatures.hasSrgb = true;
		gDeferredFullbrightProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightProgram.mShaderFiles.clear();
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
		gDeferredFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredFullbrightProgram, gDeferredSkinnedFullbrightProgram);
		success = gDeferredFullbrightProgram.createShader(NULL, NULL);
		llassert(success);
	}

    if (success)
    {
        gHUDFullbrightProgram.mName = "HUD Fullbright Shader";
        gHUDFullbrightProgram.mFeatures.calculatesAtmospherics = true;
        gHUDFullbrightProgram.mFeatures.hasGamma = true;
        gHUDFullbrightProgram.mFeatures.hasAtmospherics = true;
        gHUDFullbrightProgram.mFeatures.hasSrgb = true;
        gHUDFullbrightProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        gHUDFullbrightProgram.mShaderFiles.clear();
        gHUDFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
        gHUDFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
        gHUDFullbrightProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gHUDFullbrightProgram.clearPermutations();
        gHUDFullbrightProgram.addPermutation("IS_HUD", "1");
        success = gHUDFullbrightProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredFullbrightAlphaMaskProgram.mName = "Deferred Fullbright Alpha Masking Shader";
		gDeferredFullbrightAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasAtmospherics = true;
		gDeferredFullbrightAlphaMaskProgram.mFeatures.hasSrgb = true;		
		gDeferredFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
		gDeferredFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
        gDeferredFullbrightAlphaMaskProgram.clearPermutations();
		gDeferredFullbrightAlphaMaskProgram.addPermutation("HAS_ALPHA_MASK","1");
		gDeferredFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredFullbrightAlphaMaskProgram, gDeferredSkinnedFullbrightAlphaMaskProgram);
		success = success && gDeferredFullbrightAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}

    if (success)
    {
        gHUDFullbrightAlphaMaskProgram.mName = "HUD Fullbright Alpha Masking Shader";
        gHUDFullbrightAlphaMaskProgram.mFeatures.calculatesAtmospherics = true;
        gHUDFullbrightAlphaMaskProgram.mFeatures.hasGamma = true;
        gHUDFullbrightAlphaMaskProgram.mFeatures.hasAtmospherics = true;
        gHUDFullbrightAlphaMaskProgram.mFeatures.hasSrgb = true;
        gHUDFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        gHUDFullbrightAlphaMaskProgram.mShaderFiles.clear();
        gHUDFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
        gHUDFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
        gHUDFullbrightAlphaMaskProgram.clearPermutations();
        gHUDFullbrightAlphaMaskProgram.addPermutation("HAS_ALPHA_MASK", "1");
        gHUDFullbrightAlphaMaskProgram.addPermutation("IS_HUD", "1");
        gHUDFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gHUDFullbrightAlphaMaskProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gDeferredFullbrightAlphaMaskAlphaProgram.mName = "Deferred Fullbright Alpha Masking Alpha Shader";
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.hasGamma = true;
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.hasAtmospherics = true;
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.hasSrgb = true;
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.isDeferred = true;
        gDeferredFullbrightAlphaMaskAlphaProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        gDeferredFullbrightAlphaMaskAlphaProgram.mShaderFiles.clear();
        gDeferredFullbrightAlphaMaskAlphaProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
        gDeferredFullbrightAlphaMaskAlphaProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
        gDeferredFullbrightAlphaMaskAlphaProgram.clearPermutations();
        gDeferredFullbrightAlphaMaskAlphaProgram.addPermutation("HAS_ALPHA_MASK", "1");
        gDeferredFullbrightAlphaMaskAlphaProgram.addPermutation("IS_ALPHA", "1");
        gDeferredFullbrightAlphaMaskAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredFullbrightAlphaMaskAlphaProgram, gDeferredSkinnedFullbrightAlphaMaskAlphaProgram);
        success = success && gDeferredFullbrightAlphaMaskAlphaProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gHUDFullbrightAlphaMaskAlphaProgram.mName = "HUD Fullbright Alpha Masking Alpha Shader";
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.calculatesAtmospherics = true;
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.hasGamma = true;
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.hasAtmospherics = true;
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.hasSrgb = true;
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.isDeferred = true;
        gHUDFullbrightAlphaMaskAlphaProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        gHUDFullbrightAlphaMaskAlphaProgram.mShaderFiles.clear();
        gHUDFullbrightAlphaMaskAlphaProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER));
        gHUDFullbrightAlphaMaskAlphaProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER));
        gHUDFullbrightAlphaMaskAlphaProgram.clearPermutations();
        gHUDFullbrightAlphaMaskAlphaProgram.addPermutation("HAS_ALPHA_MASK", "1");
        gHUDFullbrightAlphaMaskAlphaProgram.addPermutation("IS_ALPHA", "1");
        gHUDFullbrightAlphaMaskAlphaProgram.addPermutation("IS_HUD", "1");
        gHUDFullbrightAlphaMaskAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = success && gHUDFullbrightAlphaMaskAlphaProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredFullbrightShinyProgram.mName = "Deferred FullbrightShiny Shader";
		gDeferredFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasAtmospherics = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightShinyProgram.mFeatures.hasSrgb = true;
		gDeferredFullbrightShinyProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightShinyProgram.mShaderFiles.clear();
		gDeferredFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyV.glsl", GL_VERTEX_SHADER));
		gDeferredFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER));
		gDeferredFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredFullbrightShinyProgram.mFeatures.hasReflectionProbes = true;
        success = make_rigged_variant(gDeferredFullbrightShinyProgram, gDeferredSkinnedFullbrightShinyProgram);
		success = success && gDeferredFullbrightShinyProgram.createShader(NULL, NULL);
		llassert(success);
	}

    if (success)
    {
        gHUDFullbrightShinyProgram.mName = "HUD FullbrightShiny Shader";
        gHUDFullbrightShinyProgram.mFeatures.calculatesAtmospherics = true;
        gHUDFullbrightShinyProgram.mFeatures.hasAtmospherics = true;
        gHUDFullbrightShinyProgram.mFeatures.hasGamma = true;
        gHUDFullbrightShinyProgram.mFeatures.hasSrgb = true;
        gHUDFullbrightShinyProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
        gHUDFullbrightShinyProgram.mShaderFiles.clear();
        gHUDFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyV.glsl", GL_VERTEX_SHADER));
        gHUDFullbrightShinyProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER));
        gHUDFullbrightShinyProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gHUDFullbrightShinyProgram.mFeatures.hasReflectionProbes = true;
        gHUDFullbrightShinyProgram.clearPermutations();
        gHUDFullbrightShinyProgram.addPermutation("IS_HUD", "1");
        success = gHUDFullbrightShinyProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredEmissiveProgram.mName = "Deferred Emissive Shader";
		gDeferredEmissiveProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredEmissiveProgram.mFeatures.hasGamma = true;
		gDeferredEmissiveProgram.mFeatures.hasAtmospherics = true;
		gDeferredEmissiveProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredEmissiveProgram.mShaderFiles.clear();
		gDeferredEmissiveProgram.mShaderFiles.push_back(make_pair("deferred/emissiveV.glsl", GL_VERTEX_SHADER));
		gDeferredEmissiveProgram.mShaderFiles.push_back(make_pair("deferred/emissiveF.glsl", GL_FRAGMENT_SHADER));
		gDeferredEmissiveProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredEmissiveProgram, gDeferredSkinnedEmissiveProgram);
		success = success && gDeferredEmissiveProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredSoftenProgram.mName = "Deferred Soften Shader";
		gDeferredSoftenProgram.mShaderFiles.clear();
		gDeferredSoftenProgram.mFeatures.hasSrgb = true;
		gDeferredSoftenProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSoftenProgram.mFeatures.hasAtmospherics = true;
		gDeferredSoftenProgram.mFeatures.hasGamma = true;
		gDeferredSoftenProgram.mFeatures.isDeferred = true;
		gDeferredSoftenProgram.mFeatures.hasShadows = use_sun_shadow;
        gDeferredSoftenProgram.mFeatures.hasReflectionProbes = mShaderLevel[SHADER_DEFERRED] > 2;

        gDeferredSoftenProgram.clearPermutations();
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightV.glsl", GL_VERTEX_SHADER));
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightF.glsl", GL_FRAGMENT_SHADER));

		gDeferredSoftenProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        if (use_sun_shadow)
        {
            gDeferredSoftenProgram.addPermutation("HAS_SUN_SHADOW", "1");
        }

		if (gSavedSettings.getBOOL("RenderDeferredSSAO"))
		{ //if using SSAO, take screen space light map into account as if shadows are enabled
			gDeferredSoftenProgram.mShaderLevel = llmax(gDeferredSoftenProgram.mShaderLevel, 2);
            gDeferredSoftenProgram.addPermutation("HAS_SSAO", "1");
		}
				
		success = gDeferredSoftenProgram.createShader(NULL, NULL);
		llassert(success);
	}

    if (success)
    {
        gHazeProgram.mName = "Haze Shader";
        gHazeProgram.mShaderFiles.clear();
        gHazeProgram.mFeatures.hasSrgb                = true;
        gHazeProgram.mFeatures.calculatesAtmospherics = true;
        gHazeProgram.mFeatures.hasAtmospherics        = true;
        gHazeProgram.mFeatures.hasGamma               = true;
        gHazeProgram.mFeatures.isDeferred             = true;
        gHazeProgram.mFeatures.hasShadows             = use_sun_shadow;
        gHazeProgram.mFeatures.hasReflectionProbes    = mShaderLevel[SHADER_DEFERRED] > 2;

        gHazeProgram.clearPermutations();
        gHazeProgram.mShaderFiles.push_back(make_pair("deferred/softenLightV.glsl", GL_VERTEX_SHADER));
        gHazeProgram.mShaderFiles.push_back(make_pair("deferred/hazeF.glsl", GL_FRAGMENT_SHADER));

        gHazeProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gHazeProgram.createShader(NULL, NULL);
        llassert(success);
    }


    if (success)
    {
        gHazeWaterProgram.mName = "Water Haze Shader";
        gHazeWaterProgram.mShaderFiles.clear();
        gHazeWaterProgram.mShaderGroup           = LLGLSLShader::SG_WATER;
        gHazeWaterProgram.mFeatures.hasSrgb                = true;
        gHazeWaterProgram.mFeatures.calculatesAtmospherics = true;
        gHazeWaterProgram.mFeatures.hasAtmospherics        = true;
        gHazeWaterProgram.mFeatures.hasGamma               = true;
        gHazeWaterProgram.mFeatures.isDeferred             = true;
        gHazeWaterProgram.mFeatures.hasShadows             = use_sun_shadow;
        gHazeWaterProgram.mFeatures.hasReflectionProbes    = mShaderLevel[SHADER_DEFERRED] > 2;

        gHazeWaterProgram.clearPermutations();
        gHazeWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterHazeV.glsl", GL_VERTEX_SHADER));
        gHazeWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterHazeF.glsl", GL_FRAGMENT_SHADER));

        gHazeWaterProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

        success = gHazeWaterProgram.createShader(NULL, NULL);
        llassert(success);
    }


	if (success)
	{
		gDeferredShadowProgram.mName = "Deferred Shadow Shader";
		gDeferredShadowProgram.mShaderFiles.clear();
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowV.glsl", GL_VERTEX_SHADER));
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER));
		gDeferredShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredShadowProgram.mRiggedVariant = &gDeferredSkinnedShadowProgram;
		success = gDeferredShadowProgram.createShader(NULL, NULL);
		llassert(success);
	}

    if (success)
    {
        gDeferredSkinnedShadowProgram.mName = "Deferred Skinned Shadow Shader";
        gDeferredSkinnedShadowProgram.mFeatures.isDeferred = true;
        gDeferredSkinnedShadowProgram.mFeatures.hasShadows = true;
        gDeferredSkinnedShadowProgram.mFeatures.hasObjectSkinning = true;
        gDeferredSkinnedShadowProgram.mShaderFiles.clear();
        gDeferredSkinnedShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowSkinnedV.glsl", GL_VERTEX_SHADER));
        gDeferredSkinnedShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER));
        gDeferredSkinnedShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        // gDeferredSkinnedShadowProgram.addPermutation("DEPTH_CLAMP", "1"); // disable depth clamp for now
        success = gDeferredSkinnedShadowProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredShadowCubeProgram.mName = "Deferred Shadow Cube Shader";
		gDeferredShadowCubeProgram.mFeatures.isDeferred = true;
		gDeferredShadowCubeProgram.mFeatures.hasShadows = true;
		gDeferredShadowCubeProgram.mShaderFiles.clear();
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowCubeV.glsl", GL_VERTEX_SHADER));
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER));
		// gDeferredShadowCubeProgram.addPermutation("DEPTH_CLAMP", "1");
		gDeferredShadowCubeProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowCubeProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredShadowFullbrightAlphaMaskProgram.mName = "Deferred Shadow Fullbright Alpha Mask Shader";
		gDeferredShadowFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskV.glsl", GL_VERTEX_SHADER));
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER));

        gDeferredShadowFullbrightAlphaMaskProgram.clearPermutations();
		gDeferredShadowFullbrightAlphaMaskProgram.addPermutation("DEPTH_CLAMP", "1");
        gDeferredShadowFullbrightAlphaMaskProgram.addPermutation("IS_FULLBRIGHT", "1");
		gDeferredShadowFullbrightAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredShadowFullbrightAlphaMaskProgram, gDeferredSkinnedShadowFullbrightAlphaMaskProgram);
        success = success && gDeferredShadowFullbrightAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}
    
    if (success)
	{
		gDeferredShadowAlphaMaskProgram.mName = "Deferred Shadow Alpha Mask Shader";
		gDeferredShadowAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;

		gDeferredShadowAlphaMaskProgram.mShaderFiles.clear();
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskV.glsl", GL_VERTEX_SHADER));
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER));
		gDeferredShadowAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = make_rigged_variant(gDeferredShadowAlphaMaskProgram, gDeferredSkinnedShadowAlphaMaskProgram);
        success = success && gDeferredShadowAlphaMaskProgram.createShader(NULL, NULL);
		llassert(success);
	}
    

    if (success)
    {
        gDeferredShadowGLTFAlphaMaskProgram.mName = "Deferred GLTF Shadow Alpha Mask Shader";
        gDeferredShadowGLTFAlphaMaskProgram.mShaderFiles.clear();
        gDeferredShadowGLTFAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/pbrShadowAlphaMaskV.glsl", GL_VERTEX_SHADER));
        gDeferredShadowGLTFAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/pbrShadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER));
        gDeferredShadowGLTFAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredShadowGLTFAlphaMaskProgram.clearPermutations();
        success = make_rigged_variant(gDeferredShadowGLTFAlphaMaskProgram, gDeferredSkinnedShadowGLTFAlphaMaskProgram);
        success = success && gDeferredShadowGLTFAlphaMaskProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gDeferredShadowGLTFAlphaBlendProgram.mName = "Deferred GLTF Shadow Alpha Blend Shader";
        gDeferredShadowGLTFAlphaBlendProgram.mShaderFiles.clear();
        gDeferredShadowGLTFAlphaBlendProgram.mShaderFiles.push_back(make_pair("deferred/pbrShadowAlphaMaskV.glsl", GL_VERTEX_SHADER));
        gDeferredShadowGLTFAlphaBlendProgram.mShaderFiles.push_back(make_pair("deferred/pbrShadowAlphaBlendF.glsl", GL_FRAGMENT_SHADER));
        gDeferredShadowGLTFAlphaBlendProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredShadowGLTFAlphaBlendProgram.clearPermutations();
        success = make_rigged_variant(gDeferredShadowGLTFAlphaBlendProgram, gDeferredSkinnedShadowGLTFAlphaBlendProgram);
        success = success && gDeferredShadowGLTFAlphaBlendProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredAvatarShadowProgram.mName = "Deferred Avatar Shadow Shader";
		gDeferredAvatarShadowProgram.mFeatures.hasSkinning = true;

		gDeferredAvatarShadowProgram.mShaderFiles.clear();
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowV.glsl", GL_VERTEX_SHADER));
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowF.glsl", GL_FRAGMENT_SHADER));
		gDeferredAvatarShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarShadowProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarAlphaShadowProgram.mName = "Deferred Avatar Alpha Shadow Shader";
		gDeferredAvatarAlphaShadowProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.clear();
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowV.glsl", GL_VERTEX_SHADER));
		gDeferredAvatarAlphaShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowF.glsl", GL_FRAGMENT_SHADER));
		gDeferredAvatarAlphaShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarAlphaShadowProgram.createShader(NULL, NULL);
        llassert(success);
	}
    if (success)
	{
		gDeferredAvatarAlphaMaskShadowProgram.mName = "Deferred Avatar Alpha Mask Shadow Shader";
		gDeferredAvatarAlphaMaskShadowProgram.mFeatures.hasSkinning  = true;
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.clear();
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaShadowV.glsl", GL_VERTEX_SHADER));
		gDeferredAvatarAlphaMaskShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaMaskShadowF.glsl", GL_FRAGMENT_SHADER));
		gDeferredAvatarAlphaMaskShadowProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarAlphaMaskShadowProgram.createShader(NULL, NULL);
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
		gDeferredTerrainProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasAtmospherics = true;
		gDeferredTerrainProgram.mFeatures.hasGamma = true;

		gDeferredTerrainProgram.mShaderFiles.clear();
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainV.glsl", GL_VERTEX_SHADER));
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainF.glsl", GL_FRAGMENT_SHADER));
		gDeferredTerrainProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gDeferredTerrainProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarProgram.mName = "Deferred Avatar Shader";
		gDeferredAvatarProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarProgram.mShaderFiles.clear();
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarV.glsl", GL_VERTEX_SHADER));
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarF.glsl", GL_FRAGMENT_SHADER));
		gDeferredAvatarProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredAvatarAlphaProgram.mName = "Deferred Avatar Alpha Shader";
		gDeferredAvatarAlphaProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasSrgb = true;
		gDeferredAvatarAlphaProgram.mFeatures.encodesNormal = true;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasAtmospherics = true;
        gDeferredAvatarAlphaProgram.mFeatures.hasGamma = true;
        gDeferredAvatarAlphaProgram.mFeatures.isDeferred = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasShadows = true;
        gDeferredAvatarAlphaProgram.mFeatures.hasReflectionProbes = true;

		gDeferredAvatarAlphaProgram.mShaderFiles.clear();
        gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaV.glsl", GL_VERTEX_SHADER));
        gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaF.glsl", GL_FRAGMENT_SHADER));

		gDeferredAvatarAlphaProgram.clearPermutations();
		gDeferredAvatarAlphaProgram.addPermutation("USE_DIFFUSE_TEX", "1");
		gDeferredAvatarAlphaProgram.addPermutation("IS_AVATAR_SKIN", "1");
		if (use_sun_shadow)
		{
			gDeferredAvatarAlphaProgram.addPermutation("HAS_SUN_SHADOW", "1");
		}

		gDeferredAvatarAlphaProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];

		success = gDeferredAvatarAlphaProgram.createShader(NULL, NULL);
		llassert(success);

		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = true;
	}

    if (success)
    {
        gExposureProgram.mName = "Exposure";
        gExposureProgram.mFeatures.hasSrgb = true;
        gExposureProgram.mFeatures.isDeferred = true;
        gExposureProgram.mShaderFiles.clear();
        gExposureProgram.clearPermutations();
        gExposureProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
        gExposureProgram.mShaderFiles.push_back(make_pair("deferred/exposureF.glsl", GL_FRAGMENT_SHADER));
        gExposureProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gExposureProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gLuminanceProgram.mName = "Luminance";
        gLuminanceProgram.mShaderFiles.clear();
        gLuminanceProgram.clearPermutations();
        gLuminanceProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
        gLuminanceProgram.mShaderFiles.push_back(make_pair("deferred/luminanceF.glsl", GL_FRAGMENT_SHADER));
        gLuminanceProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gLuminanceProgram.createShader(NULL, NULL);
        llassert(success);
    }

	if (success)
	{
		gDeferredPostGammaCorrectProgram.mName = "Deferred Gamma Correction Post Process";
		gDeferredPostGammaCorrectProgram.mFeatures.hasSrgb = true;
		gDeferredPostGammaCorrectProgram.mFeatures.isDeferred = true;
		gDeferredPostGammaCorrectProgram.mShaderFiles.clear();
        gDeferredPostGammaCorrectProgram.clearPermutations();
        gDeferredPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredGammaCorrect.glsl", GL_FRAGMENT_SHADER));
        gDeferredPostGammaCorrectProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostGammaCorrectProgram.createShader(NULL, NULL);
		llassert(success);
	}
    
    if (success)
    {
        gNoPostGammaCorrectProgram.mName = "No Post Gamma Correction Post Process";
        gNoPostGammaCorrectProgram.mFeatures.hasSrgb = true;
        gNoPostGammaCorrectProgram.mFeatures.isDeferred = true;
        gNoPostGammaCorrectProgram.mShaderFiles.clear();
        gNoPostGammaCorrectProgram.clearPermutations();
        gNoPostGammaCorrectProgram.addPermutation("NO_POST", "1");
        gNoPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
        gNoPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredGammaCorrect.glsl", GL_FRAGMENT_SHADER));
        gNoPostGammaCorrectProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gNoPostGammaCorrectProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gLegacyPostGammaCorrectProgram.mName = "Legacy Gamma Correction Post Process";
        gLegacyPostGammaCorrectProgram.mFeatures.hasSrgb = true;
        gLegacyPostGammaCorrectProgram.mFeatures.isDeferred = true;
        gLegacyPostGammaCorrectProgram.mShaderFiles.clear();
        gLegacyPostGammaCorrectProgram.clearPermutations();
        gLegacyPostGammaCorrectProgram.addPermutation("LEGACY_GAMMA", "1");
        gLegacyPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
        gLegacyPostGammaCorrectProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredGammaCorrect.glsl", GL_FRAGMENT_SHADER));
        gLegacyPostGammaCorrectProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gLegacyPostGammaCorrectProgram.createShader(NULL, NULL);
        llassert(success);
    }


	if (success && gGLManager.mGLVersion > 3.9f)
	{
		gFXAAProgram.mName = "FXAA Shader";
		gFXAAProgram.mFeatures.isDeferred = true;
		gFXAAProgram.mShaderFiles.clear();
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredV.glsl", GL_VERTEX_SHADER));
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/fxaaF.glsl", GL_FRAGMENT_SHADER));
		gFXAAProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gFXAAProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredPostProgram.mName = "Deferred Post Shader";
        gDeferredPostProgram.mFeatures.isDeferred = true;
		gDeferredPostProgram.mShaderFiles.clear();
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER));
		gDeferredPostProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredCoFProgram.mName = "Deferred CoF Shader";
		gDeferredCoFProgram.mShaderFiles.clear();
		gDeferredCoFProgram.mFeatures.isDeferred = true;
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/cofF.glsl", GL_FRAGMENT_SHADER));
		gDeferredCoFProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredCoFProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredDoFCombineProgram.mName = "Deferred DoFCombine Shader";
		gDeferredDoFCombineProgram.mFeatures.isDeferred = true;
		gDeferredDoFCombineProgram.mShaderFiles.clear();
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/dofCombineF.glsl", GL_FRAGMENT_SHADER));
		gDeferredDoFCombineProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredDoFCombineProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredPostNoDoFProgram.mName = "Deferred Post NoDoF Shader";
		gDeferredPostNoDoFProgram.mFeatures.isDeferred = true;
		gDeferredPostNoDoFProgram.mShaderFiles.clear();
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoDoFF.glsl", GL_FRAGMENT_SHADER));
		gDeferredPostNoDoFProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostNoDoFProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
		gDeferredWLSkyProgram.mName = "Deferred Windlight Sky Shader";
		gDeferredWLSkyProgram.mShaderFiles.clear();
		gDeferredWLSkyProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWLSkyProgram.mFeatures.hasAtmospherics = true;
		gDeferredWLSkyProgram.mFeatures.hasGamma = true;
		gDeferredWLSkyProgram.mFeatures.hasSrgb = true;

		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyV.glsl", GL_VERTEX_SHADER));
		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyF.glsl", GL_FRAGMENT_SHADER));
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
		gDeferredWLCloudProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLCloudProgram.mFeatures.hasGamma = true;
        gDeferredWLCloudProgram.mFeatures.hasSrgb = true;
        
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsV.glsl", GL_VERTEX_SHADER));
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsF.glsl", GL_FRAGMENT_SHADER));
		gDeferredWLCloudProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        gDeferredWLCloudProgram.addConstant( LLGLSLShader::SHADER_CONST_CLOUD_MOON_DEPTH ); // SL-14113
		success = gDeferredWLCloudProgram.createShader(NULL, NULL);
		llassert(success);
	}

	if (success)
	{
	    gDeferredWLSunProgram.mName = "Deferred Windlight Sun Program";
        gDeferredWLSunProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredWLSunProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLSunProgram.mFeatures.hasGamma = true;
        gDeferredWLSunProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLSunProgram.mFeatures.disableTextureIndex = true;
        gDeferredWLSunProgram.mFeatures.hasSrgb = true;
        gDeferredWLSunProgram.mShaderFiles.clear();
        gDeferredWLSunProgram.mShaderFiles.push_back(make_pair("deferred/sunDiscV.glsl", GL_VERTEX_SHADER));
        gDeferredWLSunProgram.mShaderFiles.push_back(make_pair("deferred/sunDiscF.glsl", GL_FRAGMENT_SHADER));
        gDeferredWLSunProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredWLSunProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        success = gDeferredWLSunProgram.createShader(NULL, NULL);
        llassert(success);
    }

    if (success)
    {
        gDeferredWLMoonProgram.mName = "Deferred Windlight Moon Program";
        gDeferredWLMoonProgram.mFeatures.calculatesAtmospherics = true;
        gDeferredWLMoonProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLMoonProgram.mFeatures.hasGamma = true;
        gDeferredWLMoonProgram.mFeatures.hasAtmospherics = true;
        gDeferredWLMoonProgram.mFeatures.hasSrgb = true;
        gDeferredWLMoonProgram.mFeatures.disableTextureIndex = true;
        
        gDeferredWLMoonProgram.mShaderFiles.clear();
        gDeferredWLMoonProgram.mShaderFiles.push_back(make_pair("deferred/moonV.glsl", GL_VERTEX_SHADER));
        gDeferredWLMoonProgram.mShaderFiles.push_back(make_pair("deferred/moonF.glsl", GL_FRAGMENT_SHADER));
        gDeferredWLMoonProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        gDeferredWLMoonProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        gDeferredWLMoonProgram.addConstant( LLGLSLShader::SHADER_CONST_CLOUD_MOON_DEPTH ); // SL-14113
 	 	success = gDeferredWLMoonProgram.createShader(NULL, NULL);
        llassert(success);
 	}

 	if (success)
	{
		gDeferredStarProgram.mName = "Deferred Star Program";
		gDeferredStarProgram.mShaderFiles.clear();
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsV.glsl", GL_VERTEX_SHADER));
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsF.glsl", GL_FRAGMENT_SHADER));
		gDeferredStarProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gDeferredStarProgram.mShaderGroup = LLGLSLShader::SG_SKY;
        gDeferredStarProgram.addConstant( LLGLSLShader::SHADER_CONST_STAR_DEPTH ); // SL-14113
		success = gDeferredStarProgram.createShader(NULL, NULL);
        llassert(success);
	}

	if (success)
	{
		gNormalMapGenProgram.mName = "Normal Map Generation Program";
		gNormalMapGenProgram.mShaderFiles.clear();
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenV.glsl", GL_VERTEX_SHADER));
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenF.glsl", GL_FRAGMENT_SHADER));
		gNormalMapGenProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		gNormalMapGenProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gNormalMapGenProgram.createShader(NULL, NULL);
	}

    if (success)
    {
        gDeferredGenBrdfLutProgram.mName = "Brdf Gen Shader";
        gDeferredGenBrdfLutProgram.mShaderFiles.clear();
        gDeferredGenBrdfLutProgram.mShaderFiles.push_back(make_pair("deferred/genbrdflutV.glsl", GL_VERTEX_SHADER));
        gDeferredGenBrdfLutProgram.mShaderFiles.push_back(make_pair("deferred/genbrdflutF.glsl", GL_FRAGMENT_SHADER));
        gDeferredGenBrdfLutProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
        success = gDeferredGenBrdfLutProgram.createShader(NULL, NULL);
    }

	if (success) {
        gPostScreenSpaceReflectionProgram.mName = "Screen Space Reflection Post";
        gPostScreenSpaceReflectionProgram.mShaderFiles.clear();
        gPostScreenSpaceReflectionProgram.mShaderFiles.push_back(make_pair("deferred/screenSpaceReflPostV.glsl", GL_VERTEX_SHADER));
        gPostScreenSpaceReflectionProgram.mShaderFiles.push_back(make_pair("deferred/screenSpaceReflPostF.glsl", GL_FRAGMENT_SHADER));
        gPostScreenSpaceReflectionProgram.mFeatures.hasScreenSpaceReflections = true;
        gPostScreenSpaceReflectionProgram.mFeatures.isDeferred                = true;
        gPostScreenSpaceReflectionProgram.mShaderLevel = 3;
        success = gPostScreenSpaceReflectionProgram.createShader(NULL, NULL);
	}

	if (success) {
		gDeferredBufferVisualProgram.mName = "Deferred Buffer Visualization Shader";
		gDeferredBufferVisualProgram.mShaderFiles.clear();
		gDeferredBufferVisualProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
		gDeferredBufferVisualProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredVisualizeBuffers.glsl", GL_FRAGMENT_SHADER));
		gDeferredBufferVisualProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
		success = gDeferredBufferVisualProgram.createShader(NULL, NULL);
	}

	return success;
}

bool LLViewerShaderMgr::loadShadersObject()
{
    LL_PROFILE_ZONE_SCOPED;
	bool success = true;

    if (success)
    {
        gObjectBumpProgram.mName = "Bump Shader";
        gObjectBumpProgram.mFeatures.encodesNormal = true;
        gObjectBumpProgram.mShaderFiles.clear();
        gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpV.glsl", GL_VERTEX_SHADER));
        gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpF.glsl", GL_FRAGMENT_SHADER));
        gObjectBumpProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
        success = make_rigged_variant(gObjectBumpProgram, gSkinnedObjectBumpProgram);
        success = success && gObjectBumpProgram.createShader(NULL, NULL);
        if (success)
        { //lldrawpoolbump assumes "texture0" has channel 0 and "texture1" has channel 1
            LLGLSLShader* shader[] = { &gObjectBumpProgram, &gSkinnedObjectBumpProgram };
            for (int i = 0; i < 2; ++i)
            {
                shader[i]->bind();
                shader[i]->uniform1i(sTexture0, 0);
                shader[i]->uniform1i(sTexture1, 1);
                shader[i]->unbind();
            }
        }
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
		gObjectAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("objects/simpleNoColorV.glsl", GL_VERTEX_SHADER));
		gObjectAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER));
		gObjectAlphaMaskNoColorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gObjectAlphaMaskNoColorProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gImpostorProgram.mName = "Impostor Shader";
		gImpostorProgram.mFeatures.disableTextureIndex = true;
		gImpostorProgram.mFeatures.hasSrgb = true;
		gImpostorProgram.mShaderFiles.clear();
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorV.glsl", GL_VERTEX_SHADER));
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorF.glsl", GL_FRAGMENT_SHADER));
		gImpostorProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gImpostorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectPreviewProgram.mName = "Object Preview Shader";
		gObjectPreviewProgram.mFeatures.disableTextureIndex = true;
		gObjectPreviewProgram.mShaderFiles.clear();
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewV.glsl", GL_VERTEX_SHADER));
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewF.glsl", GL_FRAGMENT_SHADER));
		gObjectPreviewProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
        success = make_rigged_variant(gObjectPreviewProgram, gSkinnedObjectPreviewProgram);
		success = gObjectPreviewProgram.createShader(NULL, NULL);
		gObjectPreviewProgram.mFeatures.hasLighting = true;
        gSkinnedObjectPreviewProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gPhysicsPreviewProgram.mName = "Preview Physics Shader";
		gPhysicsPreviewProgram.mFeatures.calculatesLighting = false;
		gPhysicsPreviewProgram.mFeatures.calculatesAtmospherics = false;
		gPhysicsPreviewProgram.mFeatures.hasGamma = false;
		gPhysicsPreviewProgram.mFeatures.hasAtmospherics = false;
		gPhysicsPreviewProgram.mFeatures.hasLighting = false;
		gPhysicsPreviewProgram.mFeatures.mIndexedTextureChannels = 0;
		gPhysicsPreviewProgram.mFeatures.disableTextureIndex = true;
		gPhysicsPreviewProgram.mShaderFiles.clear();
		gPhysicsPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewPhysicsV.glsl", GL_VERTEX_SHADER));
		gPhysicsPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewPhysicsF.glsl", GL_FRAGMENT_SHADER));
		gPhysicsPreviewProgram.mShaderLevel = mShaderLevel[SHADER_OBJECT];
		success = gPhysicsPreviewProgram.createShader(NULL, NULL);
		gPhysicsPreviewProgram.mFeatures.hasLighting = false;
	}

    if (!success)
    {
        mShaderLevel[SHADER_OBJECT] = 0;
        return false;
    }

	return true;
}

bool LLViewerShaderMgr::loadShadersAvatar()
{
    LL_PROFILE_ZONE_SCOPED;
#if 1 // DEPRECATED -- forward rendering is deprecated
	bool success = true;

	if (mShaderLevel[SHADER_AVATAR] == 0)
	{
		gAvatarProgram.unload();
		gAvatarEyeballProgram.unload();
		return true;
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
		gAvatarProgram.mShaderFiles.push_back(make_pair("avatar/avatarV.glsl", GL_VERTEX_SHADER));
		gAvatarProgram.mShaderFiles.push_back(make_pair("avatar/avatarF.glsl", GL_FRAGMENT_SHADER));
		gAvatarProgram.mShaderLevel = mShaderLevel[SHADER_AVATAR];
		success = gAvatarProgram.createShader(NULL, NULL);
			
		/// Keep track of avatar levels
		if (gAvatarProgram.mShaderLevel != mShaderLevel[SHADER_AVATAR])
		{
			mMaxAvatarShaderLevel = mShaderLevel[SHADER_AVATAR] = gAvatarProgram.mShaderLevel;
		}
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
		gAvatarEyeballProgram.mShaderFiles.push_back(make_pair("avatar/eyeballV.glsl", GL_VERTEX_SHADER));
		gAvatarEyeballProgram.mShaderFiles.push_back(make_pair("avatar/eyeballF.glsl", GL_FRAGMENT_SHADER));
		gAvatarEyeballProgram.mShaderLevel = mShaderLevel[SHADER_AVATAR];
		success = gAvatarEyeballProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mShaderLevel[SHADER_AVATAR] = 0;
		mMaxAvatarShaderLevel = 0;
		return false;
	}
#endif
	return true;
}

bool LLViewerShaderMgr::loadShadersInterface()
{
    LL_PROFILE_ZONE_SCOPED;
	bool success = true;

	if (success)
	{
		gHighlightProgram.mName = "Highlight Shader";
		gHighlightProgram.mShaderFiles.clear();
		gHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightV.glsl", GL_VERTEX_SHADER));
		gHighlightProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER));
		gHighlightProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = make_rigged_variant(gHighlightProgram, gSkinnedHighlightProgram);
		success = success && gHighlightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gHighlightNormalProgram.mName = "Highlight Normals Shader";
		gHighlightNormalProgram.mShaderFiles.clear();
		gHighlightNormalProgram.mShaderFiles.push_back(make_pair("interface/highlightNormV.glsl", GL_VERTEX_SHADER));
		gHighlightNormalProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER));
		gHighlightNormalProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gHighlightNormalProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gHighlightSpecularProgram.mName = "Highlight Spec Shader";
		gHighlightSpecularProgram.mShaderFiles.clear();
		gHighlightSpecularProgram.mShaderFiles.push_back(make_pair("interface/highlightSpecV.glsl", GL_VERTEX_SHADER));
		gHighlightSpecularProgram.mShaderFiles.push_back(make_pair("interface/highlightF.glsl", GL_FRAGMENT_SHADER));
		gHighlightSpecularProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gHighlightSpecularProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gUIProgram.mName = "UI Shader";
		gUIProgram.mShaderFiles.clear();
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiV.glsl", GL_VERTEX_SHADER));
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiF.glsl", GL_FRAGMENT_SHADER));
		gUIProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gUIProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingProgram.mName = "Pathfinding Shader";
		gPathfindingProgram.mShaderFiles.clear();
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingV.glsl", GL_VERTEX_SHADER));
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER));
		gPathfindingProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gPathfindingProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingNoNormalsProgram.mName = "PathfindingNoNormals Shader";
		gPathfindingNoNormalsProgram.mShaderFiles.clear();
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingNoNormalV.glsl", GL_VERTEX_SHADER));
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER));
		gPathfindingNoNormalsProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gPathfindingNoNormalsProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gGlowCombineProgram.mName = "Glow Combine Shader";
		gGlowCombineProgram.mShaderFiles.clear();
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineV.glsl", GL_VERTEX_SHADER));
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineF.glsl", GL_FRAGMENT_SHADER));
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
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAV.glsl", GL_VERTEX_SHADER));
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAF.glsl", GL_FRAGMENT_SHADER));
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

#ifdef LL_WINDOWS
	if (success)
	{
		gTwoTextureCompareProgram.mName = "Two Texture Compare Shader";
		gTwoTextureCompareProgram.mShaderFiles.clear();
		gTwoTextureCompareProgram.mShaderFiles.push_back(make_pair("interface/twotexturecompareV.glsl", GL_VERTEX_SHADER));
		gTwoTextureCompareProgram.mShaderFiles.push_back(make_pair("interface/twotexturecompareF.glsl", GL_FRAGMENT_SHADER));
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
		gOneTextureFilterProgram.mShaderFiles.push_back(make_pair("interface/onetexturefilterV.glsl", GL_VERTEX_SHADER));
		gOneTextureFilterProgram.mShaderFiles.push_back(make_pair("interface/onetexturefilterF.glsl", GL_FRAGMENT_SHADER));
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
		gSolidColorProgram.mName = "Solid Color Shader";
		gSolidColorProgram.mShaderFiles.clear();
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorV.glsl", GL_VERTEX_SHADER));
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorF.glsl", GL_FRAGMENT_SHADER));
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
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionV.glsl", GL_VERTEX_SHADER));
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER));
		gOcclusionProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        gOcclusionProgram.mRiggedVariant = &gSkinnedOcclusionProgram;
		success = gOcclusionProgram.createShader(NULL, NULL);
	}

    if (success)
    {
        gSkinnedOcclusionProgram.mName = "Skinned Occlusion Shader";
        gSkinnedOcclusionProgram.mFeatures.hasObjectSkinning = true;
        gSkinnedOcclusionProgram.mShaderFiles.clear();
        gSkinnedOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionSkinnedV.glsl", GL_VERTEX_SHADER));
        gSkinnedOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER));
        gSkinnedOcclusionProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gSkinnedOcclusionProgram.createShader(NULL, NULL);
    }

	if (success)
	{
		gOcclusionCubeProgram.mName = "Occlusion Cube Shader";
		gOcclusionCubeProgram.mShaderFiles.clear();
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionCubeV.glsl", GL_VERTEX_SHADER));
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER));
		gOcclusionCubeProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gOcclusionCubeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDebugProgram.mName = "Debug Shader";
		gDebugProgram.mShaderFiles.clear();
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugV.glsl", GL_VERTEX_SHADER));
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugF.glsl", GL_FRAGMENT_SHADER));
        gDebugProgram.mRiggedVariant = &gSkinnedDebugProgram;
		gDebugProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = make_rigged_variant(gDebugProgram, gSkinnedDebugProgram);
		success = success && gDebugProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gClipProgram.mName = "Clip Shader";
		gClipProgram.mShaderFiles.clear();
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipV.glsl", GL_VERTEX_SHADER));
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipF.glsl", GL_FRAGMENT_SHADER));
		gClipProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gClipProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gBenchmarkProgram.mName = "Benchmark Shader";
		gBenchmarkProgram.mShaderFiles.clear();
		gBenchmarkProgram.mShaderFiles.push_back(make_pair("interface/benchmarkV.glsl", GL_VERTEX_SHADER));
		gBenchmarkProgram.mShaderFiles.push_back(make_pair("interface/benchmarkF.glsl", GL_FRAGMENT_SHADER));
		gBenchmarkProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gBenchmarkProgram.createShader(NULL, NULL);
	}

    if (success)
    {
        gReflectionProbeDisplayProgram.mName = "Reflection Probe Display Shader";
        gReflectionProbeDisplayProgram.mFeatures.hasReflectionProbes = true;
        gReflectionProbeDisplayProgram.mFeatures.hasSrgb = true;
        gReflectionProbeDisplayProgram.mFeatures.calculatesAtmospherics = true;
        gReflectionProbeDisplayProgram.mFeatures.hasAtmospherics = true;
        gReflectionProbeDisplayProgram.mFeatures.hasGamma = true;
        gReflectionProbeDisplayProgram.mFeatures.isDeferred = true;
        gReflectionProbeDisplayProgram.mShaderFiles.clear();
        gReflectionProbeDisplayProgram.mShaderFiles.push_back(make_pair("interface/reflectionprobeV.glsl", GL_VERTEX_SHADER));
        gReflectionProbeDisplayProgram.mShaderFiles.push_back(make_pair("interface/reflectionprobeF.glsl", GL_FRAGMENT_SHADER));
        gReflectionProbeDisplayProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gReflectionProbeDisplayProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gCopyProgram.mName = "Copy Shader";
        gCopyProgram.mShaderFiles.clear();
        gCopyProgram.mShaderFiles.push_back(make_pair("interface/copyV.glsl", GL_VERTEX_SHADER));
        gCopyProgram.mShaderFiles.push_back(make_pair("interface/copyF.glsl", GL_FRAGMENT_SHADER));
        gCopyProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gCopyProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gCopyDepthProgram.mName = "Copy Depth Shader";
        gCopyDepthProgram.mShaderFiles.clear();
        gCopyDepthProgram.mShaderFiles.push_back(make_pair("interface/copyV.glsl", GL_VERTEX_SHADER));
        gCopyDepthProgram.mShaderFiles.push_back(make_pair("interface/copyF.glsl", GL_FRAGMENT_SHADER));
        gCopyDepthProgram.clearPermutations();
        gCopyDepthProgram.addPermutation("COPY_DEPTH", "1");
        gCopyDepthProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gCopyDepthProgram.createShader(NULL, NULL);
    }
    
	if (success)
	{
		gAlphaMaskProgram.mName = "Alpha Mask Shader";
		gAlphaMaskProgram.mShaderFiles.clear();
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskV.glsl", GL_VERTEX_SHADER));
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskF.glsl", GL_FRAGMENT_SHADER));
		gAlphaMaskProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
		success = gAlphaMaskProgram.createShader(NULL, NULL);
	}

    if (success)
    {
        gReflectionMipProgram.mName = "Reflection Mip Shader";
        gReflectionMipProgram.mFeatures.isDeferred = true;
        gReflectionMipProgram.mFeatures.hasGamma = true;
        gReflectionMipProgram.mFeatures.hasAtmospherics = true;
        gReflectionMipProgram.mFeatures.calculatesAtmospherics = true;
        gReflectionMipProgram.mShaderFiles.clear();
        gReflectionMipProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectV.glsl", GL_VERTEX_SHADER));
        gReflectionMipProgram.mShaderFiles.push_back(make_pair("interface/reflectionmipF.glsl", GL_FRAGMENT_SHADER));
        gReflectionMipProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gReflectionMipProgram.createShader(NULL, NULL);
    }

    if (success)
    {
        gGaussianProgram.mName = "Reflection Mip Shader";
        gGaussianProgram.mFeatures.isDeferred = true;
        gGaussianProgram.mFeatures.hasGamma = true;
        gGaussianProgram.mFeatures.hasAtmospherics = true;
        gGaussianProgram.mFeatures.calculatesAtmospherics = true;
        gGaussianProgram.mShaderFiles.clear();
        gGaussianProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectV.glsl", GL_VERTEX_SHADER));
        gGaussianProgram.mShaderFiles.push_back(make_pair("interface/gaussianF.glsl", GL_FRAGMENT_SHADER));
        gGaussianProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gGaussianProgram.createShader(NULL, NULL);
    }

    if (success && gGLManager.mHasCubeMapArray)
    {
        gRadianceGenProgram.mName = "Radiance Gen Shader";
        gRadianceGenProgram.mShaderFiles.clear();
        gRadianceGenProgram.mShaderFiles.push_back(make_pair("interface/radianceGenV.glsl", GL_VERTEX_SHADER));
        gRadianceGenProgram.mShaderFiles.push_back(make_pair("interface/radianceGenF.glsl", GL_FRAGMENT_SHADER));
        gRadianceGenProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gRadianceGenProgram.createShader(NULL, NULL);
    }

    if (success && gGLManager.mHasCubeMapArray)
    {
        gIrradianceGenProgram.mName = "Irradiance Gen Shader";
        gIrradianceGenProgram.mShaderFiles.clear();
        gIrradianceGenProgram.mShaderFiles.push_back(make_pair("interface/irradianceGenV.glsl", GL_VERTEX_SHADER));
        gIrradianceGenProgram.mShaderFiles.push_back(make_pair("interface/irradianceGenF.glsl", GL_FRAGMENT_SHADER));
        gIrradianceGenProgram.mShaderLevel = mShaderLevel[SHADER_INTERFACE];
        success = gIrradianceGenProgram.createShader(NULL, NULL);
    }

	if( !success )
	{
		mShaderLevel[SHADER_INTERFACE] = 0;
		return false;
	}
	
	return true;
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

