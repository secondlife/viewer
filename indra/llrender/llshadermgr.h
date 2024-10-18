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

    // clang-format off
    typedef enum
    {                                       // Shader uniform name, set in LLShaderMgr::initAttribsAndUniforms()
        MODELVIEW_MATRIX = 0,               //  "modelview_matrix"
        PROJECTION_MATRIX,                  //  "projection_matrix"
        INVERSE_PROJECTION_MATRIX,          //  "inv_proj"
        MODELVIEW_PROJECTION_MATRIX,        //  "modelview_projection_matrix"
        INVERSE_MODELVIEW_MATRIX,           //  "inv_modelview"
        IDENTITY_MATRIX,                    //  "identity_matrix"
        NORMAL_MATRIX,                      //  "normal_matrix"
        TEXTURE_MATRIX0,                    //  "texture_matrix0"
        TEXTURE_MATRIX1,                    //  "texture_matrix1"
        TEXTURE_MATRIX2,                    //  "texture_matrix2"
        TEXTURE_MATRIX3,                    //  "texture_matrix3"
        OBJECT_PLANE_S,                     //  "object_plane_s"
        OBJECT_PLANE_T,                     //  "object_plane_t"

        TEXTURE_BASE_COLOR_TRANSFORM,         //  "texture_base_color_transform" (GLTF)
        TEXTURE_NORMAL_TRANSFORM,             //  "texture_normal_transform" (GLTF)
        TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, //  "texture_metallic_roughness_transform" (GLTF)
        TEXTURE_OCCLUSION_TRANSFORM,          //  "texture_occlusion_transform" (GLTF)
        TEXTURE_EMISSIVE_TRANSFORM,           //  "texture_emissive_transform" (GLTF)
        BASE_COLOR_TEXCOORD,                  //  "base_color_texcoord" (GLTF)
        EMISSIVE_TEXCOORD,                    //  "emissive_texcoord" (GLTF)
        NORMAL_TEXCOORD,                      //  "normal_texcoord" (GLTF)
        METALLIC_ROUGHNESS_TEXCOORD,          //  "metallic_roughness_texcoord" (GLTF)
        OCCLUSION_TEXCOORD,                   //  "occlusion_texcoord" (GLTF)
        GLTF_NODE_ID,                         //  "gltf_node_id" (GLTF)
        GLTF_MATERIAL_ID,                     //  "gltf_material_id" (GLTF)

        TERRAIN_TEXTURE_TRANSFORMS,           //  "terrain_texture_transforms" (GLTF)
        TERRAIN_STAMP_SCALE,                  //  "terrain_stamp_scale"

        VIEWPORT,                           //  "viewport"
        LIGHT_POSITION,                     //  "light_position"
        LIGHT_DIRECTION,                    //  "light_direction"
        LIGHT_ATTENUATION,                  //  "light_attenuation"
        LIGHT_DEFERRED_ATTENUATION,         //  "light_deferred_attenuation"
        LIGHT_DIFFUSE,                      //  "light_diffuse"
        LIGHT_AMBIENT,                      //  "light_ambient"
        MULTI_LIGHT_COUNT,                  //  "light_count"
        MULTI_LIGHT,                        //  "light"
        MULTI_LIGHT_COL,                    //  "light_col"
        MULTI_LIGHT_FAR_Z,                  //  "far_z"
        PROJECTOR_MATRIX,                   //  "proj_mat"
        PROJECTOR_NEAR,                     //  "proj_near"
        PROJECTOR_P,                        //  "proj_p"
        PROJECTOR_N,                        //  "proj_n"
        PROJECTOR_ORIGIN,                   //  "proj_origin"
        PROJECTOR_RANGE,                    //  "proj_range"
        PROJECTOR_AMBIANCE,                 //  "proj_ambiance"
        PROJECTOR_SHADOW_INDEX,             //  "proj_shadow_idx"
        PROJECTOR_SHADOW_FADE,              //  "shadow_fade"
        PROJECTOR_FOCUS,                    //  "proj_focus"
        PROJECTOR_LOD,                      //  "proj_lod"
        PROJECTOR_AMBIENT_LOD,              //  "proj_ambient_lod"
        DIFFUSE_COLOR,                      //  "color"
        EMISSIVE_COLOR,                     //  "emissiveColor"
        METALLIC_FACTOR,                    //  "metallicFactor"
        ROUGHNESS_FACTOR,                   //  "roughnessFactor"
        MIRROR_FLAG,                        //  "mirror_flag"
        CLIP_PLANE,                         //  "clipPlane"
        CLIP_SIGN,                          //  "clipSign"
        DIFFUSE_MAP,                        //  "diffuseMap"
        ALTERNATE_DIFFUSE_MAP,              //  "altDiffuseMap"
        SPECULAR_MAP,                       //  "specularMap"
        METALLIC_ROUGHNESS_MAP,             //  "metallicRoughnessMap"
        NORMAL_MAP,                         //  "normalMap"
        OCCLUSION_MAP,                      //  "occlusionMap"
        EMISSIVE_MAP,                       //  "emissiveMap"
        BUMP_MAP,                           //  "bumpMap"
        BUMP_MAP2,                          //  "bumpMap2"
        ENVIRONMENT_MAP,                    //  "environmentMap"
        SCENE_MAP,                          //  "sceneMap"
        SCENE_DEPTH,                        //  "sceneDepth"
        REFLECTION_PROBES,                  //  "reflectionProbes"
        IRRADIANCE_PROBES,                  //  "irradianceProbes"
        HERO_PROBE,                         //  "heroProbes"
        CLOUD_NOISE_MAP,                    //  "cloud_noise_texture"
        CLOUD_NOISE_MAP_NEXT,               //  "cloud_noise_texture_next"
        LIGHTNORM,                          //  "lightnorm"
        SUNLIGHT_COLOR,                     //  "sunlight_color"
        AMBIENT,                            //  "ambient_color"
        SKY_HDR_SCALE,                      //  "sky_hdr_scale"
        SKY_SUNLIGHT_SCALE,                 //  "sky_sunlight_scale"
        SKY_AMBIENT_SCALE,                  //  "sky_ambient_scale"
        BLUE_HORIZON,                       //  "blue_horizon"
        BLUE_DENSITY,                       //  "blue_density"
        HAZE_HORIZON,                       //  "haze_horizon"
        HAZE_DENSITY,                       //  "haze_density"
        CLOUD_SHADOW,                       //  "cloud_shadow"
        DENSITY_MULTIPLIER,                 //  "density_multiplier"
        DISTANCE_MULTIPLIER,                //  "distance_multiplier"
        MAX_Y,                              //  "max_y"
        GLOW,                               //  "glow"
        CLOUD_COLOR,                        //  "cloud_color"
        CLOUD_POS_DENSITY1,                 //  "cloud_pos_density1"
        CLOUD_POS_DENSITY2,                 //  "cloud_pos_density2"
        CLOUD_SCALE,                        //  "cloud_scale"
        GAMMA,                              //  "gamma"
        SCENE_LIGHT_STRENGTH,               //  "scene_light_strength"
        LIGHT_CENTER,                       //  "center"
        LIGHT_SIZE,                         //  "size"
        LIGHT_FALLOFF,                      //  "falloff"
        BOX_CENTER,                         //  "box_center"
        BOX_SIZE,                           //  "box_size"

        GLOW_MIN_LUMINANCE,                 //  "minLuminance"
        GLOW_MAX_EXTRACT_ALPHA,             //  "maxExtractAlpha"
        GLOW_LUM_WEIGHTS,                   //  "lumWeights"
        GLOW_WARMTH_WEIGHTS,                //  "warmthWeights"
        GLOW_WARMTH_AMOUNT,                 //  "warmthAmount"
        GLOW_STRENGTH,                      //  "glowStrength"
        GLOW_DELTA,                         //  "glowDelta"
        GLOW_NOISE_MAP,                     //  "glowNoiseMap"

        MINIMUM_ALPHA,                      //  "minimum_alpha"
        EMISSIVE_BRIGHTNESS,                //  "emissive_brightness"

        DEFERRED_SHADOW_MATRIX,             //  "shadow_matrix"
        DEFERRED_ENV_MAT,                   //  "env_mat"
        DEFERRED_SHADOW_CLIP,               //  "shadow_clip"
        DEFERRED_SSAO_RADIUS,               //  "ssao_radius"
        DEFERRED_SSAO_MAX_RADIUS,           //  "ssao_max_radius"
        DEFERRED_SSAO_FACTOR,               //  "ssao_factor"
        DEFERRED_SSAO_FACTOR_INV,           //  "ssao_factor_inv"
        DEFERRED_SSAO_EFFECT_MAT,           //  "ssao_effect_mat"
        DEFERRED_SCREEN_RES,                //  "screen_res"
        DEFERRED_NEAR_CLIP,                 //  "near_clip"
        DEFERRED_SHADOW_OFFSET,             //  "shadow_offset"
        DEFERRED_SHADOW_BIAS,               //  "shadow_bias"
        DEFERRED_SPOT_SHADOW_BIAS,          //  "spot_shadow_bias"
        DEFERRED_SPOT_SHADOW_OFFSET,        //  "spot_shadow_offset"
        DEFERRED_SUN_DIR,                   //  "sun_dir"
        DEFERRED_MOON_DIR,                  //  "moon_dir"
        DEFERRED_SHADOW_RES,                //  "shadow_res"
        DEFERRED_PROJ_SHADOW_RES,           //  "proj_shadow_res"
        DEFERRED_SHADOW_TARGET_WIDTH,       //  "shadow_target_width"

        DEFERRED_SSR_ITR_COUNT,             //  "iterationCount"
        DEFERRED_SSR_RAY_STEP,              //  "rayStep"
        DEFERRED_SSR_DIST_BIAS,             //  "distanceBias"
        DEFERRED_SSR_REJECT_BIAS,           //  "depthRejectBias"
        DEFERRED_SSR_GLOSSY_SAMPLES,        //  "glossySampleCount"
        DEFERRED_SSR_NOISE_SINE,            //  "noiseSine"
        DEFERRED_SSR_ADAPTIVE_STEP_MULT,    //  "adaptiveStepMultiplier"

        MODELVIEW_DELTA_MATRIX,             //  "modelview_delta"
        INVERSE_MODELVIEW_DELTA_MATRIX,     //  "inv_modelview_delta"
        CUBE_SNAPSHOT,                      //  "cube_snapshot"

        FXAA_TC_SCALE,                      //  "tc_scale"
        FXAA_RCP_SCREEN_RES,                //  "rcp_screen_res"
        FXAA_RCP_FRAME_OPT,                 //  "rcp_frame_opt"
        FXAA_RCP_FRAME_OPT2,                //  "rcp_frame_opt2"

        DOF_FOCAL_DISTANCE,                 //  "focal_distance"
        DOF_BLUR_CONSTANT,                  //  "blur_constant"
        DOF_TAN_PIXEL_ANGLE,                //  "tan_pixel_angle"
        DOF_MAGNIFICATION,                  //  "magnification"
        DOF_MAX_COF,                        //  "max_cof"
        DOF_RES_SCALE,                      //  "res_scale"
        DOF_WIDTH,                          //  "dof_width"
        DOF_HEIGHT,                         //  "dof_height"

        DEFERRED_DEPTH,                     //  "depthMap"
        DEFERRED_SHADOW0,                   //  "shadowMap0"
        DEFERRED_SHADOW1,                   //  "shadowMap1"
        DEFERRED_SHADOW2,                   //  "shadowMap2"
        DEFERRED_SHADOW3,                   //  "shadowMap3"
        DEFERRED_SHADOW4,                   //  "shadowMap4"
        DEFERRED_SHADOW5,                   //  "shadowMap5"
        DEFERRED_POSITION,                  //  "positionMap"
        DEFERRED_DIFFUSE,                   //  "diffuseRect"
        DEFERRED_SPECULAR,                  //  "specularRect"
        DEFERRED_EMISSIVE,                  //  "emissiveRect"
        EXPOSURE_MAP,                       //  "exposureMap"
        DEFERRED_BRDF_LUT,                  //  "brdfLut"
        DEFERRED_NOISE,                     //  "noiseMap"
        DEFERRED_LIGHTFUNC,                 //  "lightFunc"
        DEFERRED_LIGHT,                     //  "lightMap"
        DEFERRED_PROJECTION,                //  "projectionMap"
        SPECULAR_COLOR,                     //  "specular_color"
        ENVIRONMENT_INTENSITY,              //  "env_intensity"

        AVATAR_MATRIX,                      //  "matrixPalette"
        AVATAR_TRANSLATION,                 //  "translationPalette"

        WATER_SCREENTEX,                    //  "screenTex"
        WATER_SCREENDEPTH,                  //  "screenDepth"
        WATER_REFTEX,                       //  "refTex"
        WATER_EYEVEC,                       //  "eyeVec"
        WATER_TIME,                         //  "time"
        WATER_WAVE_DIR1,                    //  "waveDir1"
        WATER_WAVE_DIR2,                    //  "waveDir2"
        WATER_LIGHT_DIR,                    //  "lightDir"
        WATER_SPECULAR,                     //  "specular"
        WATER_SPECULAR_EXP,                 //  "lightExp"
        WATER_FOGCOLOR,                     //  "waterFogColor"
        WATER_FOGCOLOR_LINEAR,              //  "waterFogColorLinear"
        WATER_FOGDENSITY,                   //  "waterFogDensity"
        WATER_FOGKS,                        //  "waterFogKS"
        WATER_REFSCALE,                     //  "refScale"
        WATER_WATERHEIGHT,                  //  "waterHeight"
        WATER_WATERPLANE,                   //  "waterPlane"
        WATER_NORM_SCALE,                   //  "normScale"
        WATER_FRESNEL_SCALE,                //  "fresnelScale"
        WATER_FRESNEL_OFFSET,               //  "fresnelOffset"
        WATER_BLUR_MULTIPLIER,              //  "blurMultiplier"
        WATER_SUN_ANGLE,                    //  "sunAngle"
        WATER_SCALED_ANGLE,                 //  "scaledAngle"
        WATER_SUN_ANGLE2,                   //  "sunAngle2"

        WL_CAMPOSLOCAL,                     //  "camPosLocal"

        AVATAR_WIND,                        //  "gWindDir"
        AVATAR_SINWAVE,                     //  "gSinWaveParams"
        AVATAR_GRAVITY,                     //  "gGravity"

        TERRAIN_DETAIL0,                    //  "detail_0"
        TERRAIN_DETAIL1,                    //  "detail_1"
        TERRAIN_DETAIL2,                    //  "detail_2"
        TERRAIN_DETAIL3,                    //  "detail_3"

        TERRAIN_ALPHARAMP,                  //  "alpha_ramp"
        TERRAIN_PAINTMAP,                   //  "paint_map"

        TERRAIN_DETAIL0_BASE_COLOR,                //  "detail_0_base_color" (GLTF)
        TERRAIN_DETAIL1_BASE_COLOR,                //  "detail_1_base_color" (GLTF)
        TERRAIN_DETAIL2_BASE_COLOR,                //  "detail_2_base_color" (GLTF)
        TERRAIN_DETAIL3_BASE_COLOR,                //  "detail_3_base_color" (GLTF)
        TERRAIN_DETAIL0_NORMAL,                    //  "detail_0_normal" (GLTF)
        TERRAIN_DETAIL1_NORMAL,                    //  "detail_1_normal" (GLTF)
        TERRAIN_DETAIL2_NORMAL,                    //  "detail_2_normal" (GLTF)
        TERRAIN_DETAIL3_NORMAL,                    //  "detail_3_normal" (GLTF)
        TERRAIN_DETAIL0_METALLIC_ROUGHNESS,        //  "detail_0_metallic_roughness" (GLTF)
        TERRAIN_DETAIL1_METALLIC_ROUGHNESS,        //  "detail_1_metallic_roughness" (GLTF)
        TERRAIN_DETAIL2_METALLIC_ROUGHNESS,        //  "detail_2_metallic_roughness" (GLTF)
        TERRAIN_DETAIL3_METALLIC_ROUGHNESS,        //  "detail_3_metallic_roughness" (GLTF)
        TERRAIN_DETAIL0_EMISSIVE,                  //  "detail_0_emissive" (GLTF)
        TERRAIN_DETAIL1_EMISSIVE,                  //  "detail_1_emissive" (GLTF)
        TERRAIN_DETAIL2_EMISSIVE,                  //  "detail_2_emissive" (GLTF)
        TERRAIN_DETAIL3_EMISSIVE,                  //  "detail_3_emissive" (GLTF)

        TERRAIN_BASE_COLOR_FACTORS,                //  "baseColorFactors" (GLTF)
        TERRAIN_METALLIC_FACTORS,                  //  "metallicFactors" (GLTF)
        TERRAIN_ROUGHNESS_FACTORS,                 //  "roughnessFactors" (GLTF)
        TERRAIN_EMISSIVE_COLORS,                   //  "emissiveColors" (GLTF)
        TERRAIN_MINIMUM_ALPHAS,                    //  "minimum_alphas" (GLTF)

        REGION_SCALE,                              //  "region_scale" (GLTF)

        SHINY_ORIGIN,                       //  "origin"
        DISPLAY_GAMMA,                      //  "display_gamma"

        INSCATTER_RT,                       //  "inscatter"
        SUN_SIZE,                           //  "sun_size"
        FOG_COLOR,                          //  "fog_color"

        BLEND_FACTOR,                       //  "blend_factor"
        MOISTURE_LEVEL,                     //  "moisture_level"
        DROPLET_RADIUS,                     //  "droplet_radius"
        ICE_LEVEL,                          //  "ice_level"
        RAINBOW_MAP,                        //  "rainbow_map"
        HALO_MAP,                           //  "halo_map"

        MOON_BRIGHTNESS,                    //  "moon_brightness"

        CLOUD_VARIANCE,                     //  "cloud_variance"

        REFLECTION_PROBE_AMBIANCE,          //  "reflection_probe_ambiance"
        REFLECTION_PROBE_MAX_LOD,            //  "max_probe_lod"
        REFLECTION_PROBE_STRENGTH,            //  "probe_strength"
        SH_INPUT_L1R,                       //  "sh_input_r"
        SH_INPUT_L1G,                       //  "sh_input_g"
        SH_INPUT_L1B,                       //  "sh_input_b"

        SUN_MOON_GLOW_FACTOR,               //  "sun_moon_glow_factor"
        WATER_EDGE_FACTOR,                  //  "water_edge"
        SUN_UP_FACTOR,                      //  "sun_up_factor"
        MOONLIGHT_COLOR,                    //  "moonlight_color"

        DEBUG_NORMAL_DRAW_LENGTH,           //  "debug_normal_draw_length"

        SMAA_EDGE_TEX,                      //  "edgesTex"
        SMAA_AREA_TEX,                      //  "areaTex"
        SMAA_SEARCH_TEX,                    //  "searchTex"
        SMAA_BLEND_TEX,                     //  "blendTex"

        END_RESERVED_UNIFORMS
    } eGLSLReservedUniforms;
    // clang-format on

    // singleton pattern implementation
    static LLShaderMgr * instance();

    virtual void initAttribsAndUniforms(void);

    bool attachShaderFeatures(LLGLSLShader * shader);
    void dumpObjectLog(GLuint ret, bool warns = true, const std::string& filename = "");
    void dumpShaderSource(U32 shader_code_count, GLchar** shader_code_text);
    bool    linkProgramObject(GLuint obj, bool suppress_errors = false);
    bool    validateProgramObject(GLuint obj);
    GLuint loadShaderFile(const std::string& filename, S32 & shader_level, GLenum type, std::map<std::string, std::string>* defines = NULL, S32 texture_index_channels = -1);

    // Implemented in the application to actually point to the shader directory.
    virtual std::string getShaderDirPrefix(void) = 0; // Pure Virtual

    // Implemented in the application to actually update out of date uniforms for a particular shader
    virtual void updateShaderUniforms(LLGLSLShader * shader) = 0; // Pure Virtual

    void initShaderCache(bool enabled, const LLUUID& old_cache_version, const LLUUID& current_cache_version);
    void clearShaderCache();
    void persistShaderCacheMetadata();

    bool loadCachedProgramBinary(LLGLSLShader* shader);
    bool saveCachedProgramBinary(LLGLSLShader* shader);

public:
    // Map of shader names to compiled
    std::map<std::string, GLuint> mVertexShaderObjects;
    std::map<std::string, GLuint> mFragmentShaderObjects;

    //global (reserved slot) shader parameters
    std::vector<std::string> mReservedAttribs;

    std::vector<std::string> mReservedUniforms;

    struct ProgramBinaryData
    {
        GLsizei mBinaryLength = 0;
        GLenum mBinaryFormat = 0;
        F32 mLastUsedTime = 0.0;
    };
    std::map<LLUUID, ProgramBinaryData> mShaderBinaryCache;
    bool mShaderCacheInitialized = false;
    bool mShaderCacheEnabled = false;
    std::string mShaderCacheDir;

protected:

    // our parameter manager singleton instance
    static LLShaderMgr * sInstance;

}; //LLShaderMgr

#endif
