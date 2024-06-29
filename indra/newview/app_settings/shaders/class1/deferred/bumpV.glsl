/**
 * @file bumpV.glsl
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

uniform mat4 modelview_matrix;
uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_projection_matrix;

in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec2 texcoord0;
in vec4 tangent;

out vec3 vary_mat0;
out vec3 vary_mat1;
out vec3 vary_mat2;
out vec4 vertex_color;
out vec2 vary_texcoord0;
out vec3 vary_position;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
uniform mat4 projection_matrix;
#endif

void main()
{
    //transform vertex
#ifdef HAS_SKIN
    mat4 mat = getObjectSkinnedTransform();
    mat = modelview_matrix * mat;
    vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
    vary_position = pos;
    gl_Position = projection_matrix*vec4(pos, 1.0);

    vec3 n = normalize((mat * vec4(normal.xyz+position.xyz, 1.0)).xyz-pos.xyz);
    vec3 t = normalize((mat * vec4(tangent.xyz+position.xyz, 1.0)).xyz-pos.xyz);
#else
    vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
    vec3 n = normalize(normal_matrix * normal);
    vec3 t = normalize(normal_matrix * tangent.xyz);
#endif

    vec3 b = cross(n, t) * tangent.w;
    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;

    vary_mat0 = vec3(t.x, b.x, n.x);
    vary_mat1 = vec3(t.y, b.y, n.y);
    vary_mat2 = vec3(t.z, b.z, n.z);

    vertex_color = diffuse_color;
}
