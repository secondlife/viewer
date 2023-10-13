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

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Triplanar mapping

// Pre-transformed texture coordinates for each axial uv slice (Packing: xy, yz, (-x)z, unused)
#define TerrainCoord vec4[2]
#if 0 // TODO: Revert
#define TERRAIN_TRIPLANAR_MIX_THRESHOLD 0.01
#else
#define TERRAIN_TRIPLANAR_MIX_THRESHOLD 0.01
#endif

// Positive value prevents artifacts when weights are close to zero
// TODO: Wait a minute... this doesn't prevent artifacts :( (or does it?)
#define TERRAIN_TRIPLANAR_OVERDRAW_THRESHOLD 0.0

vec4 _t_texture(sampler2D tex, vec2 uv_unflipped, float sign)
{
    // If the vertex normal is negative, flip the texture back
    // right-side up.
    vec2 uv = uv_unflipped * vec2(sign, 1);
    return texture(tex, uv);
}

vec4 _t_texture_c(sampler2D tex, vec2 uv_unflipped, float sign)
{
    vec4 c = _t_texture(tex, uv_unflipped, sign);
    c.xyz = srgb_to_linear(c.xyz);
    return c;
}

#define SAMPLE_X 1 << 2
#define SAMPLE_Y 1 << 1
#define SAMPLE_Z 1 << 0
#define terrain_coord_x terrain_coord[0].zw
#define terrain_coord_y terrain_coord[1].xy
#define terrain_coord_z terrain_coord[0].xy
#define TERRAIN_DEBUG 1 // TODO: Remove debug
struct TerrainWeight
{
    vec3 weight;
#if TERRAIN_DEBUG
    vec3 usage;
#endif
    int type;
};

TerrainWeight _t_weight(TerrainCoord terrain_coord)
{
    float sharpness = TERRAIN_TRIPLANAR_BLEND_FACTOR;
    float threshold = TERRAIN_TRIPLANAR_MIX_THRESHOLD;
    vec3 weight_signed = normalize(pow(abs(vary_vertex_normal), vec3(sharpness)));
    weight_signed -= vec3(threshold);
    TerrainWeight tw;
    tw.weight = max(vec3(0), weight_signed);
    vec3 usage = max(vec3(0), sign(weight_signed + TERRAIN_TRIPLANAR_OVERDRAW_THRESHOLD));
#if TERRAIN_DEBUG
    tw.usage = usage;
#endif
    tw.type = (int(usage.x) * SAMPLE_X) |
              (int(usage.y) * SAMPLE_Y) |
              (int(usage.z) * SAMPLE_Z);
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
    vec4 x;
    vec4 y;
    vec4 z;
    switch (tw.type)
    {
        case SAMPLE_X | SAMPLE_Y | SAMPLE_Z:
            x = _t_texture(tex, terrain_coord_x, sign(vary_vertex_normal.x));
            y = _t_texture(tex, terrain_coord_y, sign(vary_vertex_normal.y));
            z = _t_texture(tex, terrain_coord_z, sign(vary_vertex_normal.z));
            break;
        case SAMPLE_X | SAMPLE_Y:
            x = _t_texture(tex, terrain_coord_x, sign(vary_vertex_normal.x));
            y = _t_texture(tex, terrain_coord_y, sign(vary_vertex_normal.y));
            z = x;
            break;
        case SAMPLE_X | SAMPLE_Z:
            x = _t_texture(tex, terrain_coord_x, sign(vary_vertex_normal.x));
            z = _t_texture(tex, terrain_coord_z, sign(vary_vertex_normal.z));
            y = x;
            break;
        case SAMPLE_Y | SAMPLE_Z:
            y = _t_texture(tex, terrain_coord_y, sign(vary_vertex_normal.y));
            z = _t_texture(tex, terrain_coord_z, sign(vary_vertex_normal.z));
            x = y;
            break;
        case SAMPLE_X:
            x = _t_texture(tex, terrain_coord_x, sign(vary_vertex_normal.x));
            y = x;
            z = x;
            break;
        case SAMPLE_Y:
            y = _t_texture(tex, terrain_coord_y, sign(vary_vertex_normal.y));
            x = y;
            z = y;
            break;
        case SAMPLE_Z:
            z = _t_texture(tex, terrain_coord_z, sign(vary_vertex_normal.z));
            x = z;
            y = z;
            break;
        default:
            x = vec4(0);
            y = x;
            z = x;
            break;
    }

    TerrainSample ts;
    ts.x = x;
    ts.y = y;
    ts.z = z;
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
    tsn.x = ts.x.xyz;
    tsn.y = ts.y.xyz;
    tsn.z = ts.z.xyz;
    vec3 ns = sign(vary_vertex_normal);
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

vec4 terrain_texture(sampler2D tex, TerrainCoord terrain_coord)
{
    TerrainWeight tw = _t_weight(terrain_coord);

    TerrainSample ts = _t_sample(tex, terrain_coord, tw);

#if 1
    return ((ts.x * tw.weight.x) + (ts.y * tw.weight.y) + (ts.z * tw.weight.z)) / (tw.weight.x + tw.weight.y + tw.weight.z);
#else // TODO: Remove debug
    //return vec4(((tw.usage - normalize(tw.weight))) / 0.5, 1.0);
#if 1
    return vec4(tw.usage, 1.0);
#else
    //return vec4(tw.usage, 1.0);
    return vec4((tw.usage + weight) / 2.0, 1.0);
#endif
#endif
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

vec3 sample_and_mix_color3(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec3[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec3[4] samples;
    samples[0] = terrain_texture(tex0, texcoord).xyz;
    samples[1] = terrain_texture(tex1, texcoord).xyz;
    samples[2] = terrain_texture(tex2, texcoord).xyz;
    samples[3] = terrain_texture(tex3, texcoord).xyz;
    samples[0] = srgb_to_linear(samples[0]);
    samples[1] = srgb_to_linear(samples[1]);
    samples[2] = srgb_to_linear(samples[2]);
    samples[3] = srgb_to_linear(samples[3]);
    samples[0] *= factors[0];
    samples[1] *= factors[1];
    samples[2] *= factors[2];
    samples[3] *= factors[3];
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}

vec4 sample_and_mix_color4(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec4[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec4[4] samples;
    samples[0] = terrain_texture_color(tex0, texcoord);
    samples[1] = terrain_texture_color(tex1, texcoord);
    samples[2] = terrain_texture_color(tex2, texcoord);
    samples[3] = terrain_texture_color(tex3, texcoord);
    samples[0] *= factors[0];
    samples[1] *= factors[1];
    samples[2] *= factors[2];
    samples[3] *= factors[3];
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}

vec3 sample_and_mix_vector3(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, vec3[4] factors, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec3[4] samples;
    samples[0] = terrain_texture(tex0, texcoord).xyz;
    samples[1] = terrain_texture(tex1, texcoord).xyz;
    samples[2] = terrain_texture(tex2, texcoord).xyz;
    samples[3] = terrain_texture(tex3, texcoord).xyz;
    samples[0] *= factors[0];
    samples[1] *= factors[1];
    samples[2] *= factors[2];
    samples[3] *= factors[3];
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}

// Returns the unpacked normal texture in range [-1, 1]
vec3 sample_and_mix_normal(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec3[4] samples;
    samples[0] = terrain_texture_normal(tex0, texcoord).xyz;
    samples[1] = terrain_texture_normal(tex1, texcoord).xyz;
    samples[2] = terrain_texture_normal(tex2, texcoord).xyz;
    samples[3] = terrain_texture_normal(tex3, texcoord).xyz;
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}
