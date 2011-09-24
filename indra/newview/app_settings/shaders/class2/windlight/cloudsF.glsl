/** 
 * @file WLCloudsF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

VARYING vec4 vary_CloudColorSun;
VARYING vec4 vary_CloudColorAmbient;
VARYING float vary_CloudDensity;
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec2 vary_texcoord3;

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
	vec2 uv1 = vary_texcoord0.xy;
	vec2 uv2 = vary_texcoord1.xy;

	vec4 cloudColorSun = vary_CloudColorSun;
	vec4 cloudColorAmbient = vary_CloudColorAmbient;
	float cloudDensity = vary_CloudDensity;
	vec2 uv3 = vary_texcoord2.xy;
	vec2 uv4 = vary_texcoord3.xy;

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

