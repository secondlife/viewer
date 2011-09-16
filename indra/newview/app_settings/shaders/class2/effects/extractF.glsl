/**
 * @file extractF.glsl
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
 


uniform sampler2DRect RenderTexture;
uniform float extractLow;
uniform float extractHigh;
uniform vec3 lumWeights;

void main(void) 
{
	/// Get scene color
	vec3 color = vec3(texture2DRect(RenderTexture, vary_texcoord0.st));
	
	/// Extract luminance and scale up by night vision brightness
	float lum = smoothstep(extractLow, extractHigh, dot(color, lumWeights));

	gl_FragColor = vec4(vec3(lum), 1.0);
}
