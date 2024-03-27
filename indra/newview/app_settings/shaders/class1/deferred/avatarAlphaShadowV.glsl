/** 
 * @file avatarAlphaShadowV.glsl
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
 
uniform mat4 texture_matrix0;
uniform mat4 projection_matrix;
uniform float shadow_target_width;

mat4 getSkinnedTransform();
void passTextureIndex();

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

out float pos_w;
out float target_pos_x;
out vec2 vary_texcoord0;

void main()
{
	vec4 pos;
	vec3 norm;
	
	vec4 pos_in = vec4(position.xyz, 1.0);
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, normal);
	norm.y = dot(trans[1].xyz, normal);
	norm.z = dot(trans[2].xyz, normal);
	norm = normalize(norm);
	
	pos = projection_matrix * pos;

	target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;

	pos_w = pos.w;

	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

	gl_Position = pos;

	passTextureIndex();
}

