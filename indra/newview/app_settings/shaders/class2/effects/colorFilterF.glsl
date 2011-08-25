/**
 * @file colorFilterF.glsl
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
uniform float brightness;
uniform float contrast;
uniform vec3  contrastBase;
uniform float saturation;
uniform vec3  lumWeights;

const float gamma = 2.0;

void main(void) 
{
	vec3 color = vec3(texture2DRect(RenderTexture, gl_TexCoord[0].st));

	/// Modulate brightness
	color *= brightness;

	/// Modulate contrast
	color = mix(contrastBase, color, contrast);

	/// Modulate saturation
	color = mix(vec3(dot(color, lumWeights)), color, saturation);

	gl_FragColor = vec4(color, 1.0);
}
