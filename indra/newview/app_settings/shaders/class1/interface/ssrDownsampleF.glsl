/**
 * @file ssrDownsampleF.glsl
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

// dual-filter downsample: 4 diagonal taps + weighted center, one pass per mip

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform vec2 texelStep;
uniform int karisWeight;

in vec2 vary_texcoord0;

float karis(vec4 c)
{
    return 1.0 / (1.0 + dot(c.rgb, vec3(0.2126, 0.7152, 0.0722)));
}

void main()
{
    vec4 taps[5];
    taps[0] = texture(diffuseRect, vary_texcoord0);
    taps[1] = texture(diffuseRect, vary_texcoord0 + vec2(-texelStep.x, -texelStep.y));
    taps[2] = texture(diffuseRect, vary_texcoord0 + vec2( texelStep.x, -texelStep.y));
    taps[3] = texture(diffuseRect, vary_texcoord0 + vec2(-texelStep.x,  texelStep.y));
    taps[4] = texture(diffuseRect, vary_texcoord0 + vec2( texelStep.x,  texelStep.y));

    if (karisWeight > 0)
    {
        // luma-weighted average kills fireflies in the prefiltered pyramid
        vec4 sum = vec4(0.0);
        float wsum = 0.0;
        for (int i = 0; i < 5; i++)
        {
            float w = (i == 0 ? 4.0 : 1.0) * karis(taps[i]);
            sum += taps[i] * w;
            wsum += w;
        }
        frag_color = sum / wsum;
    }
    else
    {
        frag_color = (taps[0] * 4.0 + taps[1] + taps[2] + taps[3] + taps[4]) * 0.125;
    }
}
