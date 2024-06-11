/**
 * @file class1\environment\pbrterrainV.glsl
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

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

in vec3 position;
in vec3 normal;
in vec4 tangent;
in vec4 diffuse_color;
in vec2 texcoord1;

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
out vec4[2] vary_coords;
#endif
out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
out vec3 vary_normal;
out vec3 vary_tangent;
flat out float vary_sign;
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;
out vec3 vary_position;

// *HACK: tangent_space_transform should use texture_normal_transform, or maybe
// we shouldn't use tangent_space_transform at all. See the call to
// tangent_space_transform below.
uniform vec4[2] texture_base_color_transform;

vec2 terrain_texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform);
vec3 terrain_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform);

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;

    vec3 n = normal_matrix * normal;
    vary_vertex_normal = normal;
    vec3 t = normal_matrix * tangent.xyz;

    vary_tangent = normalize(t);
    // *TODO: Decide if we want this. It may be better to just calculate the
    // tangents on-the-fly in the fragment shader, due to the subtleties of the
    // effect of triplanar mapping on UVs.
    // *HACK: Should be using texture_normal_transform here. The KHR texture
    // transform spec requires handling texture transforms separately for each
    // individual texture.
    vary_tangent = normalize(terrain_tangent_space_transform(vec4(t, tangent.w), n, texture_base_color_transform));
    vary_sign = tangent.w;
    vary_normal = normalize(n);

    // Transform and pass tex coords
    // *HACK: texture_base_color_transform is used for all of these here, but
    // the KHR texture transform spec requires handling texture transforms
    // separately for each individual texture.
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
    // xy
    vary_coords[0].xy = terrain_texture_transform(position.xy, texture_base_color_transform);
    // yz
    vary_coords[0].zw = terrain_texture_transform(position.yz, texture_base_color_transform);
    // (-x)z
    vary_coords[1].xy = terrain_texture_transform(position.xz * vec2(-1, 1), texture_base_color_transform);
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
    vary_texcoord0.xy = terrain_texture_transform(position.xy, texture_base_color_transform);
#endif

    vec4 tc = vec4(texcoord1,0,1);
    vary_texcoord0.zw = tc.xy;
    vary_texcoord1.xy = tc.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = tc.xy-vec2(1.0, 0.0);
}
