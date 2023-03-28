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
uniform float dt;

void main() 
{
    int samples = 16;
    float step = 1.0/(samples-4);

    float start = step;
    float end = 1.0-step;

    float w = 0.0;

    vec3 col;

    for (float x = start; x <= end; x += step)
    {
        for (float y = start; y <= end; y += step)
        {
            vec2 tc = vec2(x,y);
            w += 1.0;
            col += texture(diffuseRect, tc).rgb + texture(emissiveRect, tc).rgb;
        }
    }

    col /= w;

    // calculate luminance the same way LLColor4::calcHSL does
    float mx = max(max(col.r, col.g), col.b);
    float mn = min(min(col.r, col.g), col.b);
    float lum = (mx + mn) * 0.5;

    frag_color = vec4(lum, lum, lum, dt);
}

