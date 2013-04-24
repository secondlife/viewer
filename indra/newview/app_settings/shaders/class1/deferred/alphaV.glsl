/** 
 * @file alphaV.glsl
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

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;

#ifdef USE_INDEXED_TEX
void passTextureIndex();
#endif

ATTRIBUTE vec3 normal;

#ifdef USE_VERTEX_COLOR
ATTRIBUTE vec4 diffuse_color;
#endif

ATTRIBUTE vec2 texcoord0;

#ifdef HAS_SKIN
mat4 getObjectSkinnedTransform();
#else
#ifdef IS_AVATAR_SKIN
mat4 getSkinnedTransform();
#endif
#endif

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

vec3 calcDirectionalLight(vec3 n, vec3 l);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

VARYING vec3 vary_ambient;
VARYING vec3 vary_directional;
VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec3 vary_pointlight_col;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

VARYING vec2 vary_texcoord0;

VARYING vec3 vary_norm;

uniform float near_clip;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

vec3 calcDirectionalLight(vec3 n, vec3 l)
{
        float a = max(dot(n,l),0.0);
        return vec3(a,a,a);
}

vec3 calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
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

	return vec3(da,da,da);	
}

void main()
{
	vec4 pos;
	vec3 norm;
	
	//transform vertex
#ifdef HAS_SKIN
	mat4 trans = getObjectSkinnedTransform();
	trans = modelview_matrix * trans;
	
	pos = trans * vec4(position.xyz, 1.0);
	
	norm = position.xyz + normal.xyz;
	norm = normalize((trans * vec4(norm, 1.0)).xyz - pos.xyz);
	vec4 frag_pos = projection_matrix * pos;
	gl_Position = frag_pos;
#else

#ifdef IS_AVATAR_SKIN
	mat4 trans = getSkinnedTransform();
	vec4 pos_in = vec4(position.xyz, 1.0);
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, normal);
	norm.y = dot(trans[1].xyz, normal);
	norm.z = dot(trans[2].xyz, normal);
	norm = normalize(norm);
	
	vec4 frag_pos = projection_matrix * pos;
	gl_Position = frag_pos;
#else
	norm = normalize(normal_matrix * normal);
	vec4 vert = vec4(position.xyz, 1.0);
	pos = (modelview_matrix * vert);
	gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
#endif
	
#endif

#ifdef USE_INDEXED_TEX
	passTextureIndex();
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
#else
	vary_texcoord0 = texcoord0;
#endif
	
	vary_norm = norm;
	vary_position = pos.xyz;

	calcAtmospherics(pos.xyz);

#ifndef USE_VERTEX_COLOR
	vec4 diffuse_color = vec4(1,1,1,1);
#endif
	//vec4 color = calcLighting(pos.xyz, norm, diffuse_color, vec4(0.));
	vec4 col = vec4(0.0, 0.0, 0.0, diffuse_color.a);
	
	vec3 diff = pow(diffuse_color.rgb, vec3(2.2));

	

	vary_pointlight_col = diff;

	
	col.rgb = vec3(0,0,0);

	// Add windlight lights
	col.rgb = atmosAmbient(col.rgb);
	
	vary_ambient = col.rgb*diff.rgb;

	vary_directional.rgb = atmosAffectDirectionalLight(1.0f);
	
	col.rgb = col.rgb*diff.rgb;
	
#ifdef USE_VERTEX_COLOR
	vertex_color = col;
#endif
	
#ifdef HAS_SKIN
	vary_fragcoord.xyz = frag_pos.xyz + vec3(0,0,near_clip);
#else

#ifdef IS_AVATAR_SKIN
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#else
	pos = modelview_projection_matrix * vert;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
#endif
	
#endif

}

