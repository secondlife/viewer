/** 
 * @file sunLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

//class 2, shadows, no SSAO

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRectShadow shadowMap0;
uniform sampler2DRectShadow shadowMap1;
uniform sampler2DRectShadow shadowMap2;
uniform sampler2DRectShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
uniform sampler2D noiseMap;

uniform sampler2D		lightFunc;


// Inputs
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

varying vec2 vary_fragcoord;
varying vec4 vary_light;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;

uniform float shadow_bias;
uniform float shadow_offset;

uniform float spot_shadow_bias;
uniform float spot_shadow_offset;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

float pcfShadow(sampler2DRectShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias*scl;
	
	float cs = shadow2DRect(shadowMap, stc.xyz).x;
	float shadow = cs;

	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(1.5, 1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(1.5, -1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-1.5, 1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-1.5, -1.5, 0.0)).x, cs);
			
	return shadow/5.0;
	
	//return shadow;
}

float pcfSpotShadow(sampler2DRectShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += spot_shadow_bias*scl;
	
	float cs = shadow2DRect(shadowMap, stc.xyz).x;
	float shadow = cs;

	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(1.5, 1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(1.5, -1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-1.5, 1.5, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-1.5, -1.5, 0.0)).x, cs);
			
	return shadow/5.0;
	
	//return shadow;
}

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias*scl;
	
	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;

	vec2 off = 1.5/proj_shadow_res;
	
	shadow += max(shadow2D(shadowMap, stc.xyz+vec3(off.x, off.y, 0.0)).x, cs);
	shadow += max(shadow2D(shadowMap, stc.xyz+vec3(off.x, -off.y, 0.0)).x, cs);
	shadow += max(shadow2D(shadowMap, stc.xyz+vec3(-off.x, off.y, 0.0)).x, cs);
	shadow += max(shadow2D(shadowMap, stc.xyz+vec3(-off.x, -off.y, 0.0)).x, cs);
	
			
	return shadow/5.0;
	
	//return shadow;
}

void main() 
{
	vec2 pos_screen = vary_fragcoord.xy;
	
	//try doing an unproject here
	
	vec4 pos = getPosition(pos_screen);
	
	vec4 nmap4 = texture2DRect(normalMap, pos_screen);
	float displace = nmap4.w;
	vec3 norm = nmap4.xyz*2.0-1.0;
	
	/*if (pos.z == 0.0) // do nothing for sky *FIX: REMOVE THIS IF/WHEN THE POSITION MAP IS BEING USED AS A STENCIL
	{
		gl_FragColor = vec4(0.0); // doesn't matter
		return;
	}*/
	
	float shadow = 1.0;
	float dp_directional_light = max(0.0, dot(norm, vary_light.xyz));

	vec3 shadow_pos = pos.xyz + displace*norm;
	vec3 offset = vary_light.xyz * (1.0-dp_directional_light);
	
	vec4 spos = vec4(shadow_pos+offset*shadow_offset, 1.0);
	
	if (spos.z > -shadow_clip.w)
	{	
		if (dp_directional_light == 0.0)
		{
			// if we know this point is facing away from the sun then we know it's in shadow without having to do a squirrelly shadow-map lookup
			shadow = 0.0;
		}
		else
		{
			vec4 lpos;
			
			if (spos.z < -shadow_clip.z)
			{
				lpos = shadow_matrix[3]*spos;
				lpos.xy *= shadow_res;
				shadow = pcfShadow(shadowMap3, lpos, 0.25);
				shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
			}
			else if (spos.z < -shadow_clip.y)
			{
				lpos = shadow_matrix[2]*spos;
				lpos.xy *= shadow_res;
				shadow = pcfShadow(shadowMap2, lpos, 0.5);
			}
			else if (spos.z < -shadow_clip.x)
			{
				lpos = shadow_matrix[1]*spos;
				lpos.xy *= shadow_res;
				shadow = pcfShadow(shadowMap1, lpos, 0.75);
			}
			else
			{
				lpos = shadow_matrix[0]*spos;
				lpos.xy *= shadow_res;
				shadow = pcfShadow(shadowMap0, lpos, 1.0);
			}
		
			// take the most-shadowed value out of these two:
			//  * the blurred sun shadow in the light (shadow) map
			//  * an unblurred dot product between the sun and this norm
			// the goal is to err on the side of most-shadow to fill-in shadow holes and reduce artifacting
			shadow = min(shadow, dp_directional_light);
			
			//lpos.xy /= lpos.w*32.0;
			//if (fract(lpos.x) < 0.1 || fract(lpos.y) < 0.1)
			//{
			//	shadow = 0.0;
			//}
			
		}
	}
	else
	{
		// more distant than the shadow map covers
		shadow = 1.0;
	}
	
	gl_FragColor[0] = shadow;
	gl_FragColor[1] = 1.0;
	
	spos.xyz = shadow_pos*offset*spot_shadow_offset;
	
	//spotlight shadow 1
	vec4 lpos = shadow_matrix[4]*spos;
	gl_FragColor[2] = pcfSpotShadow(shadowMap4, lpos, 0.8).x; 
	
	//spotlight shadow 2
	lpos = shadow_matrix[5]*spos;
	gl_FragColor[3] = pcfSpotShadow(shadowMap5, lpos, 0.8).x; 

	//gl_FragColor.rgb = pos.xyz;
	//gl_FragColor.b = shadow;
}
