/**
 * @file sumLightsV.glsl
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
 

float calcDirectionalLight(vec3 n, vec3 l);
float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float is_pointlight);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

vec4 sumLights(vec3 pos, vec3 norm, vec4 color, vec4 baseLight)
{
	vec4 col = vec4(0.0, 0.0, 0.0, color.a);
	
	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	
	// Collect normal lights
	col.rgb += light_diffuse[2].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[2], light_direction[2], light_attenuation[2].x, light_attenuation[2].z);
	col.rgb += light_diffuse[3].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[3], light_direction[3], light_attenuation[3].x, light_attenuation[3].z);
	col.rgb += light_diffuse[4].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[4], light_direction[4], light_attenuation[4].x, light_attenuation[4].z);
	col.rgb += light_diffuse[5].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[5], light_direction[5], light_attenuation[5].x, light_attenuation[5].z);
	col.rgb += light_diffuse[6].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[6], light_direction[6], light_attenuation[6].x, light_attenuation[6].z);
	col.rgb += light_diffuse[7].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[7], light_direction[7], light_attenuation[7].x, light_attenuation[7].z);
	col.rgb += light_diffuse[1].rgb*calcDirectionalLight(norm, light_position[1].xyz);
	col.rgb = scaleDownLight(col.rgb);

	// Add windlight lights
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLight(norm, light_position[0].xyz));
	col.rgb += atmosAmbient(baseLight.rgb);

	col.rgb = min(col.rgb*color.rgb, 1.0);
	
	return col;	
}

