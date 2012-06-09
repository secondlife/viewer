/** 
 * @file cofF.glsl
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

#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect depthMap;
uniform sampler2D bloomMap;

uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;
uniform float max_cof;

uniform mat4 inv_proj;
uniform vec2 screen_res;

VARYING vec2 vary_fragcoord;

float getDepth(vec2 pos_screen)
{
	float z = texture2DRect(depthMap, pos_screen.xy).r;
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

float calc_cof(float depth)
{
	float sc = (depth-focal_distance)/-depth*blur_constant;
		
	sc /= magnification;
	
	// tan_pixel_angle = pixel_length/-depth;
	float pixel_length =  tan_pixel_angle*-focal_distance;
	
	sc = sc/pixel_length;
	sc *= 1.414;
	
	return sc;
}

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	
	float depth = getDepth(tc);
	
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	
	float sc = calc_cof(depth);
	sc = min(sc, max_cof);
	sc = max(sc, -max_cof);
	
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	frag_color.rgb = diff.rgb + bloom.rgb;
	frag_color.a = sc/max_cof*0.5+0.5;
}
