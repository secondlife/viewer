/**
 * @file class3/deferred/softenLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shader_texture_lod : enable

#define FLT_MAX 3.402823466e+38

#define REFMAP_COUNT 256
#define REF_SAMPLE_COUNT 64 //maximum number of samples to consider

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2D diffuseRect;
uniform sampler2D specularRect;
uniform sampler2D normalMap;
uniform sampler2D emissiveRect; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl

const float M_PI = 3.14159265;

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
uniform sampler2D lightMap;
#endif

uniform sampler2D depthMap;
uniform sampler2D     lightFunc;

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform mat3 env_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int  sun_up_factor;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec3 getNorm(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec3  atmosFragLightingLinear(vec3 l, vec3 additive, vec3 atten);
vec3  scaleSoftClipFragLinear(vec3 l);
vec3  fullbrightAtmosTransportFragLinear(vec3 light, vec3 additive, vec3 atten);

// reflection probe interface
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
    vec2 tc, vec3 pos, vec3 norm, float glossiness);
void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, float envIntensity);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);
float getDepth(vec2 pos_screen);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

uniform vec4 waterPlane;

#ifdef WATER_FOG
vec4 applyWaterFogViewLinear(vec3 pos, vec4 color);
#endif

void calcDiffuseSpecular(vec3 baseColor, float metallic, inout vec3 diffuseColor, inout vec3 specularColor);

vec3 pbrBaseLight(vec3 diffuseColor,
                  vec3 specularColor,
                  float metallic,
                  vec3 pos,
                  vec3 norm,
                  float perceptualRoughness,
                  vec3 light_dir,
                  vec3 sunlit,
                  float scol,
                  vec3 radiance,
                  vec3 irradiance,
                  vec3 colorEmissive,
                  float ao,
                  vec3 additive,
                  vec3 atten);

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor, 
                    float perceptualRoughness, 
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l); //surface point to light


void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = getDepth(tc.xy);
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2D(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = getNorm(tc);
    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;

    vec4 baseColor     = texture2D(diffuseRect, tc);
    vec4 spec        = texture2D(specularRect, vary_fragcoord.xy); // NOTE: PBR linear Emissive

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
    vec2 scol_ambocc = texture2D(lightMap, vary_fragcoord.xy).rg;
#endif

#if defined(HAS_SUN_SHADOW)
    float scol       = max(scol_ambocc.r, baseColor.a);
#else
    float scol = 1.0;
#endif
#if defined(HAS_SSAO)
    float ambocc     = scol_ambocc.g;
#else
    float ambocc = 1.0;
#endif

    vec3  color = vec3(0);
    float bloom = 0.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVarsLinear(pos.xyz, norm.xyz, light_dir, sunlit, amblit, additive, atten);

    vec3 sunlit_linear = srgb_to_linear(sunlit);
    vec3 amblit_linear = amblit;

    bool do_atmospherics = false;

#ifndef WATER_FOG
    // when above water, mask off atmospherics below water
    if (dot(pos.xyz, waterPlane.xyz) + waterPlane.w > 0.0)
    {
        do_atmospherics = true;
    }
#else
    do_atmospherics = true;
#endif

    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = texture2D(specularRect, tc).rgb; 
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        float ao = orm.r * ambocc;

        vec3 colorEmissive = texture2D(emissiveRect, tc).rgb;
        // PBR IBL
        float gloss      = 1.0 - perceptualRoughness;
        
        sampleReflectionProbes(irradiance, radiance, tc, pos.xyz, norm.xyz, gloss);
        
        // Take maximium of legacy ambient vs irradiance sample as irradiance
        // NOTE: ao is applied in pbrIbl (see pbrBaseLight), do not apply here
        irradiance       = max(amblit_linear,irradiance);

        vec3 diffuseColor;
        vec3 specularColor;
        calcDiffuseSpecular(baseColor.rgb, metallic, diffuseColor, specularColor);

        vec3 v = -normalize(pos.xyz);
        color = vec3(1,0,1);
        color = pbrBaseLight(diffuseColor, specularColor, metallic, v, norm.xyz, perceptualRoughness, light_dir, sunlit_linear, scol, radiance, irradiance, colorEmissive, ao, additive, atten);

        
        if (do_atmospherics)
        {
            color = linear_to_srgb(color);
            color = atmosFragLightingLinear(color, additive, atten);
            color = srgb_to_linear(color);
        }
        
        
    }
    else if (!GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_ATMOS))
    {
        //should only be true of WL sky, just port over base color value
        color = srgb_to_linear(baseColor.rgb);
    }
    else
    {
        // legacy shaders are still writng sRGB to gbuffer
        baseColor.rgb = srgb_to_linear(baseColor.rgb);
        spec.rgb = srgb_to_linear(spec.rgb);

        float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);

        vec3 irradiance = vec3(0);
        vec3 glossenv = vec3(0);
        vec3 legacyenv = vec3(0);

        sampleReflectionProbesLegacy(irradiance, glossenv, legacyenv, tc, pos.xyz, norm.xyz, spec.a, envIntensity);
        
        // use sky settings ambient or irradiance map sample, whichever is brighter
        irradiance = max(amblit_linear, irradiance);

        // apply lambertian IBL only (see pbrIbl)
        color.rgb = irradiance * ambocc;

        vec3 sun_contrib = min(da, scol) * sunlit_linear;
        color.rgb += sun_contrib;
        color.rgb *= baseColor.rgb;
        
        vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

        if (spec.a > 0.0)  // specular reflection
        {
            float sa        = dot(normalize(refnormpersp), light_dir.xyz);
            vec3  dumbshiny = sunlit * scol * (texture2D(lightFunc, vec2(sa, spec.a)).r);

            // add the two types of shiny together
            vec3 spec_contrib = dumbshiny * spec.rgb;
            color.rgb += spec_contrib;

            // add radiance map
            applyGlossEnv(color, glossenv, spec, pos.xyz, norm.xyz);
        }

        color.rgb = mix(color.rgb, baseColor.rgb, baseColor.a);
        
        if (envIntensity > 0.0)
        {  // add environment map
            applyLegacyEnv(color, legacyenv, spec, pos.xyz, norm.xyz, envIntensity);
        }

        
        if (do_atmospherics)
        {
            color = linear_to_srgb(color);
            color = atmosFragLightingLinear(color, additive, atten);
            color = srgb_to_linear(color);
        }
   }

    

    #ifdef WATER_FOG
        vec4 fogged = applyWaterFogViewLinear(pos.xyz, vec4(color, bloom));
        color       = fogged.rgb;
    #endif

    frag_color.rgb = color.rgb; //output linear since local lights will be added to this shader's results
    frag_color.a = 0.0;
}
