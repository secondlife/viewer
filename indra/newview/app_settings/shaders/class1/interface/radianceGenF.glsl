/**
 * @file radianceGenF.glsl
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

out vec4 frag_color;

uniform samplerCubeArray   reflectionProbes;
uniform int sourceIdx;

in vec3 vary_dir;

//uniform float roughness;

uniform float mipLevel;
uniform int u_width;
uniform float max_probe_lod;
uniform float probe_strength;


// =============================================================================================================
// Parts of this file are (c) 2018 Sascha Willems
// SNIPPED FROM https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/prefilterenvmap.frag
/*
MIT License

Copyright (c) 2018 Sascha Willems

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// =============================================================================================================

const float PI = 3.1415926536;

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

vec2 hammersley2d(uint i, uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal)
{
    // Maps a 2D point to a hemisphere with spread based on roughness
    float alpha = roughness * roughness;
    float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(up, normal));
    vec3 tangentY = normalize(cross(normal, tangentX));

    // Convert to world Space
    return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(PI * denom*denom);
}

vec4 prefilterEnvMap(vec3 R)
{
    vec3 N = R;
    vec3 V = R;
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    float envMapDim = float(textureSize(reflectionProbes, 0).s);
    float roughness = mipLevel/max_probe_lod;
    uint numSamples = uint(max(PROBE_FILTER_SAMPLES*roughness, 1));

    float numMips = max_probe_lod+1;

    for(uint i = 0u; i < numSamples; i++) {
        vec2 Xi = hammersley2d(i, numSamples);
        vec3 H = importanceSample_GGX(Xi, roughness, N);
        vec3 L = 2.0 * dot(V, H) * H - V;
        float dotNL = clamp(dot(N, L), 0.0, 1.0);
        if(dotNL > 0.0) {
            // Filtering based on https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/

            float dotNH = clamp(dot(N, H), 0.0, 1.0);
            float dotVH = clamp(dot(V, H), 0.0, 1.0);

            // Probability Distribution Function
            float pdf = D_GGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001;
            // Slid angle of current smple
            float omegaS = 1.0 / (float(numSamples) * pdf);
            // Solid angle of 1 pixel across all cube faces
            float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);
            // Biased (+1.0) mip level for better result
            float mipLevel = roughness == 0.0 ? 0.0 : clamp(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f, max_probe_lod);
            color += textureLod(reflectionProbes, vec4(L, sourceIdx), mipLevel) * dotNL;
            totalWeight += dotNL;
        }
    }
    return (color / totalWeight);
}

void main()
{
    vec3 N = normalize(vary_dir);
    frag_color = max(prefilterEnvMap(N), vec4(0));
    frag_color.a *= probe_strength;
}
// =============================================================================================================
