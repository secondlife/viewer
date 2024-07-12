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
uniform sampler2D sceneMap;

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

vec4 getNorm(vec2 screenpos)
{
    vec4 norm = texture(normalMap, screenpos.xy);
    norm.xyz = normalize(norm.xyz);
    return norm;
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
            float perceptualRough,
            float transmission,
            vec3 transmission_btdf)
{
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = BRDF(clamp(nv, 0, 1), 1.0-perceptualRough);
    vec3 diffuseLight = irradiance;
    vec3 specularLight = radiance;

    vec3 diffuse = diffuseLight * diffuseColor;
    diffuse = mix(diffuse, transmission_btdf, transmission);
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

float applyIorToRoughness(float roughness, float ior)
{
    // Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
    // an IOR of 1.5 results in the default amount of microfacet refraction.
    return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}


vec3 getVolumeTransmissionRay(vec3 n, vec3 v, float thickness, float ior)
{
    // Direction of refracted light.
    vec3 refractionVector = refract(-v, normalize(n), 1.0 / ior);

    // The thickness is specified in local space.
    return normalize(refractionVector) * thickness * 0.2;
}

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l, // surface point to light
                    vec3 tr, // Transmission ray.
                    inout vec3 transmission_light, // Transmissive lighting.
                    vec3 intensity,
                    float ior
                    ) 
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

    vec3 lt = l - tr;

    // We use modified light and half vectors for the BTDF calculation, but the math is the same at the end of the day.
    float transmissionRougness = applyIorToRoughness(perceptualRoughness, ior);

    vec3 l_mirror = normalize(lt + 2.0 * n * dot(-lt, n));     // Mirror light reflection vector on surface
    h = normalize(l_mirror + v);            // Halfway vector between transmission light vector and v

    pbrInputs.NdotH = clamp(dot(n, h), 0.0, 1.0);
    pbrInputs.LdotH = clamp(dot(l_mirror, h), 0.0, 1.0);
    pbrInputs.NdotL = clamp(dot(n, l_mirror), 0.0, 1.0);
    pbrInputs.VdotH = clamp(dot(h, v), 0.0, 1.0);

    float G_transmission = geometricOcclusion(pbrInputs);
    vec3 F_transmission = specularReflection(pbrInputs);
    float D_transmission = microfacetDistribution(pbrInputs);

    transmission_light += intensity * (1.0 - F_transmission) * diffuseColor * D_transmission * G_transmission;

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    vec3 color = NdotL * (diffuseContrib + specContrib);

    return clamp(color, vec3(0), vec3(10));
}

vec3 pbrCalcPointLightOrSpotLight(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize,
                    float falloff,
                    float is_pointlight,
                    float ambiance,
                    float ior,
                    float thickness,
                    float transmissiveness)
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

        vec3 intensity = spot_atten * dist_atten * lightColor * 3.0; //magic number to balance with legacy materials

        vec3 tr = getVolumeTransmissionRay(n, v, thickness, ior);

        vec3 transmissive_light = vec3(0);

        color = intensity*pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, n.xyz, v, lv, tr, transmissive_light, intensity, ior);

        color = mix(color, transmissive_light, transmissiveness);
    }

    return color;
}

void calcDiffuseSpecular(vec3 baseColor, float metallic, inout vec3 diffuseColor, inout vec3 specularColor)
{
    vec3 f0 = vec3(0.04);
    diffuseColor = baseColor*(vec3(1.0)-f0);
    diffuseColor *= 1.0 - metallic;
    specularColor = mix(f0, baseColor, metallic);
}

vec3 POISSON3D_SAMPLES[128] = vec3[128](
    vec3(0.5433144, 0.1122154, 0.2501391),
    vec3(0.6575254, 0.721409, 0.16286),
    vec3(0.02888453, 0.05170321, 0.7573566),
    vec3(0.06635678, 0.8286457, 0.07157445),
    vec3(0.8957489, 0.4005505, 0.7916042),
    vec3(0.3423355, 0.5053263, 0.9193521),
    vec3(0.9694794, 0.9461077, 0.5406441),
    vec3(0.9975473, 0.02789414, 0.7320132),
    vec3(0.07781899, 0.3862341, 0.918594),
    vec3(0.4439073, 0.9686955, 0.4055861),
    vec3(0.9657035, 0.6624081, 0.7082613),
    vec3(0.7712346, 0.07273269, 0.3292839),
    vec3(0.2489169, 0.2550394, 0.1950516),
    vec3(0.7249326, 0.9328285, 0.3352458),
    vec3(0.6028461, 0.4424961, 0.5393377),
    vec3(0.2879795, 0.7427881, 0.6619173),
    vec3(0.3193627, 0.0486145, 0.08109283),
    vec3(0.1233155, 0.602641, 0.4378719),
    vec3(0.9800708, 0.211729, 0.6771586),
    vec3(0.4894537, 0.3319927, 0.8087631),
    vec3(0.4802743, 0.6358885, 0.814935),
    vec3(0.2692913, 0.9911493, 0.9934899),
    vec3(0.5648789, 0.8553897, 0.7784553),
    vec3(0.8497344, 0.7870212, 0.02065313),
    vec3(0.7503014, 0.2826185, 0.05412734),
    vec3(0.8045461, 0.6167251, 0.9532926),
    vec3(0.04225039, 0.2141281, 0.8678675),
    vec3(0.07116079, 0.9971236, 0.3396397),
    vec3(0.464099, 0.480959, 0.2775862),
    vec3(0.6346927, 0.31871, 0.6588384),
    vec3(0.449012, 0.8189669, 0.2736875),
    vec3(0.452929, 0.2119148, 0.672004),
    vec3(0.01506042, 0.7102436, 0.9800494),
    vec3(0.1970513, 0.4713539, 0.4644522),
    vec3(0.13715, 0.7253224, 0.5056525),
    vec3(0.9006432, 0.5335414, 0.02206874),
    vec3(0.9960898, 0.7961011, 0.01468861),
    vec3(0.3386469, 0.6337739, 0.9310676),
    vec3(0.1745718, 0.9114985, 0.1728188),
    vec3(0.6342545, 0.5721557, 0.4553517),
    vec3(0.1347412, 0.1137158, 0.7793725),
    vec3(0.3574478, 0.3448052, 0.08741581),
    vec3(0.7283059, 0.4753885, 0.2240275),
    vec3(0.8293507, 0.9971212, 0.2747005),
    vec3(0.6501846, 0.000688076, 0.7795712),
    vec3(0.01149416, 0.4930083, 0.792608),
    vec3(0.666189, 0.1875442, 0.7256873),
    vec3(0.8538797, 0.2107637, 0.1547532),
    vec3(0.5826825, 0.9750752, 0.9105834),
    vec3(0.8914346, 0.08266425, 0.5484225),
    vec3(0.4374518, 0.02987111, 0.7810078),
    vec3(0.2287418, 0.1443802, 0.1176908),
    vec3(0.2671157, 0.8929081, 0.8989366),
    vec3(0.5425819, 0.5524959, 0.6963879),
    vec3(0.3515188, 0.8304397, 0.0502702),
    vec3(0.3354864, 0.2130747, 0.141169),
    vec3(0.9729427, 0.3509927, 0.6098799),
    vec3(0.7585629, 0.7115368, 0.9099342),
    vec3(0.0140543, 0.6072157, 0.9436461),
    vec3(0.9190664, 0.8497264, 0.1643751),
    vec3(0.1538157, 0.3219983, 0.2984214),
    vec3(0.8854713, 0.2968667, 0.8511457),
    vec3(0.1910622, 0.03047311, 0.3571215),
    vec3(0.2456353, 0.5568692, 0.3530164),
    vec3(0.6927255, 0.8073994, 0.5808484),
    vec3(0.8089353, 0.8969175, 0.3427134),
    vec3(0.194477, 0.7985603, 0.8712182),
    vec3(0.7256182, 0.5653068, 0.3985921),
    vec3(0.9889427, 0.4584851, 0.8363391),
    vec3(0.5718582, 0.2127113, 0.2950557),
    vec3(0.5480209, 0.0193435, 0.2992659),
    vec3(0.6598953, 0.09478426, 0.92187),
    vec3(0.1385615, 0.2193868, 0.205245),
    vec3(0.7623423, 0.1790726, 0.1508465),
    vec3(0.7569032, 0.3773386, 0.4393887),
    vec3(0.5842971, 0.6538072, 0.5224424),
    vec3(0.9954313, 0.5763943, 0.9169143),
    vec3(0.001311183, 0.340363, 0.1488652),
    vec3(0.8167927, 0.4947158, 0.4454727),
    vec3(0.3978434, 0.7106082, 0.002727509),
    vec3(0.5459411, 0.7473233, 0.7062873),
    vec3(0.4151598, 0.5614617, 0.4748358),
    vec3(0.4440694, 0.1195122, 0.9624678),
    vec3(0.1081301, 0.4813806, 0.07047641),
    vec3(0.2402785, 0.3633997, 0.3898734),
    vec3(0.2317942, 0.6488295, 0.4221864),
    vec3(0.01145542, 0.9304277, 0.4105759),
    vec3(0.3563728, 0.9228861, 0.3282344),
    vec3(0.855314, 0.6949819, 0.3175117),
    vec3(0.730832, 0.01478493, 0.5728671),
    vec3(0.9304829, 0.02653277, 0.712552),
    vec3(0.4132186, 0.4127623, 0.6084146),
    vec3(0.7517329, 0.9978395, 0.1330464),
    vec3(0.5210338, 0.4318751, 0.9721575),
    vec3(0.02953994, 0.1375937, 0.9458942),
    vec3(0.1835506, 0.9896691, 0.7919457),
    vec3(0.3857062, 0.2682322, 0.1264563),
    vec3(0.6319699, 0.8735335, 0.04390657),
    vec3(0.5630485, 0.3339024, 0.993995),
    vec3(0.90701, 0.1512893, 0.8970422),
    vec3(0.3027443, 0.1144253, 0.1488708),
    vec3(0.9149003, 0.7382028, 0.7914025),
    vec3(0.07979286, 0.6892691, 0.2866171),
    vec3(0.7743186, 0.8046008, 0.4399814),
    vec3(0.3128662, 0.4362317, 0.6030678),
    vec3(0.1133721, 0.01605821, 0.391872),
    vec3(0.5185481, 0.9210006, 0.7889017),
    vec3(0.8217013, 0.325305, 0.1668191),
    vec3(0.8358996, 0.1449739, 0.3668382),
    vec3(0.1778213, 0.5599256, 0.1327691),
    vec3(0.06690693, 0.5508637, 0.07212365),
    vec3(0.9750564, 0.284066, 0.5727578),
    vec3(0.4350255, 0.8949825, 0.03574753),
    vec3(0.8931149, 0.9177974, 0.8123496),
    vec3(0.9055127, 0.989903, 0.813235),
    vec3(0.2897243, 0.3123978, 0.5083504),
    vec3(0.1519223, 0.3958645, 0.2640327),
    vec3(0.6840154, 0.6463035, 0.2346607),
    vec3(0.986473, 0.8714055, 0.3960275),
    vec3(0.6819352, 0.4169535, 0.8379834),
    vec3(0.9147297, 0.6144146, 0.7313942),
    vec3(0.6554981, 0.5014008, 0.9748477),
    vec3(0.9805915, 0.1318207, 0.2371372),
    vec3(0.5980836, 0.06796348, 0.9941338),
    vec3(0.6836596, 0.9917196, 0.2319056),
    vec3(0.5276511, 0.2745509, 0.5422578),
    vec3(0.829482, 0.03758276, 0.1240466),
    vec3(0.2698198, 0.0002266169, 0.3449324)
);

vec3 getPoissonSample(int i) {
    return POISSON3D_SAMPLES[i] * 2 - 1;
}

float random (vec2 uv);

vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
    float framebufferLod = log2(float(screen_res.x)) * applyIorToRoughness(roughness, ior);

    vec3 totalColor = textureLod(sceneMap, fragCoord.xy, framebufferLod).rgb;
    int samples = 8;
    for (int i = 0; i < samples; i++)
    {
        vec2 pixelScale = ((getPoissonSample(i).xy * 2 - 1) / screen_res) * framebufferLod * 2;
        totalColor += textureLod(sceneMap, fragCoord.xy + pixelScale, framebufferLod).rgb;
    }

    totalColor /= samples + 1;

    return totalColor;
}

vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
    if (attenuationDistance == 0.0)
    {
        // Attenuation distance is +∞ (which we indicate by zero), i.e. the transmitted color is not attenuated at all.
        return radiance;
    }
    else
    {
        // Compute light attenuation using Beer's law.
        vec3 transmittance = pow(attenuationColor, vec3(transmissionDistance / attenuationDistance));
        return transmittance * radiance;
    }
}

vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 f0, vec3 f90,
    vec4 position, float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
{
    // Dispersion will spread out the ior values for each r,g,b channel
    float halfSpread = (ior - 1.0) * 0.025 * dispersion;
    vec3 iors = vec3(ior - halfSpread, ior, ior + halfSpread);
    vec3 transmittedLight;
    float transmissionRayLength;

    for (int i = 0; i < 3; i++)
    {
        vec3 transmissionRay = getVolumeTransmissionRay(n, v, thickness, iors[i]);
        // TODO: taking length of blue ray, ideally we would take the length of the green ray. For now overwriting seems ok
        transmissionRayLength = length(transmissionRay);
        vec3 refractedRayExit = vec3(getScreenCoord(position), 1) + transmissionRay;

        // Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
        vec2 refractionCoords = refractedRayExit.xy;

        // Sample framebuffer to get pixel the refracted ray hits for this color channel.
        transmittedLight[i] = getTransmissionSample(refractionCoords, perceptualRoughness, iors[i])[i];
    }
    vec3 attenuatedColor = applyVolumeAttenuation(transmittedLight, transmissionRayLength, attenuationColor, attenuationDistance);

    // Sample GGX LUT to get the specular component.
    float NdotV = max(0, dot(n, v));
    vec2 brdfSamplePoint = clamp(vec2(NdotV, perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));

    vec2 brdf = BRDF(brdfSamplePoint.x, brdfSamplePoint.y);
    vec3 specularColor = f0 * brdf.x + f90 * brdf.y;

    //return (1.0 - specularColor);

    //return attenuatedColor;

    //return baseColor;

    return (1.0 - specularColor) * attenuatedColor * baseColor;
}

vec3 pbrBaseLight(vec3 diffuseColor,
                  vec3 specularColor,
                  float metallic,
                  vec4 pos,
                  vec3 view,
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
                  vec3 atten,
                  float thickness,
                  vec3 atten_color,
                  float atten_dist,
                  float ior,
                  float dispersion,
                  float transmission)
{
    vec3 color = vec3(0);

    float NdotV = clamp(abs(dot(norm, view)), 0.001, 1.0);

    vec3 btdf = vec3(0);

    btdf = getIBLVolumeRefraction(norm, view, perceptualRoughness, diffuseColor, vec3(0.04), vec3(1), pos, ior, thickness, atten_color, atten_dist, dispersion);

    color += pbrIbl(diffuseColor, specularColor, radiance, irradiance, ao, NdotV, perceptualRoughness, transmission, btdf);

    vec3 tr = getVolumeTransmissionRay(norm, view, thickness, ior);

    vec3 transmissive_light = vec3(0);

    vec3 puncLight = pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, norm, pos.xyz, normalize(light_dir), tr, transmissive_light, vec3(1), ior);

    color += mix(puncLight, transmissive_light, vec3(transmission)) * sunlit * 3.0 * scol;

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

