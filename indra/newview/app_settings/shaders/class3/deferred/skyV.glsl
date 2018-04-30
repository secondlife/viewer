/** 
 * @file advancedAtmoV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 
uniform vec3 cameraPosLocal;
uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 inv_proj;
uniform mat4 inv_modelview;

ATTRIBUTE vec3 position;

// Inputs
uniform vec3 camPosLocal;

out vec3 view_pos;
out vec3 view_dir;

void main()
{
    // pass through untransformed fullscreen pos (clipspace)
	gl_Position = vec4(position.xyz, 1.0);

    view_pos = (inv_proj * vec4(position, 1.0f)).xyz;

	// this will be normalized in the frag shader...
	//view_dir = (inv_modelview * view_pos).xyz;
    view_dir = view_pos - camPosLocal;
}

