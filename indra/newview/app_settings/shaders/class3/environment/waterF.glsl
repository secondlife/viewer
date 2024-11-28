/**
 * @file waterF.glsl
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

// class3/environment/waterF.glsl

out vec4 frag_color;

#ifdef HAS_SUN_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif

vec3 scaleSoftClipFragLinear(vec3 l);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applyWaterFogViewLinear(vec3 pos, vec4 color);

void mirrorClip(vec3 pos);

// PBR interface
vec2 BRDF(float NoV, float roughness);

void calcDiffuseSpecular(vec3 baseColor, float metallic, inout vec3 diffuseColor, inout vec3 specularColor);

void pbrIbl(vec3 diffuseColor,
    vec3 specularColor,
    vec3 radiance, // radiance map sample
    vec3 irradiance, // irradiance map sample
    float ao,       // ambient occlusion factor
    float nv,       // normal dot view vector
    float perceptualRoughness,
    out vec3 diffuse,
    out vec3 specular);

void pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l, // surface point to light
                    out float nl,
                    out vec3 diff,
                    out vec3 spec);

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

uniform sampler2D bumpMap;
uniform sampler2D bumpMap2;
uniform float     blend_factor;
#ifdef TRANSPARENT_WATER
uniform sampler2D screenTex;
uniform sampler2D depthMap;
#endif

uniform sampler2D refTex;

uniform float sunAngle;
uniform float sunAngle2;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform float refScale;
uniform float kd;
uniform vec2 screenRes;
uniform vec3 normScale;
uniform float fresnelScale;
uniform float fresnelOffset;
uniform float blurMultiplier;
uniform vec4 waterFogColor;
uniform vec3 waterFogColorLinear;


//bigWave is (refCoord.w, view.w);
in vec4 refCoord;
in vec4 littleWave;
in vec4 view;
in vec3 vary_position;
in vec3 vary_normal;
in vec3 vary_tangent;
in vec3 vary_light_dir;

vec3 BlendNormal(vec3 bump1, vec3 bump2)
{
    vec3 n = mix(bump1, bump2, blend_factor);
    return n;
}

vec3 srgb_to_linear(vec3 col);
vec3 linear_to_srgb(vec3 col);

vec3 vN, vT, vB;

vec3 transform_normal(vec3 vNt)
{
    return normalize(vNt.x * vT + vNt.y * vB + vNt.z * vN);
}

void sampleReflectionProbesWater(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, vec3 amblit_linear);

vec3 getPositionWithNDC(vec3 ndc);

void main()
{
    mirrorClip(vary_position);
    vN = vary_normal;
    vT = vary_tangent;
    vB = cross(vN, vT);

    vec3 pos = vary_position.xyz;

    float dist = length(pos.xyz);

    //normalize view vector
    vec3 viewVec = normalize(pos.xyz);

    //get wave normals
    vec2 bigwave = vec2(refCoord.w, view.w);
    vec3 wave1_a = texture(bumpMap, bigwave, -2      ).xyz*2.0-1.0;
    vec3 wave2_a = texture(bumpMap, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_a = texture(bumpMap, littleWave.zw).xyz*2.0-1.0;

    vec3 wave1_b = texture(bumpMap2, bigwave      ).xyz*2.0-1.0;
    vec3 wave2_b = texture(bumpMap2, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_b = texture(bumpMap2, littleWave.zw).xyz*2.0-1.0;

    //wave1_a = wave2_a = wave3_a = wave1_b = wave2_b = wave3_b = vec3(0,0,1);

    vec3 wave1 = BlendNormal(wave1_a, wave1_b);
    vec3 wave2 = BlendNormal(wave2_a, wave2_b);
    vec3 wave3 = BlendNormal(wave3_a, wave3_b);

    vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;

    //wave1 = transform_normal(wave1);
    //wave2 = transform_normal(wave2);
    //wave3 = transform_normal(wave3);

    vec3 wavef = (wave1 + wave2 * 0.4 + wave3 * 0.6) * 0.5;

    vec3 waver = wavef*3;

    vec3 up = transform_normal(vec3(0,0,1));
    float vdu = -dot(viewVec, up)*2;

    vec3 wave_ibl = wavef;
    wave_ibl.z *= 2.0;
    wave_ibl = transform_normal(normalize(wave_ibl));

    vec3 norm = transform_normal(normalize(wavef));

    vdu = clamp(vdu, 0, 1);
    wavef.z *= max(vdu*vdu*vdu, 0.1);

    wavef = normalize(wavef);

    //wavef = vec3(0, 0, 1);
    wavef = transform_normal(wavef);

    float dist2 = dist;
    dist = max(dist, 5.0);

    float dmod = sqrt(dist);

    //figure out distortion vector (ripply)
    vec2 distort2 = distort + waver.xy * refScale / max(dmod, 1.0);

    distort2 = clamp(distort2, vec2(0), vec2(0.999));

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    float shadow = 1.0f;

#ifdef HAS_SUN_SHADOW
    shadow = sampleDirectionalShadow(pos.xyz, norm.xyz, distort);
#endif

    calcAtmosphericVarsLinear(pos.xyz, wavef, vary_light_dir, sunlit, amblit, additive, atten);

    vec3 sunlit_linear = srgb_to_linear(sunlit);

#ifdef TRANSPARENT_WATER
    vec4 fb = texture(screenTex, distort2);
    float depth = texture(depthMap, distort2).r;
    vec3 refPos = getPositionWithNDC(vec3(distort2*2.0-vec2(1.0), depth*2.0-1.0));

    if (refPos.z > pos.z-0.05)
    {
        //we sampled an above water sample, don't distort
        distort2 = distort;
        fb = texture(screenTex, distort2);
        depth = texture(depthMap, distort2).r;
        refPos = getPositionWithNDC(vec3(distort2 * 2.0 - vec2(1.0), depth * 2.0 - 1.0));
    }

#else
    vec4 fb = applyWaterFogViewLinear(viewVec*2048.0, vec4(1.0));
#endif

    // fudge sample on other side of water to be a tad darker
    fb.rgb *= 0.75;

    float metallic = 0.0;
    float perceptualRoughness = 0.05;
    float gloss      = 1.0 - perceptualRoughness;

    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);
    sampleReflectionProbesWater(irradiance, radiance, distort2, pos.xyz, wave_ibl.xyz, gloss, amblit);

    irradiance       = vec3(0);

    vec3 diffuseColor = vec3(0);
    vec3 specularColor = vec3(0);
    calcDiffuseSpecular(vec3(1), metallic, diffuseColor, specularColor);

    vec3 v = -normalize(pos.xyz);

    vec3 colorEmissive = vec3(0);
    float ao = 1.0;
    vec3 light_dir = transform_normal(lightDir);

    perceptualRoughness = 0.0;
    metallic = 1.0;

    float NdotV = clamp(abs(dot(norm, v)), 0.001, 1.0);

    float nl = 0;
    vec3 diffPunc = vec3(0);
    vec3 specPunc = vec3(0);

    pbrPunctual(vec3(0), specularColor, 0.1, metallic, normalize(wavef+up*max(dist, 32.0)/32.0*(1.0-vdu)), v, normalize(light_dir), nl, diffPunc, specPunc);

    vec3 punctual = clamp(nl * (diffPunc + specPunc), vec3(0), vec3(10));

    vec3 color = punctual * sunlit_linear * 2.75 * shadow;
    vec3 iblDiff;
    vec3 iblSpec;
    pbrIbl(vec3(0), vec3(1), radiance, vec3(0), ao, NdotV, 0.0, iblDiff, iblSpec);

    color += iblDiff + iblSpec;

    float nv = clamp(abs(dot(norm.xyz, v)), 0.001, 1.0);
    vec2 brdf = BRDF(clamp(nv, 0, 1), 1.0);
    float f = 1.0-brdf.y; //1.0 - (brdf.x+brdf.y);
    f *= 0.9;
    f *= f;

    // incoming scale is [0, 1] with 0.5 being default
    // shift to 0.5 to 1.5
    f *= (fresnelScale - 0.5)+1.0;

    // incoming offset is [0, 1] with 0.5 being default
    // shift from -1 to 1
    f += (fresnelOffset - 0.5) * 2.0;

    f = clamp(f, 0, 1);

    color = ((1.0 - f) * color) + fb.rgb;

    float spec = min(max(max(punctual.r, punctual.g), punctual.b), 0.05);

    frag_color = max(vec4(color, spec), vec4(0));
}

