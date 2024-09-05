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

#define TERRAIN_PBR_DETAIL_EMISSIVE 0
#define TERRAIN_PBR_DETAIL_OCCLUSION -1
#define TERRAIN_PBR_DETAIL_NORMAL -2
#define TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS -3

#define TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE 0
#define TERRAIN_PAINT_TYPE_PBR_PAINTMAP 1

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
uniform float region_scale;
#endif

in vec3 position;
in vec3 normal;
in vec4 tangent;
in vec4 diffuse_color;
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
in vec2 texcoord1;
#endif

out vec3 vary_position;
out vec3 vary_normal;
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
#endif
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
out vec3 vary_tangents[4];
flat out float vary_signs[4];
#endif

// vary_texcoord* are used for terrain composition, vary_coords are used for terrain UVs
#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;
#elif TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
out vec2 vary_texcoord;
#endif
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
out vec4[10] vary_coords;
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
out vec4[2] vary_coords;
#endif

// *HACK: Each material uses only one texture transform, but the KHR texture
// transform spec allows handling texture transforms separately for each
// individual texture info.
uniform vec4[5] terrain_texture_transforms;

vec2 terrain_texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform);
vec4 terrain_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform);

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;

    vec3 n = normal_matrix * normal;
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
    vary_vertex_normal = normal;
#endif
    vec3 t = normal_matrix * tangent.xyz;

#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_NORMAL)
    {
        vec4[2] ttt;
        vec4 transformed_tangent;
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[0] = normalize(transformed_tangent.xyz);
        vary_signs[0] = transformed_tangent.w;
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[1] = normalize(transformed_tangent.xyz);
        vary_signs[1] = transformed_tangent.w;
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[2] = normalize(transformed_tangent.xyz);
        vary_signs[2] = transformed_tangent.w;
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        transformed_tangent = terrain_tangent_space_transform(vec4(t, tangent.w), n, ttt);
        vary_tangents[3] = normalize(transformed_tangent.xyz);
        vary_signs[3] = transformed_tangent.w;
    }
#endif
    vary_normal = normalize(n);

    // Transform and pass tex coords
    {
        vec4[2] ttt;
#define transform_xy()             terrain_texture_transform(position.xy,               ttt)
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Don't care about upside-down (transform_xy_flipped())
#define transform_yz()             terrain_texture_transform(position.yz,               ttt)
#define transform_negx_z()         terrain_texture_transform(position.xz * vec2(-1, 1), ttt)
#define transform_yz_flipped()     terrain_texture_transform(position.yz * vec2(-1, 1), ttt)
#define transform_negx_z_flipped() terrain_texture_transform(position.xz,               ttt)
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = transform_xy();
        vary_coords[0].zw = transform_yz();
        vary_coords[1].xy = transform_negx_z();
        vary_coords[1].zw = transform_yz_flipped();
        vary_coords[2].xy = transform_negx_z_flipped();
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[2].zw = transform_xy();
        vary_coords[3].xy = transform_yz();
        vary_coords[3].zw = transform_negx_z();
        vary_coords[4].xy = transform_yz_flipped();
        vary_coords[4].zw = transform_negx_z_flipped();
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[5].xy = transform_xy();
        vary_coords[5].zw = transform_yz();
        vary_coords[6].xy = transform_negx_z();
        vary_coords[6].zw = transform_yz_flipped();
        vary_coords[7].xy = transform_negx_z_flipped();
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[7].zw = transform_xy();
        vary_coords[8].xy = transform_yz();
        vary_coords[8].zw = transform_negx_z();
        vary_coords[9].xy = transform_yz_flipped();
        vary_coords[9].zw = transform_negx_z_flipped();
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
        // material 1
        ttt[0].xyz = terrain_texture_transforms[0].xyz;
        ttt[1].x = terrain_texture_transforms[0].w;
        ttt[1].y = terrain_texture_transforms[1].x;
        vary_coords[0].xy = transform_xy();
        // material 2
        ttt[0].xyz = terrain_texture_transforms[1].yzw;
        ttt[1].xy = terrain_texture_transforms[2].xy;
        vary_coords[0].zw = transform_xy();
        // material 3
        ttt[0].xy = terrain_texture_transforms[2].zw;
        ttt[0].z = terrain_texture_transforms[3].x;
        ttt[1].xy = terrain_texture_transforms[3].yz;
        vary_coords[1].xy = transform_xy();
        // material 4
        ttt[0].x = terrain_texture_transforms[3].w;
        ttt[0].yz = terrain_texture_transforms[4].xy;
        ttt[1].xy = terrain_texture_transforms[4].zw;
        vary_coords[1].zw = transform_xy();
#endif
    }

#if TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE
    vec2 tc = texcoord1.xy;
    vary_texcoord0.zw = tc.xy;
    vary_texcoord1.xy = tc.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = tc.xy-vec2(1.0, 0.0);
#elif TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
    vary_texcoord = position.xy / region_scale;
#endif
}
