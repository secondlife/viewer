/**
 * @file particleBillboardV.glsl
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

in vec2 texcoord1;
in vec4 tangent;

// texcoord1 == (0,0): quad was expanded on the CPU, leave it alone
vec4 particleBillboardEye(mat4 mv, vec3 position, vec3 center, vec2 corner)
{
    if (texcoord1.x == 0.0 && texcoord1.y == 0.0)
    {
        return mv * vec4(position, 1.0);
    }

    vec3 at    = (mv * vec4(center, 1.0)).xyz;
    vec3 up    = normalize(mat3(mv) * vec3(0.0, 0.0, 1.0));
    vec3 right = normalize(cross(at, up));
    up         = normalize(cross(right, at));

    if (tangent.w > 0.0)
    {
        vec3 v  = normalize(mat3(mv) * tangent.xyz);
        vec2 f  = normalize(vec2(dot(v, right), dot(v, up)));
        vec3 nu = f.x * right + f.y * up;
        vec3 nr = f.y * right - f.x * up;
        up    = nu;
        right = nr;
    }

    return vec4(at + right * (texcoord1.x * corner.x) + up * (texcoord1.y * corner.y), 1.0);
}
