/**
 * @file gltfpbrV.glsl
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

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec3 position;

#ifdef HAS_NORMAL
in vec3 normal;
#endif

#ifdef HAS_FRAGMENT_NORMAL
out vec3 vary_normal;
#endif

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#endif

#ifdef HAS_TEXTURE
in vec2 texcoord0;
vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
mat4 tex_mat;
#endif

#ifdef SAMPLE_BASE_COLOR_MAP
vec4[2] texture_base_color_transform;
out vec2 base_color_texcoord;
#endif

#ifdef SAMPLE_NORMAL_MAP
in vec4 tangent;

vec4[2] texture_normal_transform;

out vec2 normal_texcoord;

out vec3 vary_tangent;
flat out float vary_sign;

vec4 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
#endif

#ifdef SAMPLE_ORM_MAP
vec4[2] texture_metallic_roughness_transform;

out vec2 metallic_roughness_texcoord;
#endif

#ifdef SAMPLE_EMISSIVE_MAP
vec4[2] texture_emissive_transform;

out vec2 emissive_texcoord;
#endif

#ifdef FRAG_POSITION
out vec3 vary_position;
#endif

#ifdef ALPHA_BLEND
out vec3 vary_fragcoord;
#endif

layout (std140) uniform GLTFNodes
{
    vec4 gltf_nodes[MAX_UBO_VEC4S];
};

#ifdef TEX_ANIM
layout (std140) uniform TextureTransform
{
    vec4 texture_matrix[MAX_UBO_VEC4S];
};
#endif

layout (std140) uniform GLTFNodeInstanceMap
{
    // .x - gltf_node_id
    // .y - gltf_material_id
    // .z - texture_matrix_id
    ivec4 gltf_node_instance_map[MAX_INSTANCES_PER_GLTF_OBJECT];
};


#ifdef PLANAR_PROJECTION
vec3 prim_scale;

void planarProjection(inout vec2 tc)
{
    vec3 binormal;
    vec3 vec = position * prim_scale;

    float d = normal.x;

    if (d >= 0.5 || d <= -0.5)
    {
        if (d < 0.0)
        {
            binormal = vec3(0,-1,0);
        }
        else
        {
            binormal = vec3(0, 1, 0);
        }
    }
    else
    {
        if (normal.y > 0)
        {
            binormal = vec3(-1,0,0);
        }
        else
        {
            binormal = vec3(1,0,0);
        }
    }
    vec3 tangent;
    tangent = cross(binormal, normal);

    tc.y = -(dot(tangent,vec)*2.0 - 0.5);
    tc.x = 1.0+(dot(binormal, vec)*2.0 - 0.5);
}
#endif

uniform int gltf_base_instance;

#ifdef SAMPLE_MATERIALS_UBO
layout (std140) uniform GLTFMaterials
{
    // index by gltf_material_id

    // [gltf_material_id + [0-1]] -  base color transform
    // [gltf_material_id + [2-3]] -  normal transform
    // [gltf_material_id + [4-5]] -  metallic roughness transform
    // [gltf_material_id + [6-7]] -  emissive transform

    // Transforms are packed as follows
    // packed[0] = vec4(scale.x, scale.y, rotation, offset.x)
    // packed[1] = vec4(offset.y, *, *, *)

    // packed[1].yzw varies:
    //   base color transform -- base color factor
    //   metallic roughness transform -- .y - roughness factor, .z - metallic factor
    //   emissive transform -- emissive factor
    //   normal transform -- .y - alpha factor, .z - minimum alpha

    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

flat out int gltf_material_id;

void unpackTextureTransforms()
{
    gltf_material_id = gltf_node_instance_map[gl_InstanceID+gltf_base_instance].y;

    int idx = gltf_material_id;

#ifdef SAMPLE_BASE_COLOR_MAP
    texture_base_color_transform[0] = gltf_material_data[idx+0];
    texture_base_color_transform[1] = vec4(gltf_material_data[idx+0].w, gltf_material_data[idx+1].x, 0, 0);
#endif

#ifdef SAMPLE_NORMAL_MAP
    texture_normal_transform[0] = gltf_material_data[idx+2];
    texture_normal_transform[1] = vec4(gltf_material_data[idx+2].w, gltf_material_data[idx+3].x, 0, 0);
#endif

#ifdef SAMPLE_ORM_MAP
    texture_metallic_roughness_transform[0] = gltf_material_data[idx+4];
    texture_metallic_roughness_transform[1] = vec4(gltf_material_data[idx+4].w, gltf_material_data[idx+5].x, 0, 0);
#endif

#ifdef SAMPLE_EMISSIVE_MAP
    texture_emissive_transform[0] = gltf_material_data[idx+6];
    texture_emissive_transform[1] = vec4(gltf_material_data[idx+6].w, gltf_material_data[idx+7].x, 0, 0);
#endif
}
#else // SAMPLE_MATERIALS_UBO
void unpackTextureTransforms()
{
}
#endif

mat4 getGLTFTransform()
{
    unpackTextureTransforms();
    int gltf_node_id = gltf_node_instance_map[gl_InstanceID+gltf_base_instance].x;

    mat4 ret;
    int idx = gltf_node_id*3;
    vec4 src0 = gltf_nodes[idx+0];
    vec4 src1 = gltf_nodes[idx+1];
    vec4 src2 = gltf_nodes[idx+2];

    ret[0] = vec4(src0.xyz, 0);
    ret[1] = vec4(src1.xyz, 0);
    ret[2] = vec4(src2.xyz, 0);

    ret[3] = vec4(src0.w, src1.w, src2.w, 1);

#ifndef HAS_SKIN
    vec4 root0 = gltf_nodes[0];
    vec4 root1 = gltf_nodes[1];
    vec4 root2 = gltf_nodes[2];

    mat4 root4;
    root4[0] = vec4(root0.xyz, 0);
    root4[1] = vec4(root1.xyz, 0);
    root4[2] = vec4(root2.xyz, 0);
    root4[3] = vec4(root0.w, root1.w, root2.w, 1);

    ret = root4 * ret;
#endif

#ifdef PLANAR_PROJECTION
    prim_scale = gltf_material_data[gltf_node_instance_map[gl_InstanceID+gltf_base_instance].w].xyz;
#endif
    return ret;
}

void main()
{
    mat4 mat = getGLTFTransform();
#ifdef HAS_SKIN
    // mat should be the BindShapeMatrix for rigged objects
    mat = getObjectSkinnedTransform() * mat;
#endif

    mat = modelview_matrix * mat;
    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
    vec4 vert = projection_matrix*vec4(pos,1.0);
    gl_Position = vert;

#ifdef ALPHA_BLEND
    vary_fragcoord = vert.xyz;
#endif

#ifdef HAS_TEXTURE

#ifdef TEX_ANIM
    int idx = gltf_node_instance_map[gl_InstanceID+gltf_base_instance].z*3;
    vec4 src0 = texture_matrix[idx+0];
    vec4 src1 = texture_matrix[idx+1];
    vec4 src2 = texture_matrix[idx+2];

    tex_mat[0] = vec4(src0.xyz, 0);
    tex_mat[1] = vec4(src1.xyz, 0);
    tex_mat[2] = vec4(src2.xyz, 0);
    tex_mat[3] = vec4(src0.w, src1.w, src2.w, 1);
#else
    tex_mat[0] = vec4(1,0,0,0);
    tex_mat[1] = vec4(0,1,0,0);
    tex_mat[2] = vec4(0,0,1,0);
    tex_mat[3] = vec4(0,0,0,1);
#endif

    vec2 tc0 = texcoord0;
#ifdef PLANAR_PROJECTION
    planarProjection(tc0);
#endif

#endif

#ifdef SAMPLE_BASE_COLOR_MAP
    base_color_texcoord = texture_transform(tc0, texture_base_color_transform, tex_mat);
#endif

#ifdef FRAG_POSITION
    vary_position = pos;
#endif

#ifdef HAS_NORMAL
    vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
#endif

#ifdef SAMPLE_NORMAL_MAP
    normal_texcoord = texture_transform(tc0, texture_normal_transform, tex_mat);
    vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;

    n = normalize(n);

    vec4 transformed_tangent = tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, tex_mat);
    vary_tangent = normalize(transformed_tangent.xyz);
    vary_sign = transformed_tangent.w;
#endif

#ifdef HAS_FRAGMENT_NORMAL
    vary_normal = n;
#endif

#ifdef SAMPLE_ORM_MAP
    metallic_roughness_texcoord = texture_transform(tc0, texture_metallic_roughness_transform, tex_mat);
#endif

#ifdef SAMPLE_EMISSIVE_MAP
    emissive_texcoord = texture_transform(tc0, texture_emissive_transform, tex_mat);
#endif
}
