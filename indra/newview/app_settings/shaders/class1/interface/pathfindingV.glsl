/** 
 * @file pathfindingV.glsl
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

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;

VARYING vec4 vertex_color;

uniform float tint;
uniform float ambiance;
uniform float alpha_scale;
 
void main()
{
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

	vec3 l1 = vec3(-0.75, 1, 1.0)*0.5;
	vec3 l2 = vec3(0.5, -0.6, 0.4)*0.25;
	vec3 l3 = vec3(0.5, -0.8, 0.3)*0.5;

	float lit = max(dot(normal, l1), 0.0);
	lit += max(dot(normal, l2), 0.0);
	lit += max(dot(normal, l3), 0.0);

	lit = clamp(lit, ambiance, 1.0);
	
	vertex_color = vec4(diffuse_color.rgb * tint * lit, diffuse_color.a*alpha_scale);
}

