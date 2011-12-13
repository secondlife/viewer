/** 
 * @file alphaNonIndexedNoColorF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

uniform sampler2DRectShadow shadowMap0;
uniform sampler2DRectShadow shadowMap1;
uniform sampler2DRectShadow shadowMap2;
uniform sampler2DRectShadow shadowMap3;
uniform sampler2DRect depthMap;
uniform sampler2D diffuseMap;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform vec2 screen_res;
uniform vec2 shadow_res;

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

VARYING vec3 vary_ambient;
VARYING vec3 vary_directional;
VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec3 vary_pointlight_col;
VARYING vec2 vary_texcoord0;

uniform float shadow_bias;

uniform mat4 inv_proj;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos.xyz /= pos.w;
	pos.w = 1.0;
	return pos;
}

float pcfShadow(sampler2DRectShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias;
	
	float cs = shadow2DRect(shadowMap, stc.xyz).x;
	float shadow = cs;

	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(scl, scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(scl, -scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-scl, scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-scl, -scl, 0.0)).x, cs);
			
	return shadow/5.0;
}


void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	float shadow = 1.0;
	vec4 pos = vec4(vary_position, 1.0);
	
	vec4 spos = pos;
		
	if (spos.z > -shadow_clip.w)
	{	
		vec4 lpos;
		
		if (spos.z < -shadow_clip.z)
		{
			lpos = shadow_matrix[3]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap3, lpos, 1.5);
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}
		else if (spos.z < -shadow_clip.y)
		{
			lpos = shadow_matrix[2]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap2, lpos, 1.5);
		}
		else if (spos.z < -shadow_clip.x)
		{
			lpos = shadow_matrix[1]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap1, lpos, 1.5);
		}
		else
		{
			lpos = shadow_matrix[0]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap0, lpos, 1.5);
		}
	}
	
	vec4 diff = texture2D(diffuseMap,vary_texcoord0.xy);

	vec4 col = vec4(vary_ambient + vary_directional.rgb*shadow, 1.0);
	vec4 color = diff * col;
	
	color.rgb = atmosLighting(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	color.rgb += diff.rgb * vary_pointlight_col.rgb;

	gl_FragColor = color;
}

