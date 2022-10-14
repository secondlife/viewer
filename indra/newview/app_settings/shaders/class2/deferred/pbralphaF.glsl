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

out vec4 frag_color;

#ifdef HAS_SUN_SHADOW
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

void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv, 
        vec3 pos, vec3 norm, float glossiness);

void waterClip(vec3 pos);

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

vec3 calcPointLightOrSpotLight(vec3 diffuseColor, vec3 specularColor, 
                    float perceptualRoughness, 
                    float metallic,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize, float falloff, float is_pointlight, float ambiance)
{
    vec3 color = vec3(0,0,0);

    vec3 lv = lp.xyz - p;

    float lightDist = length(lv);

    float dist = lightDist / lightSize;
    if (dist <= 1.0)
    {
        lv /= lightDist;

        float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);

        // spotlight coefficient.
        float spot = max(dot(-ld, lv), is_pointlight);
        // spot*spot => GL_SPOT_EXPONENT=2
        float spot_atten = spot*spot;

        vec3 intensity = spot_atten * dist_atten * lightColor * 3.0;

        color = intensity*pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, n.xyz, v, lv);
    }

    return color;
}

void main()
{
    vec3 color = vec3(0,0,0);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    vec3  pos         = vary_position;

    waterClip(pos);

// IF .mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
//    vec3 col = vertex_color.rgb * diffuseLookup(vary_texcoord0.xy).rgb;
// else
    vec4 albedo = texture(diffuseMap, vary_texcoord0.xy).rgba;
    albedo.rgb = srgb_to_linear(albedo.rgb);
#ifdef HAS_ALPHA_MASK
    if (albedo.a < minimum_alpha)
    {
        discard;
    }
#endif

    vec3 baseColor = vertex_color.rgb * albedo.rgb;

    vec3 vNt = texture(bumpMap, vary_texcoord1.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;
    
    vec3 vB = sign * cross(vN, vT);
    vec3 norm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    norm *= gl_FrontFacing ? 1.0 : -1.0;

    float scol = 1.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, norm, light_dir, sunlit, amblit, additive, atten);

#ifdef HAS_SUN_SHADOW
    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
    frag *= screen_res;
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    vec3 orm = texture(specularMap, vary_texcoord2.xy).rgb; //orm is packed into "emissiveRect" to keep the data in linear color space

    float perceptualRoughness = orm.g * roughnessFactor;
    float metallic = orm.b * metallicFactor;
    float ao = orm.r;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture2D(emissiveMap, vary_texcoord0.xy).rgb);

    // PBR IBL
    float gloss      = 1.0 - perceptualRoughness;
    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);
    sampleReflectionProbes(irradiance, radiance, pos.xyz, norm.xyz, gloss);
    // Take maximium of legacy ambient vs irradiance sample as irradiance
    // NOTE: ao is applied in pbrIbl (see pbrBaseLight), do not apply here
    irradiance       = max(amblit,irradiance);

    vec3 diffuseColor;
    vec3 specularColor;
    calcDiffuseSpecular(baseColor.rgb, metallic, diffuseColor, specularColor);

    vec3 v = -normalize(pos.xyz);
    color = pbrBaseLight(diffuseColor, specularColor, metallic, v, norm.xyz, perceptualRoughness, light_dir, sunlit, scol, radiance, irradiance, colorEmissive, ao, additive, atten);

    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += calcPointLightOrSpotLight(diffuseColor, specularColor, perceptualRoughness, metallic, norm.xyz, pos.xyz, v, light_position[i].xyz, light_direction[i].xyz, light_diffuse[i].rgb, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    color.rgb += light.rgb;

    
    frag_color = vec4(color.rgb,albedo.a * vertex_color.a);
}
