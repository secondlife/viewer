/**
 * @file srgbF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

vec3 srgb_to_linear(vec3 cs)
{
    vec3 low_range = cs / vec3(12.92);
    vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
    bvec3 lte = lessThanEqual(cs,vec3(0.04045));

#ifdef OLD_SELECT
    vec3 result;
    result.r = lte.r ? low_range.r : high_range.r;
    result.g = lte.g ? low_range.g : high_range.g;
    result.b = lte.b ? low_range.b : high_range.b;
    return result;
#else
    return mix(high_range, low_range, lte);
#endif

}


vec4 srgb_to_linear4(vec4 cs)
{
    vec4 low_range = cs / vec4(12.92);
    vec4 high_range = pow((cs+vec4(0.055))/vec4(1.055), vec4(2.4));
    bvec4 lte = lessThanEqual(cs,vec4(0.04045));

#ifdef OLD_SELECT
    vec4 result;
    result.r = lte.r ? low_range.r : high_range.r;
    result.g = lte.g ? low_range.g : high_range.g;
    result.b = lte.b ? low_range.b : high_range.b;
    result.a = lte.a ? low_range.a : high_range.a;
    return result;
#else
    return mix(high_range, low_range, lte);
#endif

}

vec3 linear_to_srgb(vec3 cl)
{
    cl = clamp(cl, vec3(0), vec3(1));
    vec3 low_range  = cl * 12.92;
    vec3 high_range = 1.055 * pow(cl, vec3(0.41666)) - 0.055;
    bvec3 lt = lessThan(cl,vec3(0.0031308));

#ifdef OLD_SELECT
    vec3 result;
    result.r = lt.r ? low_range.r : high_range.r;
    result.g = lt.g ? low_range.g : high_range.g;
    result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
    return mix(high_range, low_range, lt);
#endif

}

vec3 ColorFromRadiance(vec3 radiance)
{
    return vec3(1.0) - exp(-radiance * 0.0001);
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-3;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

const mat3 inv_ACESOutputMat = mat3(0.643038, 0.0592687, 0.0059619, 0.311187, 0.931436, 0.063929, 0.0457755, 0.00929492, 0.930118 );

const mat3 inv_ACESInputMat = mat3( 1.76474, -0.147028, -0.0363368, -0.675778, 1.16025, -0.162436, -0.0889633, -0.0132237, 1.19877 );

vec3 inv_RRTAndODTFit(vec3 x)
{
    float A = 0.0245786;
    float B = 0.000090537;
    float C = 0.983729;
    float D = 0.4329510;
    float E = 0.238081;

    return (A - D * x)/(2.0 * (C * x - 1.0)) - sqrt(pow(D * x - A, vec3(2.0)) - 4.0 * (C * x - 1.0) * (B + E * x))/(2.0 * (C * x - 1.0));
}

// experimental inverse of ACES Hill tonemapping
vec3 inv_toneMapACES_Hill(vec3 color)
{
    color = inv_ACESOutputMat * color;

    // Apply RRT and ODT
    color = inv_RRTAndODTFit(color);

    color = inv_ACESInputMat * color;

    return color;
}

