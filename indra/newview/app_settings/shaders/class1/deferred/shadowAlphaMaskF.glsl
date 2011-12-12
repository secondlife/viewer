/** 
 * @file shadowAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

uniform float minimum_alpha;

uniform sampler2D diffuseMap;

VARYING vec4 post_pos;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

void main() 
{
	float alpha = diffuseLookup(vary_texcoord0.xy).a * vertex_color.a;

	if (alpha < minimum_alpha)
	{
		discard;
	}

	gl_FragColor = vec4(1,1,1,1);
	
	gl_FragDepth = max(post_pos.z/post_pos.w*0.5+0.5, 0.0);
}
