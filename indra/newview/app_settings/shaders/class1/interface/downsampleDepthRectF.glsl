/** 
 * @file debugF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect depthMap;

VARYING vec2 tc0;
VARYING vec2 tc1;
VARYING vec2 tc2;
VARYING vec2 tc3;
VARYING vec2 tc4;
VARYING vec2 tc5;
VARYING vec2 tc6;
VARYING vec2 tc7;
VARYING vec2 tc8;

void main() 
{
	vec4 depth1 = 
		vec4(texture2DRect(depthMap, tc0).r,
			texture2DRect(depthMap, tc1).r,
			texture2DRect(depthMap, tc2).r,
			texture2DRect(depthMap, tc3).r);

	vec4 depth2 = 
		vec4(texture2DRect(depthMap, tc4).r,
			texture2DRect(depthMap, tc5).r,
			texture2DRect(depthMap, tc6).r,
			texture2DRect(depthMap, tc7).r);

	depth1 = min(depth1, depth2);
	float depth = min(depth1.x, depth1.y);
	depth = min(depth, depth1.z);
	depth = min(depth, depth1.w);
	depth = min(depth, texture2DRect(depthMap, tc8).r);

	gl_FragDepth = depth;
}
