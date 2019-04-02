/** 
 * @file attachmentShadowV.glsl
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

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 texture_matrix0;
uniform float shadow_target_width;

ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 position;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;

mat4 getObjectSkinnedTransform();
void passTextureIndex();

#if !DEPTH_CLAMP
VARYING vec4 post_pos;
#endif
VARYING vec2 vary_texcoord0;
VARYING float pos_w;
VARYING float target_pos_x;
VARYING vec4 vertex_color;

void main()
{
	//transform vertex
	mat4 mat = getObjectSkinnedTransform();
	
	mat = modelview_matrix * mat;
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;

	vec4 p = projection_matrix * vec4(pos, 1.0);

	pos_w = p.w;

	target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;

	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

	vertex_color = diffuse_color;

#if !DEPTH_CLAMP
	p.z = max(p.z, -p.w+0.01);
    post_pos = p;
	gl_Position = p;
#else
	gl_Position = p;
#endif

	passTextureIndex();
}

