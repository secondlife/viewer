/**
 * @file skinnedVelocityAlphaV.glsl
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

uniform mat4 modelview_projection_matrix;
uniform mat4 projection_matrix;
uniform mat4 last_modelview_matrix;
uniform mat4 texture_matrix0;

in vec3 position;
in vec4 weight4;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec2 vary_texcoord0;
out vec4 vertex_color;

uniform mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT];

mat4 getObjectSkinnedTransform();

void writeVaryVelocity(vec4 pos, vec4 last_pos);

mat4 getLastObjectSkinnedTransform()
{
    vec4 w = fract(weight4);
    vec4 index = floor(weight4);

    index = min(index, vec4(MAX_JOINTS_PER_MESH_OBJECT-1));
    index = max(index, vec4(0.0));

    w *= 1.0/(w.x+w.y+w.z+w.w);

    int i1 = int(index.x);
    int i2 = int(index.y);
    int i3 = int(index.z);
    int i4 = int(index.w);

    mat3 mat = mat3(lastMatrixPalette[i1])*w.x;
         mat += mat3(lastMatrixPalette[i2])*w.y;
         mat += mat3(lastMatrixPalette[i3])*w.z;
         mat += mat3(lastMatrixPalette[i4])*w.w;

    vec3 trans = vec3(lastMatrixPalette[i1][0].w, lastMatrixPalette[i1][1].w, lastMatrixPalette[i1][2].w)*w.x;
         trans += vec3(lastMatrixPalette[i2][0].w, lastMatrixPalette[i2][1].w, lastMatrixPalette[i2][2].w)*w.y;
         trans += vec3(lastMatrixPalette[i3][0].w, lastMatrixPalette[i3][1].w, lastMatrixPalette[i3][2].w)*w.z;
         trans += vec3(lastMatrixPalette[i4][0].w, lastMatrixPalette[i4][1].w, lastMatrixPalette[i4][2].w)*w.w;

    mat4 ret;
    ret[0] = vec4(mat[0], 0);
    ret[1] = vec4(mat[1], 0);
    ret[2] = vec4(mat[2], 0);
    ret[3] = vec4(trans, 1.0);

    return ret;
}

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);

    vec4 current_clip = modelview_projection_matrix * pos;

    mat4 last_mat = getLastObjectSkinnedTransform();
    vec4 last_clip = projection_matrix * (last_modelview_matrix * (last_mat * pos));

    gl_Position = current_clip;

    writeVaryVelocity(current_clip, last_clip);

    vary_texcoord0 = (texture_matrix0 * vec4(texcoord0, 0, 1)).xy;
    vertex_color = diffuse_color;
}
