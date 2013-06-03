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
 
float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da);
vec3 calcPointLightSpecular(inout vec4 specular, vec3 view, vec3 v, vec3 n, vec3 l, float r, float pw, vec3 lightCol);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 atmosGetDiffuseSunlightColor();
vec3 scaleDownLight(vec3 light);

uniform vec4 light_position[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	vec4 col = vec4(0.0, 0.0, 0.0, color.a);
	
	vec3 view = normalize(pos);
	
	/// collect all the specular values from each calcXXXLightSpecular() function
	vec4 specularSum = vec4(0.0);
	
	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += light_diffuse[1].rgb * calcDirectionalLightSpecular(specularColor, view, norm, light_position[1].xyz,light_diffuse[1].rgb, 1.0);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[2].xyz, light_attenuation[2].x, light_attenuation[2].y, light_diffuse[2].rgb); 
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[3].xyz, light_attenuation[3].x, light_attenuation[3].y, light_diffuse[3].rgb); 
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[4].xyz, light_attenuation[4].x, light_attenuation[4].y, light_diffuse[4].rgb); 
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[5].xyz, light_attenuation[5].x, light_attenuation[5].y, light_diffuse[5].rgb); 
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[6].xyz, light_attenuation[6].x, light_attenuation[6].y, light_diffuse[6].rgb); 
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, light_position[7].xyz, light_attenuation[7].x, light_attenuation[7].y, light_diffuse[7].rgb); 
	col.rgb = scaleDownLight(col.rgb);
						
	// Add windlight lights
	col.rgb += atmosAmbient(baseCol.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLightSpecular(specularSum, view, norm, light_position[0].xyz,atmosGetDiffuseSunlightColor()*baseCol.a, 1.0));

	col.rgb = min(col.rgb*color.rgb, 1.0);
	specularColor.rgb = min(specularColor.rgb*specularSum.rgb, 1.0);

	col.rgb += specularColor.rgb;
	return col;	
}
