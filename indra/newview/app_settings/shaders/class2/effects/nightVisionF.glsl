/**
 * @file nightVisionF.glsl
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

uniform sampler2DRect RenderTexture;
uniform sampler2D NoiseTexture;
uniform float brightMult;
uniform float noiseStrength;

VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;

float luminance(vec3 color)
{
	/// CALCULATING LUMINANCE (Using NTSC lum weights)
	/// http://en.wikipedia.org/wiki/Luma_%28video%29
	return dot(color, vec3(0.299, 0.587, 0.114));
}

void main(void) 
{
	/// Get scene color
	vec3 color = vec3(texture2DRect(RenderTexture, vary_texcoord0.st));
	
	/// Extract luminance and scale up by night vision brightness
	float lum = luminance(color) * brightMult;

	/// Convert into night vision color space
	/// Newer NVG colors (crisper and more saturated)
	vec3 outColor = (lum * vec3(0.91, 1.21, 0.9)) + vec3(-0.07, 0.1, -0.12); 

	/// Add noise
	float noiseValue = texture2D(NoiseTexture, vary_texcoord1.st).r;
	noiseValue = (noiseValue - 0.5) * noiseStrength;

	/// Older NVG colors (more muted)
	// vec3 outColor = (lum * vec3(0.82, 0.75, 0.83)) + vec3(0.05, 0.32, -0.11); 
	
	outColor += noiseValue;

	gl_FragColor = vec4(outColor, 1.0);
}
