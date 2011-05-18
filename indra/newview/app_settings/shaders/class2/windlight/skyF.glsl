/** 
 * @file WLSkyF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

varying vec4 vary_HazeColor;

uniform sampler2D cloud_noise_texture;
uniform vec4 gamma;

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light) {
	//soft clip effect:
	light = 1. - clamp(light, vec3(0.), vec3(1.));
	light = 1. - pow(light, gamma.xxx);

	return light;
}

void main()
{
	// Potential Fill-rate optimization.  Add cloud calculation 
	// back in and output alpha of 0 (so that alpha culling kills 
	// the fragment) if the sky wouldn't show up because the clouds 
	// are fully opaque.

	vec4 color;
	color = vary_HazeColor;
	color *= 2.;

	/// Gamma correct for WL (soft clip effect).
	gl_FragColor.rgb = scaleSoftClip(color.rgb);
	gl_FragColor.a = 1.0;
}

