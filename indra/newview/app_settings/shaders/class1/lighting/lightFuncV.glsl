/** 
 * @file lightFuncV.glsl
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
 



float calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return a;
}


float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	//normalize light vector
	lv *= 1.0/d;
	
	//distance attenuation
	float da = clamp(1.0/(la * d), 0.0, 1.0);
	
	// spotlight coefficient.
	float spot = max(dot(-ln, lv), is_pointlight);
	da *= spot*spot; // GL_SPOT_EXPONENT=2

	//angular attenuation
	da *= calcDirectionalLight(n, lv);

	return da;	
}

