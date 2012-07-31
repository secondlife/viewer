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

#include "llfeaturemanager.h"
#include "llviewershadermgr.h"

#include "llfile.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llworld.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llsky.h"
#include "llvosky.h"
#include "llrender.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

#ifdef LL_RELEASE_FOR_DOWNLOAD
#define UNIFORM_ERRS LL_WARNS_ONCE("Shader")
#else
#define UNIFORM_ERRS LL_ERRS("Shader")
#endif

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
LLGLSLShader			gTransformBinormalProgram;

//utility shaders
LLGLSLShader	gOcclusionProgram;
LLGLSLShader	gOcclusionCubeProgram;
LLGLSLShader	gCustomAlphaProgram;
LLGLSLShader	gGlowCombineProgram;
LLGLSLShader	gSplatTextureRectProgram;
LLGLSLShader	gGlowCombineFXAAProgram;
LLGLSLShader	gTwoTextureAddProgram;
LLGLSLShader	gOneTextureNoColorProgram;
LLGLSLShader	gDebugProgram;
LLGLSLShader	gClipProgram;
LLGLSLShader	gAlphaMaskProgram;

//object shaders
LLGLSLShader		gObjectSimpleProgram;
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
LLGLSLShader		gUnderWaterProgram;

//interface shaders
LLGLSLShader		gHighlightProgram;
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


// Effects Shaders
LLGLSLShader			gGlowProgram;
LLGLSLShader			gGlowExtractProgram;
LLGLSLShader			gPostColorFilterProgram;
LLGLSLShader			gPostNightVisionProgram;

// Deferred rendering shaders
LLGLSLShader			gDeferredImpostorProgram;
LLGLSLShader			gDeferredWaterProgram;
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
LLGLSLShader			gDeferredTreeProgram;
LLGLSLShader			gDeferredTreeShadowProgram;
LLGLSLShader			gDeferredAvatarProgram;
LLGLSLShader			gDeferredAvatarAlphaProgram;
LLGLSLShader			gDeferredLightProgram;
LLGLSLShader			gDeferredMultiLightProgram;
LLGLSLShader			gDeferredSpotLightProgram;
LLGLSLShader			gDeferredMultiSpotLightProgram;
LLGLSLShader			gDeferredSunProgram;
LLGLSLShader			gDeferredBlurLightProgram;
LLGLSLShader			gDeferredSoftenProgram;
LLGLSLShader			gDeferredShadowProgram;
LLGLSLShader			gDeferredShadowCubeProgram;
LLGLSLShader			gDeferredShadowAlphaMaskProgram;
LLGLSLShader			gDeferredAvatarShadowProgram;
LLGLSLShader			gDeferredAttachmentShadowProgram;
LLGLSLShader			gDeferredAlphaProgram;
LLGLSLShader			gDeferredAvatarEyesProgram;
LLGLSLShader			gDeferredFullbrightProgram;
LLGLSLShader			gDeferredEmissiveProgram;
LLGLSLShader			gDeferredPostProgram;
LLGLSLShader			gDeferredCoFProgram;
LLGLSLShader			gDeferredDoFCombineProgram;
LLGLSLShader			gFXAAProgram;
LLGLSLShader			gDeferredPostNoDoFProgram;
LLGLSLShader			gDeferredWLSkyProgram;
LLGLSLShader			gDeferredWLCloudProgram;
LLGLSLShader			gDeferredStarProgram;
LLGLSLShader			gNormalMapGenProgram;

LLViewerShaderMgr::LLViewerShaderMgr() :
	mVertexShaderLevel(SHADER_COUNT, 0),
	mMaxAvatarShaderLevel(0)
{	
	/// Make sure WL Sky is the first program
	//ONLY shaders that need WL Param management should be added here
	mShaderList.push_back(&gWLSkyProgram);
	mShaderList.push_back(&gWLCloudProgram);
	mShaderList.push_back(&gAvatarProgram);
	mShaderList.push_back(&gObjectShinyProgram);
	mShaderList.push_back(&gObjectShinyNonIndexedProgram);
	mShaderList.push_back(&gWaterProgram);
	mShaderList.push_back(&gAvatarEyeballProgram); 
	mShaderList.push_back(&gObjectSimpleProgram);
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
	mShaderList.push_back(&gDeferredAlphaProgram);
	mShaderList.push_back(&gDeferredSkinnedAlphaProgram);
	mShaderList.push_back(&gDeferredFullbrightProgram);
	mShaderList.push_back(&gDeferredEmissiveProgram);
	mShaderList.push_back(&gDeferredAvatarEyesProgram);
	mShaderList.push_back(&gDeferredWaterProgram);
	mShaderList.push_back(&gDeferredAvatarAlphaProgram);
	mShaderList.push_back(&gDeferredWLSkyProgram);
	mShaderList.push_back(&gDeferredWLCloudProgram);
}

LLViewerShaderMgr::~LLViewerShaderMgr()
{
	mVertexShaderLevel.clear();
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

void LLViewerShaderMgr::initAttribsAndUniforms(void)
{
	if (mReservedAttribs.empty())
	{
		LLShaderMgr::initAttribsAndUniforms();

		mAvatarUniforms.push_back("matrixPalette");
		mAvatarUniforms.push_back("gWindDir");
		mAvatarUniforms.push_back("gSinWaveParams");
		mAvatarUniforms.push_back("gGravity");

		mWLUniforms.push_back("camPosLocal");

		mTerrainUniforms.reserve(5);
		mTerrainUniforms.push_back("detail_0");
		mTerrainUniforms.push_back("detail_1");
		mTerrainUniforms.push_back("detail_2");
		mTerrainUniforms.push_back("detail_3");
		mTerrainUniforms.push_back("alpha_ramp");

		mGlowUniforms.push_back("glowDelta");
		mGlowUniforms.push_back("glowStrength");

		mGlowExtractUniforms.push_back("minLuminance");
		mGlowExtractUniforms.push_back("maxExtractAlpha");
		mGlowExtractUniforms.push_back("lumWeights");
		mGlowExtractUniforms.push_back("warmthWeights");
		mGlowExtractUniforms.push_back("warmthAmount");

		mShinyUniforms.push_back("origin");

		mWaterUniforms.reserve(12);
		mWaterUniforms.push_back("screenTex");
		mWaterUniforms.push_back("screenDepth");
		mWaterUniforms.push_back("refTex");
		mWaterUniforms.push_back("eyeVec");
		mWaterUniforms.push_back("time");
		mWaterUniforms.push_back("d1");
		mWaterUniforms.push_back("d2");
		mWaterUniforms.push_back("lightDir");
		mWaterUniforms.push_back("specular");
		mWaterUniforms.push_back("lightExp");
		mWaterUniforms.push_back("fogCol");
		mWaterUniforms.push_back("kd");
		mWaterUniforms.push_back("refScale");
		mWaterUniforms.push_back("waterHeight");
	}	
}
	

//============================================================================
// Set Levels

S32 LLViewerShaderMgr::getVertexShaderLevel(S32 type)
{
	return LLPipeline::sDisableShaders ? 0 : mVertexShaderLevel[type];
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

	LLGLSLShader::sIndexedTextureChannels = llmax(llmin(gGLManager.mNumTextureImageUnits, (S32) gSavedSettings.getU32("RenderMaxTextureIndex")), 1);

	//NEVER use more than 16 texture channels (work around for prevalent driver bug)
	LLGLSLShader::sIndexedTextureChannels = llmin(LLGLSLShader::sIndexedTextureChannels, 16);

	if (gGLManager.mGLSLVersionMajor < 1 ||
		(gGLManager.mGLSLVersionMajor == 1 && gGLManager.mGLSLVersionMinor <= 20))
	{ //NEVER use indexed texture rendering when GLSL version is 1.20 or earlier
		LLGLSLShader::sIndexedTextureChannels = 1;
	}

	reentrance = true;

	if (LLRender::sGLCoreProfile)
	{  
		if (!gSavedSettings.getBOOL("VertexShaderEnable"))
		{ //vertex shaders MUST be enabled to use core profile
			gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
		}
	}
	
	//setup preprocessor definitions
	LLShaderMgr::instance()->mDefinitions["NUM_TEX_UNITS"] = llformat("%d", gGLManager.mNumTextureImageUnits);
	
	// Make sure the compiled shader map is cleared before we recompile shaders.
	mShaderObjects.clear();
	
	initAttribsAndUniforms();
	gPipeline.releaseGLBuffers();

	if (gSavedSettings.getBOOL("VertexShaderEnable"))
	{
		LLPipeline::sWaterReflections = gGLManager.mHasCubeMap;
		LLPipeline::sRenderGlow = gSavedSettings.getBOOL("RenderGlow"); 
		LLPipeline::updateRenderDeferred();
	}
	else
	{
		LLPipeline::sRenderGlow = FALSE;
		LLPipeline::sWaterReflections = FALSE;
	}
	
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
	LL_INFOS("ShaderLoading") << llformat("Using GLSL %d.%d", gGLManager.mGLSLVersionMajor, gGLManager.mGLSLVersionMinor) << llendl;

	for (S32 i = 0; i < SHADER_COUNT; i++)
	{
		mVertexShaderLevel[i] = 0;
	}
	mMaxAvatarShaderLevel = 0;

	LLGLSLShader::sNoFixedFunction = false;
	LLVertexBuffer::unbind();
	if (LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable") 
		&& (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 10)
		&& gSavedSettings.getBOOL("VertexShaderEnable"))
	{
		//using shaders, disable fixed function
		LLGLSLShader::sNoFixedFunction = true;

		S32 light_class = 2;
		S32 env_class = 2;
		S32 obj_class = 2;
		S32 effect_class = 2;
		S32 wl_class = 2;
		S32 water_class = 2;
		S32 deferred_class = 0;
		S32 transform_class = gGLManager.mHasTransformFeedback ? 1 : 0;

		if (LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
		    gSavedSettings.getBOOL("RenderDeferred") &&
			gSavedSettings.getBOOL("RenderAvatarVP") &&
			gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
		{
			if (gSavedSettings.getS32("RenderShadowDetail") > 0)
			{ //shadows
				deferred_class = 2;
			}
			else
			{ //no shadows
				deferred_class = 1;
			}

			//make sure hardware skinning is enabled
			//gSavedSettings.setBOOL("RenderAvatarVP", TRUE);
			
			//make sure atmospheric shaders are enabled
			//gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
		}


		if (!(LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders")
			  && gSavedSettings.getBOOL("WindLightUseAtmosShaders")))
		{
			// user has disabled WindLight in their settings, downgrade
			// windlight shaders to stub versions.
			wl_class = 1;
		}

		
		// Trigger a full rebuild of the fallback skybox / cubemap if we've toggled windlight shaders
		if (mVertexShaderLevel[SHADER_WINDLIGHT] != wl_class && gSky.mVOSkyp.notNull())
		{
			gSky.mVOSkyp->forceSkyUpdate();
		}

		
		// Load lighting shaders
		mVertexShaderLevel[SHADER_LIGHTING] = light_class;
		mVertexShaderLevel[SHADER_INTERFACE] = light_class;
		mVertexShaderLevel[SHADER_ENVIRONMENT] = env_class;
		mVertexShaderLevel[SHADER_WATER] = water_class;
		mVertexShaderLevel[SHADER_OBJECT] = obj_class;
		mVertexShaderLevel[SHADER_EFFECT] = effect_class;
		mVertexShaderLevel[SHADER_WINDLIGHT] = wl_class;
		mVertexShaderLevel[SHADER_DEFERRED] = deferred_class;
		mVertexShaderLevel[SHADER_TRANSFORM] = transform_class;

		BOOL loaded = loadBasicShaders();

		if (loaded)
		{
			gPipeline.mVertexShadersEnabled = TRUE;
			gPipeline.mVertexShadersLoaded = 1;

			// Load all shaders to set max levels
			loaded = loadShadersEnvironment();

			if (loaded)
			{
				loaded = loadShadersWater();
			}

			if (loaded)
			{
				loaded = loadShadersWindLight();
			}

			if (loaded)
			{
				loaded = loadShadersEffects();
			}

			if (loaded)
			{
				loaded = loadShadersInterface();
			}
			
			if (loaded)
			{
				loaded = loadTransformShaders();
			}

			if (loaded)
			{
				// Load max avatar shaders to set the max level
				mVertexShaderLevel[SHADER_AVATAR] = 3;
				mMaxAvatarShaderLevel = 3;
				
				if (gSavedSettings.getBOOL("RenderAvatarVP") && loadShadersObject())
				{ //hardware skinning is enabled and rigged attachment shaders loaded correctly
					BOOL avatar_cloth = gSavedSettings.getBOOL("RenderAvatarCloth");
					S32 avatar_class = 1;
				
					// cloth is a class3 shader
					if(avatar_cloth)
					{
						avatar_class = 3;
					}

					// Set the actual level
					mVertexShaderLevel[SHADER_AVATAR] = avatar_class;
					loadShadersAvatar();
					if (mVertexShaderLevel[SHADER_AVATAR] != avatar_class)
					{
						if (mVertexShaderLevel[SHADER_AVATAR] == 0)
						{
							gSavedSettings.setBOOL("RenderAvatarVP", FALSE);
						}
						if(llmax(mVertexShaderLevel[SHADER_AVATAR]-1,0) >= 3)
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
					mVertexShaderLevel[SHADER_AVATAR] = 0;
					mVertexShaderLevel[SHADER_DEFERRED] = 0;

					if (gSavedSettings.getBOOL("RenderAvatarVP"))
					{
						gSavedSettings.setBOOL("RenderDeferred", FALSE);
						gSavedSettings.setBOOL("RenderAvatarCloth", FALSE);
						gSavedSettings.setBOOL("RenderAvatarVP", FALSE);
					}

					loadShadersAvatar(); // unloads

					loaded = loadShadersObject();
				}
			}

			if (!loaded)
			{ //some shader absolutely could not load, try to fall back to a simpler setting
				if (gSavedSettings.getBOOL("WindLightUseAtmosShaders"))
				{ //disable windlight and try again
					gSavedSettings.setBOOL("WindLightUseAtmosShaders", FALSE);
					reentrance = false;
					setShaders();
					return;
				}

				if (gSavedSettings.getBOOL("VertexShaderEnable"))
				{ //disable shaders outright and try again
					gSavedSettings.setBOOL("VertexShaderEnable", FALSE);
					reentrance = false;
					setShaders();
					return;
				}
			}		

			if (loaded && !loadShadersDeferred())
			{ //everything else succeeded but deferred failed, disable deferred and try again
				gSavedSettings.setBOOL("RenderDeferred", FALSE);
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
			mVertexShaderLevel[SHADER_LIGHTING] = 0;
			mVertexShaderLevel[SHADER_INTERFACE] = 0;
			mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
			mVertexShaderLevel[SHADER_WATER] = 0;
			mVertexShaderLevel[SHADER_OBJECT] = 0;
			mVertexShaderLevel[SHADER_EFFECT] = 0;
			mVertexShaderLevel[SHADER_WINDLIGHT] = 0;
			mVertexShaderLevel[SHADER_AVATAR] = 0;
		}
	}
	else
	{
		LLGLSLShader::sNoFixedFunction = false;
		gPipeline.mVertexShadersEnabled = FALSE;
		gPipeline.mVertexShadersLoaded = 0;
		mVertexShaderLevel[SHADER_LIGHTING] = 0;
		mVertexShaderLevel[SHADER_INTERFACE] = 0;
		mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
		mVertexShaderLevel[SHADER_WATER] = 0;
		mVertexShaderLevel[SHADER_OBJECT] = 0;
		mVertexShaderLevel[SHADER_EFFECT] = 0;
		mVertexShaderLevel[SHADER_WINDLIGHT] = 0;
		mVertexShaderLevel[SHADER_AVATAR] = 0;
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
	gAlphaMaskProgram.unload();
	gUIProgram.unload();
	gPathfindingProgram.unload();
	gPathfindingNoNormalsProgram.unload();
	gCustomAlphaProgram.unload();
	gGlowCombineProgram.unload();
	gSplatTextureRectProgram.unload();
	gGlowCombineFXAAProgram.unload();
	gTwoTextureAddProgram.unload();
	gOneTextureNoColorProgram.unload();
	gSolidColorProgram.unload();

	gObjectFullbrightNoColorProgram.unload();
	gObjectFullbrightNoColorWaterProgram.unload();
	gObjectSimpleProgram.unload();
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

	gWLSkyProgram.unload();
	gWLCloudProgram.unload();

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
	gTransformBinormalProgram.unload();

	mVertexShaderLevel[SHADER_LIGHTING] = 0;
	mVertexShaderLevel[SHADER_OBJECT] = 0;
	mVertexShaderLevel[SHADER_AVATAR] = 0;
	mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
	mVertexShaderLevel[SHADER_WATER] = 0;
	mVertexShaderLevel[SHADER_INTERFACE] = 0;
	mVertexShaderLevel[SHADER_EFFECT] = 0;
	mVertexShaderLevel[SHADER_WINDLIGHT] = 0;
	mVertexShaderLevel[SHADER_TRANSFORM] = 0;

	gPipeline.mVertexShadersLoaded = 0;
}

BOOL LLViewerShaderMgr::loadBasicShaders()
{
	// Load basic dependency shaders first
	// All of these have to load for any shaders to function
	
#if LL_DARWIN // Mac can't currently handle all 8 lights, 
	S32 sum_lights_class = 2;
#else 
	S32 sum_lights_class = 3;

	// class one cards will get the lower sum lights
	// class zero we're not going to think about
	// since a class zero card COULD be a ridiculous new card
	// and old cards should have the features masked
	if(LLFeatureManager::getInstance()->getGPUClass() == GPU_CLASS_1)
	{
		sum_lights_class = 2;
	}
#endif

	// If we have sun and moon only checked, then only sum those lights.
	if (gPipeline.getLightingDetail() == 0)
	{
		sum_lights_class = 1;
	}

	// Use the feature table to mask out the max light level to use.  Also make sure it's at least 1.
	S32 max_light_class = gSavedSettings.getS32("RenderShaderLightingMaxLevel");
	sum_lights_class = llclamp(sum_lights_class, 1, max_light_class);

	// Load the Basic Vertex Shaders at the appropriate level. 
	// (in order of shader function call depth for reference purposes, deepest level first)

	vector< pair<string, S32> > shaders;
	shaders.push_back( make_pair( "windlight/atmosphericsVarsV.glsl",		mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsVarsWaterV.glsl",	mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsHelpersV.glsl",	mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "lighting/lightFuncV.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsV.glsl",				sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightV.glsl",					mVertexShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/lightFuncSpecularV.glsl",		mVertexShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "lighting/sumLightsSpecularV.glsl",		sum_lights_class ) );
	shaders.push_back( make_pair( "lighting/lightSpecularV.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	shaders.push_back( make_pair( "windlight/atmosphericsV.glsl",			mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	shaders.push_back( make_pair( "avatar/avatarSkinV.glsl",				1 ) );
	shaders.push_back( make_pair( "avatar/objectSkinV.glsl",				1 ) );
	if (gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)
	{
		shaders.push_back( make_pair( "objects/indexedTextureV.glsl",			1 ) );
	}
	shaders.push_back( make_pair( "objects/nonindexedTextureV.glsl",		1 ) );

	// We no longer have to bind the shaders to global glhandles, they are automatically added to a map now.
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_VERTEX_SHADER_ARB
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_VERTEX_SHADER_ARB) == 0)
		{
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
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "windlight/atmosphericsVarsF.glsl",		mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "windlight/atmosphericsVarsWaterF.glsl",		mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "windlight/gammaF.glsl",					mVertexShaderLevel[SHADER_WINDLIGHT]) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "windlight/atmosphericsF.glsl",			mVertexShaderLevel[SHADER_WINDLIGHT] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "windlight/transportF.glsl",				mVertexShaderLevel[SHADER_WINDLIGHT] ) );	
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "environment/waterFogF.glsl",				mVertexShaderLevel[SHADER_WATER] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightNonIndexedF.glsl",					mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightAlphaMaskNonIndexedF.glsl",					mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightNonIndexedF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightNonIndexedAlphaMaskF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightWaterNonIndexedF.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightWaterAlphaMaskNonIndexedF.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightWaterNonIndexedF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightWaterNonIndexedAlphaMaskF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightShinyNonIndexedF.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightShinyNonIndexedF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightShinyWaterNonIndexedF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(-1);	 shaders.push_back( make_pair( "lighting/lightFullbrightShinyWaterNonIndexedF.glsl", mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightF.glsl",					mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightAlphaMaskF.glsl",					mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightAlphaMaskF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightWaterF.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightWaterAlphaMaskF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightWaterF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightWaterAlphaMaskF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightShinyF.glsl",				mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightShinyF.glsl",	mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightShinyWaterF.glsl",			mVertexShaderLevel[SHADER_LIGHTING] ) );
	index_channels.push_back(ch);	 shaders.push_back( make_pair( "lighting/lightFullbrightShinyWaterF.glsl", mVertexShaderLevel[SHADER_LIGHTING] ) );
	
	for (U32 i = 0; i < shaders.size(); i++)
	{
		// Note usage of GL_FRAGMENT_SHADER_ARB
		if (loadShaderFile(shaders[i].first, shaders[i].second, GL_FRAGMENT_SHADER_ARB, index_channels[i]) == 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersEnvironment()
{
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_ENVIRONMENT] == 0)
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
		gTerrainProgram.mFeatures.mIndexedTextureChannels = 0;
		gTerrainProgram.mFeatures.disableTextureIndex = true;
		gTerrainProgram.mFeatures.hasGamma = true;
		gTerrainProgram.mShaderFiles.clear();
		gTerrainProgram.mShaderFiles.push_back(make_pair("environment/terrainV.glsl", GL_VERTEX_SHADER_ARB));
		gTerrainProgram.mShaderFiles.push_back(make_pair("environment/terrainF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTerrainProgram.mShaderLevel = mVertexShaderLevel[SHADER_ENVIRONMENT];
		success = gTerrainProgram.createShader(NULL, &mTerrainUniforms);
	}

	if (!success)
	{
		mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
		return FALSE;
	}
	
	LLWorld::getInstance()->updateWaterObjects();
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersWater()
{
	BOOL success = TRUE;
	BOOL terrainWaterSuccess = TRUE;

	if (mVertexShaderLevel[SHADER_WATER] == 0)
	{
		gWaterProgram.unload();
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
		gWaterProgram.mShaderFiles.clear();
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gWaterProgram.mShaderFiles.push_back(make_pair("environment/waterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_WATER];
		success = gWaterProgram.createShader(NULL, &mWaterUniforms);
	}

	if (success)
	{
		//load under water vertex shader
		gUnderWaterProgram.mName = "Underwater Shader";
		gUnderWaterProgram.mFeatures.calculatesAtmospherics = true;
		gUnderWaterProgram.mShaderFiles.clear();
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gUnderWaterProgram.mShaderFiles.push_back(make_pair("environment/underWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gUnderWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_WATER];
		gUnderWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		
		success = gUnderWaterProgram.createShader(NULL, &mWaterUniforms);
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
		gTerrainWaterProgram.mShaderFiles.push_back(make_pair("environment/terrainV.glsl", GL_VERTEX_SHADER_ARB));
		gTerrainWaterProgram.mShaderFiles.push_back(make_pair("environment/terrainWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTerrainWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_ENVIRONMENT];
		gTerrainWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		terrainWaterSuccess = gTerrainWaterProgram.createShader(NULL, &mTerrainUniforms);
	}	

	/// Keep track of water shader levels
	if (gWaterProgram.mShaderLevel != mVertexShaderLevel[SHADER_WATER]
		|| gUnderWaterProgram.mShaderLevel != mVertexShaderLevel[SHADER_WATER])
	{
		mVertexShaderLevel[SHADER_WATER] = llmin(gWaterProgram.mShaderLevel, gUnderWaterProgram.mShaderLevel);
	}

	if (!success)
	{
		mVertexShaderLevel[SHADER_WATER] = 0;
		return FALSE;
	}

	// if we failed to load the terrain water shaders and we need them (using class2 water),
	// then drop down to class1 water.
	if (mVertexShaderLevel[SHADER_WATER] > 1 && !terrainWaterSuccess)
	{
		mVertexShaderLevel[SHADER_WATER]--;
		return loadShadersWater();
	}
	
	LLWorld::getInstance()->updateWaterObjects();

	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersEffects()
{
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_EFFECT] == 0)
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
		gGlowProgram.mShaderLevel = mVertexShaderLevel[SHADER_EFFECT];
		success = gGlowProgram.createShader(NULL, &mGlowUniforms);
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
		gGlowExtractProgram.mShaderLevel = mVertexShaderLevel[SHADER_EFFECT];
		success = gGlowExtractProgram.createShader(NULL, &mGlowExtractUniforms);
		if (!success)
		{
			LLPipeline::sRenderGlow = FALSE;
		}
	}
	
	return success;

}

BOOL LLViewerShaderMgr::loadShadersDeferred()
{
	if (mVertexShaderLevel[SHADER_DEFERRED] == 0)
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
		gDeferredLightProgram.unload();
		gDeferredMultiLightProgram.unload();
		gDeferredSpotLightProgram.unload();
		gDeferredMultiSpotLightProgram.unload();
		gDeferredSunProgram.unload();
		gDeferredBlurLightProgram.unload();
		gDeferredSoftenProgram.unload();
		gDeferredShadowProgram.unload();
		gDeferredShadowCubeProgram.unload();
		gDeferredShadowAlphaMaskProgram.unload();
		gDeferredAvatarShadowProgram.unload();
		gDeferredAttachmentShadowProgram.unload();
		gDeferredAvatarProgram.unload();
		gDeferredAvatarAlphaProgram.unload();
		gDeferredAlphaProgram.unload();
		gDeferredFullbrightProgram.unload();
		gDeferredEmissiveProgram.unload();
		gDeferredAvatarEyesProgram.unload();
		gDeferredPostProgram.unload();		
		gDeferredCoFProgram.unload();		
		gDeferredDoFCombineProgram.unload();
		gFXAAProgram.unload();
		gDeferredWaterProgram.unload();
		gDeferredWLSkyProgram.unload();
		gDeferredWLCloudProgram.unload();
		gDeferredStarProgram.unload();
		gNormalMapGenProgram.unload();
		return TRUE;
	}

	BOOL success = TRUE;

	if (success)
	{
		gDeferredDiffuseProgram.mName = "Deferred Diffuse Shader";
		gDeferredDiffuseProgram.mShaderFiles.clear();
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseIndexedF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDiffuseProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredDiffuseProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Alpha Mask Shader";
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskIndexedF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDiffuseAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredDiffuseAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredDiffuseAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask Shader";
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskProgram.createShader(NULL, NULL);
	}
	
	if (success)
	{
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mName = "Deferred Diffuse Non-Indexed Alpha Mask Shader";
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderFiles.push_back(make_pair("deferred/diffuseAlphaMaskNoColorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredNonIndexedDiffuseProgram.mName = "Non Indexed Deferred Diffuse Shader";
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.clear();
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredNonIndexedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredNonIndexedDiffuseProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredNonIndexedDiffuseProgram.createShader(NULL, NULL);
	}
		

	if (success)
	{
		gDeferredSkinnedDiffuseProgram.mName = "Deferred Skinned Diffuse Shader";
		gDeferredSkinnedDiffuseProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedDiffuseProgram.mShaderFiles.clear();
		gDeferredSkinnedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedDiffuseProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedDiffuseProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredSkinnedDiffuseProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredSkinnedBumpProgram.mName = "Deferred Skinned Bump Shader";
		gDeferredSkinnedBumpProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedBumpProgram.mShaderFiles.clear();
		gDeferredSkinnedBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedBumpProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredSkinnedBumpProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredSkinnedAlphaProgram.mName = "Deferred Skinned Alpha Shader";
		gDeferredSkinnedAlphaProgram.mFeatures.atmosphericHelpers = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasObjectSkinning = true;
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasGamma = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasAtmospherics = true;
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredSkinnedAlphaProgram.mFeatures.hasLighting = false;
		gDeferredSkinnedAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredSkinnedAlphaProgram.mFeatures.disableTextureIndex = true;
		gDeferredSkinnedAlphaProgram.mShaderFiles.clear();
		gDeferredSkinnedAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSkinnedAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaNonIndexedF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSkinnedAlphaProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		
		success = gDeferredSkinnedAlphaProgram.createShader(NULL, NULL);
		
		// Hack to include uniforms for lighting without linking in lighting file
		gDeferredSkinnedAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredSkinnedAlphaProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gDeferredBumpProgram.mName = "Deferred Bump Shader";
		gDeferredBumpProgram.mShaderFiles.clear();
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredBumpProgram.mShaderFiles.push_back(make_pair("deferred/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredBumpProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredBumpProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredTreeProgram.mName = "Deferred Tree Shader";
		gDeferredTreeProgram.mShaderFiles.clear();
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTreeProgram.mShaderFiles.push_back(make_pair("deferred/treeF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTreeProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredTreeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredTreeShadowProgram.mName = "Deferred Tree Shadow Shader";
		gDeferredTreeShadowProgram.mShaderFiles.clear();
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTreeShadowProgram.mShaderFiles.push_back(make_pair("deferred/treeShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTreeShadowProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredTreeShadowProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredImpostorProgram.mName = "Deferred Impostor Shader";
		gDeferredImpostorProgram.mShaderFiles.clear();
		gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredImpostorProgram.mShaderFiles.push_back(make_pair("deferred/impostorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredImpostorProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredImpostorProgram.createShader(NULL, NULL);
	}

	if (success)
	{		
		gDeferredLightProgram.mName = "Deferred Light Shader";
		gDeferredLightProgram.mShaderFiles.clear();
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredLightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredLightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredMultiLightProgram.mName = "Deferred MultiLight Shader";
		gDeferredMultiLightProgram.mShaderFiles.clear();
		gDeferredMultiLightProgram.mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredMultiLightProgram.mShaderFiles.push_back(make_pair("deferred/multiPointLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredMultiLightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredMultiLightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredSpotLightProgram.mName = "Deferred SpotLight Shader";
		gDeferredSpotLightProgram.mShaderFiles.clear();
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/pointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/spotLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredSpotLightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredSpotLightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredMultiSpotLightProgram.mName = "Deferred MultiSpotLight Shader";
		gDeferredMultiSpotLightProgram.mShaderFiles.clear();
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/multiPointLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredMultiSpotLightProgram.mShaderFiles.push_back(make_pair("deferred/multiSpotLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredMultiSpotLightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredMultiSpotLightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		std::string fragment;
		std::string vertex = "deferred/sunLightV.glsl";

		if (gSavedSettings.getBOOL("RenderDeferredSSAO"))
		{
			fragment = "deferred/sunLightSSAOF.glsl";
		}
		else
		{
			fragment = "deferred/sunLightF.glsl";
			if (mVertexShaderLevel[SHADER_DEFERRED] == 1)
			{ //no shadows, no SSAO, no frag coord
				vertex = "deferred/sunLightNoFragCoordV.glsl";
			}
		}

		gDeferredSunProgram.mName = "Deferred Sun Shader";
		gDeferredSunProgram.mShaderFiles.clear();
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(vertex, GL_VERTEX_SHADER_ARB));
		gDeferredSunProgram.mShaderFiles.push_back(make_pair(fragment, GL_FRAGMENT_SHADER_ARB));
		gDeferredSunProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredSunProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredBlurLightProgram.mName = "Deferred Blur Light Shader";
		gDeferredBlurLightProgram.mShaderFiles.clear();
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredBlurLightProgram.mShaderFiles.push_back(make_pair("deferred/blurLightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredBlurLightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredBlurLightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredAlphaProgram.mName = "Deferred Alpha Shader";
		gDeferredAlphaProgram.mFeatures.atmosphericHelpers = true;
		gDeferredAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAlphaProgram.mFeatures.hasGamma = true;
		gDeferredAlphaProgram.mFeatures.hasAtmospherics = true;
		gDeferredAlphaProgram.mFeatures.hasLighting = false;
		gDeferredAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredAlphaProgram.mFeatures.disableTextureIndex = true; //hack to disable auto-setup of texture channels
		if (mVertexShaderLevel[SHADER_DEFERRED] < 1)
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
		gDeferredAlphaProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];

		success = gDeferredAlphaProgram.createShader(NULL, NULL);

		// Hack
		gDeferredAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredAlphaProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gDeferredAvatarEyesProgram.mName = "Deferred Avatar Eyes Shader";
		gDeferredAvatarEyesProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarEyesProgram.mFeatures.hasGamma = true;
		gDeferredAvatarEyesProgram.mFeatures.hasTransport = true;
		gDeferredAvatarEyesProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarEyesProgram.mShaderFiles.clear();
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/avatarEyesV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarEyesProgram.mShaderFiles.push_back(make_pair("deferred/diffuseF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarEyesProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarEyesProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredFullbrightProgram.mName = "Deferred Fullbright Shader";
		gDeferredFullbrightProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredFullbrightProgram.mFeatures.hasGamma = true;
		gDeferredFullbrightProgram.mFeatures.hasTransport = true;
		gDeferredFullbrightProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredFullbrightProgram.mShaderFiles.clear();
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredFullbrightProgram.mShaderFiles.push_back(make_pair("deferred/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredFullbrightProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredFullbrightProgram.createShader(NULL, NULL);
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
		gDeferredEmissiveProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredEmissiveProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		// load water shader
		gDeferredWaterProgram.mName = "Deferred Water Shader";
		gDeferredWaterProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredWaterProgram.mFeatures.hasGamma = true;
		gDeferredWaterProgram.mFeatures.hasTransport = true;
		gDeferredWaterProgram.mShaderFiles.clear();
		gDeferredWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWaterProgram.mShaderFiles.push_back(make_pair("deferred/waterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredWaterProgram.createShader(NULL, &mWaterUniforms);
	}

	if (success)
	{
		gDeferredSoftenProgram.mName = "Deferred Soften Shader";
		gDeferredSoftenProgram.mShaderFiles.clear();
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredSoftenProgram.mShaderFiles.push_back(make_pair("deferred/softenLightF.glsl", GL_FRAGMENT_SHADER_ARB));

		gDeferredSoftenProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];

		if (gSavedSettings.getBOOL("RenderDeferredSSAO"))
		{ //if using SSAO, take screen space light map into account as if shadows are enabled
			gDeferredSoftenProgram.mShaderLevel = llmax(gDeferredSoftenProgram.mShaderLevel, 2);
		}
				
		success = gDeferredSoftenProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredShadowProgram.mName = "Deferred Shadow Shader";
		gDeferredShadowProgram.mShaderFiles.clear();
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredShadowProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredShadowCubeProgram.mName = "Deferred Shadow Cube Shader";
		gDeferredShadowCubeProgram.mShaderFiles.clear();
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowCubeV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowCubeProgram.mShaderFiles.push_back(make_pair("deferred/shadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredShadowCubeProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowCubeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredShadowAlphaMaskProgram.mName = "Deferred Shadow Alpha Mask Shader";
		gDeferredShadowAlphaMaskProgram.mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
		gDeferredShadowAlphaMaskProgram.mShaderFiles.clear();
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredShadowAlphaMaskProgram.mShaderFiles.push_back(make_pair("deferred/shadowAlphaMaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredShadowAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredShadowAlphaMaskProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredAvatarShadowProgram.mName = "Deferred Avatar Shadow Shader";
		gDeferredAvatarShadowProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarShadowProgram.mShaderFiles.clear();
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarShadowProgram.mShaderFiles.push_back(make_pair("deferred/avatarShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarShadowProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarShadowProgram.createShader(NULL, &mAvatarUniforms);
	}

	if (success)
	{
		gDeferredAttachmentShadowProgram.mName = "Deferred Attachment Shadow Shader";
		gDeferredAttachmentShadowProgram.mFeatures.hasObjectSkinning = true;
		gDeferredAttachmentShadowProgram.mShaderFiles.clear();
		gDeferredAttachmentShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentShadowV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAttachmentShadowProgram.mShaderFiles.push_back(make_pair("deferred/attachmentShadowF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAttachmentShadowProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredAttachmentShadowProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gTerrainProgram.mName = "Deferred Terrain Shader";
		gDeferredTerrainProgram.mShaderFiles.clear();
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredTerrainProgram.mShaderFiles.push_back(make_pair("deferred/terrainF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredTerrainProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredTerrainProgram.createShader(NULL, &mTerrainUniforms);
	}

	if (success)
	{
		gDeferredAvatarProgram.mName = "Avatar Shader";
		gDeferredAvatarProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarProgram.mShaderFiles.clear();
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarProgram.mShaderFiles.push_back(make_pair("deferred/avatarF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredAvatarProgram.createShader(NULL, &mAvatarUniforms);
	}

	if (success)
	{
		gDeferredAvatarAlphaProgram.mName = "Avatar Alpha Shader";
		gDeferredAvatarAlphaProgram.mFeatures.atmosphericHelpers = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasSkinning = true;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.calculatesAtmospherics = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasGamma = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasAtmospherics = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = false;
		gDeferredAvatarAlphaProgram.mFeatures.isAlphaLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.disableTextureIndex = true;
		gDeferredAvatarAlphaProgram.mShaderFiles.clear();
		gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/avatarAlphaNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredAvatarAlphaProgram.mShaderFiles.push_back(make_pair("deferred/alphaNonIndexedNoColorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredAvatarAlphaProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];

		success = gDeferredAvatarAlphaProgram.createShader(NULL, &mAvatarUniforms);

		gDeferredAvatarAlphaProgram.mFeatures.calculatesLighting = true;
		gDeferredAvatarAlphaProgram.mFeatures.hasLighting = true;
	}

	if (success)
	{
		gFXAAProgram.mName = "FXAA Shader";
		gFXAAProgram.mShaderFiles.clear();
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredV.glsl", GL_VERTEX_SHADER_ARB));
		gFXAAProgram.mShaderFiles.push_back(make_pair("deferred/fxaaF.glsl", GL_FRAGMENT_SHADER_ARB));
		gFXAAProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gFXAAProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredPostProgram.mName = "Deferred Post Shader";
		gDeferredPostProgram.mShaderFiles.clear();
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredPostProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredCoFProgram.mName = "Deferred CoF Shader";
		gDeferredCoFProgram.mShaderFiles.clear();
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredCoFProgram.mShaderFiles.push_back(make_pair("deferred/cofF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredCoFProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredCoFProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredDoFCombineProgram.mName = "Deferred DoFCombine Shader";
		gDeferredDoFCombineProgram.mShaderFiles.clear();
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredDoFCombineProgram.mShaderFiles.push_back(make_pair("deferred/dofCombineF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredDoFCombineProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredDoFCombineProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredPostNoDoFProgram.mName = "Deferred Post Shader";
		gDeferredPostNoDoFProgram.mShaderFiles.clear();
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoDoFF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredPostNoDoFProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		success = gDeferredPostNoDoFProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDeferredWLSkyProgram.mName = "Deferred Windlight Sky Shader";
		//gWLSkyProgram.mFeatures.hasGamma = true;
		gDeferredWLSkyProgram.mShaderFiles.clear();
		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWLSkyProgram.mShaderFiles.push_back(make_pair("deferred/skyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWLSkyProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		gDeferredWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gDeferredWLSkyProgram.createShader(NULL, &mWLUniforms);
	}

	if (success)
	{
		gDeferredWLCloudProgram.mName = "Deferred Windlight Cloud Program";
		gDeferredWLCloudProgram.mShaderFiles.clear();
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredWLCloudProgram.mShaderFiles.push_back(make_pair("deferred/cloudsF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredWLCloudProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		gDeferredWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gDeferredWLCloudProgram.createShader(NULL, &mWLUniforms);
	}

	if (success)
	{
		gDeferredStarProgram.mName = "Deferred Star Program";
		gDeferredStarProgram.mShaderFiles.clear();
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsV.glsl", GL_VERTEX_SHADER_ARB));
		gDeferredStarProgram.mShaderFiles.push_back(make_pair("deferred/starsF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDeferredStarProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		gDeferredStarProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gDeferredStarProgram.createShader(NULL, &mWLUniforms);
	}

	if (success)
	{
		gNormalMapGenProgram.mName = "Normal Map Generation Program";
		gNormalMapGenProgram.mShaderFiles.clear();
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenV.glsl", GL_VERTEX_SHADER_ARB));
		gNormalMapGenProgram.mShaderFiles.push_back(make_pair("deferred/normgenF.glsl", GL_FRAGMENT_SHADER_ARB));
		gNormalMapGenProgram.mShaderLevel = mVertexShaderLevel[SHADER_DEFERRED];
		gNormalMapGenProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gNormalMapGenProgram.createShader(NULL, NULL);
	}

	return success;
}

BOOL LLViewerShaderMgr::loadShadersObject()
{
	BOOL success = TRUE;
	
	if (mVertexShaderLevel[SHADER_OBJECT] == 0)
	{
		gObjectShinyProgram.unload();
		gObjectFullbrightShinyProgram.unload();
		gObjectFullbrightShinyWaterProgram.unload();
		gObjectShinyWaterProgram.unload();
		gObjectFullbrightNoColorProgram.unload();
		gObjectFullbrightNoColorWaterProgram.unload();
		gObjectSimpleProgram.unload();
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
		gObjectSimpleNonIndexedProgram.mFeatures.disableTextureIndex = true;
		gObjectSimpleNonIndexedProgram.mShaderFiles.clear();
		gObjectSimpleNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectSimpleNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectSimpleNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectSimpleNonIndexedTexGenProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectSimpleNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectSimpleNonIndexedTexGenWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectAlphaMaskNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectAlphaMaskNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectAlphaMaskNoColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectAlphaMaskNoColorWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gTreeProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gTreeWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.clear();
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNonIndexedWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectEmissiveNonIndexedProgram.mShaderFiles.clear();
		gObjectEmissiveNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveNonIndexedProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectEmissiveNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightNoColorProgram.mFeatures.disableTextureIndex = true;
		gObjectFullbrightNoColorProgram.mShaderFiles.clear();
		gObjectFullbrightNoColorProgram.mShaderFiles.push_back(make_pair("objects/fullbrightNoColorV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightNoColorProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightNoColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightNoColorWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectShinyNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectShinyNonIndexedProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectShinyNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		gObjectShinyNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectShinyNonIndexedWaterProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectFullbrightShinyNonIndexedProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightShinyNonIndexedProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		gObjectFullbrightShinyNonIndexedWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightShinyNonIndexedWaterProgram.createShader(NULL, &mShinyUniforms);
	}

	if (success)
	{
		gImpostorProgram.mName = "Impostor Shader";
		gImpostorProgram.mFeatures.disableTextureIndex = true;
		gImpostorProgram.mShaderFiles.clear();
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorV.glsl", GL_VERTEX_SHADER_ARB));
		gImpostorProgram.mShaderFiles.push_back(make_pair("objects/impostorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gImpostorProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gImpostorProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gObjectPreviewProgram.mName = "Simple Shader";
		gObjectPreviewProgram.mFeatures.calculatesLighting = true;
		gObjectPreviewProgram.mFeatures.calculatesAtmospherics = true;
		gObjectPreviewProgram.mFeatures.hasGamma = true;
		gObjectPreviewProgram.mFeatures.hasAtmospherics = true;
		gObjectPreviewProgram.mFeatures.hasLighting = true;
		gObjectPreviewProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectPreviewProgram.mFeatures.disableTextureIndex = true;
		gObjectPreviewProgram.mShaderFiles.clear();
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/previewV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectPreviewProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectPreviewProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectPreviewProgram.createShader(NULL, NULL);
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
		gObjectSimpleProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectSimpleProgram.createShader(NULL, NULL);
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
		gObjectSimpleWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectBumpProgram.mShaderFiles.clear();
		gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectBumpProgram.mShaderFiles.push_back(make_pair("objects/bumpF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectBumpProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectBumpProgram.createShader(NULL, NULL);

		if (success)
		{ //lldrawpoolbump assumes "texture0" has channel 0 and "texture1" has channel 1
			gObjectBumpProgram.bind();
			gObjectBumpProgram.uniform1i("texture0", 0);
			gObjectBumpProgram.uniform1i("texture1", 1);
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
		gObjectSimpleAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectSimpleWaterAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightProgram.mShaderFiles.clear();
		gObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectEmissiveProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectEmissiveProgram.mShaderFiles.clear();
		gObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/emissiveV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectEmissiveProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectEmissiveWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightAlphaMaskProgram.mFeatures.mIndexedTextureChannels = 0;
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.clear();
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightV.glsl", GL_VERTEX_SHADER_ARB));
		gObjectFullbrightAlphaMaskProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
		gObjectFullbrightAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectFullbrightWaterAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
		gObjectShinyProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectShinyProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectShinyWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		gObjectShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectShinyWaterProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectFullbrightShinyProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		success = gObjectFullbrightShinyProgram.createShader(NULL, &mShinyUniforms);
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
		gObjectFullbrightShinyWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
		gObjectFullbrightShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
		success = gObjectFullbrightShinyWaterProgram.createShader(NULL, &mShinyUniforms);
	}

	if (mVertexShaderLevel[SHADER_AVATAR] > 0)
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
			gSkinnedObjectSimpleProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectSimpleProgram.mShaderFiles.clear();
			gSkinnedObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectSimpleProgram.mShaderFiles.push_back(make_pair("objects/simpleF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectSimpleProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectFullbrightProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectEmissiveProgram.mShaderFiles.clear();
			gSkinnedObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/emissiveSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectEmissiveProgram.mShaderFiles.push_back(make_pair("objects/fullbrightF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectEmissiveProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectEmissiveWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectFullbrightShinyProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinySkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightShinyProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightShinyProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightShinyProgram.createShader(NULL, &mShinyUniforms);
		}

		if (success)
		{
			gSkinnedObjectShinySimpleProgram.mName = "Skinned Shiny Simple Shader";
			gSkinnedObjectShinySimpleProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasGamma = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.isShiny = true;
			gSkinnedObjectShinySimpleProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectShinySimpleProgram.mShaderFiles.clear();
			gSkinnedObjectShinySimpleProgram.mShaderFiles.push_back(make_pair("objects/shinySimpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectShinySimpleProgram.mShaderFiles.push_back(make_pair("objects/shinyF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectShinySimpleProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectShinySimpleProgram.createShader(NULL, &mShinyUniforms);
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
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.clear();
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectSimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/simpleWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectSimpleWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectFullbrightWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectFullbrightWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
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
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.clear();
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinySkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderFiles.push_back(make_pair("objects/fullbrightShinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectFullbrightShinyWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectFullbrightShinyWaterProgram.createShader(NULL, &mShinyUniforms);
		}

		if (success)
		{
			gSkinnedObjectShinySimpleWaterProgram.mName = "Skinned Shiny Simple Water Shader";
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.calculatesLighting = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.calculatesAtmospherics = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasGamma = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasAtmospherics = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasObjectSkinning = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.isShiny = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.hasWaterFog = true;
			gSkinnedObjectShinySimpleWaterProgram.mFeatures.disableTextureIndex = true;
			gSkinnedObjectShinySimpleWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.clear();
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/shinySimpleSkinnedV.glsl", GL_VERTEX_SHADER_ARB));
			gSkinnedObjectShinySimpleWaterProgram.mShaderFiles.push_back(make_pair("objects/shinyWaterF.glsl", GL_FRAGMENT_SHADER_ARB));
			gSkinnedObjectShinySimpleWaterProgram.mShaderLevel = mVertexShaderLevel[SHADER_OBJECT];
			success = gSkinnedObjectShinySimpleWaterProgram.createShader(NULL, &mShinyUniforms);
		}
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_OBJECT] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersAvatar()
{
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_AVATAR] == 0)
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
		gAvatarProgram.mShaderLevel = mVertexShaderLevel[SHADER_AVATAR];
		success = gAvatarProgram.createShader(NULL, &mAvatarUniforms);
			
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
			gAvatarWaterProgram.mShaderLevel = llmin(mVertexShaderLevel[SHADER_AVATAR], 1);	
			gAvatarWaterProgram.mShaderGroup = LLGLSLShader::SG_WATER;				
			success = gAvatarWaterProgram.createShader(NULL, &mAvatarUniforms);
		}

		/// Keep track of avatar levels
		if (gAvatarProgram.mShaderLevel != mVertexShaderLevel[SHADER_AVATAR])
		{
			mMaxAvatarShaderLevel = mVertexShaderLevel[SHADER_AVATAR] = gAvatarProgram.mShaderLevel;
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
		gAvatarPickProgram.mShaderLevel = mVertexShaderLevel[SHADER_AVATAR];
		success = gAvatarPickProgram.createShader(NULL, &mAvatarUniforms);
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
		gAvatarEyeballProgram.mShaderLevel = mVertexShaderLevel[SHADER_AVATAR];
		success = gAvatarEyeballProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_AVATAR] = 0;
		mMaxAvatarShaderLevel = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersInterface()
{
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_INTERFACE] == 0)
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
		gHighlightProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];		
		success = gHighlightProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gUIProgram.mName = "UI Shader";
		gUIProgram.mShaderFiles.clear();
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiV.glsl", GL_VERTEX_SHADER_ARB));
		gUIProgram.mShaderFiles.push_back(make_pair("interface/uiF.glsl", GL_FRAGMENT_SHADER_ARB));
		gUIProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gUIProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingProgram.mName = "Pathfinding Shader";
		gPathfindingProgram.mShaderFiles.clear();
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingV.glsl", GL_VERTEX_SHADER_ARB));
		gPathfindingProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER_ARB));
		gPathfindingProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gPathfindingProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gPathfindingNoNormalsProgram.mName = "PathfindingNoNormals Shader";
		gPathfindingNoNormalsProgram.mShaderFiles.clear();
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingNoNormalV.glsl", GL_VERTEX_SHADER_ARB));
		gPathfindingNoNormalsProgram.mShaderFiles.push_back(make_pair("interface/pathfindingF.glsl", GL_FRAGMENT_SHADER_ARB));
		gPathfindingNoNormalsProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gPathfindingNoNormalsProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gCustomAlphaProgram.mName = "Custom Alpha Shader";
		gCustomAlphaProgram.mShaderFiles.clear();
		gCustomAlphaProgram.mShaderFiles.push_back(make_pair("interface/customalphaV.glsl", GL_VERTEX_SHADER_ARB));
		gCustomAlphaProgram.mShaderFiles.push_back(make_pair("interface/customalphaF.glsl", GL_FRAGMENT_SHADER_ARB));
		gCustomAlphaProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gCustomAlphaProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gSplatTextureRectProgram.mName = "Splat Texture Rect Shader";
		gSplatTextureRectProgram.mShaderFiles.clear();
		gSplatTextureRectProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectV.glsl", GL_VERTEX_SHADER_ARB));
		gSplatTextureRectProgram.mShaderFiles.push_back(make_pair("interface/splattexturerectF.glsl", GL_FRAGMENT_SHADER_ARB));
		gSplatTextureRectProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gSplatTextureRectProgram.createShader(NULL, NULL);
		if (success)
		{
			gSplatTextureRectProgram.bind();
			gSplatTextureRectProgram.uniform1i("screenMap", 0);
			gSplatTextureRectProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineProgram.mName = "Glow Combine Shader";
		gGlowCombineProgram.mShaderFiles.clear();
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowCombineProgram.mShaderFiles.push_back(make_pair("interface/glowcombineF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowCombineProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gGlowCombineProgram.createShader(NULL, NULL);
		if (success)
		{
			gGlowCombineProgram.bind();
			gGlowCombineProgram.uniform1i("glowMap", 0);
			gGlowCombineProgram.uniform1i("screenMap", 1);
			gGlowCombineProgram.unbind();
		}
	}

	if (success)
	{
		gGlowCombineFXAAProgram.mName = "Glow CombineFXAA Shader";
		gGlowCombineFXAAProgram.mShaderFiles.clear();
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAV.glsl", GL_VERTEX_SHADER_ARB));
		gGlowCombineFXAAProgram.mShaderFiles.push_back(make_pair("interface/glowcombineFXAAF.glsl", GL_FRAGMENT_SHADER_ARB));
		gGlowCombineFXAAProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gGlowCombineFXAAProgram.createShader(NULL, NULL);
		if (success)
		{
			gGlowCombineFXAAProgram.bind();
			gGlowCombineFXAAProgram.uniform1i("glowMap", 0);
			gGlowCombineFXAAProgram.uniform1i("screenMap", 1);
			gGlowCombineFXAAProgram.unbind();
		}
	}


	if (success)
	{
		gTwoTextureAddProgram.mName = "Two Texture Add Shader";
		gTwoTextureAddProgram.mShaderFiles.clear();
		gTwoTextureAddProgram.mShaderFiles.push_back(make_pair("interface/twotextureaddV.glsl", GL_VERTEX_SHADER_ARB));
		gTwoTextureAddProgram.mShaderFiles.push_back(make_pair("interface/twotextureaddF.glsl", GL_FRAGMENT_SHADER_ARB));
		gTwoTextureAddProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gTwoTextureAddProgram.createShader(NULL, NULL);
		if (success)
		{
			gTwoTextureAddProgram.bind();
			gTwoTextureAddProgram.uniform1i("tex0", 0);
			gTwoTextureAddProgram.uniform1i("tex1", 1);
		}
	}

	if (success)
	{
		gOneTextureNoColorProgram.mName = "One Texture No Color Shader";
		gOneTextureNoColorProgram.mShaderFiles.clear();
		gOneTextureNoColorProgram.mShaderFiles.push_back(make_pair("interface/onetexturenocolorV.glsl", GL_VERTEX_SHADER_ARB));
		gOneTextureNoColorProgram.mShaderFiles.push_back(make_pair("interface/onetexturenocolorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOneTextureNoColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gOneTextureNoColorProgram.createShader(NULL, NULL);
		if (success)
		{
			gOneTextureNoColorProgram.bind();
			gOneTextureNoColorProgram.uniform1i("tex0", 0);
		}
	}

	if (success)
	{
		gSolidColorProgram.mName = "Solid Color Shader";
		gSolidColorProgram.mShaderFiles.clear();
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorV.glsl", GL_VERTEX_SHADER_ARB));
		gSolidColorProgram.mShaderFiles.push_back(make_pair("interface/solidcolorF.glsl", GL_FRAGMENT_SHADER_ARB));
		gSolidColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gSolidColorProgram.createShader(NULL, NULL);
		if (success)
		{
			gSolidColorProgram.bind();
			gSolidColorProgram.uniform1i("tex0", 0);
			gSolidColorProgram.unbind();
		}
	}

	if (success)
	{
		gOcclusionProgram.mName = "Occlusion Shader";
		gOcclusionProgram.mShaderFiles.clear();
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionV.glsl", GL_VERTEX_SHADER_ARB));
		gOcclusionProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOcclusionProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gOcclusionProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gOcclusionCubeProgram.mName = "Occlusion Cube Shader";
		gOcclusionCubeProgram.mShaderFiles.clear();
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionCubeV.glsl", GL_VERTEX_SHADER_ARB));
		gOcclusionCubeProgram.mShaderFiles.push_back(make_pair("interface/occlusionF.glsl", GL_FRAGMENT_SHADER_ARB));
		gOcclusionCubeProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gOcclusionCubeProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gDebugProgram.mName = "Debug Shader";
		gDebugProgram.mShaderFiles.clear();
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugV.glsl", GL_VERTEX_SHADER_ARB));
		gDebugProgram.mShaderFiles.push_back(make_pair("interface/debugF.glsl", GL_FRAGMENT_SHADER_ARB));
		gDebugProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gDebugProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gClipProgram.mName = "Clip Shader";
		gClipProgram.mShaderFiles.clear();
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipV.glsl", GL_VERTEX_SHADER_ARB));
		gClipProgram.mShaderFiles.push_back(make_pair("interface/clipF.glsl", GL_FRAGMENT_SHADER_ARB));
		gClipProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gClipProgram.createShader(NULL, NULL);
	}

	if (success)
	{
		gAlphaMaskProgram.mName = "Alpha Mask Shader";
		gAlphaMaskProgram.mShaderFiles.clear();
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskV.glsl", GL_VERTEX_SHADER_ARB));
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gAlphaMaskProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_INTERFACE] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLViewerShaderMgr::loadShadersWindLight()
{	
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_WINDLIGHT] < 2)
	{
		gWLSkyProgram.unload();
		gWLCloudProgram.unload();
		return TRUE;
	}

	if (success)
	{
		gWLSkyProgram.mName = "Windlight Sky Shader";
		//gWLSkyProgram.mFeatures.hasGamma = true;
		gWLSkyProgram.mShaderFiles.clear();
		gWLSkyProgram.mShaderFiles.push_back(make_pair("windlight/skyV.glsl", GL_VERTEX_SHADER_ARB));
		gWLSkyProgram.mShaderFiles.push_back(make_pair("windlight/skyF.glsl", GL_FRAGMENT_SHADER_ARB));
		gWLSkyProgram.mShaderLevel = mVertexShaderLevel[SHADER_WINDLIGHT];
		gWLSkyProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gWLSkyProgram.createShader(NULL, &mWLUniforms);
	}

	if (success)
	{
		gWLCloudProgram.mName = "Windlight Cloud Program";
		//gWLCloudProgram.mFeatures.hasGamma = true;
		gWLCloudProgram.mShaderFiles.clear();
		gWLCloudProgram.mShaderFiles.push_back(make_pair("windlight/cloudsV.glsl", GL_VERTEX_SHADER_ARB));
		gWLCloudProgram.mShaderFiles.push_back(make_pair("windlight/cloudsF.glsl", GL_FRAGMENT_SHADER_ARB));
		gWLCloudProgram.mShaderLevel = mVertexShaderLevel[SHADER_WINDLIGHT];
		gWLCloudProgram.mShaderGroup = LLGLSLShader::SG_SKY;
		success = gWLCloudProgram.createShader(NULL, &mWLUniforms);
	}

	return success;
}

BOOL LLViewerShaderMgr::loadTransformShaders()
{
	BOOL success = TRUE;
	
	if (mVertexShaderLevel[SHADER_TRANSFORM] < 1)
	{
		gTransformPositionProgram.unload();
		gTransformTexCoordProgram.unload();
		gTransformNormalProgram.unload();
		gTransformColorProgram.unload();
		gTransformBinormalProgram.unload();
		return TRUE;
	}

	if (success)
	{
		gTransformPositionProgram.mName = "Position Transform Shader";
		gTransformPositionProgram.mShaderFiles.clear();
		gTransformPositionProgram.mShaderFiles.push_back(make_pair("transform/positionV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformPositionProgram.mShaderLevel = mVertexShaderLevel[SHADER_TRANSFORM];

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
		gTransformTexCoordProgram.mShaderLevel = mVertexShaderLevel[SHADER_TRANSFORM];

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
		gTransformNormalProgram.mShaderLevel = mVertexShaderLevel[SHADER_TRANSFORM];

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
		gTransformColorProgram.mShaderLevel = mVertexShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"color_out",
		};
	
		success = gTransformColorProgram.createShader(NULL, NULL, 1, varyings);
	}

	if (success)
	{
		gTransformBinormalProgram.mName = "Binormal Transform Shader";
		gTransformBinormalProgram.mShaderFiles.clear();
		gTransformBinormalProgram.mShaderFiles.push_back(make_pair("transform/binormalV.glsl", GL_VERTEX_SHADER_ARB));
		gTransformBinormalProgram.mShaderLevel = mVertexShaderLevel[SHADER_TRANSFORM];

		const char* varyings[] = {
			"binormal_out",
		};
	
		success = gTransformBinormalProgram.createShader(NULL, NULL, 1, varyings);
	}

	
	return success;
}

std::string LLViewerShaderMgr::getShaderDirPrefix(void)
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/class");
}

void LLViewerShaderMgr::updateShaderUniforms(LLGLSLShader * shader)
{
	LLWLParamManager::getInstance()->updateShaderUniforms(shader);
	LLWaterParamManager::getInstance()->updateShaderUniforms(shader);
}

LLViewerShaderMgr::shader_iter LLViewerShaderMgr::beginShaders() const
{
	return mShaderList.begin();
}

LLViewerShaderMgr::shader_iter LLViewerShaderMgr::endShaders() const
{
	return mShaderList.end();
}
