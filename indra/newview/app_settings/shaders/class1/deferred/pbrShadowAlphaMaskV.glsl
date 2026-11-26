/**
 * @file pbrShadowAlphaMaskV.glsl
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

uniform mat4 texture_matrix0;
#if defined(HAS_SKIN)
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat4 modelview_projection_matrix;
#endif

uniform vec4[2] texture_base_color_transform;
vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

uniform float shadow_target_width;

#ifdef TRANSMISSION
#if !defined(HAS_SKIN)
uniform mat3 normal_matrix;
#endif
uniform vec4[2] texture_transmission_transform;
uniform vec4[2] texture_normal_transform;
in vec3 normal;
in vec4 tangent;
out vec2 transmission_texcoord;
out vec2 normal_texcoord;
out vec3 vary_normal;
out vec3 vary_tangent;
flat out float vary_sign;

vec4 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
#endif

in vec3 position;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec4 post_pos;
out float target_pos_x;
out vec4 vertex_color;
out vec2 vary_texcoord0;

void passTextureIndex();

void main()
{
    //transform vertex
#if defined(HAS_SKIN)
    vec4 pre_pos = vec4(position.xyz, 1.0);
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec4 pos = mat * pre_pos;
    pos = projection_matrix * pos;
#else
    vec4 pre_pos = vec4(position.xyz, 1.0);
    vec4 pos = modelview_projection_matrix * pre_pos;
#endif

    target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;

    post_pos = pos;

    gl_Position = pos;

    passTextureIndex();

    vary_texcoord0 = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    vertex_color = diffuse_color;

#ifdef TRANSMISSION
    transmission_texcoord = texture_transform(texcoord0, texture_transmission_transform, texture_matrix0);
    normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0);
#if defined(HAS_SKIN)
    vec3 skinned_pos = (mat * pre_pos).xyz;
    vec3 view_normal = normalize((mat * vec4(normal.xyz + position.xyz, 1.0)).xyz - skinned_pos);
    vec3 view_tangent = (mat * vec4(tangent.xyz + position.xyz, 1.0)).xyz - skinned_pos;
#else
    vec3 view_normal = normalize(normal_matrix * normal);
    vec3 view_tangent = normal_matrix * tangent.xyz;
#endif
    vec4 transformed_tangent = tangent_space_transform(vec4(view_tangent, tangent.w), view_normal, texture_normal_transform, texture_matrix0);
    vary_normal = view_normal;
    vary_tangent = normalize(transformed_tangent.xyz);
    vary_sign = transformed_tangent.w;
#endif
}
