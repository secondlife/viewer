/** 
 * @file terrainWaterF.glsl
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

#ifndef gl_FragColor
out vec4 gl_FragColor;
#endif

// this class1 shader is just a copy of terrainF

uniform sampler2D detail0;
uniform sampler2D detail1;
uniform sampler2D alphaRamp;

VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec2 vary_texcoord3;

void main() 
{
	float a = texture2D(alphaRamp, vary_texcoord1.xy).a;
	vec3 color = mix(texture2D(detail1, vary_texcoord2.xy).rgb,
					 texture2D(detail0, vary_texcoord0.xy).rgb,
					 a);

	gl_FragColor.rgb = color;
	gl_FragColor.a = texture2D(alphaRamp, vary_texcoord3.xy).a;
}
