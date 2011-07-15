/** 
 * @file gammaF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform vec4 gamma;

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light) {
	// For compatibility with lower cards. Do nothing.
	return light;
}

vec3 fullbrightScaleSoftClip(vec3 light) {
	return scaleSoftClip(light);
}

