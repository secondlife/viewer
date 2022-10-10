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

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform sampler2DRect emissiveRect; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
uniform sampler2D altDiffuseMap; // PBR: irradiance, skins/default/textures/default_irradiance.png

const float M_PI = 3.14159265;

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
uniform sampler2DRect lightMap;
#endif

uniform sampler2DRect depthMap;
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
        vec3 pos, vec3 norm, float glossiness);
void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyEnv, 
        vec3 pos, vec3 norm, float glossiness, float envIntensity);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef WATER_FOG
vec4 applyWaterFogViewLinear(vec3 pos, vec4 color);
#endif

// PBR interface
vec3 pbrIbl(vec3 diffuseColor,
            vec3 specularColor,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,       // normal dot view vector
            float perceptualRoughness);

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor, 
                    float perceptualRoughness, 
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l); //surface point to light


void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = texture2DRect(depthMap, tc.xy).r;
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = getNorm(tc);
    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;

    vec4 baseColor     = texture2DRect(diffuseRect, tc);
    vec4 spec        = texture2DRect(specularRect, vary_fragcoord.xy); // NOTE: PBR linear Emissive

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
    vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;
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

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = texture2DRect(specularRect, tc).rgb; 
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        float ao = orm.r * ambocc;

        vec3 colorEmissive = texture2DRect(emissiveRect, tc).rgb;

        // PBR IBL
        float gloss      = 1.0 - perceptualRoughness;
        vec3  irradiance = vec3(0);
        vec3  radiance  = vec3(0);
        sampleReflectionProbes(irradiance, radiance, pos.xyz, norm.xyz, gloss);
        
        // Take maximium of legacy ambient vs irradiance sample as irradiance
        // NOTE: ao is applied in pbrIbl, do not apply here
        irradiance       = max(amblit,irradiance);

        vec3 f0 = vec3(0.04);
        vec3 diffuseColor = baseColor.rgb*(vec3(1.0)-f0);
        diffuseColor *= 1.0 - metallic;

        vec3 specularColor = mix(f0, baseColor.rgb, metallic);

        vec3 v = -normalize(pos.xyz);
        float NdotV = clamp(abs(dot(norm.xyz, v)), 0.001, 1.0);
        
        color.rgb += pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, norm.xyz, v, normalize(light_dir)) * sunlit * 2.75 * scol;
        color.rgb += colorEmissive*0.5;
        
        color.rgb += pbrIbl(diffuseColor, specularColor, radiance, irradiance, ao, NdotV, perceptualRoughness);

        color = atmosFragLightingLinear(color, additive, atten);
        color  = scaleSoftClipFragLinear(color);
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

        sampleReflectionProbesLegacy(irradiance, glossenv, legacyenv, pos.xyz, norm.xyz, spec.a, envIntensity);
        
        // use sky settings ambient or irradiance map sample, whichever is brighter
        irradiance = max(amblit, irradiance);

        // apply lambertian IBL only (see pbrIbl)
        color.rgb = irradiance * ambocc;

        vec3 sun_contrib = min(da, scol) * sunlit;
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

        color = mix(atmosFragLightingLinear(color, additive, atten), fullbrightAtmosTransportFragLinear(color, additive, atten), baseColor.a);
        color = scaleSoftClipFragLinear(color);
    }

    #ifdef WATER_FOG
        vec4 fogged = applyWaterFogViewLinear(pos.xyz, vec4(color, bloom));
        color       = fogged.rgb;
    #endif

    frag_color.rgb = color.rgb; //output linear since local lights will be added to this shader's results
    frag_color.a = 0.0;
}
