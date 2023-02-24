/** 
 * @file pbgglowV.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
uniform mat4 modelview_projection_matrix;
#endif

uniform mat4 texture_matrix0;

uniform mat3 texture_basecolor_matrix;
uniform mat3 texture_emissive_matrix;

in vec3 position;
in vec4 emissive;

in vec2 texcoord0;

out vec2 basecolor_texcoord;
out vec2 emissive_texcoord;
 
out vec4 vertex_emissive;

vec2 texture_transform(vec2 vertex_texcoord, mat3 khr_gltf_transform, mat4 sl_animation_transform);

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

    basecolor_texcoord = texture_transform(texcoord0, texture_basecolor_matrix, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_matrix, texture_matrix0);

    vertex_emissive = emissive;
}

