/** 
 * @file postgiF.glsl
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

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect giLightMap;
uniform sampler2D	noiseMap;
uniform sampler2D	giMip;
uniform sampler2DRect edgeMap;


uniform vec2 delta;
uniform float kern_scale;
uniform float gi_edge_weight;
uniform float gi_blur_brightness;

VARYING vec2 vary_fragcoord;

void main() 
{
	vec2 dlt = kern_scale*delta;
	float defined_weight = 0.0; 
	vec3 col = vec3(0.0); 
	
	float e = 1.0;
	
	for (int i = 1; i < 8; i++)
	{
		vec2 tc = vary_fragcoord.xy + float(i) * dlt;
		
		e = max(e, 0.0);
		float wght = e;
		
	   	col += texture2DRect(giLightMap, tc).rgb*wght;
		defined_weight += wght;
				
		e *= e;
		e -=(texture2DRect(edgeMap, tc.xy-dlt*0.25).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.25).a)*gi_edge_weight;
	}

	e = 1.0;
	
	for (int i = 1; i < 8; i++)
	{
		vec2 tc = vary_fragcoord.xy - float(i) * dlt;
		
		e = max(e,0.0);
		float wght = e;
		
	   	col += texture2DRect(giLightMap, tc).rgb*wght;
		defined_weight += wght;
	
		e *= e;
		e -= (texture2DRect(edgeMap, tc.xy-dlt*0.25).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.25).a)*gi_edge_weight;
		
	}
	
	col /= max(defined_weight, 0.01);

	gl_FragColor.rgb = col * gi_blur_brightness;
}
