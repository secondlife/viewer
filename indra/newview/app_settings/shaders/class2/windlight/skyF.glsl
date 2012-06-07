/** 
 * @file WLSkyF.glsl
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
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

VARYING vec4 vary_HazeColor;

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
	frag_color.rgb = scaleSoftClip(color.rgb);
	frag_color.a = 1.0;
}

