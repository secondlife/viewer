/** 
 * @file normgenF.glsl
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
 
/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D alphaMap;

in vec2 vary_texcoord0;

uniform float stepX;
uniform float stepY;
uniform float norm_scale;

void main()
{
	float c = texture(alphaMap, vary_texcoord0).r;

	vec3 right = vec3(norm_scale, 0, (texture(alphaMap, vary_texcoord0+vec2(stepX, 0)).r-c)*255);
	vec3 left = vec3(-norm_scale, 0, (texture(alphaMap, vary_texcoord0-vec2(stepX, 0)).r-c)*255);
	vec3 up = vec3(0, -norm_scale, (texture(alphaMap, vary_texcoord0-vec2(0, stepY)).r-c)*255);
	vec3 down = vec3(0, norm_scale, (texture(alphaMap, vary_texcoord0+vec2(0, stepY)).r-c)*255);
	
	vec3 norm = cross(right, down) + cross(down, left) + cross(left,up) + cross(up, right);
	
	norm = normalize(norm);
	norm *= 0.5;
	norm += 0.5;	

	frag_color = vec4(norm, c);
}
