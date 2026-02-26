/**
 * @file class1/deferred/globalF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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


 // Global helper functions included in every fragment shader
 // DO NOT declare sampler uniforms here as OS X doesn't compile
 // them out

uniform float mirror_flag;
uniform vec4 clipPlane;
uniform float clipSign;

void mirrorClip(vec3 pos)
{
    if (mirror_flag > 0)
    {
        if ((dot(pos.xyz, clipPlane.xyz) + clipPlane.w) < 0.0)
        {
                discard;
        }
    }
}

vec4 encodeNormal(vec3 n, float env, float gbuffer_flag)
{
    float f = sqrt(8 * n.z + 8);
    return vec4(n.xy / f + 0.5, env, gbuffer_flag);
}

vec4 decodeNormal(vec4 norm)
{
    vec2 fenc = norm.xy*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec4 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

// 4x4 Bayer dither matrix normalized to [0, 1]
const float BAYER_PATTERN[16] = float[16](
     0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
    12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
     3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
    15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

// Discard fragment based on dithered alpha threshold.
// Fragments with alpha < 0.05 are always discarded.
// Fragments with alpha >= threshold are always kept.
// Fragments in between are dithered using a 4x4 Bayer pattern.
void bayerDitherDiscard(float alpha, float threshold)
{
    if (alpha < 0.05)
    {
        discard;
    }

    if (alpha < threshold)
    {
        ivec2 ipos = ivec2(mod(gl_FragCoord.xy, 4.0));
        if (alpha < BAYER_PATTERN[ipos.y * 4 + ipos.x])
        {
            discard;
        }
    }
}
