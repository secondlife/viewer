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

#define PBR_USE_GGX_APPROX         1
#define PBR_USE_GGX_EMS_HACK       1
#define PBR_USE_IRRADIANCE_HACK    1


#define DEBUG_PBR_PACKORM0         0 // Rough=0, Metal=0
#define DEBUG_PBR_PACKORM1         0 // Rough=1, Metal=1
#define DEBUG_PBR_TANGENT1         1 // Tangent = 1,0,0
#define DEBUG_PBR_VERT2CAM1        0 // vertex2camera = 0,0,1

// Pass input through "as is"
#define DEBUG_PBR_DIFFUSE_MAP      0 // Output: use diffuse in G-Buffer
#define DEBUG_PBR_EMISSIVE         0 // Output: Emissive
#define DEBUG_PBR_METAL            0 // Output: grayscale Metal map
#define DEBUG_PBR_NORMAL_MAP       0 // Output: Normal -- also need to set DEBUG_NORMAL_MAP in pbropaqueF
#define DEBUG_PBR_OCCLUSION        0 // Output: grayscale Occlusion map
#define DEBUG_PBR_ORM              0 // Output: Packed Occlusion Roughness Metal
#define DEBUG_PBR_ROUGH_PERCEPTUAL 0 // Output: grayscale Perceptual Roughness map
#define DEBUG_PBR_ROUGH_ALPHA      0 // Output: grayscale Alpha Roughness

#define DEBUG_PBR_TANGENT          0 // Output: Tangent
#define DEBUG_PBR_BITANGENT        0 // Output: Bitangent
#define DEBUG_PBR_DOT_BV           0 // Output: graysacle dot(Bitangent,Vertex2Camera)
#define DEBUG_PBR_DOT_TV           0 // Output: grayscale dot(Tangent  ,Vertex2Camera)

// IBL Spec
#define DEBUG_PBR_NORMAL           0 // Output: passed in normal
#define DEBUG_PBR_V2C_RAW          0 // Output: vertex2camera
#define DEBUG_PBR_DOT_NV           0 // Output: grayscale dot(Normal   ,Vertex2Camera)
#define DEBUG_PBR_BRDF_UV          0 // Output: red green BRDF UV         (GGX input)
#define DEBUG_PBR_BRDF_SCALE_BIAS  0 // Output: red green BRDF Scale Bias (GGX output)
#define DEBUG_PBR_FRESNEL          0 // Output: roughness dependent fresnel
#define DEBUG_PBR_KSPEC            0 // Output: K spec
#define DEBUG_PBR_REFLECTION_DIR   0 // Output: reflection dir
#define DEBUG_PBR_SPEC_REFLECTION  0 // Output: environment reflection
#define DEBUG_PBR_FSS_ESS_GGX      0 // Output: FssEssGGX
#define DEBUG_PBR_SPEC             0 // Output: Final spec

// IBL Diffuse
#define DEBUG_PBR_DIFFUSE_C        0 // Output: diffuse non metal mix
#define DEBUG_PBR_IRRADIANCE_RAW   0 // Output: Diffuse Irradiance pre-mix
#define DEBUG_PBR_IRRADIANCE       0 // Output: Diffuse Irradiance
#define DEBUG_PBR_FSS_ESS_LAMBERT  0 // Output: FssEssLambert
#define DEBUG_PBR_EMS              0 // Output: Ems
#define DEBUG_PBR_AVG              0 // Output: Avg
#define DEBUG_PBR_FMS_EMS          0 // Output: FmsEms
#define DEBUG_PBR_DIFFUSE_K        0 // Output: diffuse FssEssLambert + FmsEms
#define DEBUG_PBR_DIFFUSE_PRE_AO   0 // Output: diffuse pre AO
#define DEBUG_PBR_DIFFUSE          0 // Output: diffuse post AO

// Atmospheric Lighting
#define DEBUG_PBR_AMBOCC           0 // Output: ambient occlusion
#define DEBUG_PBR_DIRECT_AMBIENT   0 // Output: da
#define DEBUG_PBR_SUN_LIT          0 // Ouput: sunlit
#define DEBUG_PBR_SUN_CONTRIB      0 // Output: sun_contrib
#define DEBUG_PBR_SKY_ADDITIVE     0 // Output: additive
#define DEBUG_PBR_SKY_ATTEN        0 // Output: atten

#define DEBUG_PBR_IOR              0 // Output: grayscale IOR
#define DEBUG_PBR_REFLECT0_BASE    0 // Output: black reflect0 default from ior
#define DEBUG_PBR_REFLECT0_MIX     0 // Output: diffuse reflect0 calculated from ior
#define DEBUG_PBR_REFLECTANCE      0 // Output: diffuse reflectance -- NOT USED
#define DEBUG_PBR_SPEC_WEIGHT      0 // Output: specWeight
#define DEBUG_PBR_V2C_REMAP        0 // Output: vertex2camera (remap [-1,1] -> [0,1])
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
uniform sampler2DRect emissiveRect;

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
float getAmbientClamp();
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

uniform vec3 view_dir; // PBR

// Approximate Environment BRDF
vec2 getGGXApprox( vec2 uv )
{
    vec2  st    = vec2(1.) - uv;
    float d     = (st.x * st.x * 0.5) * (st.y * st.y);
    float scale = 1.0 - d;
    float bias  = d;
    return vec2( scale, bias );
}

vec2 getGGX( vec2 brdfPoint )
{
    // TODO: use GGXLUT
    // texture2D(GGXLUT, brdfPoint).rg;
#if PBR_USE_GGX_APPROX
    return getGGXApprox( brdfPoint);
#endif
}

vec3 calcBaseReflect0(float ior)
{
    vec3   reflect0 = vec3(pow((ior - 1.0) / (ior + 1.0), 2.0));
    return reflect0;
}

void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = texture2DRect(depthMap, tc.xy).r;
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = getNorm(tc);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
    float light_gamma = 1.0 / 1.3;
    da                = pow(da, light_gamma);

    vec4 diffuse     = texture2DRect(diffuseRect, tc);
    vec4 spec        = texture2DRect(specularRect, vary_fragcoord.xy);


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

    //vec3 amb_vec = env_mat * norm.xyz;

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
//      vec3 colorClearCoat    = vec3(0);
//      vec3 colorSheen        = vec3(0);
//      vec3 colorTransmission = vec3(0);

        vec3 packedORM        = texture2DRect(emissiveRect, tc).rgb; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
#if DEBUG_PBR_PACK_ORM0
             packedORM        = vec3(0,0,0);
#endif
#if DEBUG_PBR_PACK_ORM1
             packedORM        = vec3(1,1,1);
#endif
        float IOR             = 1.5;         // default Index Of Refraction 1.5 (dielectrics)
        vec3  reflect0        = vec3(0.04);  // -> incidence reflectance 0.04
#if HAS_IOR
              reflect0        = calcBaseReflect0(IOR);
#endif
#if DEBUG_PBR_REFLECT0_BASE
        vec3  debug_reflect0  = reflect0;
#endif

        float metal      = packedORM.b;
        vec3  c_diff     = mix(diffuse.rgb,vec3(0),metal);
        vec3  reflect90  = vec3(0);
        vec3  v          = -normalize(pos.xyz);
#if DEBUG_PBR_VERT2CAM1
              v = vec3(0,0,1);
#endif
        vec3  n          = norm.xyz;
//      vec3  t          = texture2DRect(tangentMap, tc).rgb;
#if DEBUG_PBR_TANGENT1
        vec3  t          = vec3(1,0,0);
#endif
        vec3  b          = cross( n,t);
        vec3  reflectVN  = normalize(reflect(-v,n));

        float dotNV = clamp(dot(n,v),0,1);
        float dotTV = clamp(dot(t,v),0,1);
        float dotBV = clamp(dot(b,v),0,1);

        // Reference: getMetallicRoughnessInfo
        float perceptualRough = packedORM.g;
        float alphaRough      = perceptualRough * perceptualRough;
        vec3  colorDiff       = mix( diffuse.rgb, vec3(0)    , metal);
              reflect0        = mix( reflect0   , diffuse.rgb, metal); // reflect at 0 degrees
              reflect90       = vec3(1);                               // reflect at 90 degrees
#if DEBUG_PBR_REFLECTANCE
        float reflectance     = max( max( reflect0.r, reflect0.g ), reflect0.b );
#endif

        // Common to RadianceGGX and RadianceLambertian
        float specWeight = 1.0;
        vec2  brdfPoint  = clamp(vec2(dotNV, perceptualRough), vec2(0,0), vec2(1,1));
        vec2  vScaleBias = getGGX( brdfPoint); // Environment BRDF: scale and bias applied to reflect0
        vec3  fresnelR   = max(vec3(1.0 - perceptualRough), reflect0) - reflect0; // roughness dependent fresnel
        vec3  kSpec      = reflect0 + fresnelR*pow(1.0 - dotNV, 5.0);

        // Reference: getIBLRadianceGGX
        // https://forum.substance3d.com/index.php?topic=3243.0
        // Glossiness
        // This map is the inverse of the roughness map.
        vec3  irradiance = vec3(0);
        vec3  specLight  = vec3(0);
        float gloss      = 1.0 - perceptualRough;
        sampleReflectionProbes(irradiance, specLight, legacyenv, pos.xyz, norm.xyz, gloss, 0.0);
#if DEBUG_PBR_IRRADIANCE_RAW
        vec3 debug_irradiance = irradiance;
#endif
        irradiance       = max(amblit,irradiance);
#if PBR_USE_IRRADIANCE_HACK
        irradiance      += amblit*0.5*vec3(dot(n, light_dir));
#endif
        specLight        = srgb_to_linear(specLight);
#if HAS_IBL
        kSpec          = mix( kSpec, iridescenceFresnel, iridescenceFactor);
#endif
        vec3 FssEssGGX = kSpec*vScaleBias.x + vScaleBias.y;
        colorSpec += specWeight * specLight * FssEssGGX;

        // Reference: getIBLRadianceLambertian
        vec3  FssEssLambert = specWeight * kSpec * vScaleBias.x + vScaleBias.y; // NOTE: Very similar to FssEssRadiance but with extra specWeight term
        float Ems           = (1.0 - vScaleBias.x + vScaleBias.y);
#if PBR_USE_GGX_EMS_HACK
              Ems           = alphaRough; // With GGX approximation Ems = 0 so use substitute
#endif
        vec3  avg           = specWeight * (reflect0 + (1.0 - reflect0) / 21.0);
        vec3  AvgEms        = avg * Ems;
        vec3  FmsEms        = AvgEms * FssEssLambert / (1.0 - AvgEms);
        vec3  kDiffuse      = c_diff * (1.0 - FssEssLambert + FmsEms);
        colorDiffuse       += (FmsEms + kDiffuse) * irradiance;
    #if DEBUG_PBR_DIFFUSE_PRE_AO
        vec3 debug_diffuse  = colorDiffuse;
    #endif
        colorDiffuse *= packedORM.r; // Occlusion -- NOTE: pbropaque will need occlusion_strength pre-multiplied into spec.r

        color.rgb = colorDiffuse + colorEmissive + colorSpec;

    #if DEBUG_PBR_DIFFUSE
        color.rgb = colorDiffuse;
    #endif
    #if DEBUG_PBR_EMISSIVE
        color.rgb = colorEmissive;
    #endif
    #if DEBUG_PBR_METAL
        color.rgb = vec3(metal);
    #endif
    #if DEBUG_PBR_NORMAL_MAP
        color.rgb = diffuse.rgb;
    #endif
    #if DEBUG_PBR_OCCLUSION
        color.rgb = vec3(packedORM.r);
    #endif
    #if DEBUG_PBR_ORM
        color.rgb = packedORM;
    #endif
    #if DEBUG_PBR_ROUGH_PERCEPTUAL
        color.rgb = vec3(perceptualRough);
    #endif
    #if DEBUG_PBR_ROUGH_ALPHA
        color.rgb = vec3(alphaRough);
    #endif

    #if DEBUG_PBR_NORMAL
        color.rgb = norm.xyz*0.5 + vec3(0.5);
        color.rgb = srgb_to_linear(color.rgb);
    #endif
    #if DEBUG_PBR_TANGENT
        color.rgb = t;
    #endif
    #if DEBUG_PBR_BITANGENT
        color.rgb = b;
    #endif
    #if DEBUG_PBR_DOT_NV
        color.rgb = vec3(dotNV);
    #endif
    #if DEBUG_PBR_DOT_TV
        color.rgb = vec3(dotTV);
    #endif
    #if DEBUG_PBR_DOT_BV
        color.rgb = vec3(dotBV);
    #endif

    #if DEBUG_PBR_AVG
        color.rgb = avg;
    #endif
    #if DEBUG_PBR_BRDF_UV
        color.rgb = vec3(brdfPoint,0.0);
        color.rgb = linear_to_srgb(color.rgb);
    #endif
    #if DEBUG_PBR_BRDF_SCALE_BIAS
        color.rgb = vec3(vScaleBias,0.0);
    #endif
    #if DEBUG_PBR_DIFFUSE_C
        color.rgb = c_diff;
    #endif
    #if DEBUG_PBR_DIFFUSE_K
        color.rgb = kDiffuse;
    #endif
    #if DEBUG_PBR_DIFFUSE_MAP
        color.rgb = diffuse.rgb;
    #endif
    #if DEBUG_PBR_DIFFUSE_PRE_AO
        color.rgb = debug_diffuse;
    #endif
    #if DEBUG_PBR_EMS
        color.rgb = vec3(Ems);
    #endif
    #if DEBUG_PBR_EMS_AVG
        color.rgb = AvgEms;
    #endif
    #if DEBUG_PBR_FMS_EMS
        color.rgb = FmsEms;
    #endif
    #if DEBUG_PBR_FSS_ESS_GGX
        color.rgb = FssEssGGX; // spec
    #endif
    #if DEBUG_PBR_FSS_ESS_LAMBERT
        color.rgb = FssEssLambert; // diffuse
    #endif
    #if DEBUG_PBR_FRESNEL
        color.rgb = fresnelR;
    #endif
    #if DEBUG_PBR_IOR
        color.rgb = vec3(IOR);
    #endif
    #if DEBUG_PBR_IRRADIANCE_RAW
        color.rgb = debug_irradiance;
    #endif
    #if DEBUG_PBR_IRRADIANCE
        color.rgb = irradiance;
    #endif
    #if DEBUG_PBR_KSPEC
        color.rgb = kSpec;
    #endif
    #if DEBUG_PBR_REFLECT0_BASE
        color.rgb = vec3(debug_reflect0);
    #endif
    #if DEBUG_PBR_REFLECT0_MIX
        color.rgb = vec3(reflect0);
    #endif
    #if DEBUG_PBR_REFLECTANCE
        color.rgb = vec3(reflectance);
    #endif
    #if DEBUG_PBR_REFLECTION_DIR
        color.rgb = reflect(-v, n);  // NOTE: equivalent to normalize(reflect(pos.xyz, norm.xyz));
    #endif
    #if DEBUG_PBR_SPEC
        color.rgb = colorSpec;
    #endif
    #if DEBUG_PBR_SPEC_REFLECTION
        color.rgb = specLight;
    #endif
    #if DEBUG_PBR_SPEC_WEIGHT
        color.rgb = vec3(specWeight);
    #endif

#if DEBUG_PBR_AMBOCC
        color.rgb = vec3(ambocc);
#endif
#if DEBUG_PBR_DIRECT_AMBIENT
        color.rgb = vec3(da);
#endif
#if DEBUG_PBR_SKY_ADDITIVE
        color.rgb = additive;
#endif
#if DEBUG_PBR_SKY_ATTEN
        color.rgb = atten;
#endif

#if DEBUG_PBR_SUN_LIT
        color.rgb = sunlit;
color = srgb_to_linear(color);
#endif
#if DEBUG_PBR_SUN_CONTRIB
        color.rgb = sun_contrib;
#endif
    #if DEBUG_PBR_V2C_RAW
        color.rgb = v;
    #endif
    #if DEBUG_PBR_V2C_REMAP
        color.rgb = v*0.5 + vec3(0.5);
    #endif
        frag_color.rgb = color.rgb; // PBR is done in linear
    }
else
{
    diffuse.rgb = linear_to_srgb(diffuse.rgb); // SL-14035

    sampleReflectionProbes(ambenv, glossenv, legacyenv, pos.xyz, norm.xyz, spec.a, envIntensity);

    amblit = max(ambenv, amblit);
    color.rgb = amblit*ambocc;

    //float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
    //ambient *= 0.5;
    //ambient *= ambient;
    //ambient = (1.0 - ambient);
    //color.rgb *= ambient;

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
        legacyenv *= 0.5*diffuse.a+0.5;;
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
    //color = vec3(ambocc);
    //color = ambenv;
    //color.b = diffuse.a;
    frag_color.rgb = srgb_to_linear(color.rgb);
}
    frag_color.a = bloom;
}
