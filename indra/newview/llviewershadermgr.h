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

class LLViewerShaderMgr: public LLShaderMgr
{
public:
	static BOOL sInitialized;

	LLViewerShaderMgr();
	/* virtual */ ~LLViewerShaderMgr();

	// singleton pattern implementation
	static LLViewerShaderMgr * instance();

	void initAttribsAndUniforms(void);
	void setShaders();
	void unloadShaders();
	S32 getVertexShaderLevel(S32 type);
	BOOL loadBasicShaders();
	BOOL loadShadersEffects();
	BOOL loadShadersDeferred();
	BOOL loadShadersObject();
	BOOL loadShadersAvatar();
	BOOL loadShadersEnvironment();
	BOOL loadShadersWater();
	BOOL loadShadersInterface();
	BOOL loadShadersWindLight();

	std::vector<S32> mVertexShaderLevel;
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

	typedef enum 
	{
		MATERIAL_COLOR = 0,
		SPECULAR_COLOR,
		BINORMAL,
		OBJECT_WEIGHT,
		END_RESERVED_ATTRIBS
	} eGLSLReservedAttribs;
	
	typedef enum
	{
		DIFFUSE_MAP = 0,
		SPECULAR_MAP,
		BUMP_MAP,
		ENVIRONMENT_MAP,
		CLOUD_NOISE_MAP,
		FULLBRIGHT,
		LIGHTNORM,
		SUNLIGHT_COLOR,
		AMBIENT,
		BLUE_HORIZON,
		BLUE_DENSITY,
		HAZE_HORIZON,
		HAZE_DENSITY,
		CLOUD_SHADOW,
		DENSITY_MULTIPLIER,
		DISTANCE_MULTIPLIER,
		MAX_Y,
		GLOW,
		CLOUD_COLOR,
		CLOUD_POS_DENSITY1,
		CLOUD_POS_DENSITY2,
		CLOUD_SCALE,
		GAMMA,
		SCENE_LIGHT_STRENGTH,
		DEFERRED_DEPTH,
		DEFERRED_SHADOW0,
		DEFERRED_SHADOW1,
		DEFERRED_SHADOW2,
		DEFERRED_SHADOW3,
		DEFERRED_SHADOW4,
		DEFERRED_SHADOW5,
		DEFERRED_NORMAL,
		DEFERRED_POSITION,
		DEFERRED_DIFFUSE,
		DEFERRED_SPECULAR,
		DEFERRED_NOISE,
		DEFERRED_LIGHTFUNC,
		DEFERRED_LIGHT,
		DEFERRED_LUMINANCE,
		DEFERRED_GI_LIGHT,
		DEFERRED_GI_MIP,
		DEFERRED_EDGE,
		DEFERRED_BLOOM,
		DEFERRED_SUN_LIGHT,
		DEFERRED_LOCAL_LIGHT,
		DEFERRED_PROJECTION,
		DEFERRED_GI_DIFFUSE,
		DEFERRED_GI_SPECULAR,
		DEFERRED_GI_NORMAL,
		DEFERRED_GI_MIN_POS,
		DEFERRED_GI_MAX_POS,
		DEFERRED_GI_DEPTH,
		DEFERRED_GI_LAST_DIFFUSE,
		DEFERRED_GI_LAST_NORMAL,
		DEFERRED_GI_LAST_MIN_POS,
		DEFERRED_GI_LAST_MAX_POS,
		END_RESERVED_UNIFORMS
	} eGLSLReservedUniforms;

	typedef enum
	{
		SHINY_ORIGIN = END_RESERVED_UNIFORMS
	} eShinyUniforms;

	typedef enum
	{
		WATER_SCREENTEX = END_RESERVED_UNIFORMS,
		WATER_SCREENDEPTH,
		WATER_REFTEX,
		WATER_EYEVEC,
		WATER_TIME,
		WATER_WAVE_DIR1,
		WATER_WAVE_DIR2,
		WATER_LIGHT_DIR,
		WATER_SPECULAR,
		WATER_SPECULAR_EXP,
		WATER_FOGCOLOR,
		WATER_FOGDENSITY,
		WATER_REFSCALE,
		WATER_WATERHEIGHT,
	} eWaterUniforms;

	typedef enum
	{
		WL_CAMPOSLOCAL = END_RESERVED_UNIFORMS,
		WL_WATERHEIGHT
	} eWLUniforms;

	typedef enum
	{
		TERRAIN_DETAIL0 = END_RESERVED_UNIFORMS,
		TERRAIN_DETAIL1,
		TERRAIN_DETAIL2,
		TERRAIN_DETAIL3,
		TERRAIN_ALPHARAMP
	} eTerrainUniforms;

	typedef enum
	{
		GLOW_DELTA = END_RESERVED_UNIFORMS
	} eGlowUniforms;

	typedef enum
	{
		AVATAR_WEIGHT = END_RESERVED_ATTRIBS,
		AVATAR_CLOTHING,
		AVATAR_WIND,
		AVATAR_SINWAVE,
		AVATAR_GRAVITY
	} eAvatarAttribs;

	typedef enum
	{
		AVATAR_MATRIX = END_RESERVED_UNIFORMS
	} eAvatarUniforms;

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

	shader_iter beginShaders() const
	{
		return mShaderList.begin();
	}

	shader_iter endShaders() const
	{
		return mShaderList.end();
	}


	/* virtual */ std::string getShaderDirPrefix(void); // Virtual

	/* virtual */ void updateShaderUniforms(LLGLSLShader * shader); // Virtual

private:
	
	std::vector<std::string> mShinyUniforms;

	//water parameters
	std::vector<std::string> mWaterUniforms;

	std::vector<std::string> mWLUniforms;

	//terrain parameters
	std::vector<std::string> mTerrainUniforms;

	//glow parameters
	std::vector<std::string> mGlowUniforms;

	std::vector<std::string> mGlowExtractUniforms;

	//avatar shader parameter tables
	std::vector<std::string> mAvatarAttribs;

	std::vector<std::string> mAvatarUniforms;

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

//object shaders
extern LLGLSLShader			gObjectSimpleProgram;
extern LLGLSLShader			gObjectSimpleWaterProgram;
extern LLGLSLShader			gObjectFullbrightProgram;
extern LLGLSLShader			gObjectFullbrightWaterProgram;

extern LLGLSLShader			gObjectSimpleLODProgram;
extern LLGLSLShader			gObjectFullbrightLODProgram;

extern LLGLSLShader			gObjectFullbrightShinyProgram;
extern LLGLSLShader			gObjectFullbrightShinyWaterProgram;
extern LLGLSLShader			gObjectShinyProgram;
extern LLGLSLShader			gObjectShinyWaterProgram;

extern LLGLSLShader			gSkinnedObjectSimpleProgram;
extern LLGLSLShader			gSkinnedObjectFullbrightProgram;
extern LLGLSLShader			gSkinnedObjectFullbrightShinyProgram;
extern LLGLSLShader			gSkinnedObjectShinySimpleProgram;

extern LLGLSLShader			gSkinnedObjectSimpleWaterProgram;
extern LLGLSLShader			gSkinnedObjectFullbrightWaterProgram;
extern LLGLSLShader			gSkinnedObjectFullbrightShinyWaterProgram;
extern LLGLSLShader			gSkinnedObjectShinySimpleWaterProgram;

//environment shaders
extern LLGLSLShader			gTerrainProgram;
extern LLGLSLShader			gTerrainWaterProgram;
extern LLGLSLShader			gWaterProgram;
extern LLGLSLShader			gUnderWaterProgram;
extern LLGLSLShader			gGlowProgram;
extern LLGLSLShader			gGlowExtractProgram;

//interface shaders
extern LLGLSLShader			gHighlightProgram;

// avatar shader handles
extern LLGLSLShader			gAvatarProgram;
extern LLGLSLShader			gAvatarWaterProgram;
extern LLGLSLShader			gAvatarEyeballProgram;
extern LLGLSLShader			gAvatarPickProgram;

// WindLight shader handles
extern LLGLSLShader			gWLSkyProgram;
extern LLGLSLShader			gWLCloudProgram;

// Post Process Shaders
extern LLGLSLShader			gPostColorFilterProgram;
extern LLGLSLShader			gPostNightVisionProgram;

// Deferred rendering shaders
extern LLGLSLShader			gDeferredImpostorProgram;
extern LLGLSLShader			gDeferredEdgeProgram;
extern LLGLSLShader			gDeferredWaterProgram;
extern LLGLSLShader			gDeferredDiffuseProgram;
extern LLGLSLShader			gDeferredSkinnedDiffuseProgram;
extern LLGLSLShader			gDeferredSkinnedBumpProgram;
extern LLGLSLShader			gDeferredSkinnedAlphaProgram;
extern LLGLSLShader			gDeferredBumpProgram;
extern LLGLSLShader			gDeferredTerrainProgram;
extern LLGLSLShader			gDeferredTreeProgram;
extern LLGLSLShader			gDeferredLightProgram;
extern LLGLSLShader			gDeferredMultiLightProgram;
extern LLGLSLShader			gDeferredSpotLightProgram;
extern LLGLSLShader			gDeferredMultiSpotLightProgram;
extern LLGLSLShader			gDeferredSunProgram;
extern LLGLSLShader			gDeferredGIProgram;
extern LLGLSLShader			gDeferredGIFinalProgram;
extern LLGLSLShader			gDeferredBlurLightProgram;
extern LLGLSLShader			gDeferredAvatarProgram;
extern LLGLSLShader			gDeferredSoftenProgram;
extern LLGLSLShader			gDeferredShadowProgram;
extern LLGLSLShader			gDeferredPostGIProgram;
extern LLGLSLShader			gDeferredPostProgram;
extern LLGLSLShader			gDeferredPostNoDoFProgram;
extern LLGLSLShader			gDeferredAvatarShadowProgram;
extern LLGLSLShader			gDeferredAttachmentShadowProgram;
extern LLGLSLShader			gDeferredAlphaProgram;
extern LLGLSLShader			gDeferredFullbrightProgram;
extern LLGLSLShader			gDeferredAvatarAlphaProgram;
extern LLGLSLShader			gDeferredWLSkyProgram;
extern LLGLSLShader			gDeferredWLCloudProgram;

extern LLGLSLShader			gLuminanceGatherProgram;

//current avatar shader parameter pointer
extern GLint				gAvatarMatrixParam;


#endif
