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

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
vec3  atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3  scaleSoftClipFrag(vec3 l);
vec3  fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3  fullbrightScaleSoftClip(vec3 light);

// reflection probe interface
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyEnv, 
        vec3 pos, vec3 norm, float glossiness, float envIntensity);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

// PBR interface
void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
void initMaterial( vec3 diffuse, vec3 packedORM,
        out float alphaRough, out vec3 c_diff, out vec3 reflect0, out vec3 reflect90, out float specWeight );
// perform PBR image based lighting according to GLTF spec
// all parameters are in linear space
void pbrIbl(out vec3 colorDiffuse, // diffuse color output
            out vec3 colorSpec, // specular color output,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,
            float perceptualRough, // roughness factor
            float gloss,        // 1.0 - roughness factor
            vec3 reflect0,
            vec3 c_diff);

void pbrDirectionalLight(inout vec3 colorDiffuse,
    inout vec3 colorSpec,
    vec3 sunlit,
    float scol,
    vec3 reflect0,
    vec3 reflect90,
    vec3 c_diff,
    float alphaRough,
    float vh,
    float nl,
    float nv,
    float nh);

void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = texture2DRect(depthMap, tc.xy).r;
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = getNorm(tc);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    float light_gamma = 1.0 / 1.3;

    vec4 diffuse     = texture2DRect(diffuseRect, tc);
    vec4 spec        = texture2DRect(specularRect, vary_fragcoord.xy); // NOTE: PBR linear Emissive

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
    vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;
    scol_ambocc      = pow(scol_ambocc, vec2(light_gamma));
    float scol       = max(scol_ambocc.r, diffuse.a);
    float ambocc     = scol_ambocc.g;
#else
    float scol = 1.0;
    float ambocc = 1.0;
#endif

    vec3  color = vec3(0);
    float bloom = 0.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVars(pos.xyz, light_dir, ambocc, sunlit, amblit, additive, atten, true);

    vec3 ambenv;
    vec3 glossenv;
    vec3 legacyenv;

    bool hasPBR = GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR);
    if (hasPBR)
    {
        // 5.22.2. material.pbrMetallicRoughness.baseColorTexture
        // The first three components (RGB) MUST be encoded with the sRGB transfer function.
        //
        // 5.19.7. material.emissiveTexture
        // This texture contains RGB components encoded with the sRGB transfer function.
        //
        // 5.22.5. material.pbrMetallicRoughness.metallicRoughnessTexture
        // These values MUST be encoded with a linear transfer function.

        vec3 colorDiffuse      = vec3(0);
        vec3 colorEmissive     = spec.rgb; // PBR sRGB Emissive.  See: pbropaqueF.glsl
        vec3 colorSpec         = vec3(0);

        vec3 packedORM        = texture2DRect(emissiveRect, tc).rgb; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
        float IOR             = 1.5;         // default Index Of Refraction 1.5 (dielectrics)
        float ao         = packedORM.r;
        float metal      = packedORM.b;
        vec3  v          = -normalize(pos.xyz);
        vec3  n          = norm.xyz;

        vec3  h, l;
        float nh, nl, nv, vh, lightDist;
        calcHalfVectors(light_dir, n, v, h, l, nh, nl, nv, vh, lightDist);

        float perceptualRough = packedORM.g;  // NOTE: do NOT clamp here to be consistent with Blender, Blender is wrong and Substance is right
        
        vec3 c_diff, reflect0, reflect90;
        float alphaRough, specWeight;
        initMaterial( diffuse.rgb, packedORM, alphaRough, c_diff, reflect0, reflect90, specWeight );

        float gloss      = 1.0 - perceptualRough;
        vec3  irradiance = vec3(0);
        vec3  radiance  = vec3(0);
        sampleReflectionProbes(irradiance, radiance, legacyenv, pos.xyz, norm.xyz, gloss, 0.0);
        irradiance       = max(amblit,irradiance) * ambocc;

        pbrIbl(colorDiffuse, colorSpec, radiance, irradiance, ao, nv, perceptualRough, gloss, reflect0, c_diff);
        
        // Add in sun/moon punctual light
        if (nl > 0.0 || nv > 0.0)
        {
            pbrDirectionalLight(colorDiffuse, colorSpec, srgb_to_linear(sunlit), scol, reflect0, reflect90, c_diff, alphaRough, vh, nl, nv, nh);
        }

        color.rgb = colorDiffuse + colorEmissive + colorSpec;

        color  = linear_to_srgb(color);
        color *= atten.r;
        color += 2.0*additive;
        color  = scaleSoftClipFrag(color);
        color  = srgb_to_linear(color);

        frag_color.rgb = color.rgb; //output linear since local lights will be added to this shader's results
    }
    else
    {
        float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
        da                = pow(da, light_gamma);

        diffuse.rgb = linear_to_srgb(diffuse.rgb); // SL-14035

        sampleReflectionProbes(ambenv, glossenv, legacyenv, pos.xyz, norm.xyz, spec.a, envIntensity);
        ambenv.rgb = linear_to_srgb(ambenv.rgb); 
        glossenv.rgb = linear_to_srgb(glossenv.rgb);
        legacyenv.rgb = linear_to_srgb(legacyenv.rgb);

        amblit = max(ambenv, amblit);
        color.rgb = amblit*ambocc;

        vec3 sun_contrib = min(da, scol) * sunlit;
        color.rgb += sun_contrib;
        color.rgb = min(color.rgb, vec3(1,1,1));
        color.rgb *= diffuse.rgb;

        vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

        if (spec.a > 0.0)  // specular reflection
        {
            float sa        = dot(normalize(refnormpersp), light_dir.xyz);
            vec3  dumbshiny = sunlit * scol * (texture2D(lightFunc, vec2(sa, spec.a)).r);

            // add the two types of shiny together
            vec3 spec_contrib = dumbshiny * spec.rgb;
            bloom             = dot(spec_contrib, spec_contrib) / 6;
            color.rgb += spec_contrib;

            // add reflection map - EXPERIMENTAL WORK IN PROGRESS
            applyGlossEnv(color, glossenv, spec, pos.xyz, norm.xyz);
        }

        color.rgb = mix(color.rgb, diffuse.rgb, diffuse.a);

        if (envIntensity > 0.0)
        {  // add environmentmap
            //fudge darker
            legacyenv *= 0.5*diffuse.a+0.5;
            applyLegacyEnv(color, legacyenv, spec, pos.xyz, norm.xyz, envIntensity);
        }

        if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_ATMOS))
        {
            color = mix(atmosFragLighting(color, additive, atten), fullbrightAtmosTransportFrag(color, additive, atten), diffuse.a);
            color = mix(scaleSoftClipFrag(color), fullbrightScaleSoftClip(color), diffuse.a);
        }

    #ifdef WATER_FOG
        vec4 fogged = applyWaterFogView(pos.xyz, vec4(color, bloom));
        color       = fogged.rgb;
        bloom       = fogged.a;
    #endif

        // convert to linear as fullscreen lights need to sum in linear colorspace
        // and will be gamma (re)corrected downstream...
        //color = ambenv;
        //color.b = diffuse.a;
        frag_color.rgb = srgb_to_linear(color.rgb);
    }

    frag_color.a = bloom;
}
