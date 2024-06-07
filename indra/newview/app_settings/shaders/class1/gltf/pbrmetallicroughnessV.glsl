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
uniform mat4 texture_matrix0;

uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_normal_transform;
uniform vec4[2] texture_metallic_roughness_transform;
uniform vec4[2] texture_emissive_transform;

in vec3 position;
in vec4 diffuse_color;
in vec2 texcoord0;
out vec2 base_color_texcoord;
out vec2 emissive_texcoord;
out vec4 vertex_color;
out vec3 vary_position;

#ifndef UNLIT
in vec3 normal;
in vec4 tangent;
out vec2 normal_texcoord;
out vec2 metallic_roughness_texcoord;
out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;
vec3 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
#endif

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);


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

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);

#ifndef UNLIT
    normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0);
    metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0);
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
    vary_tangent = normalize(tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, texture_matrix0));
    vary_sign = tangent.w;
    vary_normal = n;
#endif

    vertex_color = diffuse_color;
#ifdef ALPHA_BLEND
    vary_fragcoord = vert.xyz;
#endif
}




