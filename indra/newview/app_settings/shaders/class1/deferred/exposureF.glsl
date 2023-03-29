/** 
 * @file exposureF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D emissiveRect;
uniform sampler2D exposureMap;

uniform float dt;
uniform vec2 noiseVec;

// calculate luminance the same way LLColor4::calcHSL does
float lum(vec3 col)
{
    float mx = max(max(col.r, col.g), col.b);
    float mn = min(min(col.r, col.g), col.b);
    return (mx + mn) * 0.5;
}

void main() 
{
    float step = 1.0/32.0;

    float start = step;
    float end = 1.0-step;

    float w = 0.0;

    vec3 col;

    vec2 nz = noiseVec * step * 0.5;

    for (float x = start; x <= end; x += step)
    {
        for (float y = start; y <= end; y += step)
        {
            vec2 tc = vec2(x,y) + nz;
            vec3 c = texture(diffuseRect, tc).rgb + texture(emissiveRect, tc).rgb;
            float L = max(lum(c), 0.25);

            float d = length(vec2(0.5)-tc);
            d = 1.0-d;
            d *= d;
            d *= d;
            d *= d;
            L *= d;

            w += L;

            col += c * L;
        }
    }

    col /= w;

    float L = lum(col);

    float s = clamp(0.1/L, 0.5, 4.0);

    float prev = texture(exposureMap, vec2(0.5,0.5)).r;
    s = mix(prev, s, min(dt*2.0, 0.04));

    frag_color = vec4(s, s, s, dt);
}

