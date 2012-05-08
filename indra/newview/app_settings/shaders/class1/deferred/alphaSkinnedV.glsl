/** 
 * @file alphaSkinnedV.glsl
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

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec2 texcoord0;

mat4 getObjectSkinnedTransform();
void calcAtmospherics(vec3 inPositionEye);

float calcDirectionalLight(vec3 n, vec3 l);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);

VARYING vec3 vary_position;
VARYING vec3 vary_ambient;
VARYING vec3 vary_directional;
VARYING vec3 vary_normal;
VARYING vec3 vary_fragcoord;
VARYING vec3 vary_pointlight_col;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;


uniform float near_clip;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

float calcDirectionalLight(vec3 n, vec3 l)
{
        float a = max(dot(n,l),0.0);
        return a;
}

float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = dot(lv,lv);
	
	float da = 0.0;

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist2 = d/la;
		da = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= max(dot(n, lv), 0.0);		
	}

	return da;	
}

void main()
{
	vary_texcoord0 = texcoord0;
				
	vec4 pos;
	vec3 norm;
	
	mat4 trans = getObjectSkinnedTransform();
	trans = modelview_matrix * trans;
	
	pos = trans * vec4(position.xyz, 1.0);
	
	norm = position.xyz + normal.xyz;
	norm = normalize(( trans*vec4(norm, 1.0) ).xyz-pos.xyz);
	
	vec4 frag_pos = projection_matrix * pos;
	gl_Position = frag_pos;
	
	vary_position = pos.xyz;
	vary_normal = norm;	
	
	calcAtmospherics(pos.xyz);

	vec4 col = vec4(0.0, 0.0, 0.0, diffuse_color.a);

	// Collect normal lights
	col.rgb += light_diffuse[2].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[2], light_direction[2], light_attenuation[2].x, light_attenuation[2].y, light_attenuation[2].z);
	col.rgb += light_diffuse[3].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[3], light_direction[3], light_attenuation[3].x, light_attenuation[3].y, light_attenuation[3].z);
	col.rgb += light_diffuse[4].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[4], light_direction[4], light_attenuation[4].x, light_attenuation[4].y, light_attenuation[4].z);
	col.rgb += light_diffuse[5].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[5], light_direction[5], light_attenuation[5].x, light_attenuation[5].y, light_attenuation[5].z);
	col.rgb += light_diffuse[6].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[6], light_direction[6], light_attenuation[6].x, light_attenuation[6].y, light_attenuation[6].z);
	col.rgb += light_diffuse[7].rgb*calcPointLightOrSpotLight(pos.xyz, norm, light_position[7], light_direction[7], light_attenuation[7].x, light_attenuation[7].y, light_attenuation[7].z);

	vary_pointlight_col = col.rgb*diffuse_color.rgb;

	col.rgb = vec3(0,0,0);

	// Add windlight lights
	col.rgb = atmosAmbient(vec3(0.));
	
	vary_ambient = col.rgb*diffuse_color.rgb;
	vary_directional = diffuse_color.rgb*atmosAffectDirectionalLight(max(calcDirectionalLight(norm, light_position[0].xyz), (1.0-diffuse_color.a)*(1.0-diffuse_color.a)));
	
	col.rgb = min(col.rgb*diffuse_color.rgb, 1.0);
	
	vertex_color = col;

	
	
	vary_fragcoord.xyz = frag_pos.xyz + vec3(0,0,near_clip);
}


