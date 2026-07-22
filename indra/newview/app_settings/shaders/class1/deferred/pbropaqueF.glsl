/**
 * @file pbropaqueF.glsl
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

/*[EXTRA_CODE_HERE]*/


#ifndef IS_HUD

// deferred opaque implementation

uniform sampler2D diffuseMap;  //always in sRGB space

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;
uniform float specularFactor;
uniform vec3 specularColorFactor;
uniform float emissiveStrength;
uniform float ior;
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness

out vec4 frag_data[4];

in vec3 vary_position;
in vec4 vertex_color;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;

in vec2 base_color_texcoord;
in vec2 normal_texcoord;
in vec2 metallic_roughness_texcoord;
in vec2 emissive_texcoord;

uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

uniform vec4 clipPlane;
uniform float clipSign;

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

uniform mat3 normal_matrix;

void main()
{
    mirrorClip(vary_position);

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);

    basecolor *= vertex_color;

    if (basecolor.a < minimum_alpha)
    {
        discard;
    }

    vec3 col = basecolor.rgb;

    // from mikktspace.com
    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 spec = texture(specularMap, metallic_roughness_texcoord.xy).rgb;

    float perceptualRoughness = spec.g * roughnessFactor;
    float metallic = spec.b * metallicFactor;
    float ao = spec.r;

    // KHR_materials_ior + KHR_materials_specular
    float ior_f0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    vec3 dielectric_f0 = min(vec3(ior_f0) * specularColorFactor, vec3(1.0)) * specularFactor;
    float dielectric_f90 = specularFactor;

    vec3 f0 = mix(dielectric_f0, col, metallic);
    vec3 diffuse = mix(col * (1.0 - max(dielectric_f0.r, max(dielectric_f0.g, dielectric_f0.b))), vec3(0.0), metallic);
    float specWeight = mix(dielectric_f90, 1.0, metallic);

    // KHR_materials_emissive_strength
    vec3 emissive = emissiveColor * emissiveStrength;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    tnorm *= gl_FrontFacing ? 1.0 : -1.0;

    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(diffuse, specWeight), vec4(0));                                         // Diffuse + specularWeight
    frag_data[1] = max(vec4(f0, perceptualRoughness), vec4(0));                                     // F0 + roughness
    frag_data[2] = encodeNormal(tnorm, ao, GBUFFER_FLAG_HAS_PBR);                                  // normal, occlusion, flags

#if defined(HAS_EMISSIVE)
    frag_data[3] = max(vec4(emissive, ior), vec4(0));                                               // Emissive + IOR
#endif
}

#else

// forward fullbright implementation for HUDs

uniform sampler2D diffuseMap;  //always in sRGB space

uniform vec3 emissiveColor;
uniform sampler2D emissiveMap;

out vec4 frag_color;

in vec3 vary_position;
in vec4 vertex_color;

in vec2 base_color_texcoord;
in vec2 emissive_texcoord;

uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

void main()
{
    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;

    basecolor.a *= vertex_color.a;

    if (basecolor.a < minimum_alpha)
    {
        discard;
    }

    vec3 col = vertex_color.rgb * srgb_to_linear(basecolor.rgb);

    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    col += emissive;

    // HUDs are rendered after gamma correction, output in sRGB space
    frag_color.rgb = linear_to_srgb(col);
    frag_color.a = 0.0;
}

#endif

