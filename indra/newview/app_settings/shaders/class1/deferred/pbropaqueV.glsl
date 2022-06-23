/** 
 * @file pbropaqueV.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;

#ifdef HAS_NORMAL_MAP
    ATTRIBUTE vec4 tangent;
    ATTRIBUTE vec2 texcoord1;
    VARYING   vec3 vary_mat0;
    VARYING   vec3 vary_mat1;
    VARYING   vec3 vary_mat2;
#endif

#if HAS_SPECULAR_MAP
    ATTRIBUTE vec2 texcoord2;
#endif

VARYING vec3 vary_position;
VARYING vec3 vary_normal;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1; // normal map
VARYING vec2 vary_texcoord2; // specular map

void passTextureIndex();

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_position = (modelview_matrix * vec4(position.xyz,1.0)).xyz;
    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    passTextureIndex();
    vary_normal = normalize(normal_matrix * normal);

#ifdef HAS_NORMAL_MAP
    //vary_texcoord1 = (texture_matrix0 * vec4(texcoord1,0,1)).xy;
    vary_texcoord1 = texcoord1;
//  vary_mat0 = vec3(t.x, b.x, n.x);
//  vary_mat1 = vec3(t.y, b.y, n.y);
//  vary_mat2 = vec3(t.z, b.z, n.z);
#endif

#ifdef HAS_SPECULAR_MAP
    //vary_texcoord2 = (texture_matrix0 * vec4(texcoord2,0,1)).xy;
    vary_texcoord2 = texcoord2;
#endif

    vertex_color = diffuse_color;
}
