/**
 * @file avatarVelocityV.glsl
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

uniform mat4 projection_matrix;
uniform vec4 lastMatrixPalette[45];

in vec3 position;
in vec4 weight;
in vec2 texcoord0;

out vec2 vary_texcoord0;

mat4 getSkinnedTransform();

void writeVaryVelocity(vec4 pos, vec4 last_pos);

mat4 getLastSkinnedTransform()
{
    mat4 ret;
    int i = int(floor(weight.x));
    float x = fract(weight.x);

    ret[0] = mix(lastMatrixPalette[i+0],  lastMatrixPalette[i+1],  x);
    ret[1] = mix(lastMatrixPalette[i+15], lastMatrixPalette[i+16], x);
    ret[2] = mix(lastMatrixPalette[i+30], lastMatrixPalette[i+31], x);
    ret[3] = vec4(0,0,0,1);

    return ret;
}

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);

    mat4 current_skin = getSkinnedTransform();
    vec4 current_clip = projection_matrix * (current_skin * pos);

    mat4 last_skin = getLastSkinnedTransform();
    vec4 last_clip = projection_matrix * (last_skin * pos);

    gl_Position = current_clip;

    writeVaryVelocity(current_clip, last_clip);

    vary_texcoord0 = texcoord0;
}
