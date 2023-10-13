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

#define TERRAIN_PBR_DETAIL_EMISSIVE 0

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
#define TerrainCoord vec4[2]
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
#define TerrainCoord vec2
#endif

#define MIX_X    1 << 3
#define MIX_Y    1 << 4
#define MIX_Z    1 << 5
#define MIX_W    1 << 6

// TODO: Decide if this struct needs to be declared
struct TerrainMix
{
    vec4 weight;
    int type;
};

TerrainMix get_terrain_mix_weights(float alpha1, float alpha2, float alphaFinal);

// TODO: Decide if this struct needs to be declared
struct PBRMix
{
    vec4 col;       // RGB color with alpha, linear space
    vec3 orm;       // Occlusion, roughness, metallic
    vec3 vNt;       // Unpacked normal texture sample, vector
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    vec3 emissive;  // RGB emissive color, linear space
#endif
};

PBRMix init_pbr_mix();

PBRMix terrain_sample_and_multiply_pbr(
    TerrainCoord terrain_coord
    , sampler2D tex_col
    , sampler2D tex_orm
    , sampler2D tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    , sampler2D tex_emissive
#endif
    , vec4 factor_col
    , vec3 factor_orm
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    , vec3 factor_emissive
#endif
    );

PBRMix mix_pbr(PBRMix mix1, PBRMix mix2, float mix2_weight);

out vec4 frag_data[4];

uniform sampler2D alpha_ramp;

// *TODO: More configurable quality level which disables PBR features on machines
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
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
uniform sampler2D detail_0_emissive;
uniform sampler2D detail_1_emissive;
uniform sampler2D detail_2_emissive;
uniform sampler2D detail_3_emissive;
#endif

uniform vec4[4] baseColorFactors; // See also vertex_color in pbropaqueV.glsl
uniform vec4 metallicFactors;
uniform vec4 roughnessFactors;
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
uniform vec3[4] emissiveColors;
#endif
uniform vec4 minimum_alphas; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
in vec4[2] vary_coords;
#endif
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
in vec4 vary_texcoord0;
in vec4 vary_texcoord1;

vec2 encode_normal(vec3 n);

float terrain_mix(TerrainMix tm, vec4 tms4);

void main()
{

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
    TerrainCoord terrain_texcoord = vary_coords;
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
    TerrainCoord terrain_texcoord = vary_texcoord0.xy;
#endif

    float alpha1 = texture(alpha_ramp, vary_texcoord0.zw).a;
    float alpha2 = texture(alpha_ramp,vary_texcoord1.xy).a;
    float alphaFinal = texture(alpha_ramp, vary_texcoord1.zw).a;

    TerrainMix tm = get_terrain_mix_weights(alpha1, alpha2, alphaFinal);

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3[4] orm_factors;
    orm_factors[0] = vec3(1.0, roughnessFactors.x, metallicFactors.x);
    orm_factors[1] = vec3(1.0, roughnessFactors.y, metallicFactors.y);
    orm_factors[2] = vec3(1.0, roughnessFactors.z, metallicFactors.z);
    orm_factors[3] = vec3(1.0, roughnessFactors.w, metallicFactors.w);

    PBRMix mix = init_pbr_mix();
    PBRMix mix2;
    switch (tm.type & MIX_X)
    {
    case MIX_X:
        mix2 = terrain_sample_and_multiply_pbr(
            terrain_texcoord
            , detail_0_base_color
            , detail_0_metallic_roughness
            , detail_0_normal
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , detail_0_emissive
#endif
            , baseColorFactors[0]
            , orm_factors[0]
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , emissiveColors[0]
#endif
        );
        mix = mix_pbr(mix, mix2, tm.weight.x);
        break;
    default:
        break;
    }
    switch (tm.type & MIX_Y)
    {
    case MIX_Y:
        mix2 = terrain_sample_and_multiply_pbr(
            terrain_texcoord
            , detail_1_base_color
            , detail_1_metallic_roughness
            , detail_1_normal
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , detail_1_emissive
#endif
            , baseColorFactors[1]
            , orm_factors[1]
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , emissiveColors[1]
#endif
        );
        mix = mix_pbr(mix, mix2, tm.weight.y);
        break;
    default:
        break;
    }
    switch (tm.type & MIX_Z)
    {
    case MIX_Z:
        mix2 = terrain_sample_and_multiply_pbr(
            terrain_texcoord
            , detail_2_base_color
            , detail_2_metallic_roughness
            , detail_2_normal
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , detail_2_emissive
#endif
            , baseColorFactors[2]
            , orm_factors[2]
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , emissiveColors[2]
#endif
        );
        mix = mix_pbr(mix, mix2, tm.weight.z);
        break;
    default:
        break;
    }
    switch (tm.type & MIX_W)
    {
    case MIX_W:
        mix2 = terrain_sample_and_multiply_pbr(
            terrain_texcoord
            , detail_3_base_color
            , detail_3_metallic_roughness
            , detail_3_normal
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , detail_3_emissive
#endif
            , baseColorFactors[3]
            , orm_factors[3]
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , emissiveColors[3]
#endif
        );
        mix = mix_pbr(mix, mix2, tm.weight.w);
        break;
    default:
        break;
    }

    float minimum_alpha = terrain_mix(tm, minimum_alphas);
    if (mix.col.a < minimum_alpha)
    {
        discard;
    }
    float base_color_factor_alpha = terrain_mix(tm, vec4(baseColorFactors[0].z, baseColorFactors[1].z, baseColorFactors[2].z, baseColorFactors[3].z));

    // from mikktspace.com
    vec3 vNt = mix.vNt;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;
    
    vec3 vB = vary_sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    tnorm *= gl_FrontFacing ? 1.0 : -1.0;
   
    frag_data[0] = max(vec4(mix.col.xyz, 0.0), vec4(0));                                                   // Diffuse
    frag_data[1] = max(vec4(mix.orm.rgb, base_color_factor_alpha), vec4(0));                                    // PBR linear packed Occlusion, Roughness, Metal.
    frag_data[2] = max(vec4(encode_normal(tnorm), base_color_factor_alpha, GBUFFER_FLAG_HAS_PBR), vec4(0)); // normal, environment intensity, flags
    frag_data[3] = max(vec4(mix.emissive,0), vec4(0));                                                // PBR sRGB Emissive
}

