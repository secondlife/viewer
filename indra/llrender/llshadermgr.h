/** 
 * @file llshadermgr.h
 * @brief Shader Manager
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

#ifndef LL_SHADERMGR_H
#define LL_SHADERMGR_H

#include "llgl.h"
#include "llglslshader.h"

class LLShaderMgr
{
public:
	LLShaderMgr();
	virtual ~LLShaderMgr();

	typedef enum
	{
		MODELVIEW_MATRIX = 0,
		PROJECTION_MATRIX,
		INVERSE_PROJECTION_MATRIX,
		MODELVIEW_PROJECTION_MATRIX,
		NORMAL_MATRIX,
		TEXTURE_MATRIX0,
		TEXTURE_MATRIX1,
		TEXTURE_MATRIX2,
		TEXTURE_MATRIX3,
		VIEWPORT,
		LIGHT_POSITION,
		LIGHT_DIRECTION,
		LIGHT_ATTENUATION,
		LIGHT_DIFFUSE,
		LIGHT_AMBIENT,
		MULTI_LIGHT_COUNT,
		MULTI_LIGHT,
		MULTI_LIGHT_COL,
		MULTI_LIGHT_FAR_Z,
		PROJECTOR_MATRIX,
		PROJECTOR_NEAR,
		PROJECTOR_P,
		PROJECTOR_N,
		PROJECTOR_ORIGIN,
		PROJECTOR_RANGE,
		PROJECTOR_AMBIANCE,
		PROJECTOR_SHADOW_INDEX,
		PROJECTOR_SHADOW_FADE,
		PROJECTOR_FOCUS,
		PROJECTOR_LOD,
		PROJECTOR_AMBIENT_LOD,
		DIFFUSE_COLOR,
		DIFFUSE_MAP,
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
		LIGHT_CENTER,
		LIGHT_SIZE,
		LIGHT_FALLOFF,
		BOX_CENTER,
		BOX_SIZE,

		GLOW_MIN_LUMINANCE,
		GLOW_MAX_EXTRACT_ALPHA,
		GLOW_LUM_WEIGHTS,
		GLOW_WARMTH_WEIGHTS,
		GLOW_WARMTH_AMOUNT,
		GLOW_STRENGTH,
		GLOW_DELTA,

		MINIMUM_ALPHA,

		DEFERRED_SHADOW_MATRIX,
		DEFERRED_ENV_MAT,
		DEFERRED_SHADOW_CLIP,
		DEFERRED_SUN_WASH,
		DEFERRED_SHADOW_NOISE,
		DEFERRED_BLUR_SIZE,
		DEFERRED_SSAO_RADIUS,
		DEFERRED_SSAO_MAX_RADIUS,
		DEFERRED_SSAO_FACTOR,
		DEFERRED_SSAO_FACTOR_INV,
		DEFERRED_SSAO_EFFECT_MAT,
		DEFERRED_SCREEN_RES,
		DEFERRED_NEAR_CLIP,
		DEFERRED_SHADOW_OFFSET,
		DEFERRED_SHADOW_BIAS,
		DEFERRED_SPOT_SHADOW_BIAS,
		DEFERRED_SPOT_SHADOW_OFFSET,
		DEFERRED_SUN_DIR,
		DEFERRED_SHADOW_RES,
		DEFERRED_PROJ_SHADOW_RES,
		DEFERRED_DEPTH_CUTOFF,
		DEFERRED_NORM_CUTOFF,
		DEFERRED_SHADOW_TARGET_WIDTH,

		FXAA_TC_SCALE,
		FXAA_RCP_SCREEN_RES,
		FXAA_RCP_FRAME_OPT,
		FXAA_RCP_FRAME_OPT2,

		DOF_FOCAL_DISTANCE,
		DOF_BLUR_CONSTANT,
		DOF_TAN_PIXEL_ANGLE,
		DOF_MAGNIFICATION,
		DOF_MAX_COF,
		DOF_RES_SCALE,
		DOF_WIDTH,
		DOF_HEIGHT,

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
		DEFERRED_BLOOM,
		DEFERRED_PROJECTION,
		END_RESERVED_UNIFORMS
	} eGLSLReservedUniforms;

	// singleton pattern implementation
	static LLShaderMgr * instance();

	virtual void initAttribsAndUniforms(void);

	BOOL attachShaderFeatures(LLGLSLShader * shader);
	void dumpObjectLog(GLhandleARB ret, BOOL warns = TRUE);
	BOOL	linkProgramObject(GLhandleARB obj, BOOL suppress_errors = FALSE);
	BOOL	validateProgramObject(GLhandleARB obj);
	GLhandleARB loadShaderFile(const std::string& filename, S32 & shader_level, GLenum type, S32 texture_index_channels = -1);

	// Implemented in the application to actually point to the shader directory.
	virtual std::string getShaderDirPrefix(void) = 0; // Pure Virtual

	// Implemented in the application to actually update out of date uniforms for a particular shader
	virtual void updateShaderUniforms(LLGLSLShader * shader) = 0; // Pure Virtual

public:
	// Map of shader names to compiled
	std::map<std::string, GLhandleARB> mShaderObjects;

	//global (reserved slot) shader parameters
	std::vector<std::string> mReservedAttribs;

	std::vector<std::string> mReservedUniforms;

	//preprocessor definitions (name/value)
	std::map<std::string, std::string> mDefinitions;

protected:

	// our parameter manager singleton instance
	static LLShaderMgr * sInstance;

}; //LLShaderMgr

#endif
