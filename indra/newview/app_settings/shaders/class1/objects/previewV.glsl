/** 
 * @file previewV.glsl
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

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;

uniform vec4 color;

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

//===================================================================================================
//declare these here explicitly to separate them from atmospheric lighting elsewhere to work around
//drivers that are picky about functions being declared but not defined even if they aren't called
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
//====================================================================================================


void main()
{
	//transform vertex
	vec4 pos = (modelview_matrix * vec4(position.xyz, 1.0));
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
	
	vec3 norm = normalize(normal_matrix * normal);

	vec4 col = vec4(0,0,0,1);

	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += light_diffuse[1].rgb * calcDirectionalLight(norm, light_position[1].xyz);
	col.rgb += light_diffuse[2].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[2], light_direction[2], light_attenuation[2].x, light_attenuation[2].z);
	col.rgb += light_diffuse[3].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[3], light_direction[3], light_attenuation[3].x, light_attenuation[3].z);
		
	vertex_color = col*color;
}
