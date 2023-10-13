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


in vec3 vary_vertex_normal;

vec3 srgb_to_linear(vec3 c);

// A relatively agressive threshold ensures that only one or two materials are used in most places
#define TERRAIN_RAMP_MIX_THRESHOLD 0.1

#define MIX_X 1 << 3
#define MIX_Y 1 << 2
#define MIX_Z 1 << 1
#define MIX_W 1 << 0

#define TERRAIN_DEBUG 1 // TODO: Remove debug
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

float terrain_mix(vec4 samples, float alpha1, float alpha2, float alphaFinal)
{
    TerrainMix tm = _t_mix(alpha1, alpha2, alphaFinal);
    // Assume weights add to 1
    return tm.weight.x * samples.x +
           tm.weight.y * samples.y +
           tm.weight.z * samples.z +
           tm.weight.w * samples.w;
}

#if 0 // TODO: Decide if still needed, and if so, use _t_mix internally for weights
vec3 terrain_mix(vec3[4] samples, float alpha1, float alpha2, float alphaFinal)
{
    return mix( mix(samples[3], samples[2], alpha2), mix(samples[1], samples[0], alpha1), alphaFinal );
}

vec4 terrain_mix(vec4[4] samples, float alpha1, float alpha2, float alphaFinal)
{
    return mix( mix(samples[3], samples[2], alpha2), mix(samples[1], samples[0], alpha1), alphaFinal );
}
#endif

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
#define TERRAIN_TRIPLANAR_MIX_THRESHOLD 0.01

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

#define SAMPLE_X 1 << 2
#define SAMPLE_Y 1 << 1
#define SAMPLE_Z 1 << 0
struct TerrainWeight
{
    vec3 weight;
    int type;
};

TerrainWeight _t_weight(TerrainCoord terrain_coord)
{
    float sharpness = TERRAIN_TRIPLANAR_BLEND_FACTOR;
    float threshold = TERRAIN_TRIPLANAR_MIX_THRESHOLD;
    vec3 weight_signed = pow(abs(vary_vertex_normal), vec3(sharpness));
    weight_signed /= (weight_signed.x + weight_signed.y + weight_signed.z);
    weight_signed -= vec3(threshold);
    TerrainWeight tw;
    // *NOTE: Make sure the threshold doesn't affect the materials
    tw.weight = max(vec3(0), weight_signed);
    tw.weight /= (tw.weight.x + tw.weight.y + tw.weight.z);
    ivec3 usage = ivec3(round(max(vec3(0), sign(weight_signed))));
    tw.type = ((usage.x) * SAMPLE_X) |
              ((usage.y) * SAMPLE_Y) |
              ((usage.z) * SAMPLE_Z);
    return tw;
}

struct TerrainSample
{
    vec4 x;
    vec4 y;
    vec4 z;
};

TerrainSample _t_sample(sampler2D tex, TerrainCoord terrain_coord, TerrainWeight tw)
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

TerrainSampleNormal _t_sample_n(sampler2D tex, TerrainCoord terrain_coord, TerrainWeight tw)
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

TerrainSample _t_sample_c(sampler2D tex, TerrainCoord terrain_coord, TerrainWeight tw)
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
    TerrainWeight tw = _t_weight(terrain_coord);

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
    TerrainWeight tw = _t_weight(terrain_coord);

    TerrainSampleNormal ts = _t_sample_n(tex, terrain_coord, tw);

    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
}

// Triplanar sampling of colors. Colors are converted to linear space before blending.
vec4 terrain_texture_color(sampler2D tex, TerrainCoord terrain_coord)
{
    TerrainWeight tw = _t_weight(terrain_coord);

    TerrainSample ts = _t_sample_c(tex, terrain_coord, tw);

    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
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
