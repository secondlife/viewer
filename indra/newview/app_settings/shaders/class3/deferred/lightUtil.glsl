/** 
 * @file lightInfo.glsl
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

struct DirectionalLightInfo
{
    vec4 pos;
    float depth;
    vec4 normal;
    vec3 normalizedLightDirection;
    vec3 normalizedToLight;
    float lightIntensity;
    vec3 lightDiffuseColor;
    float specExponent;
    float shadow;
};

struct SpotLightInfo
{
    vec4 pos;
    float depth;
    vec4 normal;
    vec3 normalizedLightDirection;
    vec3 normalizedToLight;
    float lightIntensity;
    float attenuation;
    float distanceToLight;
    vec3 lightDiffuseColor;
    float innerHalfAngleCos;
    float outerHalfAngleCos;
    float spotExponent;
    float specExponent;
    float shadow;
};

struct PointLightInfo
{
    vec4 pos;
    float depth;
    vec4 normal;
    vec3 normalizedToLight;
    float lightIntensity;
    float attenuation;
    float distanceToLight;
    vec3 lightDiffuseColor;
    float lightRadius;
    float specExponent;
    vec3 worldspaceLightDirection;
    float shadow;
};

float attenuate(float attenuationSelection, float distanceToLight)
{
// LLRENDER_REVIEW
// sh/could eventually consume attenuation func defined in texture
    return (attenuationSelection == 0.0f) ? 1.0f : // none
           (attenuationSelection <  1.0f) ? (1.0f / distanceToLight) : // linear atten 
           (attenuationSelection <  2.0f) ? (1.0f / (distanceToLight*distanceToLight)) // quadratic atten
										  : (1.0f / (distanceToLight*distanceToLight*distanceToLight));	// cubic atten    
}


vec3 lightDirectional(struct DirectionalLightInfo dli)
{
    float lightIntensity = dli.lightIntensity;
	lightIntensity *= dot(dli.normal.xyz, dli.normalizedLightDirection);
    //lightIntensity *= directionalShadowSample(vec4(dli.pos.xyz, 1.0f), dli.depth, dli.directionalShadowMap, dli.directionalShadowMatrix);
	return lightIntensity * dli.lightDiffuseColor;
}


vec3 lightSpot(struct SpotLightInfo sli)    
{
	float penumbraRange = (sli.outerHalfAngleCos - sli.innerHalfAngleCos);
    float coneAngleCos = max(dot(sli.normalizedLightDirection, sli.normalizedToLight), 0.0);
	float coneAttenFactor = (coneAngleCos <= sli.outerHalfAngleCos) ? 1.0f : pow(smoothstep(1,0, sli.outerHalfAngleCos / penumbraRange), sli.spotExponent);
    float distanceAttenuation = attenuate(sli.attenuation, sli.distanceToLight);
    float lightIntensity = sli.lightIntensity;
    lightIntensity *= distanceAttenuation;
	lightIntensity *= max(dot(sli.normal.xyz, sli.normalizedLightDirection), 0.0);
	lightIntensity *= coneAttenFactor;
    lightIntensity *= sli.shadow;
	return lightIntensity * sli.lightDiffuseColor;
}

vec3 lightPoint(struct PointLightInfo pli)
{
    float padRadius = pli.lightRadius * 0.1; // distance for which to perform smoothed dropoff past light radius
	float distanceAttenuation =	attenuate(pli.attenuation, pli.distanceToLight);
    float lightIntensity = pli.lightIntensity;
    lightIntensity*= distanceAttenuation;    
	lightIntensity *= clamp((padRadius - pli.distanceToLight + pli.lightRadius) / padRadius, 0.0, 1.0);
    lightIntensity *= pli.shadow;
    lightIntensity *= max(dot(pli.normal.xyz, pli.normalizedToLight), 0.0);
	return lightIntensity * pli.lightDiffuseColor;
}
