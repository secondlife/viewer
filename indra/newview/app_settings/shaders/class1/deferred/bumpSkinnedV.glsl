/** 
 * @file bumpV.glsl
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
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;
ATTRIBUTE vec3 binormal;

VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

mat4 getObjectSkinnedTransform();

void main()
{
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = modelview_matrix * mat;
	
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	
	vec3 n = normalize((mat * vec4(normal.xyz+position.xyz, 1.0)).xyz-pos.xyz);
	vec3 b = normalize((mat * vec4(binormal.xyz+position.xyz, 1.0)).xyz-pos.xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
	
	gl_Position = projection_matrix*vec4(pos, 1.0);
	vertex_color = diffuse_color;
}
