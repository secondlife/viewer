/**
 * @file screenSpaceReflFilterF.glsl
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseMap;
uniform vec2 pixelSize;
uniform int filterDir;
uniform float filterScale;

in vec2 vary_fragcoord;

void main()
{
    vec2 tc = vary_fragcoord.xy;

    const float weights[4] = float[](0.214607, 0.189879, 0.131514, 0.071303);

    vec2 dir = (filterDir == 0) ? vec2(pixelSize.x, 0.0) : vec2(0.0, pixelSize.y);
    dir *= filterScale;

    vec4 center = texture(diffuseMap, tc);
    float w0 = weights[0] * center.a;

    vec4 color = center * w0;
    float total_weight = w0;

    for (int i = 1; i < 4; i++)
    {
        vec2 offset = dir * float(i);

        vec4 s1 = texture(diffuseMap, tc + offset);
        float w1 = weights[i] * s1.a;
        color += s1 * w1;
        total_weight += w1;

        vec4 s2 = texture(diffuseMap, tc - offset);
        float w2 = weights[i] * s2.a;
        color += s2 * w2;
        total_weight += w2;
    }

    frag_color = color / max(total_weight, 0.001);
}
