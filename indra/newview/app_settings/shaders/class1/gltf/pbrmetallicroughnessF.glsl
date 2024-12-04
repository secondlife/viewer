/**
 * @file pbrmetallicroughnessF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/


// GLTF pbrMetallicRoughness implementation

uniform int gltf_material_id;

vec3 emissiveColor = vec3(0,0,0);
float metallicFactor = 1.0;
float roughnessFactor = 1.0;
float minimum_alpha = -1.0;

layout (std140) uniform GLTFMaterials
{
    // see pbrmetallicroughnessV.glsl for packing
    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

void unpackMaterial()
{
    if (gltf_material_id > -1)
    {
        int idx = gltf_material_id*12;
        emissiveColor = gltf_material_data[idx+10].rgb;
        roughnessFactor = gltf_material_data[idx+11].g;
        metallicFactor = gltf_material_data[idx+11].b;
        minimum_alpha -= gltf_material_data[idx+11].a;
    }
}

// ==================================
// needed by all variants
// ==================================
uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D emissiveMap;
in vec3 vary_position;
in vec4 vertex_color;
in vec2 base_color_uv;
in vec2 emissive_uv;

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);
// ==================================


// ==================================
// needed by all lit variants
// ==================================
#ifndef UNLIT
uniform sampler2D normalMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D occlusionMap;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 normal_uv;
in vec2 metallic_roughness_uv;
in vec2 occlusion_uv;
#endif
// ==================================


// ==================================
// needed by all alpha variants
// ==================================
#ifdef ALPHA_BLEND
in vec3 vary_fragcoord;
uniform vec4 clipPlane;
uniform float clipSign;
void waterClip(vec3 pos);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color);
#endif
// ==================================


// ==================================
// needed by lit alpha
// ==================================
#if defined(ALPHA_BLEND) && !defined(UNLIT)

#ifdef HAS_SUN_SHADOW
uniform sampler2D lightMap;
uniform vec2 screen_res;
#endif

// Lights
// See: LLRender::syncLightState()
uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];
uniform vec2 light_deferred_attenuation[8]; // light size and falloff

uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, bool transparent, vec3 amblit_linear);

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

vec3 pbrCalcPointLightOrSpotLight(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize, float falloff, float is_pointlight, float ambiance);

#endif
// ==================================


// ==================================
// output definition
// ==================================
#if defined(ALPHA_BLEND) || defined(UNLIT)
out vec4 frag_color;
#else
out vec4 frag_data[4];
#endif
// ==================================


void main()
{
    unpackMaterial();
// ==================================
// all variants
//   mirror clip
//   base color
//   masking
//   emissive
// ==================================
    vec3 pos = vary_position;
    mirrorClip(pos);

#ifdef ALPHA_BLEND
    //waterClip(pos);
#endif

    vec4 basecolor = texture(diffuseMap, base_color_uv.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
    basecolor *= vertex_color;

    if (basecolor.a < minimum_alpha)
    {
        discard;
    }

    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_uv.xy).rgb);
// ==================================

// ==================================
// all lit variants
//   prepare norm
//   prepare orm
// ==================================
#ifndef UNLIT
    // from mikktspace.com
    vec3 vNt = texture(normalMap, normal_uv.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 norm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );
    norm *= gl_FrontFacing ? 1.0 : -1.0;

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 orm = texture(metallicRoughnessMap, metallic_roughness_uv.xy).rgb;
    orm.r = texture(occlusionMap, occlusion_uv.xy).r;
    orm.g *= roughnessFactor;
    orm.b *= metallicFactor;
#endif
// ==================================

// ==================================
// non alpha output
// ==================================
#ifndef ALPHA_BLEND
#ifdef UNLIT
    vec4 color = basecolor;
    color.rgb += emissive.rgb;
    frag_color = color;
#else
    frag_data[0] = max(vec4(basecolor.rgb, 0.0), vec4(0));
    frag_data[1] = max(vec4(orm.rgb,0.0), vec4(0));
    frag_data[2] = encodeNormal(norm, 0, GBUFFER_FLAG_HAS_PBR);

//#if defined(HAS_EMISSIVE)
    frag_data[3] = max(vec4(emissive,0), vec4(0));
//#endif
#endif
#endif


// ==================================
// alpha implementation
// ==================================
#ifdef ALPHA_BLEND

    float scol = 1.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    vec3  light_dir;

#ifdef UNLIT
    light_dir = vec3(0,0,1);
    vec3 norm = vec3(0,0,1);
#else
    light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;
#endif

    calcAtmosphericVarsLinear(pos.xyz, norm, light_dir, sunlit, amblit, additive, atten);

#ifndef UNLIT
    vec3 sunlit_linear = srgb_to_linear(sunlit);

    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;

#ifdef HAS_SUN_SHADOW
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    float perceptualRoughness = orm.g * roughnessFactor;
    float metallic = orm.b * metallicFactor;

    // PBR IBL
    float gloss      = 1.0 - perceptualRoughness;
    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);
    sampleReflectionProbes(irradiance, radiance, vary_position.xy*0.5+0.5, pos.xyz, norm.xyz, gloss, true, amblit);

    vec3 diffuseColor;
    vec3 specularColor;
    calcDiffuseSpecular(basecolor.rgb, metallic, diffuseColor, specularColor);

    vec3 v = -normalize(pos.xyz);

    vec3 color = pbrBaseLight(diffuseColor, specularColor, metallic, v, norm.xyz, perceptualRoughness, light_dir, sunlit_linear, scol, radiance, irradiance, emissive, orm.r, additive, atten);

    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += pbrCalcPointLightOrSpotLight(diffuseColor, specularColor, perceptualRoughness, metallic, norm.xyz, pos.xyz, v, light_position[i].xyz, light_direction[i].xyz, light_diffuse[i].rgb, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    color.rgb += light.rgb;

    color.rgb = applySkyAndWaterFog(pos.xyz, additive, atten, vec4(color, 1.0)).rgb;

    float a = basecolor.a*vertex_color.a;

    frag_color = max(vec4(color.rgb,a), vec4(0));
#else // UNLIT
    vec4 color = basecolor;
    color.rgb += emissive.rgb;
    frag_color = color;
#endif
#endif  // ALPHA_BLEND
}

