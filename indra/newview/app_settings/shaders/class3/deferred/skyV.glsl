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
ATTRIBUTE vec2 texcoord0;

VARYING vec2 vary_frag;

void main()
{
    // pass through untransformed fullscreen pos at back of frustum for proper sky depth testing
	gl_Position = vec4(position.xy, 1.0f, 1.0);
    vary_frag = texcoord0;
}

