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


#ifndef IS_HUD

//deferred opaque implementation

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec3 position;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#endif

#ifdef SAMPLE_BASE_COLOR_MAP
uniform mat4 texture_matrix0;
uniform vec4[2] texture_base_color_transform;

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

in vec2 texcoord0;
out vec2 base_color_texcoord;
#endif

#ifdef SAMPLE_NORMAL_MAP
in vec3 normal;
in vec4 tangent;

uniform vec4[2] texture_normal_transform;

out vec2 normal_texcoord;

out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;

vec4 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);
#endif

#ifdef SAMPLE_ORM_MAP
uniform vec4[2] texture_metallic_roughness_transform;

out vec2 metallic_roughness_texcoord;
#endif

#ifdef SAMPLE_EMISSIVE_MAP
uniform vec4[2] texture_emissive_transform;

out vec2 emissive_texcoord;
#endif

#ifdef MIRROR_CLIP
out vec3 vary_position;
#endif

layout (std140) uniform GLTFNodes
{
    mat3x4 gltf_nodes[MAX_NODES_PER_GLTF_OBJECT];
};

layout (std140) uniform GLTFNodeInstanceMap
{
    ivec4 gltf_node_instance_map[MAX_INSTANCES_PER_GLTF_OBJECT];
};


uniform int gltf_base_instance;

mat4 getGLTFTransform()
{
    int gltf_node_id = gltf_node_instance_map[gl_InstanceID+gltf_base_instance].x;
    mat4 ret;
    mat3x4 src = gltf_nodes[gltf_node_id];

    ret[0] = vec4(src[0].xyz, 0);
    ret[1] = vec4(src[1].xyz, 0);
    ret[2] = vec4(src[2].xyz, 0);

    ret[3] = vec4(src[0].w, src[1].w, src[2].w, 1);

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
    gl_Position = projection_matrix*vec4(pos,1.0);

#ifdef SAMPLE_BASE_COLOR_MAP
    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
#endif

#ifdef MIRROR_CLIP
    vary_position = pos;
#endif

#ifdef SAMPLE_NORMAL_MAP
    normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0);
    vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
    vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;

    n = normalize(n);

    vec4 transformed_tangent = tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, texture_matrix0);
    vary_tangent = normalize(transformed_tangent.xyz);
    vary_sign = transformed_tangent.w;
    vary_normal = n;
#endif

#ifdef SAMPLE_ORM_MAP
    metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0);
#endif

#ifdef SAMPLE_EMISSIVE_MAP
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);
#endif
}

#else

// fullbright HUD implementation

uniform mat4 modelview_projection_matrix;

uniform mat4 texture_matrix0;

uniform vec4[2] texture_base_color_transform;
uniform vec4[2] texture_emissive_transform;

in vec3 position;
in vec2 texcoord0;

out vec2 base_color_texcoord;
out vec2 emissive_texcoord;

vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform);

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    base_color_texcoord = texture_transform(texcoord0, texture_base_color_transform, texture_matrix0);
    emissive_texcoord = texture_transform(texcoord0, texture_emissive_transform, texture_matrix0);
}

#endif


