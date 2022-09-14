/** 
 * @file class1\deferred\pbralphaF.glsl
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

/*[EXTRA_CODE_HERE]*/

#define DIFFUSE_ALPHA_MODE_NONE     0
#define DIFFUSE_ALPHA_MODE_BLEND    1
#define DIFFUSE_ALPHA_MODE_MASK     2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
uniform sampler2D specularMap; // PBR: Packed: Occlusion, Metal, Roughness

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
uniform sampler2DRect lightMap;
#endif

uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
  #ifdef DEFINE_GL_FRAGCOLOR
    out vec4 frag_color;
  #else
    #define frag_color gl_FragColor
  #endif
#else
  #ifdef DEFINE_GL_FRAGCOLOR
    out vec4 frag_data[4];
  #else
    #define frag_data gl_FragData
  #endif
#endif

#ifdef HAS_SHADOW
  VARYING vec3 vary_fragcoord;
  uniform vec2 screen_res;
#endif

VARYING vec3 vary_position;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec3 vary_normal;
VARYING vec3 vary_tangent;
flat in float vary_sign;


#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

// Lights
// See: LLRender::syncLightState()
uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];
uniform vec2 light_deferred_attenuation[8]; // light size and falloff

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

// These are in deferredUtil.glsl but we can't set: mFeatures.isDeferred to include it
vec3 BRDFLambertian( vec3 reflect0, vec3 reflect90, vec3 c_diff, float specWeight, float vh );
vec3 BRDFSpecularGGX( vec3 reflect0, vec3 reflect90, float alphaRoughness, float specWeight, float vh, float nl, float nv, float nh );
void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3 scaleSoftClipFrag(vec3 l);

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
vec2 getGGX( vec2 brdfPoint );
void initMaterial( vec3 diffuse, vec3 packedORM,
        out float alphaRough, out vec3 c_diff, out vec3 reflect0, out vec3 reflect90, out float specWeight );
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyEnv, 
        vec3 pos, vec3 norm, float glossiness, float envIntensity);

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

// lp = light position
// la = linear attenuation, light radius
// fa = falloff
// See: LLRender::syncLightState()
vec3 calcPointLightOrSpotLight(vec3 reflect0, vec3 reflect90, float alphaRough, vec3 c_diff,
    vec3 lightColor, vec3 diffuse, vec3 p, vec3 v, vec3 n, vec4 lp, vec3 ln,
    float lightSize, float lightFalloff, float is_pointlight, float ambiance)
{
    vec3 intensity = vec3(0);

    vec3 lv = lp.xyz - p;
    vec3  h, l;
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(lv,n,v,h,l,nh,nl,nv,vh,lightDist);

    float dist = lightDist/lightSize;

    if (dist <= 1.0 && nl > 0.0)
    {
        float dist_atten = calcLegacyDistanceAttenuation(dist,lightFalloff);

        float specWeight = 1.0;

        lv = normalize(lv);
        float spot = max(dot(-ln, lv), is_pointlight);
        nl *= spot * spot;

        if (nl > 0.0)
        {
            vec3 color = vec3(0);
            intensity = dist_atten * nl * lightColor; 
            color += intensity * BRDFLambertian(reflect0, reflect90, c_diff, specWeight, vh);
            color += intensity * BRDFSpecularGGX(reflect0, reflect90, alphaRough, specWeight, vh, nl, nv, nh);
            return color;
        }
    }
    return intensity;
}

void main()
{
    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    vec3  pos         = vary_position;

    float scol = 1.0;
    float ambocc = 1.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVars(pos.xyz, light_dir, ambocc, sunlit, amblit, additive, atten, true);

// IF .mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
//    vec3 col = vertex_color.rgb * diffuseLookup(vary_texcoord0.xy).rgb;
// else
    vec4 albedo = texture2D(diffuseMap, vary_texcoord0.xy).rgba;
    albedo.rgb = srgb_to_linear(albedo.rgb);
#ifdef HAS_ALPHA_MASK
    if (albedo.a < minimum_alpha)
    {
        discard;
    }
#endif

    vec3 base = vertex_color.rgb * albedo.rgb;

    vec3 vNt = texture2D(bumpMap, vary_texcoord1.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;
    
    vec3 vB = sign * cross(vN, vT);
    vec3 norm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    norm *= gl_FrontFacing ? 1.0 : -1.0;

#ifdef HAS_SHADOW
    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
    frag *= screen_res;
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerFetchedTexture::sWhiteImagep since roughnessFactor and metallicFactor are multiplied in
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 packedORM = texture2D(specularMap, vary_texcoord2.xy).rgb;  // PBR linear packed Occlusion, Roughness, Metal. See: lldrawpoolapha.cpp

    packedORM.g *= roughnessFactor;
    packedORM.b *= metallicFactor;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture2D(emissiveMap, vary_texcoord0.xy).rgb);

    vec3 colorDiffuse     = vec3(0);
    vec3 colorSpec        = vec3(0);

    float IOR             = 1.5;         // default Index Of Refraction 1.5 (dielectrics)
    float ao              = packedORM.r;
    float perceptualRough = packedORM.g;
    float metal           = packedORM.b;

    vec3  v               = -normalize(vary_position.xyz);
    vec3  n               = norm.xyz;

    vec3  h, l;
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(light_dir, n, v, h, l, nh, nl, nv, vh, lightDist);

    vec3 c_diff, reflect0, reflect90;
    float alphaRough, specWeight;
    initMaterial( base, packedORM, alphaRough, c_diff, reflect0, reflect90, specWeight );

    float gloss      = 1.0 - perceptualRough;
    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);
    vec3  legacyenv = vec3(0);
    sampleReflectionProbes(irradiance, radiance, legacyenv, pos.xyz, norm.xyz, gloss, 0.0);
    irradiance       = max(amblit,irradiance) * ambocc;

    pbrIbl(colorDiffuse, colorSpec, radiance, irradiance, ao, nv, perceptualRough, gloss, reflect0, c_diff);
    
    // Sun/Moon Lighting
    if (nl > 0.0 || nv > 0.0)
    {
        pbrDirectionalLight(colorDiffuse, colorSpec, srgb_to_linear(sunlit), scol, reflect0, reflect90, c_diff, alphaRough, vh, nl, nv, nh);
    }

    vec3 col = colorDiffuse + colorEmissive + colorSpec;
    
    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += calcPointLightOrSpotLight( reflect0, reflect90, alphaRough, c_diff, light_diffuse[i].rgb, base.rgb, pos.xyz, v, n, light_position[i], light_direction[i].xyz, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w );

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    col.rgb += light.rgb;

    col.rgb = linear_to_srgb(col.rgb);
    col *= atten.r;
    col += 2.0*additive;
    col  = scaleSoftClipFrag(col);

    frag_color = vec4(col,albedo.a * vertex_color.a);
}
