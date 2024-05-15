/**
 * @file avatarV.glsl
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

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

mat4 getSkinnedTransform();

in vec4 weight;

out vec3 vary_normal;
out vec2 vary_texcoord0;
out vec3 vary_position;

void main()
{
    vary_texcoord0 = texcoord0;

    vec4 pos;
    vec3 norm;

    vec4 pos_in = vec4(position.xyz, 1.0);
    mat4 trans = getSkinnedTransform();
    pos.x = dot(trans[0], pos_in);
    pos.y = dot(trans[1], pos_in);
    pos.z = dot(trans[2], pos_in);
    pos.w = 1.0;

    norm.x = dot(trans[0].xyz, normal);
    norm.y = dot(trans[1].xyz, normal);
    norm.z = dot(trans[2].xyz, normal);
    norm = normalize(norm);

    vary_normal = norm;

    vary_position = pos.xyz;
    gl_Position = projection_matrix * pos;
}


