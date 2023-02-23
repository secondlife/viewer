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
vec2 texture_transform(vec2 vertex_texcoord, mat3 khr_gltf_transform, mat4 sl_animation_transform)
{
    vec2 texcoord = vertex_texcoord;

    // Convert to left-handed coordinate system. The offset of 1 is necessary
    // for rotations to be applied correctly.
    // In the future, we could bake this coordinate conversion into the uniform
    // that khr_gltf_transform comes from, since it's applied immediately
    // before.
    texcoord.y = 1.0 - texcoord.y;
    texcoord = (khr_gltf_transform * vec3(texcoord, 1.0)).xy;
    // Convert back to right-handed coordinate system
    texcoord.y = 1.0 - texcoord.y;
    texcoord = (sl_animation_transform * vec4(texcoord, 0, 1)).xy;

    // To make things more confusing, all SL image assets are upside-down
    // We may need an additional sign flip here when we implement a Vulkan backend

    return texcoord;
}
