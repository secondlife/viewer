/**
 * @file class3/deferred/screenSpaceReflAlphaV.glsl
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

uniform mat4 modelview_matrix;

#ifdef HAS_SKIN
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif

uniform mat4 texture_matrix0;
uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_metallic_roughness_transform;

in vec3 position;
in vec3 normal;
in vec2 texcoord0;
in vec4 diffuse_color;

out vec3 vary_position;
out vec3 vary_normal;
out vec2 base_color_texcoord;
out vec2 metallic_roughness_texcoord;
out vec4 vertex_color;

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec3 pos = (mat * vec4(position.xyz, 1.0)).xyz;
    vary_normal = normalize((mat * vec4(normal.xyz + position.xyz, 1.0)).xyz - pos.xyz);
    gl_Position = projection_matrix * vec4(pos, 1.0);
#else
    vary_normal = normalize(normal_matrix * normal);
    vec3 pos = (modelview_matrix * vec4(position.xyz, 1.0)).xyz;
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
#endif
    vary_position = pos;

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0);

    vertex_color = diffuse_color;
}
