/** 
 * @file class1\deferred\pbrterrainUtilF.glsl
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

/**
 * Triplanar mapping implementation adapted from Inigo Quilez' example shader,
 * MIT license.
 * https://www.shadertoy.com/view/MtsGWH
 * Copyright © 2015 Inigo Quilez
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions: The above copyright
 * notice and this permission notice shall be included in all copies or
 * substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define TERRAIN_PBR_DETAIL_EMISSIVE 0

in vec3 vary_vertex_normal;

vec3 srgb_to_linear(vec3 c);

// A relatively agressive threshold for terrain material mixing sampling
// cutoff. This ensures that only one or two materials are used in most places,
// making PBR terrain blending more performant. Should be greater than 0 to work.
#define TERRAIN_RAMP_MIX_THRESHOLD 0.1
// A small threshold for triplanar mapping sampling cutoff. This and
// TERRAIN_TRIPLANAR_BLEND_FACTOR together ensures that only one or two samples
// per texture are used in most places, making triplanar mapping more
// performant. Should be greater than 0 to work.
// There's also an artistic design choice in the use of these factors, and the
// use of triplanar generally. Don't take these triplanar constants for granted.
#define TERRAIN_TRIPLANAR_MIX_THRESHOLD 0.01

#define SAMPLE_X 1 << 0
#define SAMPLE_Y 1 << 1
#define SAMPLE_Z 1 << 2
#define MIX_X    1 << 3
#define MIX_Y    1 << 4
#define MIX_Z    1 << 5
#define MIX_W    1 << 6

struct PBRMix
{
    vec4 col;       // RGB color with alpha, linear space
    vec3 orm;       // Occlusion, roughness, metallic
    vec3 vNt;       // Unpacked normal texture sample, vector
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    vec3 emissive;  // RGB emissive color, linear space
#endif
};

PBRMix init_pbr_mix()
{
    PBRMix mix;
    mix.col = vec4(0);
    mix.orm = vec3(0);
    mix.vNt = vec3(0);
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    mix.emissive = vec3(0);
#endif
    return mix;
}

// Usage example, for two weights:
// vec2 weights = ... // Weights must add up to 1
// PBRMix mix = init_pbr_mix();
// PBRMix mix1 = ...
// mix = mix_pbr(mix, mix1, weights.x);
// PBRMix mix2 = ...
// mix = mix_pbr(mix, mix2, weights.y);
PBRMix mix_pbr(PBRMix mix1, PBRMix mix2, float mix2_weight)
{
    PBRMix mix;
    mix.col      = mix1.col      + (mix2.col      * mix2_weight);
    mix.orm      = mix1.orm      + (mix2.orm      * mix2_weight);
    mix.vNt      = mix1.vNt      + (mix2.vNt      * mix2_weight);
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    mix.emissive = mix1.emissive + (mix2.emissive * mix2_weight);
#endif
    return mix;
}

PBRMix sample_pbr(
    vec2 uv
    , sampler2D tex_col
    , sampler2D tex_orm
    , sampler2D tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    , sampler2D tex_emissive
#endif
    )
{
    PBRMix mix;
    mix.col = texture(tex_col, uv);
    mix.col.rgb = srgb_to_linear(mix.col.rgb);
    mix.orm = texture(tex_orm, uv).xyz;
    mix.vNt = texture(tex_vNt, uv).xyz*2.0-1.0;
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    mix.emissive = srgb_to_linear(texture(tex_emissive, uv).xyz);
#endif
    return mix;
}

struct TerrainTriplanar
{
    vec3 weight;
    int type;
};

#if 0 // TODO: Remove debug
#define TERRAIN_DEBUG 1
#endif
struct TerrainMix
{
    vec4 weight;
    int type;
#if TERRAIN_DEBUG
    ivec4 usage;
#endif
};

#define TerrainMixSample vec4[4]
#define TerrainMixSample3 vec3[4]

TerrainMix _t_mix(float alpha1, float alpha2, float alphaFinal)
{
    TerrainMix tm;
    vec4 sample_x = vec4(1,0,0,0);
    vec4 sample_y = vec4(0,1,0,0);
    vec4 sample_z = vec4(0,0,1,0);
    vec4 sample_w = vec4(0,0,0,1);

    tm.weight = mix( mix(sample_w, sample_z, alpha2), mix(sample_y, sample_x, alpha1), alphaFinal );
    tm.weight -= TERRAIN_RAMP_MIX_THRESHOLD;
    ivec4 usage = max(ivec4(0), ivec4(ceil(tm.weight)));
    // Prevent negative weights and keep weights balanced
    tm.weight = tm.weight*vec4(usage);
    tm.weight /= (tm.weight.x + tm.weight.y + tm.weight.z + tm.weight.w);

    tm.type = (usage.x * MIX_X) |
              (usage.y * MIX_Y) |
              (usage.z * MIX_Z) |
              (usage.w * MIX_W);
#if TERRAIN_DEBUG // TODO: Remove debug
    tm.usage = usage;
#endif
    return tm;
}

TerrainTriplanar _t_triplanar()
{
    float sharpness = TERRAIN_TRIPLANAR_BLEND_FACTOR;
    float threshold = TERRAIN_TRIPLANAR_MIX_THRESHOLD;
    vec3 weight_signed = pow(abs(vary_vertex_normal), vec3(sharpness));
    weight_signed /= (weight_signed.x + weight_signed.y + weight_signed.z);
    weight_signed -= vec3(threshold);
    TerrainTriplanar tw;
    // *NOTE: Make sure the threshold doesn't affect the materials
    tw.weight = max(vec3(0), weight_signed);
    tw.weight /= (tw.weight.x + tw.weight.y + tw.weight.z);
    ivec3 usage = ivec3(round(max(vec3(0), sign(weight_signed))));
    tw.type = ((usage.x) * SAMPLE_X) |
              ((usage.y) * SAMPLE_Y) |
              ((usage.z) * SAMPLE_Z);
    return tw;
}

float terrain_mix(vec4 samples, float alpha1, float alpha2, float alphaFinal)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    // Assume weights add to 1
    return tm.weight.x * samples.x +
           tm.weight.y * samples.y +
           tm.weight.z * samples.z +
           tm.weight.w * samples.w;
}

vec4 terrain_mix(TerrainMix tm, TerrainMixSample tms)
{
    // Assume weights add to 1
    return tm.weight.x * tms[0] +
           tm.weight.y * tms[1] +
           tm.weight.z * tms[2] +
           tm.weight.w * tms[3];
}

vec3 terrain_mix(TerrainMix tm, TerrainMixSample3 tms3)
{
    // Assume weights add to 1
    return tm.weight.x * tms3[0] +
           tm.weight.y * tms3[1] +
           tm.weight.z * tms3[2] +
           tm.weight.w * tms3[3];
}

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Triplanar mapping

// Pre-transformed texture coordinates for each axial uv slice (Packing: xy, yz, (-x)z, unused)
#define TerrainCoord vec4[2]

vec4 _t_texture(sampler2D tex, vec2 uv_unflipped, float sign_or_zero)
{
    // Handle case where sign is 0
    float sign = (2.0*sign_or_zero) + 1.0;
    sign /= abs(sign);
    // If the vertex normal is negative, flip the texture back
    // right-side up.
    vec2 uv = uv_unflipped * vec2(sign, 1);
    return texture(tex, uv);
}

vec2 _t_uv(vec2 uv_unflipped, float sign_or_zero)
{
    // Handle case where sign is 0
    float sign = (2.0*sign_or_zero) + 1.0;
    sign /= abs(sign);
    // If the vertex normal is negative, flip the texture back
    // right-side up.
    vec2 uv = uv_unflipped * vec2(sign, 1);
    return uv;
}

vec3 _t_normal_post_1(vec3 vNt0, float sign_or_zero)
{
    // Assume normal is unpacked
    vec3 vNt1 = vNt0;
    // Get sign
    float sign = sign_or_zero;
    // Handle case where sign is 0
    sign = (2.0*sign) + 1.0;
    sign /= abs(sign);
    // If the sign is negative, rotate normal by 180 degrees
    vNt1.xy = (min(0, sign) * vNt1.xy) + (min(0, -sign) * -vNt1.xy);
    return vNt1;
}

// Triplanar-specific normal texture fixes
vec3 _t_normal_post_x(vec3 vNt0)
{
    vec3 vNt_x = _t_normal_post_1(vNt0, sign(vary_vertex_normal.x));
    // *HACK: Transform normals according to orientation of the UVs
    vNt_x.xy = vec2(-vNt_x.y, vNt_x.x);
    return vNt_x;
}
vec3 _t_normal_post_y(vec3 vNt0)
{
    vec3 vNt_y = _t_normal_post_1(vNt0, sign(vary_vertex_normal.y));
    // *HACK: Transform normals according to orientation of the UVs
    vNt_y.xy = -vNt_y.xy;
    return vNt_y;
}
vec3 _t_normal_post_z(vec3 vNt0)
{
    vec3 vNt_z = _t_normal_post_1(vNt0, sign(vary_vertex_normal.z));
    return vNt_z;
}

struct TerrainSample
{
    vec4 x;
    vec4 y;
    vec4 z;
};

TerrainSample _t_sample(sampler2D tex, TerrainCoord terrain_coord, TerrainTriplanar tw)
{
    TerrainSample ts;

    // The switch..case is broken up into three parts deliberately.  A single
    // switch..case caused unexplained, "ant trail" seams in terrain. (as seen
    // on Nvidia/Windows 10).  The extra two branches are not free, but it's
    // still a performance win compared to sampling along all three axes for
    // every terrain fragment.
    #define do_sample_x() _t_texture(tex, terrain_coord[0].zw, sign(vary_vertex_normal.x))
    #define do_sample_y() _t_texture(tex, terrain_coord[1].xy, sign(vary_vertex_normal.y))
    #define do_sample_z() _t_texture(tex, terrain_coord[0].xy, sign(vary_vertex_normal.z))
    switch (tw.type & SAMPLE_X)
    {
        case SAMPLE_X:
            ts.x = do_sample_x();
        break;
        default:
            ts.x = vec4(1.0, 0.0, 1.0, 1.0);
        break;
    }
    switch (tw.type & SAMPLE_Y)
    {
        case SAMPLE_Y:
            ts.y = do_sample_y();
        break;
        default:
            ts.y = vec4(1.0, 0.0, 1.0, 1.0);
        break;
    }
    switch (tw.type & SAMPLE_Z)
    {
        case SAMPLE_Z:
            ts.z = do_sample_z();
        break;
        default:
            ts.z = vec4(1.0, 0.0, 1.0, 1.0);
        break;
    }

    return ts;
}

struct TerrainSampleNormal
{
    vec3 x;
    vec3 y;
    vec3 z;
};

TerrainSampleNormal _t_sample_n(sampler2D tex, TerrainCoord terrain_coord, TerrainTriplanar tw)
{
    TerrainSample ts = _t_sample(tex, terrain_coord, tw);
    TerrainSampleNormal tsn;
    // Unpack normals
    tsn.x = ts.x.xyz*2.0-1.0;
    tsn.y = ts.y.xyz*2.0-1.0;
    tsn.z = ts.z.xyz*2.0-1.0;
    // Get sign
    vec3 ns = sign(vary_vertex_normal);
    // Handle case where sign is 0
    ns = (2.0*ns) + 1.0;
    ns /= abs(ns);
    // If the sign is negative, rotate normal by 180 degrees
    tsn.x.xy = (min(0, ns.x) * tsn.x.xy) + (min(0, -ns.x) * -tsn.x.xy);
    tsn.y.xy = (min(0, ns.y) * tsn.y.xy) + (min(0, -ns.y) * -tsn.y.xy);
    tsn.z.xy = (min(0, ns.z) * tsn.z.xy) + (min(0, -ns.z) * -tsn.z.xy);
    // *HACK: Transform normals according to orientation of the UVs
    tsn.x.xy = vec2(-tsn.x.y, tsn.x.x);
    tsn.y.xy = -tsn.y.xy;
    return tsn;
}

TerrainSample _t_sample_c(sampler2D tex, TerrainCoord terrain_coord, TerrainTriplanar tw)
{
    TerrainSample ts = _t_sample(tex, terrain_coord, tw);
    ts.x.xyz = srgb_to_linear(ts.x.xyz);
    ts.y.xyz = srgb_to_linear(ts.y.xyz);
    ts.z.xyz = srgb_to_linear(ts.z.xyz);
    return ts;
}

// Triplanar sampling of things that are neither colors nor normals (i.e. orm)
vec4 terrain_texture(sampler2D tex, TerrainCoord terrain_coord)
{
    TerrainTriplanar tw = _t_triplanar();

    TerrainSample ts = _t_sample(tex, terrain_coord, tw);

    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
}

// Specialized triplanar normal texture sampling implementation, taking into
// account how the rotation of the texture affects the lighting and trying to
// negate that.
// *TODO: Decide if we want this. It may be better to just calculate the
// tangents on-the-fly here rather than messing with the normals, due to the
// subtleties of the effects of triplanar mapping on UVs. These sampled normals
// are only valid on the faces of a cube.
// *NOTE: Bottom face has not been tested
vec3 terrain_texture_normal(sampler2D tex, TerrainCoord terrain_coord)
{
    TerrainTriplanar tw = _t_triplanar();

    TerrainSampleNormal ts = _t_sample_n(tex, terrain_coord, tw);

    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
}

// Triplanar sampling of colors. Colors are converted to linear space before blending.
vec4 terrain_texture_color(sampler2D tex, TerrainCoord terrain_coord)
{
    TerrainTriplanar tw = _t_triplanar();

    TerrainSample ts = _t_sample_c(tex, terrain_coord, tw);

    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
}

PBRMix terrain_sample_pbr(
    TerrainCoord terrain_coord
    , TerrainTriplanar tw
    , sampler2D tex_col
    , sampler2D tex_orm
    , sampler2D tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    , sampler2D tex_emissive
#endif
    )
{
    PBRMix mix = init_pbr_mix();

    #define get_uv_x() _t_uv(terrain_coord[0].zw, sign(vary_vertex_normal.x))
    #define get_uv_y() _t_uv(terrain_coord[1].xy, sign(vary_vertex_normal.y))
    #define get_uv_z() _t_uv(terrain_coord[0].xy, sign(vary_vertex_normal.z))
    switch (tw.type & SAMPLE_X)
    {
    case SAMPLE_X:
        PBRMix mix_x = sample_pbr(
            get_uv_x()
            , tex_col
            , tex_orm
            , tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , tex_emissive
#endif
            );
        // Triplanar-specific normal texture fix
        mix_x.vNt = _t_normal_post_x(mix_x.vNt);
        mix = mix_pbr(mix, mix_x, tw.weight.x);
        break;
    default:
        break;
    }

    switch (tw.type & SAMPLE_Y)
    {
    case SAMPLE_Y:
        PBRMix mix_y = sample_pbr(
            get_uv_y()
            , tex_col
            , tex_orm
            , tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , tex_emissive
#endif
            );
        // Triplanar-specific normal texture fix
        mix_y.vNt = _t_normal_post_y(mix_y.vNt);
        mix = mix_pbr(mix, mix_y, tw.weight.y);
        break;
    default:
        break;
    }

    switch (tw.type & SAMPLE_Z)
    {
    case SAMPLE_Z:
        PBRMix mix_z = sample_pbr(
            get_uv_z()
            , tex_col
            , tex_orm
            , tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
            , tex_emissive
#endif
            );
        // Triplanar-specific normal texture fix
        mix_z.vNt = _t_normal_post_z(mix_z.vNt);
        mix = mix_pbr(mix, mix_z, tw.weight.z);
        break;
    default:
        break;
    }
    
    return mix;
}

#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
#define TerrainCoord vec2
vec4 terrain_texture(sampler2D tex, TerrainCoord terrain_coord)
{
    return texture(tex, terrain_coord);
}

vec3 terrain_texture_normal(sampler2D tex, TerrainCoord terrain_coord)
{
    return texture(tex, terrain_coord).xyz*2.0-1.0;
}

vec4 terrain_texture_color(sampler2D tex, TerrainCoord terrain_coord)
{
    vec4 col = texture(tex, terrain_coord);
    col.xyz = srgb_to_linear(col.xyz);
    return col;
}

// TODO: Implement this for the more complex triplanar case
PBRMix terrain_sample_pbr(
    TerrainCoord terrain_coord
    , sampler2D tex_col
    , sampler2D tex_orm
    , sampler2D tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
    , sampler2D tex_emissive
#endif
    )
{
    return sample_pbr(
        terrain_coord
        , tex_col
        , tex_orm
        , tex_vNt
#if (TERRAIN_PBR_DETAIL >= TERRAIN_PBR_DETAIL_EMISSIVE)
        , tex_emissive
#endif
        );
}
#endif

// The goal of _tmix_sample and related functions is to only sample textures when necessary, ignoring if the weights are low.
// *TODO: Currently, there is much more switch..case branching than needed. This could be simplified by branching per-material rather than per-texture.

TerrainMixSample _tmix_sample(TerrainMix tm, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMixSample tms;

    switch (tm.type & MIX_X)
    {
        case MIX_X:
            tms[0] = terrain_texture(tex0, texcoord);
            break;
        default:
            tms[0] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Y)
    {
        case MIX_Y:
            tms[1] = terrain_texture(tex1, texcoord);
            break;
        default:
            tms[1] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Z)
    {
        case MIX_Z:
            tms[2] = terrain_texture(tex2, texcoord);
            break;
        default:
            tms[2] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_W)
    {
        case MIX_W:
            tms[3] = terrain_texture(tex3, texcoord);
            break;
        default:
            tms[3] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }

    return tms;
}

TerrainMixSample _tmix_sample_color(TerrainMix tm, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMixSample tmix;

    switch (tm.type & MIX_X)
    {
        case MIX_X:
            tmix[0] = terrain_texture_color(tex0, texcoord);
            break;
        default:
            tmix[0] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Y)
    {
        case MIX_Y:
            tmix[1] = terrain_texture_color(tex1, texcoord);
            break;
        default:
            tmix[1] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Z)
    {
        case MIX_Z:
            tmix[2] = terrain_texture_color(tex2, texcoord);
            break;
        default:
            tmix[2] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }
    switch (tm.type & MIX_W)
    {
        case MIX_W:
            tmix[3] = terrain_texture_color(tex3, texcoord);
            break;
        default:
            tmix[3] = vec4(1.0, 0.0, 1.0, 1.0);
            break;
    }

    return tmix;
}

TerrainMixSample3 _tmix_sample_normal(TerrainMix tm, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMixSample3 tmix3;

    switch (tm.type & MIX_X)
    {
        case MIX_X:
            tmix3[0] = terrain_texture_normal(tex0, texcoord);
            break;
        default:
            tmix3[0] = vec3(1.0, 0.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Y)
    {
        case MIX_Y:
            tmix3[1] = terrain_texture_normal(tex1, texcoord);
            break;
        default:
            tmix3[1] = vec3(1.0, 0.0, 1.0);
            break;
    }
    switch (tm.type & MIX_Z)
    {
        case MIX_Z:
            tmix3[2] = terrain_texture_normal(tex2, texcoord);
            break;
        default:
            tmix3[2] = vec3(1.0, 0.0, 1.0);
            break;
    }
    switch (tm.type & MIX_W)
    {
        case MIX_W:
            tmix3[3] = terrain_texture_normal(tex3, texcoord);
            break;
        default:
            tmix3[3] = vec3(1.0, 0.0, 1.0);
            break;
    }

    return tmix3;
}

vec3 sample_and_mix_color3(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec3[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    TerrainMixSample tms = _tmix_sample_color(tm, texcoord, tex0, tex1, tex2, tex3);
    vec3[4] tms3;
    tms3[0] = tms[0].xyz;
    tms3[1] = tms[1].xyz;
    tms3[2] = tms[2].xyz;
    tms3[3] = tms[3].xyz;
    tms3[0] *= factors[0];
    tms3[1] *= factors[1];
    tms3[2] *= factors[2];
    tms3[3] *= factors[3];
    return terrain_mix(tm, tms3);
}

vec4 sample_and_mix_color4(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec4[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    TerrainMixSample tms = _tmix_sample_color(tm, texcoord, tex0, tex1, tex2, tex3);
    tms[0] *= factors[0];
    tms[1] *= factors[1];
    tms[2] *= factors[2];
    tms[3] *= factors[3];
    return terrain_mix(tm, tms);
}

vec3 sample_and_mix_vector3(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec3[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    TerrainMixSample tms = _tmix_sample(tm, texcoord, tex0, tex1, tex2, tex3);
    vec3[4] tms3;
    tms3[0] = tms[0].xyz;
    tms3[1] = tms[1].xyz;
    tms3[2] = tms[2].xyz;
    tms3[3] = tms[3].xyz;
    tms3[0] *= factors[0];
    tms3[1] *= factors[1];
    tms3[2] *= factors[2];
    tms3[3] *= factors[3];
    return terrain_mix(tm, tms3);
}

// Returns the unpacked normal texture in range [-1, 1]
vec3 sample_and_mix_normal(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    TerrainMixSample3 tms3 = _tmix_sample_normal(tm, texcoord, tex0, tex1, tex2, tex3);
    return terrain_mix(tm, tms3);
}
