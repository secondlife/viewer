/** 
 * @file class1/deferred/textureUtilV.glsl
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

// This shader code is taken from the sample code on the KHR_texture_transform
// spec page page, plus or minus some sign error corrections (I think because the GLSL
// matrix constructor is backwards?):
// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform
// Previously (6494eed242b1), we passed in a single, precalculated matrix
// uniform per transform into the shaders. However, that was found to produce
// small-but-noticeable discrepancies with the GLTF sample model
// "TextureTransformTest", likely due to numerical precision differences. In
// the interest of parity with other renderers, calculate the transform
// directly in the shader. -Cosmic,2023-02-24
vec2 khr_texture_transform(vec2 texcoord, vec2 scale, float rotation, vec2 offset)
{
    mat3 scale_mat = mat3(scale.x,0,0, 0,scale.y,0, 0,0,1);
    mat3 offset_mat = mat3(1,0,0, 0,1,0, offset.x, offset.y, 1);
    mat3 rotation_mat = mat3(
        cos(rotation),-sin(rotation), 0,
        sin(rotation), cos(rotation), 0,
                    0,             0, 1
    );

    mat3 transform = offset_mat * rotation_mat * scale_mat;

    return (transform * vec3(texcoord, 1)).xy;
}

// vertex_texcoord - The UV texture coordinates sampled from the vertex at
//     runtime. Per SL convention, this is in a right-handed UV coordinate
//     system. Collada models also have right-handed UVs.
// khr_gltf_transform - The texture transform matrix as defined in the
//     KHR_texture_transform GLTF extension spec. It assumes a left-handed UV
//     coordinate system. GLTF models also have left-handed UVs.
// sl_animation_transform - The texture transform matrix for texture
//     animations, available through LSL script functions such as
//     LlSetTextureAnim. It assumes a right-handed UV coordinate system.
// texcoord - The final texcoord to use for image sampling
vec2 texture_transform(vec2 vertex_texcoord, vec4[2] khr_gltf_transform, mat4 sl_animation_transform)
{
    vec2 texcoord = vertex_texcoord;

    // Apply texture animation first to avoid shearing and other artifacts
    texcoord = (sl_animation_transform * vec4(texcoord, 0, 1)).xy;
    // Convert to left-handed coordinate system. The offset of 1 is necessary
    // for rotations to be applied correctly.
    texcoord.y = 1.0 - texcoord.y;
    texcoord = khr_texture_transform(texcoord, khr_gltf_transform[0].xy, khr_gltf_transform[0].z, khr_gltf_transform[1].xy);
    // Convert back to right-handed coordinate system
    texcoord.y = 1.0 - texcoord.y;

    // To make things more confusing, all SL image assets are upside-down
    // We may need an additional sign flip here when we implement a Vulkan backend

    return texcoord;
}

// Take the rotation only from both transforms and apply to the tangent. This
// accounts for the change of the topology of the normal texture when a texture
// rotation is applied to it.
// *HACK: Assume the imported GLTF model did not have both normal texture
// transforms and tangent vertices. The use of this function is inconsistent
// with the GLTF sample viewer when that is the case. See getNormalInfo in
// https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Viewer/47a191931461a6f2e14de48d6da0f0eb6ec2d147/source/Renderer/shaders/material_info.glsl
// We may want to account for this case during GLTF model import.
// -Cosmic,2023-06-06
vec3 tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec4[2] khr_gltf_transform, mat4 sl_animation_transform)
{
    vec2 weights = vec2(0, 1);

    // Apply texture animation first to avoid shearing and other artifacts (rotation only)
    mat2 sl_rot_scale;
    sl_rot_scale[0][0] = sl_animation_transform[0][0];
    sl_rot_scale[0][1] = sl_animation_transform[0][1];
    sl_rot_scale[1][0] = sl_animation_transform[1][0];
    sl_rot_scale[1][1] = sl_animation_transform[1][1];
    weights = sl_rot_scale * weights;
    // Remove scale
    weights = normalize(weights);

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
}
