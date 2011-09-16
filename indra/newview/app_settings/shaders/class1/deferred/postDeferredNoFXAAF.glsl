/** 
 * @file postDeferredF.glsl
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

#ifndef gl_FragColor
out vec4 gl_FragColor;
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect edgeMap;
uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2D bloomMap;

uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;

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
	float sc = abs(depth-focal_distance)/-depth*blur_constant;
		
	sc /= magnification;
	
	// tan_pixel_angle = pixel_length/-depth;
	float pixel_length =  tan_pixel_angle*-focal_distance;
	
	sc = sc/pixel_length;
	sc *= 1.414;
	
	return sc;
}

void dofSampleNear(inout vec4 diff, inout float w, float cur_sc, vec2 tc)
{
	float d = getDepth(tc);
	
	float sc = calc_cof(d);
	
	float wg = 0.25;
		
	vec4 s = texture2DRect(diffuseRect, tc);
	// de-weight dull areas to make highlights 'pop'
	wg += s.r+s.g+s.b;
	
	diff += wg*s;
	
	w += wg;
}

void dofSample(inout vec4 diff, inout float w, float min_sc, float cur_depth, vec2 tc)
{
	float d = getDepth(tc);
	
	float sc = calc_cof(d);
	
	if (sc > min_sc //sampled pixel is more "out of focus" than current sample radius
	   || d < cur_depth) //sampled pixel is further away than current pixel
	{
		float wg = 0.25;
		
		vec4 s = texture2DRect(diffuseRect, tc);
		// de-weight dull areas to make highlights 'pop'
		wg += s.r+s.g+s.b;
	
		diff += wg*s;
		
		w += wg;
	}
}


void main() 
{
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
		
	vec2 tc = vary_fragcoord.xy;
	
	float depth = getDepth(tc);
	
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	
	{ 
		float w = 1.0;
		
		float sc = calc_cof(depth);
		sc = min(abs(sc), 10.0);
		
		float fd = depth*0.5f;
		
		float PI = 3.14159265358979323846264;

		// sample quite uniformly spaced points within a circle, for a circular 'bokeh'		
		//if (depth < focal_distance)
		{
			while (sc > 0.5)
			{
				int its = int(max(1.0,(sc*3.7)));
				for (int i=0; i<its; ++i)
				{
					float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
					float samp_x = sc*sin(ang);
					float samp_y = sc*cos(ang);
					// you could test sample coords against an interesting non-circular aperture shape here, if desired.
					dofSample(diff, w, sc, depth, vary_fragcoord.xy + vec2(samp_x,samp_y));
				}
				sc -= 1.0;
			}
		}
		
		diff /= w;
	}
		
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
}
