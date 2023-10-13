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

#define TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT 3 // TODO: Move definition to config

#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
// Pre-transformed texture coordinates for each axial uv slice (Packing: xy, yz, zx, unused)
#define TerrainCoord vec4[2]
// Triplanar mapping
vec4 terrain_texture(sampler2D tex, TerrainCoord terrain_coord)
{
    vec4 x = texture(tex, terrain_coord[0].zw);
    vec4 y = texture(tex, terrain_coord[1].xy);
    vec4 z = texture(tex, terrain_coord[0].xy);
    float sharpness = 8.0;
    vec3 weight = pow(abs(vary_vertex_normal), vec3(sharpness));
    return ((x * weight.x) + (y * weight.y) + (z * weight.z)) / (weight.x + weight.y + weight.z);
}
#elif TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 1
#define TerrainCoord vec2
vec4 terrain_texture(sampler2D tex, TerrainCoord terrain_coord)
{
    return texture(tex, terrain_coord);
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
    samples[0] = terrain_texture(tex0, texcoord);
    samples[1] = terrain_texture(tex1, texcoord);
    samples[2] = terrain_texture(tex2, texcoord);
    samples[3] = terrain_texture(tex3, texcoord);
    samples[0].xyz = srgb_to_linear(samples[0].xyz);
    samples[1].xyz = srgb_to_linear(samples[1].xyz);
    samples[2].xyz = srgb_to_linear(samples[2].xyz);
    samples[3].xyz = srgb_to_linear(samples[3].xyz);
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

vec3 sample_and_mix_vector3_no_scale(float alpha1, float alpha2, float alphaFinal, TerrainCoord texcoord, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3)
{
    vec3[4] samples;
    samples[0] = terrain_texture(tex0, texcoord).xyz;
    samples[1] = terrain_texture(tex1, texcoord).xyz;
    samples[2] = terrain_texture(tex2, texcoord).xyz;
    samples[3] = terrain_texture(tex3, texcoord).xyz;
    return terrain_mix(samples, alpha1, alpha2, alphaFinal);
}
