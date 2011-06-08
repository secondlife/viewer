/** 
 * @file gammaF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform vec4 gamma;

vec3 getAtmosAttenuation();

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light) {
	//soft clip effect:
	light = 1. - clamp(light, vec3(0.), vec3(1.));
	light = 1. - pow(light, gamma.xxx);

	return light;
}

vec3 fullbrightScaleSoftClip(vec3 light) {
	return mix(scaleSoftClip(light.rgb), light.rgb, getAtmosAttenuation());
}

