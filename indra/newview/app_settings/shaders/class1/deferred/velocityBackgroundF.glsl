/**
 * @file velocityBackgroundF.glsl
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

out vec4 frag_color;

in vec2 vary_fragcoord;

uniform mat4 projection_matrix;
uniform mat4 inv_proj;
uniform mat4 current_modelview_matrix;
uniform mat4 last_modelview_matrix;

void main()
{
    // camera-rotation-only reprojection velocity for the sky: treat the pixel
    // as a direction at infinity, carry it into last frame's view, reproject.
    // camera translation cannot move a point at infinity, so directions suffice
    vec2 ndc = vary_fragcoord * 2.0 - 1.0;

    vec4 vp = inv_proj * vec4(ndc, 1.0, 1.0);
    vec3 dir_view = vp.xyz;

    // modelview is rigid for the camera, so transpose == inverse rotation
    vec3 dir_world = transpose(mat3(current_modelview_matrix)) * dir_view;
    vec3 last_dir_view = mat3(last_modelview_matrix) * dir_world;

    vec4 last_clip = projection_matrix * vec4(last_dir_view, 0.0);
    vec2 last_ndc = last_clip.xy / max(last_clip.w, 1e-6);

    frag_color = vec4(ndc - last_ndc, 0.0, 1.0);
}
