/** 
 * @file simpleV.glsl
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

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;
void passTextureIndex();
ATTRIBUTE vec2 texcoord0;
ATTRIBUTE vec3 normal;

uniform vec4 color;
uniform vec4 object_plane_t;
uniform vec4 object_plane_s;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

vec4 texgen_object(vec4  vpos, vec4 tc, mat4 mat, vec4 tp0, vec4 tp1)
{
	vec4 tcoord;
	
	tcoord.x = dot(vpos, tp0);
	tcoord.y = dot(vpos, tp1);
	tcoord.z = tc.z;
	tcoord.w = tc.w;
	
	tcoord = mat * tcoord; 
	
	return tcoord; 
}

void main()
{
	//transform vertex
	vec4 vert = vec4(position.xyz,1.0);
	passTextureIndex();
	vec4 pos = (modelview_matrix * vert);
	gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
	vary_texcoord0.xy = texgen_object(vec4(position.xyz, 1.0), vec4(texcoord0,0,1), texture_matrix0, object_plane_s, object_plane_t).xy;
		
	vec3 norm = normalize(normal_matrix * normal);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm, color, vec4(0.));
	vertex_color = color;

	
}
