/**
 * @file transmissiveUtil.glsl
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

uniform sampler2D sceneMap;
uniform vec2 screen_res;

float calcLegacyDistanceAttenuation(float distance, float falloff);

vec2 getScreenCoord(vec4 clip);

vec2 BRDF(float NoV, float roughness);

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

vec3 diffuse(PBRInfo pbrInputs);
vec3 specularReflection(PBRInfo pbrInputs);
float geometricOcclusion(PBRInfo pbrInputs);
float microfacetDistribution(PBRInfo pbrInputs);


// set colorDiffuse and colorSpec to the results of GLTF PBR style IBL
vec3 pbrIblTransmission(vec3 diffuseColor,
            vec3 specularColor,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,       // normal dot view vector
            float perceptualRough,
            float transmission,
            vec3 transmission_btdf)
{
    vec3 color = vec3(1, 0, 1);

    return color;
}

float applyIorToRoughness(float roughness, float ior)
{
    return 0;
}

vec3 getVolumeTransmissionRay(vec3 n, vec3 v, float thickness, float ior)
{
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 pbrPunctualTransmission(vec3 diffuseColor, vec3 specularColor,
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
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 pbrCalcPointLightOrSpotLightTransmission(vec3 diffuseColor, vec3 specularColor,
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
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 f0, vec3 f90,
    vec4 position, float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
{
    vec3 color = vec3(1, 0, 1);

    return color;
}

vec3 pbrBaseLightTransmission(vec3 diffuseColor,
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
    vec3 color = vec3(1, 0, 1);

    return color;
}
