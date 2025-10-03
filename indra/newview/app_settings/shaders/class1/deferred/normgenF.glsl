/**
 * @file normgenF.glsl
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

/*[EXTRA_CODE_HERE]*/



// generate a normal map using an approximation of the old emboss bump map "brightness/darkness" technique
// srcMap is a source color image, output should be a normal

out vec4 frag_color;

uniform sampler2D srcMap;

in vec2 vary_texcoord0;

uniform float stepX;
uniform float stepY;
uniform float norm_scale;
uniform int bump_code;

#define BE_BRIGHTNESS 1
#define BE_DARKNESS 2

// get luminance or inverse luminance depending on bump_code
float getBumpValue(vec2 texcoord)
{
    vec3 c = texture(srcMap, texcoord).rgb;

    vec3 WEIGHT = vec3(0.2995, 0.5875, 0.1145);

    float l = dot(c, WEIGHT);

    if (bump_code == BE_DARKNESS)
    {
        l = 1.0 - l;
    }

    return l;
}


void main()
{
    float c = getBumpValue(vary_texcoord0);

    float scaler = 512.0;

    vec3 right = vec3(norm_scale, 0, (getBumpValue(vary_texcoord0+vec2(stepX, 0))-c)*scaler);
    vec3 left = vec3(-norm_scale, 0, (getBumpValue(vary_texcoord0-vec2(stepX, 0))-c)*scaler);
    vec3 up = vec3(0, -norm_scale, (getBumpValue(vary_texcoord0-vec2(0, stepY))-c)*scaler);
    vec3 down = vec3(0, norm_scale, (getBumpValue(vary_texcoord0+vec2(0, stepY))-c)*scaler);

    vec3 norm = cross(right, down) + cross(down, left) + cross(left,up) + cross(up, right);

    norm = normalize(norm);
    norm *= 0.5;
    norm += 0.5;

    frag_color = vec4(norm, c);
}
