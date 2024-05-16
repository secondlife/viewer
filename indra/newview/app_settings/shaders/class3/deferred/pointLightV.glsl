/**
 * @file class3\deferred\pointLightV.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;

in vec3 position;

uniform vec3 center;
uniform float size;

out vec4 vary_fragcoord;
out vec3 trans_center;

void main()
{
    //transform vertex
    vec3 p = position*size+center;
    vec4 pos = modelview_projection_matrix * vec4(p.xyz, 1.0);
    vary_fragcoord = pos;
    trans_center = (modelview_matrix*vec4(center.xyz, 1.0)).xyz;
    gl_Position = pos;
}
