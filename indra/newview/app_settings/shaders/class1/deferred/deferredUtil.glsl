/** 
 * @file class1/deferred/deferredUtil.glsl
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


/*  Parts of this file are taken from Sascha Willem's Vulkan GLTF refernce implementation
MIT License

Copyright (c) 2018 Sascha Willems

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

uniform sampler2D   normalMap;
uniform sampler2D   depthMap;
uniform sampler2D projectionMap; // rgba
uniform sampler2D brdfLut;

// projected lighted params
uniform mat4 proj_mat; //screen space to light space projector
uniform vec3 proj_n; // projector normal
uniform vec3 proj_p; //plane projection is emitting from (in screen space)
uniform float proj_focus; // distance from plane to begin blurring
uniform float proj_lod  ; // (number of mips in proj map)
uniform float proj_range; // range between near clip and far clip plane of projection
uniform float proj_ambiance;

// light params
uniform vec3 color; // light_color
uniform float size; // light_size

uniform mat4 inv_proj;
uniform vec2 screen_res;

const float M_PI = 3.14159265;
const float ONE_OVER_PI = 0.3183098861;

vec3 srgb_to_linear(vec3 cs);
vec3 atmosFragLightingLinear(vec3 light, vec3 additive, vec3 atten);

float calcLegacyDistanceAttenuation(float distance, float falloff)
{
    float dist_atten = 1.0 - clamp((distance + falloff)/(1.0 + falloff), 0.0, 1.0);
    dist_atten *= dist_atten;

    // Tweak falloff slightly to match pre-EEP attenuation
    // NOTE: this magic number also shows up in a great many other places, search for dist_atten *= to audit
    dist_atten *= 2.0;
    return dist_atten;
}

// In:
//   lv  unnormalized surface to light vector
//   n   normal of the surface
//   pos unnormalized camera to surface vector
// Out:
//   l   normalized surace to light vector
//   nl  diffuse angle
//   nh  specular angle
void calcHalfVectors(vec3 lv, vec3 n, vec3 v,
    out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist)
{
    l  = normalize(lv);
    h  = normalize(l + v);
    nh = clamp(dot(n, h), 0.0, 1.0);
    nl = clamp(dot(n, l), 0.0, 1.0);
    nv = clamp(dot(n, v), 0.0, 1.0);
    vh = clamp(dot(v, h), 0.0, 1.0);

    lightDist = length(lv);
}

// In:
//   light_center
//   pos
// Out:
//   dist
//   l_dist
//   lv
//   proj_tc  Projector Textue Coordinates
bool clipProjectedLightVars(vec3 light_center, vec3 pos, out float dist, out float l_dist, out vec3 lv, out vec4 proj_tc )
{
    lv = light_center - pos.xyz;
    dist = length(lv);
    bool clipped = (dist >= size);
    if ( !clipped )
    {
        dist /= size;

        l_dist = -dot(lv, proj_n);
        vec4 projected_point = (proj_mat * vec4(pos.xyz, 1.0));
        clipped = (projected_point.z < 0.0);
        projected_point.xyz /= projected_point.w;
        proj_tc = projected_point;
    }

    return clipped;
}

vec2 getScreenCoordinate(vec2 screenpos)
{
    vec2 sc = screenpos.xy * 2.0;
    return sc - vec2(1.0, 1.0);
}

// See: https://aras-p.info/texts/CompactNormalStorage.html
//      Method #4: Spheremap Transform, Lambert Azimuthal Equal-Area projection
vec3 getNorm(vec2 screenpos)
{
   vec2 enc = texture(normalMap, screenpos.xy).xy;
   vec2 fenc = enc*4-2;
   float f = dot(fenc,fenc);
   float g = sqrt(1-f/4);
   vec3 n;
   n.xy = fenc*g;
   n.z = 1-f/2;
   return n;
}

vec3 getNormalFromPacked(vec4 packedNormalEnvIntensityFlags)
{
   vec2 enc = packedNormalEnvIntensityFlags.xy;
   vec2 fenc = enc*4-2;
   float f = dot(fenc,fenc);
   float g = sqrt(1-f/4);
   vec3 n;
   n.xy = fenc*g;
   n.z = 1-f/2;
   return normalize(n); // TODO: Is this normalize redundant?
}

// return packedNormalEnvIntensityFlags since GBUFFER_FLAG_HAS_PBR needs .w
// See: C++: addDeferredAttachments(), GLSL: softenLightF
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity)
{
    vec4 packedNormalEnvIntensityFlags = texture(normalMap, screenpos.xy);
    n = getNormalFromPacked( packedNormalEnvIntensityFlags );
    envIntensity = packedNormalEnvIntensityFlags.z;
    return packedNormalEnvIntensityFlags;
}

// get linear depth value given a depth buffer sample d and znear and zfar values
float linearDepth(float d, float znear, float zfar)
{
    d = d * 2.0 - 1.0;
    return znear * 2.0 * zfar / (zfar + znear - d * (zfar - znear));
}

float linearDepth01(float d, float znear, float zfar)
{
    return linearDepth(d, znear, zfar) / zfar;
}

float getDepth(vec2 pos_screen)
{
    float depth = texture(depthMap, pos_screen).r;
    return depth;
}

vec4 getTexture2DLodAmbient(vec2 tc, float lod)
{
#ifndef FXAA_GLSL_120
    vec4 ret = textureLod(projectionMap, tc, lod);
#else
    vec4 ret = texture(projectionMap, tc);
#endif
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = tc-vec2(0.5);
    float d = dot(dist,dist);
    ret *= min(clamp((0.25-d)/0.25, 0.0, 1.0), 1.0);

    return ret;
}

vec4 getTexture2DLodDiffuse(vec2 tc, float lod)
{
#ifndef FXAA_GLSL_120
    vec4 ret = textureLod(projectionMap, tc, lod);
#else
    vec4 ret = texture(projectionMap, tc);
#endif
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc-vec2(0.5));
    float det = min(lod/(proj_lod*0.5), 1.0);
    float d = min(dist.x, dist.y);
    float edge = 0.25*det;
    ret *= clamp(d/edge, 0.0, 1.0);

    return ret;
}

// lit     This is set by the caller: if (nl > 0.0) { lit = attenuation * nl * noise; }
// Uses:
//   color   Projected spotlight color
vec3 getProjectedLightAmbiance(float amb_da, float attenuation, float lit, float nl, float noise, vec2 projected_uv)
{
    vec4 amb_plcol = getTexture2DLodAmbient(projected_uv, proj_lod);
    vec3 amb_rgb   = amb_plcol.rgb * amb_plcol.a;

    amb_da += proj_ambiance;
    amb_da += (nl*nl*0.5+0.5) * proj_ambiance;
    amb_da *= attenuation * noise;
    amb_da = min(amb_da, 1.0-lit);

    return (amb_da * color.rgb * amb_rgb);
}

// Returns projected light in Linear
// Uses global spotlight color:
//  color
// NOTE: projected.a will be pre-multiplied with projected.rgb
vec3 getProjectedLightDiffuseColor(float light_distance, vec2 projected_uv)
{
    float diff = clamp((light_distance - proj_focus)/proj_range, 0.0, 1.0);
    float lod = diff * proj_lod;
    vec4 plcol = getTexture2DLodDiffuse(projected_uv.xy, lod);

    return color.rgb * plcol.rgb * plcol.a;
}

vec4 texture2DLodSpecular(vec2 tc, float lod)
{
#ifndef FXAA_GLSL_120
    vec4 ret = textureLod(projectionMap, tc, lod);
#else
    vec4 ret = texture(projectionMap, tc);
#endif
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc-vec2(0.5));
    float det = min(lod/(proj_lod*0.5), 1.0);
    float d = min(dist.x, dist.y);
    d *= min(1, d * (proj_lod - lod)); // BUG? extra factor compared to diffuse causes N repeats
    float edge = 0.25*det;
    ret *= clamp(d/edge, 0.0, 1.0);

    return ret;
}

// See: clipProjectedLightVars()
vec3 getProjectedLightSpecularColor(vec3 pos, vec3 n )
{
    vec3 slit = vec3(0);
    vec3 ref = reflect(normalize(pos), n);

    //project from point pos in direction ref to plane proj_p, proj_n
    vec3 pdelta = proj_p-pos;
    float l_dist = length(pdelta);
    float ds = dot(ref, proj_n);
    if (ds < 0.0)
    {
        vec3 pfinal = pos + ref * dot(pdelta, proj_n)/ds;
        vec4 stc = (proj_mat * vec4(pfinal.xyz, 1.0));
        if (stc.z > 0.0)
        {
            stc /= stc.w;
            slit = getProjectedLightDiffuseColor( l_dist, stc.xy ); // NOTE: Using diffuse due to texture2DLodSpecular() has extra: d *= min(1, d * (proj_lod - lod));
        }
    }
    return slit; // specular light
}

vec3 getProjectedLightSpecularColor(float light_distance, vec2 projected_uv)
{
    float diff = clamp((light_distance - proj_focus)/proj_range, 0.0, 1.0);
    float lod = diff * proj_lod;
    vec4 plcol = getTexture2DLodDiffuse(projected_uv.xy, lod); // NOTE: Using diffuse due to texture2DLodSpecular() has extra: d *= min(1, d * (proj_lod - lod));

    return color.rgb * plcol.rgb * plcol.a;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

// get position given a normalized device coordinate
vec3 getPositionWithNDC(vec3 ndc)
{
    vec4 pos = inv_proj * vec4(ndc, 1.0);
    return pos.xyz / pos.w;
}

vec4 getPositionWithDepth(vec2 pos_screen, float depth)
{
    vec2 sc = getScreenCoordinate(pos_screen);
    vec3 ndc = vec3(sc.x, sc.y, 2.0*depth-1.0);
    return vec4(getPositionWithNDC(ndc), 1.0);
}

vec2 getScreenCoord(vec4 clip)
{
    vec4 ndc = clip;
         ndc.xyz /= clip.w;
    vec2 screen = vec2( ndc.xy * 0.5 );
         screen += 0.5;
    return screen;
}

vec2 getScreenXY(vec4 clip)
{
    vec4 ndc = clip;
         ndc.xyz /= clip.w;
    vec2 screen = vec2( ndc.xy * 0.5 );
         screen += 0.5;
         screen *= screen_res;
    return screen;
}

// Color utils

vec3 colorize_dot(float x)
{
    if (x > 0.0) return vec3( 0, x, 0 );
    if (x < 0.0) return vec3(-x, 0, 0 );
                 return vec3( 0, 0, 1 );
}

vec3 hue_to_rgb(float hue)
{
    if (hue > 1.0) return vec3(0.5);
    vec3 rgb = abs(hue * 6. - vec3(3, 2, 4)) * vec3(1, -1, -1) + vec3(-1, 2, 2);
    return clamp(rgb, 0.0, 1.0);
}

// PBR Utils

vec2 BRDF(float NoV, float roughness)
{
    return texture(brdfLut, vec2(NoV, roughness)).rg;
}

// set colorDiffuse and colorSpec to the results of GLTF PBR style IBL
vec3 pbrIbl(vec3 diffuseColor,
            vec3 specularColor,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,       // normal dot view vector
            float perceptualRough)
{
    // retrieve a scale and bias to F0. See [1], Figure 3
	vec2 brdf = BRDF(clamp(nv, 0, 1), 1.0-perceptualRough);
	vec3 diffuseLight = irradiance;
	vec3 specularLight = radiance;
    
	vec3 diffuse = diffuseLight * diffuseColor;
	vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

	return (diffuse + specular) * ao;
}


// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
}

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor, 
                    float perceptualRoughness, 
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l) //surface point to light
{
    // make sure specular highlights from punctual lights don't fall off of polished surfaces
    perceptualRoughness = max(perceptualRoughness, 8.0/255.0);
    
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0f;

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	PBRInfo pbrInputs = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);

	// Calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * (diffuseContrib + specContrib);

    return clamp(color, vec3(0), vec3(10));
}

void calcDiffuseSpecular(vec3 baseColor, float metallic, inout vec3 diffuseColor, inout vec3 specularColor)
{
    vec3 f0 = vec3(0.04);
    diffuseColor = baseColor*(vec3(1.0)-f0);
    diffuseColor *= 1.0 - metallic;
    specularColor = mix(f0, baseColor, metallic);
}

vec3 pbrBaseLight(vec3 diffuseColor, vec3 specularColor, float metallic, vec3 v, vec3 norm, float perceptualRoughness, vec3 light_dir, vec3 sunlit, float scol, vec3 radiance, vec3 irradiance, vec3 colorEmissive, float ao, vec3 additive, vec3 atten)
{
    vec3 color = vec3(0);

    float NdotV = clamp(abs(dot(norm, v)), 0.001, 1.0);
    
    color += pbrIbl(diffuseColor, specularColor, radiance, irradiance, ao, NdotV, perceptualRoughness);
    
    color += pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, norm, v, normalize(light_dir)) * sunlit * 3.0 * scol; //magic number to balance with legacy materials

    color += colorEmissive;

    return color;
}

uniform vec4 waterPlane;
uniform float waterSign;

// discard if given position in eye space is on the wrong side of the waterPlane according to waterSign
void waterClip(vec3 pos)
{
    // TODO: make this less branchy
    if (waterSign > 0)
    {
        if ((dot(pos.xyz, waterPlane.xyz) + waterPlane.w) < 0.0)
        {
            discard;
        }
    }
    else
    {
        if ((dot(pos.xyz, waterPlane.xyz) + waterPlane.w) > 0.0)
        {
            discard;
        }
    }

}

