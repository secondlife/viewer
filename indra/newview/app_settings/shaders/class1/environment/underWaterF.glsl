/**
 * @file underWaterF.glsl
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
 


uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;   
uniform sampler2D screenTex;

uniform float refScale;
uniform vec4 waterFogColor;

//bigWave is (refCoord.w, view.w);
varying vec4 refCoord;
varying vec4 littleWave;
varying vec4 view;

void main() 
{
	vec4 color;    
	
	//get bigwave normal
	vec3 wavef = texture2D(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0;
    
	//get detail normals
	vec3 dcol = texture2D(bumpMap, littleWave.xy).rgb*0.75;
	dcol += texture2D(bumpMap, littleWave.zw).rgb*1.25;
	    
	//interpolate between big waves and little waves (big waves in deep water)
	wavef = (wavef+dcol)*0.5;

	//crunch normal to range [-1,1]
	wavef -= vec3(1,1,1);
	
	//figure out distortion vector (ripply)   
	vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
	distort = distort+wavef.xy*refScale;
		
	vec4 fb = texture2D(screenTex, distort);
	
	gl_FragColor.rgb = mix(waterFogColor.rgb, fb.rgb, waterFogColor.a * 0.001 + 0.999);
	gl_FragColor.a = fb.a;
}
