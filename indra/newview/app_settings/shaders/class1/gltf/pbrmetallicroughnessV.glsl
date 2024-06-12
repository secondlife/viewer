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

#ifdef HAS_SKIN
uniform mat4 projection_matrix;
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif

uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_normal_transform;
uniform vec4[2] texture_metallic_roughness_transform;
uniform vec4[2] texture_emissive_transform;
uniform vec4[2] texture_occlusion_transform;

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

#ifdef MULTI_UV
in vec2 texcoord1;
uniform int base_color_texcoord;
uniform int emissive_texcoord;
#ifndef UNLIT
uniform int normal_texcoord;
uniform int metallic_roughness_texcoord;
uniform int occlusion_texcoord;
#endif
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
in uvec4 joint;
in vec4 weight4;

layout (std140) uniform GLTFJoints
{
    // list of OBBs for user override probes
    mat3x4 gltf_joints[MAX_JOINTS_PER_GLTF_OBJECT];
};

mat4 getGLTFSkinTransform()
{
    int i;

    vec4 w = weight4;

    uint i1 = joint.x;
    uint i2 = joint.y;
    uint i3 = joint.z;
    uint i4 = joint.w;

    mat3 mat = mat3(gltf_joints[i1])*w.x;
         mat += mat3(gltf_joints[i2])*w.y;
         mat += mat3(gltf_joints[i3])*w.z;
         mat += mat3(gltf_joints[i4])*w.w;

    vec3 trans = vec3(gltf_joints[i1][0].w,gltf_joints[i1][1].w,gltf_joints[i1][2].w)*w.x;
         trans += vec3(gltf_joints[i2][0].w,gltf_joints[i2][1].w,gltf_joints[i2][2].w)*w.y;
         trans += vec3(gltf_joints[i3][0].w,gltf_joints[i3][1].w,gltf_joints[i3][2].w)*w.z;
         trans += vec3(gltf_joints[i4][0].w,gltf_joints[i4][1].w,gltf_joints[i4][2].w)*w.w;

    mat4 ret;

    ret[0] = vec4(mat[0], 0);
    ret[1] = vec4(mat[1], 0);
    ret[2] = vec4(mat[2], 0);
    ret[3] = vec4(trans, 1.0);

    return ret;

#ifdef IS_AMD_CARD
   // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
   mat3x4 dummy1 = gltf_joints[0];
   mat3x4 dummy2 = gltf_joints[MAX_JOINTS_PER_GLTF_OBJECT-1];
#endif

}

#endif

void main()
{
#ifdef HAS_SKIN
    mat4 mat = getGLTFSkinTransform();

    mat = modelview_matrix * mat;

    vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
    vary_position = pos;

    vec4 vert = projection_matrix * vec4(pos, 1.0);
    gl_Position = vert;

#else
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
    //transform vertex
    vec4 vert = modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position = vert;
#endif

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
#ifdef HAS_SKIN
    vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
    vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;
#else //HAS_SKIN
    vec3 n = normal_matrix * normal;
    vec3 t = normal_matrix * tangent.xyz;
#endif

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




