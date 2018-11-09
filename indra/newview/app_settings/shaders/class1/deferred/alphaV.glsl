/** 
 * @file alphaV.glsl
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

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;

#ifdef USE_INDEXED_TEX
void passTextureIndex();
#endif

ATTRIBUTE vec3 normal;

#ifdef USE_VERTEX_COLOR
ATTRIBUTE vec4 diffuse_color;
#endif

ATTRIBUTE vec2 texcoord0;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifdef IS_AVATAR_SKIN
mat4 getSkinnedTransform();
#endif
#endif

VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

uniform float near_clip;

void main()
{
	vec4 pos;
	vec3 norm;
	
	//transform vertex
#ifdef HAS_SKIN
	mat4 trans = getObjectSkinnedTransform();
	trans = modelview_matrix * trans;
	
	pos = trans * vec4(position.xyz, 1.0);
	
	norm = position.xyz + normal.xyz;
	norm = normalize((trans * vec4(norm, 1.0)).xyz - pos.xyz);
	vec4 frag_pos = projection_matrix * pos;
	gl_Position = frag_pos;
#else

#ifdef IS_AVATAR_SKIN
	mat4 trans = getSkinnedTransform();
	vec4 pos_in = vec4(position.xyz, 1.0);
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, normal);
	norm.y = dot(trans[1].xyz, normal);
	norm.z = dot(trans[2].xyz, normal);
	norm = normalize(norm);
	
	vec4 frag_pos = projection_matrix * pos;
	gl_Position = frag_pos;
#else
	norm = normalize(normal_matrix * normal);
	vec4 vert = vec4(position.xyz, 1.0);
	pos = (modelview_matrix * vert);
	gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
#endif
	
#endif

#ifdef USE_INDEXED_TEX
	passTextureIndex();
#endif

	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
	
	vary_norm = norm;
	vary_position = pos.xyz;

#ifdef USE_VERTEX_COLOR
	vertex_color = diffuse_color;
#endif
	
#ifdef HAS_SKIN
	vary_fragcoord.xyz = frag_pos.xyz + vec3(0,0,near_clip);
#else

#ifdef IS_AVATAR_SKIN
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#else
	pos = modelview_projection_matrix * vert;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#endif
	
#endif

}

