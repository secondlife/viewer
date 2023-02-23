/** 
 * @file llviewershadermgr.h
 * @brief Viewer Shader Manager
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

#ifndef LL_VIEWER_SHADER_MGR_H
#define LL_VIEWER_SHADER_MGR_H

#include "llshadermgr.h"
#include "llmaterial.h"

#define LL_DEFERRED_MULTI_LIGHT_COUNT 16

class LLViewerShaderMgr: public LLShaderMgr
{
public:
	static BOOL sInitialized;
	static bool sSkipReload;

	LLViewerShaderMgr();
	/* virtual */ ~LLViewerShaderMgr();

	// singleton pattern implementation
	static LLViewerShaderMgr * instance();
	static void releaseInstance();

	void initAttribsAndUniforms(void);
	void setShaders();
	void unloadShaders();
    S32  getShaderLevel(S32 type);

    // loadBasicShaders in case of a failure returns
    // name of a file error happened at, otherwise
    // returns an empty string
    std::string loadBasicShaders();
	BOOL loadShadersEffects();
	BOOL loadShadersDeferred();
	BOOL loadShadersObject();
	BOOL loadShadersAvatar();
	BOOL loadShadersWater();
	BOOL loadShadersInterface();

	std::vector<S32> mShaderLevel;
	S32	mMaxAvatarShaderLevel;

	enum EShaderClass
	{
		SHADER_LIGHTING,
		SHADER_OBJECT,
		SHADER_AVATAR,
		SHADER_ENVIRONMENT,
		SHADER_INTERFACE,
		SHADER_EFFECT,
		SHADER_WINDLIGHT,
		SHADER_WATER,
		SHADER_DEFERRED,
		SHADER_COUNT
	};

	// simple model of forward iterator
	// http://www.sgi.com/tech/stl/ForwardIterator.html
	class shader_iter
	{
	private:
		friend bool operator == (shader_iter const & a, shader_iter const & b);
		friend bool operator != (shader_iter const & a, shader_iter const & b);

		typedef std::vector<LLGLSLShader *>::const_iterator base_iter_t;
	public:
		shader_iter()
		{
		}

		shader_iter(base_iter_t iter) : mIter(iter)
		{
		}

		LLGLSLShader & operator * () const
		{
			return **mIter;
		}

		LLGLSLShader * operator -> () const
		{
			return *mIter;
		}

		shader_iter & operator++ ()
		{
			++mIter;
			return *this;
		}

		shader_iter operator++ (int)
		{
			return mIter++;
		}

	private:
		base_iter_t mIter;
	};

	shader_iter beginShaders() const;
	shader_iter endShaders() const;

	/* virtual */ std::string getShaderDirPrefix(void);

	/* virtual */ void updateShaderUniforms(LLGLSLShader * shader);

private:
	// the list of shaders we need to propagate parameters to.
	std::vector<LLGLSLShader *> mShaderList;

}; //LLViewerShaderMgr

inline bool operator == (LLViewerShaderMgr::shader_iter const & a, LLViewerShaderMgr::shader_iter const & b)
{
	return a.mIter == b.mIter;
}

inline bool operator != (LLViewerShaderMgr::shader_iter const & a, LLViewerShaderMgr::shader_iter const & b)
{
	return a.mIter != b.mIter;
}

extern LLVector4			gShinyOrigin;

//utility shaders
extern LLGLSLShader			gOcclusionProgram;
extern LLGLSLShader			gOcclusionCubeProgram;
extern LLGLSLShader			gGlowCombineProgram;
extern LLGLSLShader			gReflectionMipProgram;
extern LLGLSLShader         gGaussianProgram;
extern LLGLSLShader         gRadianceGenProgram;
extern LLGLSLShader         gIrradianceGenProgram;
extern LLGLSLShader			gGlowCombineFXAAProgram;
extern LLGLSLShader			gDebugProgram;
extern LLGLSLShader			gClipProgram;
extern LLGLSLShader			gBenchmarkProgram;
extern LLGLSLShader         gReflectionProbeDisplayProgram;
extern LLGLSLShader         gCopyProgram;
extern LLGLSLShader         gCopyDepthProgram;

//output tex0[tc0] - tex1[tc1]
extern LLGLSLShader			gTwoTextureCompareProgram;
//discard some fragments based on user-set color tolerance
extern LLGLSLShader			gOneTextureFilterProgram;
						

//object shaders
extern LLGLSLShader		gObjectPreviewProgram;
extern LLGLSLShader        gPhysicsPreviewProgram;
extern LLGLSLShader        gSkinnedObjectFullbrightAlphaMaskProgram;
extern LLGLSLShader		gObjectBumpProgram;
extern LLGLSLShader        gSkinnedObjectBumpProgram;
extern LLGLSLShader		gObjectAlphaMaskNoColorProgram;
extern LLGLSLShader		gObjectAlphaMaskNoColorWaterProgram;

//environment shaders
extern LLGLSLShader			gWaterProgram;
extern LLGLSLShader			gWaterEdgeProgram;
extern LLGLSLShader			gUnderWaterProgram;
extern LLGLSLShader			gGlowProgram;
extern LLGLSLShader			gGlowExtractProgram;

//interface shaders
extern LLGLSLShader			gHighlightProgram;
extern LLGLSLShader			gHighlightNormalProgram;
extern LLGLSLShader			gHighlightSpecularProgram;

extern LLGLSLShader			gDeferredHighlightProgram;

extern LLGLSLShader			gPathfindingProgram;
extern LLGLSLShader			gPathfindingNoNormalsProgram;

// avatar shader handles
extern LLGLSLShader			gAvatarProgram;
extern LLGLSLShader			gAvatarWaterProgram;
extern LLGLSLShader			gAvatarEyeballProgram;
extern LLGLSLShader			gImpostorProgram;

// Post Process Shaders
extern LLGLSLShader         gPostScreenSpaceReflectionProgram;

// Deferred rendering shaders
extern LLGLSLShader			gDeferredImpostorProgram;
extern LLGLSLShader			gDeferredDiffuseProgram;
extern LLGLSLShader			gDeferredDiffuseAlphaMaskProgram;
extern LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskProgram;
extern LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
extern LLGLSLShader			gDeferredNonIndexedDiffuseProgram;
extern LLGLSLShader			gDeferredBumpProgram;
extern LLGLSLShader			gDeferredTerrainProgram;
extern LLGLSLShader			gDeferredTerrainWaterProgram;
extern LLGLSLShader			gDeferredTreeProgram;
extern LLGLSLShader			gDeferredTreeShadowProgram;
extern LLGLSLShader			gDeferredLightProgram;
extern LLGLSLShader			gDeferredMultiLightProgram[LL_DEFERRED_MULTI_LIGHT_COUNT];
extern LLGLSLShader			gDeferredSpotLightProgram;
extern LLGLSLShader			gDeferredMultiSpotLightProgram;
extern LLGLSLShader			gDeferredSunProgram;
extern LLGLSLShader			gDeferredBlurLightProgram;
extern LLGLSLShader			gDeferredAvatarProgram;
extern LLGLSLShader			gDeferredSoftenProgram;
extern LLGLSLShader			gDeferredSoftenWaterProgram;
extern LLGLSLShader			gDeferredShadowProgram;
extern LLGLSLShader			gDeferredShadowCubeProgram;
extern LLGLSLShader			gDeferredShadowAlphaMaskProgram;
extern LLGLSLShader         gDeferredShadowGLTFAlphaMaskProgram;
extern LLGLSLShader			gDeferredShadowFullbrightAlphaMaskProgram;
extern LLGLSLShader			gDeferredPostProgram;
extern LLGLSLShader			gDeferredCoFProgram;
extern LLGLSLShader			gDeferredDoFCombineProgram;
extern LLGLSLShader			gFXAAProgram;
extern LLGLSLShader			gDeferredPostNoDoFProgram;
extern LLGLSLShader			gDeferredPostGammaCorrectProgram;
extern LLGLSLShader			gDeferredAvatarShadowProgram;
extern LLGLSLShader			gDeferredAvatarAlphaShadowProgram;
extern LLGLSLShader			gDeferredAvatarAlphaMaskShadowProgram;
extern LLGLSLShader			gDeferredAlphaProgram;
extern LLGLSLShader			gHUDAlphaProgram;
extern LLGLSLShader			gDeferredAlphaImpostorProgram;
extern LLGLSLShader			gDeferredFullbrightProgram;
extern LLGLSLShader			gHUDFullbrightProgram;
extern LLGLSLShader			gDeferredFullbrightAlphaMaskProgram;
extern LLGLSLShader			gHUDFullbrightAlphaMaskProgram;
extern LLGLSLShader			gDeferredFullbrightAlphaMaskAlphaProgram;
extern LLGLSLShader			gHUDFullbrightAlphaMaskAlphaProgram;
extern LLGLSLShader			gDeferredAlphaWaterProgram;
extern LLGLSLShader			gDeferredFullbrightWaterProgram;
extern LLGLSLShader			gDeferredFullbrightWaterAlphaProgram;
extern LLGLSLShader			gDeferredFullbrightAlphaMaskWaterProgram;
extern LLGLSLShader			gDeferredEmissiveProgram;
extern LLGLSLShader			gDeferredAvatarEyesProgram;
extern LLGLSLShader			gDeferredAvatarAlphaProgram;
extern LLGLSLShader			gDeferredWLSkyProgram;
extern LLGLSLShader			gDeferredWLCloudProgram;
extern LLGLSLShader			gDeferredWLSunProgram;
extern LLGLSLShader			gDeferredWLMoonProgram;
extern LLGLSLShader			gDeferredStarProgram;
extern LLGLSLShader			gDeferredFullbrightShinyProgram;
extern LLGLSLShader         gHUDFullbrightShinyProgram;
extern LLGLSLShader			gNormalMapGenProgram;
extern LLGLSLShader         gDeferredGenBrdfLutProgram;

// Deferred materials shaders
extern LLGLSLShader			gDeferredMaterialProgram[LLMaterial::SHADER_COUNT*2];
extern LLGLSLShader			gDeferredMaterialWaterProgram[LLMaterial::SHADER_COUNT*2];

extern LLGLSLShader         gHUDPBROpaqueProgram;
extern LLGLSLShader         gPBRGlowProgram;
extern LLGLSLShader         gDeferredPBROpaqueProgram;
extern LLGLSLShader         gDeferredPBRAlphaProgram;
extern LLGLSLShader         gHUDPBRAlphaProgram;
#endif
