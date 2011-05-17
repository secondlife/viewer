/** 
 * @file lightSpecularV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

// All lights, no specular highlights

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol);

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	return sumLightsSpecular(pos, norm, color, specularColor, baseCol);
}

