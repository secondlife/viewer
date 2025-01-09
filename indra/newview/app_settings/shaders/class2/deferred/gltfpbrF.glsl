/**
 * @file gltfpbrF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
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

// deferred opaque implementation

#ifdef DEBUG
uniform vec4 debug_color;
#endif

vec4 baseColorFactor;
float bp_glow;
float metallicFactor;
float roughnessFactor;
vec3 emissiveColor;

#ifdef ALPHA_MASK
float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

#ifdef SAMPLE_BASE_COLOR_MAP
uniform sampler2D diffuseMap;  //always in sRGB space
in vec2 base_color_texcoord;
#endif


#ifdef SAMPLE_ORM_MAP
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
in vec2 metallic_roughness_texcoord;
#endif

#ifdef HAS_FRAGMENT_NORMAL
in vec3 vary_normal;
#endif

#ifdef SAMPLE_NORMAL_MAP
uniform sampler2D bumpMap;
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 normal_texcoord;
#endif

#ifdef SAMPLE_EMISSIVE_MAP
uniform sampler2D emissiveMap;
in vec2 emissive_texcoord;
#endif

#ifdef OUTPUT_BASE_COLOR_ONLY
out vec4 frag_color;
#else
vec4 encodeNormal(vec3 n, float env, float flag);
out vec4 frag_data[4];
#endif

#ifdef FRAG_POSITION
in vec3 vary_position;
#endif

#ifdef MIRROR_CLIP
void mirrorClip(vec3 pos);
#endif

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef SAMPLE_MATERIALS_UBO
layout (std140) uniform GLTFMaterials
{
    // index by gltf_material_id

    // [gltf_material_id + [0-1]] -  base color transform
    // [gltf_material_id + [2-3]] -  normal transform
    // [gltf_material_id + [4-5]] -  metallic roughness transform
    // [gltf_material_id + [6-7]] -  emissive transform

    // Transforms are packed as follows
    // packed[0] = vec4(scale.x, scale.y, rotation, offset.x)
    // packed[1] = vec4(offset.y, *, *, *)

    // packed[1].yzw varies:
    //   base color transform -- base color factor
    //   normal transform -- .y - alpha factor, .z - minimum alpha, .w - glow
    //   metallic roughness transform -- .y - roughness factor, .z - metallic factor
    //   emissive transform -- emissive factor


    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

flat in int gltf_material_id;

void unpackMaterial()
{
    int idx = gltf_material_id;

    baseColorFactor.rgb = gltf_material_data[idx+1].yzw;
    baseColorFactor.a = gltf_material_data[idx+3].y;
    bp_glow = gltf_material_data[idx+3].w;
    roughnessFactor = gltf_material_data[idx+5].y;
    metallicFactor = gltf_material_data[idx+5].z;
    emissiveColor = gltf_material_data[idx+7].yzw;

#ifdef ALPHA_MASK
    minimum_alpha = gltf_material_data[idx+3].z;
#endif
}
#else // SAMPLE_MATERIALS_UBO
void unpackMaterial()
{
}
#endif

#ifdef ALPHA_BLEND
in vec3 vary_fragcoord;

// Lights
// See: LLRender::syncLightState()

uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;

uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];
uniform vec2 light_deferred_attenuation[8]; // light size and falloff

void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color);

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

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l); //surface point to light

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

void main()
{
    unpackMaterial();
#ifdef MIRROR_CLIP
    mirrorClip(vary_position);
#endif

    vec4 basecolor = vec4(1);
#ifdef SAMPLE_BASE_COLOR_MAP
    basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;

#ifdef GAMMA_CORRECT_BASE_COLOR
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
#endif
    basecolor *= baseColorFactor;

#ifdef ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

#endif

#ifdef HAS_FRAGMENT_NORMAL
#ifdef SAMPLE_NORMAL_MAP
    // from mikktspace.com
    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );
#else
    vec3 tnorm = normalize(vary_normal);
#endif

#ifdef DOUBLE_SIDED
    tnorm *= gl_FrontFacing ? 1.0 : -1.0;
#endif
#endif

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 spec = vec3(1, roughnessFactor, metallicFactor);

#ifdef SAMPLE_ORM_MAP
    spec *= texture(specularMap, metallic_roughness_texcoord.xy).rgb;
#endif

    vec3 emissive = emissiveColor;

#ifdef SAMPLE_EMISSIVE_MAP
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);
#endif

#ifdef OUTPUT_BASE_COLOR_ONLY
#ifdef ALPHA_BLEND

    vec3 pos = vary_position;
    float ao = spec.r;
    float perceptualRoughness = spec.g;
    float metallic = spec.b;

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;

    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;

    float scol = 1.0;
#ifdef HAS_SUN_SHADOW
    scol = sampleDirectionalShadow(pos.xyz, tnorm.xyz, frag);
#endif

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, tnorm, light_dir, sunlit, amblit, additive, atten);

    vec3 sunlit_linear = srgb_to_linear(sunlit);

    // PBR IBL
    float gloss      = 1.0 - perceptualRoughness;
    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);
    sampleReflectionProbes(irradiance, radiance, vary_position.xy*0.5+0.5, pos.xyz, tnorm.xyz, gloss, true, amblit);

    vec3 diffuseColor;
    vec3 specularColor;
    calcDiffuseSpecular(basecolor.rgb, metallic, diffuseColor, specularColor);

    vec3 v = -normalize(pos.xyz);

    vec3 color = pbrBaseLight(diffuseColor, specularColor, metallic, v, tnorm.xyz, perceptualRoughness, light_dir, sunlit_linear, scol, radiance, irradiance, emissive, ao, additive, atten);

    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += pbrCalcPointLightOrSpotLight(diffuseColor, specularColor, perceptualRoughness, metallic, tnorm.xyz, pos.xyz, v, light_position[i].xyz, light_direction[i].xyz, light_diffuse[i].rgb, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    color.rgb += light.rgb;

    color.rgb = applySkyAndWaterFog(pos.xyz, additive, atten, vec4(color, 1.0)).rgb;

    basecolor.rgb = color.rgb;

#else // ALPHA_BLEND
#ifdef SAMPLE_EMISSIVE_MAP
    basecolor.rgb += emissive;
#endif
#endif

#ifdef OUTPUT_SRGB
    basecolor.rgb = linear_to_srgb(basecolor.rgb);
#endif

#ifdef DEBUG
    basecolor = debug_color;
#endif

    frag_color = basecolor;
#else
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(basecolor.rgb, bp_glow), vec4(0));
    frag_data[1] = max(vec4(spec.rgb,0.0), vec4(0));
    frag_data[2] = encodeNormal(tnorm, 0, GBUFFER_FLAG_HAS_PBR);
#if defined(HAS_EMISSIVE)
    frag_data[3] = max(vec4(emissive,0), vec4(0));
#endif
#endif
}

