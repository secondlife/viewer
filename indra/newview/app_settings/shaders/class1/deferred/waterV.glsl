/** 
 * @file waterV.glsl
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
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;


void calcAtmospherics(vec3 inPositionEye);

uniform vec2 d1;
uniform vec2 d2;
uniform float time;
uniform vec3 eyeVec;
uniform float waterHeight;

VARYING vec4 refCoord;
VARYING vec4 littleWave;
VARYING vec4 view;

VARYING vec4 vary_position;

float wave(vec2 v, float t, float f, vec2 d, float s) 
{
   return (dot(d, v)*f + t*s)*f;
}

void main()
{
	//transform vertex
	vec4 pos = vec4(position.xyz, 1.0);
	mat4 modelViewProj = modelview_projection_matrix;
	
	vec4 oPosition;
		    
	//get view vector
	vec3 oEyeVec;
	oEyeVec.xyz = pos.xyz-eyeVec;
		
	float d = length(oEyeVec.xy);
	float ld = min(d, 2560.0);
	
	pos.xy = eyeVec.xy + oEyeVec.xy/d*ld;
	view.xyz = oEyeVec;
		
	d = clamp(ld/1536.0-0.5, 0.0, 1.0);	
	d *= d;
		
	oPosition = vec4(position, 1.0);
	oPosition.z = mix(oPosition.z, max(eyeVec.z*0.75, 0.0), d);
	vary_position = modelview_matrix * oPosition;
	oPosition = modelViewProj * oPosition;
	
	refCoord.xyz = oPosition.xyz + vec3(0,0,0.2);
	
	//get wave position parameter (create sweeping horizontal waves)
	vec3 v = pos.xyz;
	v.x += (cos(v.x*0.08/*+time*0.01*/)+sin(v.y*0.02))*6.0;
	    
	//push position for further horizon effect.
	pos.xyz = oEyeVec.xyz*(waterHeight/oEyeVec.z);
	pos.w = 1.0;
	pos = modelview_matrix*pos;
	
	calcAtmospherics(view.xyz);
		
	//pass wave parameters to pixel shader
	vec2 bigWave =  (v.xy) * vec2(0.04,0.04)  + d1 * time * 0.055;
	//get two normal map (detail map) texture coordinates
	littleWave.xy = (v.xy) * vec2(0.45, 0.9)   + d2 * time * 0.13;
	littleWave.zw = (v.xy) * vec2(0.1, 0.2) + d1 * time * 0.1;
	view.w = bigWave.y;
	refCoord.w = bigWave.x;
	
	gl_Position = oPosition;
}
