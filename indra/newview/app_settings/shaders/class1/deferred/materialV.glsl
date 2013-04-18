/** 
 * @file bumpV.glsl
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

#ifdef HAS_SKIN
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif

uniform mat4 texture_matrix0;


ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;


#if HAS_NORMAL_MAP
ATTRIBUTE vec3 binormal;
ATTRIBUTE vec2 texcoord1;

VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;

VARYING vec2 vary_texcoord1;
#else
VARYING vec3 vary_normal;
#endif

#if HAS_SPECULAR_MAP
ATTRIBUTE vec2 texcoord2;
VARYING vec2 vary_texcoord2;
#endif
 
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

void main()
{
#ifdef HAS_SKIN
	mat4 mat = getObjectSkinnedTransform();

	mat = modelview_matrix * mat;

	vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;

	gl_Position = projection_matrix*vec4(pos,1.0);

#else
	//transform vertex
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0); 

#endif
	
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
	
#if HAS_NORMAL_MAP
	vary_texcoord1 = (texture_matrix0 * vec4(texcoord1,0,1)).xy;
#endif

#if HAS_SPECULAR_MAP
	vary_texcoord2 = (texture_matrix0 * vec4(texcoord2,0,1)).xy;
#endif

#ifdef HAS_SKIN
	vec3 n = normalize((mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz);
#if HAS_NORMAL_MAP
	vec3 b = normalize((mat*vec4(binormal.xyz+position.xyz,1.0)).xyz-pos.xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
#else
	vary_normal  = n;
#endif

#else
	vec3 n = normalize(normal_matrix * normal);
#if HAS_NORMAL_MAP
	vec3 b = normalize(normal_matrix * binormal);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
#else
	vary_normal = n;
#endif

#endif
	
	vertex_color = diffuse_color;
}
