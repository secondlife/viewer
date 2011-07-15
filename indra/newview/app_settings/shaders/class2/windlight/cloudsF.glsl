/** 
 * @file WLCloudsF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

varying vec4 vary_CloudColorSun;
varying vec4 vary_CloudColorAmbient;
varying float vary_CloudDensity;

uniform sampler2D cloud_noise_texture;
uniform vec4 cloud_pos_density1;
uniform vec4 cloud_pos_density2;
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
	// Set variables
	vec2 uv1 = gl_TexCoord[0].xy;
	vec2 uv2 = gl_TexCoord[1].xy;

	vec4 cloudColorSun = vary_CloudColorSun;
	vec4 cloudColorAmbient = vary_CloudColorAmbient;
	float cloudDensity = vary_CloudDensity;
	vec2 uv3 = gl_TexCoord[2].xy;
	vec2 uv4 = gl_TexCoord[3].xy;

	// Offset texture coords
	uv1 += cloud_pos_density1.xy;	//large texture, visible density
	uv2 += cloud_pos_density1.xy;	//large texture, self shadow
	uv3 += cloud_pos_density2.xy;	//small texture, visible density
	uv4 += cloud_pos_density2.xy;	//small texture, self shadow


	// Compute alpha1, the main cloud opacity
	float alpha1 = (texture2D(cloud_noise_texture, uv1).x - 0.5) + (texture2D(cloud_noise_texture, uv3).x - 0.5) * cloud_pos_density2.z;
	alpha1 = min(max(alpha1 + cloudDensity, 0.) * 10. * cloud_pos_density1.z, 1.);

	// And smooth
	alpha1 = 1. - alpha1 * alpha1;
	alpha1 = 1. - alpha1 * alpha1;	


	// Compute alpha2, for self shadowing effect
	// (1 - alpha2) will later be used as percentage of incoming sunlight
	float alpha2 = (texture2D(cloud_noise_texture, uv2).x - 0.5);
	alpha2 = min(max(alpha2 + cloudDensity, 0.) * 2.5 * cloud_pos_density1.z, 1.);

	// And smooth
	alpha2 = 1. - alpha2;
	alpha2 = 1. - alpha2 * alpha2;	

	// Combine
	vec4 color;
	color = (cloudColorSun*(1.-alpha2) + cloudColorAmbient);
	color *= 2.;

	/// Gamma correct for WL (soft clip effect).
	gl_FragColor.rgb = scaleSoftClip(color.rgb);
	gl_FragColor.a = alpha1;
}

