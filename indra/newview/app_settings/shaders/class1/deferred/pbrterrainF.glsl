/** 
 * @file class1\deferred\terrainF.glsl
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

out vec4 frag_data[4];

uniform sampler2D alpha_ramp;

// TODO: Bind the right textures and uniforms during shader setup
// *TODO: Configurable quality level which disables PBR features on machines
// with limited texture availability
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures
uniform sampler2D detail_0_base_color;
uniform sampler2D detail_1_base_color;
uniform sampler2D detail_2_base_color;
uniform sampler2D detail_3_base_color;
uniform sampler2D detail_0_normal;
uniform sampler2D detail_1_normal;
uniform sampler2D detail_2_normal;
uniform sampler2D detail_3_normal;
uniform sampler2D detail_0_metallic_roughness;
uniform sampler2D detail_1_metallic_roughness;
uniform sampler2D detail_2_metallic_roughness;
uniform sampler2D detail_3_metallic_roughness;
uniform sampler2D detail_0_emissive;
uniform sampler2D detail_1_emissive;
uniform sampler2D detail_2_emissive;
uniform sampler2D detail_3_emissive;

// TODO: Needs new uniforms
// *TODO: More efficient packing?
uniform vec4 metallicFactors;
uniform vec4 roughnessFactors;
uniform vec3[4] emissiveColors;
uniform vec4 minimum_alphas; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
in vec4 vary_texcoord0;
in vec4 vary_texcoord1;

vec2 encode_normal(vec3 n);
vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

// TODO: This mixing function feels like it can be optimized. The terrain code's use of texcoord1 is dubious. It feels like the same thing can be accomplished with less memory bandwidth by calculating the offsets on-the-fly
float terrain_mix(vec4 samples, float alpha1, float alpha2, float alphaFinal)
{
    return mix( mix(samples.w, samples.z, alpha2), mix(samples.y, samples.x, alpha1), alphaFinal );
}

vec3 terrain_mix(vec3[4] samples, float alpha1, float alpha2, float alphaFinal)
{
    return mix( mix(samples[3], samples[2], alpha2), mix(samples[1], samples[0], alpha1), alphaFinal );
}

vec4 terrain_mix(vec4[4] samples, float alpha1, float alpha2, float alphaFinal)
{
    return mix( mix(samples[3], samples[2], alpha2), mix(samples[1], samples[0], alpha1), alphaFinal );
}

vec4 sample_and_mix_color(float alpha1, float alpha2, float alphaFinal, vec2 texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec4[4] samples;
    samples[0] = srgb_to_linear(texture2D(tex0, texcoord));
    samples[1] = srgb_to_linear(texture2D(tex1, texcoord));
    samples[2] = srgb_to_linear(texture2D(tex2, texcoord));
    samples[3] = srgb_to_linear(texture2D(tex3, texcoord));
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}

vec4 sample_and_mix_vector(float alpha1, float alpha2, float alphaFinal, vec2 texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec4[4] samples;
    samples[0] = texture2D(tex0, texcoord);
    samples[1] = texture2D(tex1, texcoord);
    samples[2] = texture2D(tex2, texcoord);
    samples[3] = texture2D(tex3, texcoord);
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}

// TODO: Implement
// TODO: Don't forget calls to srgb_to_linear during texture sampling
// TODO: Wherever base color alpha is not 1.0, blend with black
void main()
{
    float alpha1 = texture2D(alpha_ramp, vary_texcoord0.zw).a;
    float alpha2 = texture2D(alpha_ramp,vary_texcoord1.xy).a;
    float alphaFinal = texture2D(alpha_ramp, vary_texcoord1.zw).a;

    vec4 base_color = sample_and_mix_color(alpha1, alpha2, alphaFinal, vary_texcoord0.xy, detail_0_basecolor, detail_1_basecolor, detail_2_basecolor, detail_3_basecolor);
    vec4 normal_texture = sample_and_mix_vector(alpha1, alpha2, alphaFinal, vary_texcoord0.xy, detail_0_normal, detail_1_normal, detail_2_normal, detail_3_normal);
    vec4 metallic_roughness = sample_and_mix_vector(alpha1, alpha2, alphaFinal, vary_texcoord0.xy, detail_0_metallic_roughness, detail_1_metallic_roughness, detail_2_metallic_roughness, detail_3_metallic_roughness);
    vec4 emissive_texture = sample_and_mix_color(alpha1, alpha2, alphaFinal, vary_texcoord0.xy, detail_0_emissive, detail_1_emissive, detail_2_emissive, detail_3_emissive);

    float metallicFactor = terrain_mix(metallicFactors, alpha1, alpha2, alphaFinal);
    float roughnessFactor = terrain_mix(roughnessFactors, alpha1, alpha2, alphaFinal);
    vec3 emissiveColor = terrain_mix(emissiveColors, alpha1, alpha2, alphaFinal);
    float minimum_alpha = terrain_mix(minimum_alphas, alpha1, alpha2, alphaFinal);

    // TODO: OOh, we need blending for every GLTF uniform too
    // TODO: Unfork
    if (base_color.a < minimum_alpha)
    {
        base_color.rgb *= vec3(0.0);
    }

    vec3 col = base_color.rgb;

    // from mikktspace.com
    vec3 vNt = normal_texture.xyz*2.0-1.0;
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
    vec3 spec = metallic_roughness.rgb;
    
    spec.g *= roughness_factor;
    spec.b *= metallic_factor;

    vec3 emissive = emssive_color;
    emissive *= emissive_texture.rgb;

    tnorm *= gl_FrontFacing ? 1.0 : -1.0;

   
    frag_data[0] = max(vec4(col, 0.0), vec4(0));                                                   // Diffuse
    // TODO: What is packed into vertex_color.a?
    frag_data[1] = max(vec4(spec.rgb,vertex_color.a), vec4(0));                                    // PBR linear packed Occlusion, Roughness, Metal.
    // TODO: What is environment intensity and do we want it?
    frag_data[2] = max(vec4(encode_normal(tnorm), 0.0, GBUFFER_FLAG_HAS_PBR | GBUFFER_FLAG_HAS_ATMOS), vec4(0)); // normal, environment intensity, flags
    frag_data[3] = max(vec4(emissive,0), vec4(0));                                                // PBR sRGB Emissive
}

