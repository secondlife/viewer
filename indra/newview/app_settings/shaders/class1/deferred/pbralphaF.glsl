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

#define PBR_USE_IBL             1
#define PBR_USE_SUN             1
#define PBR_USE_IRRADIANCE_HACK 1

#define DIFFUSE_ALPHA_MODE_NONE     0
#define DIFFUSE_ALPHA_MODE_BLEND    1
#define DIFFUSE_ALPHA_MODE_MASK     2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

#define DEBUG_PBR_LIGHT_TYPE 0 // Output Diffuse=0.75, Emissive=0, ORM=0,0,0

#define DEBUG_BASIC         0
#define DEBUG_VERTEX        0
#define DEBUG_NORMAL_MAP    0 // Output packed normal map "as is" to diffuse
#define DEBUG_NORMAL_OUT    0 // Output unpacked normal to diffuse
#define DEBUG_ORM           0 // Output Occlusion Roughness Metal "as is" to diffuse
#define DEBUG_POSITION      0

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

uniform samplerCube environmentMap;
uniform mat3        env_mat;
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

#ifdef HAS_NORMAL_MAP
VARYING vec3 vary_normal;
VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;
VARYING vec2 vary_texcoord1;
#endif

#ifdef HAS_SPECULAR_MAP
    VARYING vec2 vary_texcoord2;
#endif

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

// Lights
// See: LLRender::syncLightState()
uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];

vec2 encode_normal(vec3 n);
vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

// These are in deferredUtil.glsl but we can't set: mFeatures.isDeferred to include it
vec3 BRDFLambertian( vec3 reflect0, vec3 reflect90, vec3 c_diff, float specWeight, float vh );
vec3 BRDFSpecularGGX( vec3 reflect0, vec3 reflect90, float alphaRoughness, float specWeight, float vh, float nl, float nv, float nh );
void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
vec2 getGGX( vec2 brdfPoint );
void initMaterial( vec3 diffuse, vec3 packedORM,
        out float alphaRough, out vec3 c_diff, out vec3 reflect0, out vec3 reflect90, out float specWeight );
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyEnv, 
        vec3 pos, vec3 norm, float glossiness, float envIntensity);

vec3 hue_to_rgb(float hue);

// lp = light position
// la = linear attenuation, light radius
// fa = falloff
// See: LLRender::syncLightState()
vec3 calcPointLightOrSpotLight(vec3 reflect0, vec3 c_diff,
    vec3 lightColor, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln,
    float la, float fa, float is_pointlight, float ambiance)
{
    vec3 intensity = vec3(0);

    vec3 lv = lp.xyz - v;
    vec3  h, l;
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(lv,n,v,h,l,nh,nl,nv,vh,lightDist);

    if (lightDist > 0.0)
    {
        float falloff_factor = (12.0 * fa) - 9.0;
        float inverted_la = falloff_factor / la;

        float dist = lightDist / inverted_la;

        float dist_atten = calcLegacyDistanceAttenuation(dist,fa);
        if (dist_atten <= 0.0)
            return intensity;

        vec3 reflect90 = vec3(1);
        float specWeight = 1.0;

        lv = normalize(lv);
        float spot = max(dot(-ln, lv), is_pointlight);
        nl *= spot * spot;

        if (nl > 0.0)
            intensity = dist_atten * nl * lightColor * BRDFLambertian(reflect0, reflect90, c_diff, specWeight, vh);
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

//    vec3 base = vertex_color.rgb * albedo.rgb * albedo.a;
    vec3 base = vertex_color.rgb * albedo.rgb;

#ifdef HAS_NORMAL_MAP
    vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);
    norm.xyz = normalize(norm.xyz * 2 - 1);

    vec3 tnorm = vec3(dot(norm.xyz,vary_mat0),
                      dot(norm.xyz,vary_mat1),
                      dot(norm.xyz,vary_mat2));
#else
    vec4 norm = vec4(0,0,0,1.0);
//    vec3 tnorm = vary_normal;
    vec3 tnorm = vec3(0,0,1);
#endif

    tnorm = normalize(tnorm.xyz);
    norm.xyz = tnorm.xyz;

#if HAS_SHADOW
    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
    frag *= screen_res;
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerFetchedTexture::sWhiteImagep since roughnessFactor and metallicFactor are multiplied in
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
#ifdef HAS_SPECULAR_MAP
    vec3 packedORM = texture2D(specularMap, vary_texcoord2.xy).rgb;  // PBR linear packed Occlusion, Roughness, Metal. See: lldrawpoolapha.cpp
#else
    vec3 packedORM = vec3(1,0,0);
#endif

    packedORM.g *= roughnessFactor;
    packedORM.b *= metallicFactor;

    vec3 colorEmissive = emissiveColor;
#ifdef HAS_EMISSIVE_MAP
    colorEmissive *= texture2D(emissiveMap, vary_texcoord0.xy).rgb;
#endif

    vec3 colorDiffuse     = vec3(0);
    vec3 colorSpec        = vec3(0);

    float IOR             = 1.5;         // default Index Of Refraction 1.5 (dielectrics)
    float ao              = packedORM.r;
    float perceptualRough = packedORM.g;
    float metal           = packedORM.b;

    vec3  v               = -normalize(vary_position.xyz);
    vec3  n               = norm.xyz;
    vec3  t               = vec3(1,0,0);
    vec3  b               = normalize(cross(n,t));
    vec3  reflectVN       = normalize(reflect(-v,n));

    vec3  h, l;
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(light_dir, n, v, h, l, nh, nl, nv, vh, lightDist);

    vec3 c_diff, reflect0, reflect90;
    float alphaRough, specWeight;
    initMaterial( base, packedORM, alphaRough, c_diff, reflect0, reflect90, specWeight );

    // Common to RadianceGGX and RadianceLambertian
    vec2  brdfPoint  = clamp(vec2(nv, perceptualRough), vec2(0,0), vec2(1,1));
    vec2  vScaleBias = getGGX( brdfPoint); // Environment BRDF: scale and bias applied to reflect0
    vec3  fresnelR   = max(vec3(1.0 - perceptualRough), reflect0) - reflect0; // roughness dependent fresnel
    vec3  kSpec      = reflect0 + fresnelR*pow(1.0 - nv, 5.0);

    vec3 legacyenv;

    vec3  irradiance = vec3(0);
    vec3  specLight  = vec3(0);
    float gloss      = 1.0 - perceptualRough;
    sampleReflectionProbes(irradiance, specLight, legacyenv, pos.xyz, norm.xyz, gloss, 0.0);
#if PBR_USE_IRRADIANCE_HACK
        irradiance       = max(amblit,irradiance) * ambocc;
#else
irradiance = vec3(amblit);
#endif

    vec3 FssEssGGX   = kSpec*vScaleBias.x + vScaleBias.y;
#if PBR_USE_IBL
    colorSpec       += specWeight * specLight * FssEssGGX;
#endif

    vec3  FssEssLambert = specWeight * kSpec * vScaleBias.x + vScaleBias.y; // NOTE: Very similar to FssEssRadiance but with extra specWeight term
    float Ems           = 1.0 - (vScaleBias.x + vScaleBias.y);
    vec3  avg           = specWeight * (reflect0 + (1.0 - reflect0) / 21.0);
    vec3  AvgEms        = avg * Ems;
    vec3  FmsEms        = AvgEms * FssEssLambert / (1.0 - AvgEms);
    vec3  kDiffuse      = c_diff * (1.0 - FssEssLambert + FmsEms);
#if PBR_USE_IBL
    colorDiffuse       += (FmsEms + kDiffuse) * irradiance;
#endif

    colorDiffuse *= ao;
    colorSpec    *= ao;

    // Sun/Moon Lighting
    if (nl > 0.0 || nv > 0.0)
    {
        float scale = 4.9;
        vec3 sunColor = srgb_to_linear(sunlit * scale); // NOTE: Midday should have strong sunlight

        // scol = sun shadow
        vec3 intensity  = ambocc * sunColor * nl * scol;
        vec3 sunDiffuse = intensity * BRDFLambertian (reflect0, reflect90, c_diff    , specWeight, vh);
        vec3 sunSpec    = intensity * BRDFSpecularGGX(reflect0, reflect90, alphaRough, specWeight, vh, nl, nv, nh);
#if PBR_USE_SUN
             colorDiffuse += sunDiffuse;
             colorSpec    += sunSpec;
#endif
        }
    vec3 col = colorDiffuse + colorEmissive + colorSpec;
    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += srgb_to_linear(vec3(scol)) * calcPointLightOrSpotLight( reflect0, c_diff, srgb_to_linear(2.2*light_diffuse[i].rgb), albedo.rgb, pos.xyz, n, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w );

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

#if !defined(LOCAL_LIGHT_KILL)
    col += light;
#endif // !defined(LOCAL_LIGHT_KILL)

#if DEBUG_PBR_LIGHT_TYPE
    col.rgb  = vec3(0.75);
    emissive = vec3(0);
    spec.rgb = vec3(0);
#endif
#if DEBUG_BASIC
    col.rgb = vec3( 1, 0, 1 );
#endif
#if DEBUG_VERTEX
    col.rgb = vertex_color.rgb;
#endif
#if DEBUG_NORMAL_MAP
    col.rgb = texture2D(bumpMap, vary_texcoord1.xy).rgb;
#endif
#if DEBUG_NORMAL_OUT
    col.rgb = vary_normal;
#endif
#if DEBUG_ORM
    col.rgb = linear_to_srgb(spec);
#endif
#if DEBUG_POSITION
    col.rgb = vary_position.xyz;
#endif

//    col.rgb = linear_to_srgb(col.rgb);
//    frag_color = vec4(albedo.rgb,albedo.a);
//    frag_color = vec4(base.rgb,albedo.a);
//    frag_color = vec4(irradiance,albedo.a);
//    frag_color = vec4(colorDiffuse,albedo.a);
//    frag_color = vec4(colorEmissive,albedo.a);
//    frag_color = vec4(sun_dir,albedo.a);
//    frag_color = vec4(sunlit,albedo.a);
    col = linear_to_srgb(col.rgb);
    frag_color = vec4(col,albedo.a);
}
