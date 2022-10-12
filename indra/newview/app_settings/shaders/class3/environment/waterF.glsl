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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

vec3 scaleSoftClipFragLinear(vec3 l);
vec3 atmosFragLightingLinear(vec3 light, vec3 additive, vec3 atten);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applyWaterFogViewLinear(vec3 pos, vec4 color);

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

uniform sampler2D bumpMap;
uniform sampler2D bumpMap2;
uniform float     blend_factor;
#ifdef TRANSPARENT_WATER
uniform sampler2D screenTex;
uniform sampler2D screenDepth;
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
VARYING vec4 refCoord;
VARYING vec4 littleWave;
VARYING vec4 view;
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

void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
    vec3 pos, vec3 norm, float glossiness, float envIntensity);

vec3 vN, vT, vB;

vec3 transform_normal(vec3 vNt)
{
    return normalize(vNt.x * vT + vNt.y * vB + vNt.z * vN);
}

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
    vec3 pos, vec3 norm, float glossiness, bool errorCorrect);

vec3 getPositionWithNDC(vec3 ndc);

void main() 
{
	vec4 color;

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
    vec3 wave2_a = texture2D(bumpMap, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_a = texture2D(bumpMap, littleWave.zw).xyz*2.0-1.0;

    vec3 wave1_b = texture(bumpMap2, bigwave      ).xyz*2.0-1.0;
    vec3 wave2_b = texture2D(bumpMap2, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_b = texture2D(bumpMap2, littleWave.zw).xyz*2.0-1.0;

    vec3 wave1 = BlendNormal(wave1_a, wave1_b);
    vec3 wave2 = BlendNormal(wave2_a, wave2_b);
    vec3 wave3 = BlendNormal(wave3_a, wave3_b);

    wave1 = transform_normal(wave1);
    wave2 = transform_normal(wave2);
    wave3 = transform_normal(wave3);

    vec3 wavef = (wave1 + wave2 * 0.4 + wave3 * 0.6) * 0.5;

    //wavef.z *= max(-viewVec.z, 0.1);

    wavef = normalize(wavef);

	//get base fresnel components	
	
	vec3 df = vec3(
					dot(viewVec, wave1),
					dot(viewVec, (wave2 + wave3) * 0.5),
					dot(viewVec, wave3)
				 ) * fresnelScale + fresnelOffset;
		    
	vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
	
	float dist2 = dist;
	dist = max(dist, 5.0);
	
	float dmod = sqrt(dist);
	
	vec2 dmod_scale = vec2(dmod*dmod, dmod);
	
    float df1 = df.x + df.y + df.z;

    //wavef = normalize(wavef - vary_normal);
    //wavef = vary_normal;

    vec3 waver = reflect(viewVec, -wavef)*3;

	//figure out distortion vector (ripply)   
    vec2 distort2 = distort + waver.xy * refScale / max(dmod * df1, 1.0);
    distort2 = clamp(distort2, vec2(0), vec2(0.99));
 
#ifdef TRANSPARENT_WATER
    vec4 fb = texture2D(screenTex, distort2);
    float depth = texture2D(screenDepth, distort2).r;
    vec3 refPos = getPositionWithNDC(vec3(distort2*2.0-vec2(1.0), depth*2.0-1.0));

    if (refPos.z > pos.z-0.05)
    {
        //we sampled an above water sample, don't distort
        distort2 = distort;
        fb = texture2D(screenTex, distort2);
        depth = texture2D(screenDepth, distort2).r;
        refPos = getPositionWithNDC(vec3(distort2 * 2.0 - vec2(1.0), depth * 2.0 - 1.0));
    }

    fb = applyWaterFogViewLinear(refPos, fb);
#else
    vec4 fb = vec4(waterFogColorLinear.rgb, 0.0);
#endif

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVarsLinear(pos.xyz, wavef, vary_light_dir, sunlit, amblit, additive, atten);
    sunlit = vec3(1); // TODO -- figure out why sunlit is breaking at some view angles
    vec3 v = -viewVec;
    float NdotV = clamp(abs(dot(wavef.xyz, v)), 0.001, 1.0);

    float metallic = fresnelOffset * 0.1; // fudge -- use fresnelOffset as metalness
    float roughness = 0.1;
    float gloss = 1.0 - roughness;

    vec3 baseColor = vec3(0.25);
    vec3 f0 = vec3(0.04);
    vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= gloss;

    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

    vec3 refnorm = normalize(wavef + vary_normal);
    //vec3 refnorm = wavef;
    
    vec3 irradiance = vec3(0);
    vec3 radiance = vec3(0);
    sampleReflectionProbes(irradiance, radiance, pos, refnorm, gloss, true);
    radiance *= 0.5;
    irradiance = fb.rgb;

    color.rgb = pbrIbl(diffuseColor, specularColor, radiance, irradiance, gloss, NdotV, 0.0);
    
    // fudge -- for punctual lighting, pretend water is metallic
    diffuseColor = vec3(0);
    specularColor = vec3(1);
    roughness = 0.1;
    float scol = 1.0; // TODO -- incorporate shadow map

    //color.rgb += pbrPunctual(diffuseColor, specularColor, roughness, metallic, wavef, v, vary_light_dir) * sunlit * 2.75 * scol;

    //get specular component
    float spec = clamp(dot(vary_light_dir, (reflect(viewVec, wavef))), 0.0, 1.0);

    //harden specular
    spec = pow(spec, 128.0);

    color.rgb += spec * specular;

	color.rgb = atmosFragLightingLinear(color.rgb, additive, atten);
	color.rgb = scaleSoftClipFragLinear(color.rgb);

    color.a = 0.f;
    //color.rgb = fb.rgb;
    //color.rgb = vec3(depth*depth*depth*depth);
    //color.rgb = srgb_to_linear(normalize(refPos) * 0.5 + 0.5);
    //color.rgb = srgb_to_linear(normalize(pos) * 0.5 + 0.5);
    //color.rgb = srgb_to_linear(wavef * 0.5 + 0.5);

    //color.rgb = radiance;
	frag_color = color;

#if defined(WATER_EDGE)
    gl_FragDepth = 0.9999847f;
#endif
	
}

