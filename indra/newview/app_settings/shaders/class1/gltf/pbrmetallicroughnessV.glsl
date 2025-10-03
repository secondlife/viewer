/**
 * @file pbrmetallicroughnessV.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
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

// GLTF pbrMetallicRoughness implementation

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

#ifdef MULTI_UV
in vec2 texcoord1;
int base_color_texcoord = 0;
int emissive_texcoord = 0;
#ifndef UNLIT
int normal_texcoord = 0;
int metallic_roughness_texcoord = 0;
int occlusion_texcoord = 0;
#endif
#endif

uniform int gltf_material_id;

layout (std140) uniform GLTFMaterials
{
    // index by gltf_material_id*12

    // [gltf_material_id + [0-1]] -  base color transform
    // [gltf_material_id + [2-3]] -  normal transform
    // [gltf_material_id + [4-5]] -  metallic roughness transform
    // [gltf_material_id + [6-7]] -  emissive transform
    // [gltf_material_id + [8-9]] -  occlusion transform
    // [gltf_material_id + 10]    -  emissive factor
    // [gltf_material_id + 11]    -  .r unused, .g roughness, .b metalness, .a minimum alpha

    // Transforms are packed as follows
    // packed[0] = vec4(scale.x, scale.y, rotation, offset.x)
    // packed[1] = vec4(mScale.y, texcoord, 0, 0)
    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

vec4[2] texture_base_color_transform;
vec4[2] texture_normal_transform;
vec4[2] texture_metallic_roughness_transform;
vec4[2] texture_emissive_transform;
vec4[2] texture_occlusion_transform;

void unpackTextureTransforms()
{
    if (gltf_material_id != -1)
    {
        int idx = gltf_material_id*12;

        texture_base_color_transform[0] = gltf_material_data[idx+0];
        texture_base_color_transform[1] = gltf_material_data[idx+1];

        texture_normal_transform[0] = gltf_material_data[idx+2];
        texture_normal_transform[1] = gltf_material_data[idx+3];

        texture_metallic_roughness_transform[0] = gltf_material_data[idx+4];
        texture_metallic_roughness_transform[1] = gltf_material_data[idx+5];

        texture_emissive_transform[0] = gltf_material_data[idx+6];
        texture_emissive_transform[1] = gltf_material_data[idx+7];

        texture_occlusion_transform[0] = gltf_material_data[idx+8];
        texture_occlusion_transform[1] = gltf_material_data[idx+9];

#ifdef MULTI_UV
        base_color_texcoord = int(gltf_material_data[idx+1].g);
        emissive_texcoord = int(gltf_material_data[idx+7].g);
#ifndef UNLIT
        normal_texcoord = int(gltf_material_data[idx+3].g);
        metallic_roughness_texcoord = int(gltf_material_data[idx+5].g);
        occlusion_texcoord = int(gltf_material_data[idx+9].g);
#endif
#endif
    }
    else
    {
        texture_base_color_transform[0] = vec4(1.0, 1.0, 0.0, 0.0);
        texture_base_color_transform[1] = vec4(0.0, 0.0, 0.0, 0.0);

        texture_normal_transform[0] = vec4(1.0, 1.0, 0.0, 0.0);
        texture_normal_transform[1] = vec4(0.0, 0.0, 0.0, 0.0);

        texture_metallic_roughness_transform[0] = vec4(1.0, 1.0, 0.0, 0.0);
        texture_metallic_roughness_transform[1] = vec4(0.0, 0.0, 0.0, 0.0);

        texture_emissive_transform[0] = vec4(1.0, 1.0, 0.0, 0.0);
        texture_emissive_transform[1] = vec4(0.0, 0.0, 0.0, 0.0);

        texture_occlusion_transform[0] = vec4(1.0, 1.0, 0.0, 0.0);
        texture_occlusion_transform[1] = vec4(0.0, 0.0, 0.0, 0.0);
    }
}


in vec3 position;
in vec4 diffuse_color;
in vec2 texcoord0;
out vec2 base_color_uv;
out vec2 emissive_uv;
out vec4 vertex_color;
out vec3 vary_position;

#ifndef UNLIT
in vec3 normal;
in vec4 tangent;
out vec2 normal_uv;
out vec2 metallic_roughness_uv;
out vec2 occlusion_uv;
out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;
#endif

vec2 gltf_texture_transform(vec2 texcoord, vec4[2] p)
{
    texcoord.y = 1.0 - texcoord.y;

    vec2 Scale = p[0].xy;
    float Rotation = -p[0].z;
    vec2 Offset = vec2(p[0].w, p[1].x);

    mat3 translation = mat3(1,0,0, 0,1,0, Offset.x, Offset.y, 1);
    mat3 rotation = mat3(
        cos(Rotation), sin(Rotation), 0,
        -sin(Rotation), cos(Rotation), 0,
        0, 0, 1);

    mat3 scale = mat3(Scale.x,0,0, 0,Scale.y,0, 0,0,1);

    mat3 matrix = translation * rotation * scale;

    vec2 uvTransformed = ( matrix * vec3(texcoord.xy, 1) ).xy;

    uvTransformed.y = 1.0 - uvTransformed.y;

    return uvTransformed;
}

#ifndef UNLIT
vec3 gltf_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform)
{ //derived from tangent_space_transform in textureUtilV.glsl
    vec2 weights = vec2(0, 1);

    // Convert to left-handed coordinate system
    weights.y = -weights.y;

    // Apply KHR_texture_transform (rotation only)
    float khr_rotation = khr_gltf_transform[0].z;
    mat2 khr_rotation_mat = mat2(
        cos(khr_rotation),-sin(khr_rotation),
        sin(khr_rotation), cos(khr_rotation)
    );
    weights = khr_rotation_mat * weights;

    // Convert back to right-handed coordinate system
    weights.y = -weights.y;

    // Similar to the MikkTSpace-compatible method of extracting the binormal
    // from the normal and tangent, as seen in the fragment shader
    vec3 vertex_binormal = vertex_tangent.w * cross(vertex_normal, vertex_tangent.xyz);

    return (weights.x * vertex_binormal.xyz) + (weights.y * vertex_tangent.xyz);

    return vertex_tangent.xyz;
}
#endif

#ifdef ALPHA_BLEND
out vec3 vary_fragcoord;
#endif

#ifdef HAS_SKIN

layout (std140) uniform GLTFJoints
{
    vec4 gltf_joints[MAX_NODES_PER_GLTF_OBJECT];
};


in uvec4 joint;
in vec4 weight4;

mat4 getGLTFTransform()
{
    int i;

    vec4 w = weight4;

    uint i1 = joint.x*3u;
    uint i2 = joint.y*3u;
    uint i3 = joint.z*3u;
    uint i4 = joint.w*3u;

    // lerp the joints
    vec4 v0 = gltf_joints[i1+0u] * w.x + gltf_joints[i2+0u] * w.y + gltf_joints[i3+0u] * w.z + gltf_joints[i4+0u] * w.w;
    vec4 v1 = gltf_joints[i1+1u] * w.x + gltf_joints[i2+1u] * w.y + gltf_joints[i3+1u] * w.z + gltf_joints[i4+1u] * w.w;
    vec4 v2 = gltf_joints[i1+2u] * w.x + gltf_joints[i2+2u] * w.y + gltf_joints[i3+2u] * w.z + gltf_joints[i4+2u] * w.w;

    //unpack into return matrix
    mat4 ret;
    ret[0] = vec4(v0.xyz, 0);
    ret[1] = vec4(v1.xyz, 0);
    ret[2] = vec4(v2.xyz, 0);
    ret[3] = vec4(v0.w, v1.w, v2.w, 1.0);

    return ret;
}

#else

layout (std140) uniform GLTFNodes
{
    vec4 gltf_nodes[MAX_NODES_PER_GLTF_OBJECT];
};

uniform int gltf_node_id = 0;

mat4 getGLTFTransform()
{
    mat4 ret;
    int idx = gltf_node_id*3;

    vec4 src0 = gltf_nodes[idx+0];
    vec4 src1 = gltf_nodes[idx+1];
    vec4 src2 = gltf_nodes[idx+2];

    ret[0] = vec4(src0.xyz, 0);
    ret[1] = vec4(src1.xyz, 0);
    ret[2] = vec4(src2.xyz, 0);

    ret[3] = vec4(src0.w, src1.w, src2.w, 1);

    return ret;
}

#endif

void main()
{
    unpackTextureTransforms();
    mat4 mat = getGLTFTransform();

    mat = modelview_matrix * mat;

    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
    vary_position = pos;

    vec4 vert = projection_matrix * vec4(pos, 1.0);
    gl_Position = vert;

    vec2 bcuv;
    vec2 emuv;

#ifdef MULTI_UV
    vec2 uv[2];
    uv[0] = texcoord0;
    uv[1] = texcoord1;

    bcuv = uv[base_color_texcoord];
    emuv = uv[emissive_texcoord];
#else
    bcuv = texcoord0;
    emuv = texcoord0;
#endif

    base_color_uv = gltf_texture_transform(bcuv, texture_base_color_transform);
    emissive_uv = gltf_texture_transform(emuv, texture_emissive_transform);

#ifndef UNLIT
    vec2 normuv;
    vec2 rmuv;
    vec2 ouv;
#ifdef MULTI_UV
    normuv = uv[normal_texcoord];
    rmuv = uv[metallic_roughness_texcoord];
    ouv = uv[occlusion_texcoord];
#else
    normuv = texcoord0;
    rmuv = texcoord0;
    ouv = texcoord0;
#endif
    normal_uv = gltf_texture_transform(normuv, texture_normal_transform);
    metallic_roughness_uv = gltf_texture_transform(rmuv, texture_metallic_roughness_transform);
    occlusion_uv = gltf_texture_transform(ouv, texture_occlusion_transform);
#endif

#ifndef UNLIT
    vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
    vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;

    n = normalize(n);
    vary_tangent = normalize(gltf_tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform));
    vary_sign = tangent.w;
    vary_normal = n;
#endif

    vertex_color = diffuse_color;
#ifdef ALPHA_BLEND
    vary_fragcoord = vert.xyz;
#endif
}




