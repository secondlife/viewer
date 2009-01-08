/** 
 * @file llviewershadermgr.h
 * @brief Viewer Shader Manager
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_VIEWER_SHADER_MGR_H
#define LL_VIEWER_SHADER_MGR_H

#include "llshadermgr.h"

class LLViewerShaderMgr: public LLShaderMgr
{
public:
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
extern LLGLSLShader			gObjectShinyProgram;
extern LLGLSLShader			gObjectShinyWaterProgram;

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
extern LLGLSLShader			gDeferredDiffuseProgram;

//current avatar shader parameter pointer
extern GLint				gAvatarMatrixParam;


#endif
