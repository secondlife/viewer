/** 
 * @file sunLightSSAOF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

//class 2 -- shadows and SSAO

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRectShadow shadowMap0;
uniform sampler2DRectShadow shadowMap1;
uniform sampler2DRectShadow shadowMap2;
uniform sampler2DRectShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
uniform sampler2D noiseMap;


// Inputs
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;
uniform vec3 sun_dir;

uniform float shadow_bias;
uniform float shadow_offset;

uniform float spot_shadow_bias;
uniform float spot_shadow_offset;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).r;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

vec2 getKern(int i)
{
	vec2 kern[8];
	// exponentially (^2) distant occlusion samples spread around origin
	kern[0] = vec2(-1.0, 0.0) * 0.125*0.125;
	kern[1] = vec2(1.0, 0.0) * 0.250*0.250;
	kern[2] = vec2(0.0, 1.0) * 0.375*0.375;
	kern[3] = vec2(0.0, -1.0) * 0.500*0.500;
	kern[4] = vec2(0.7071, 0.7071) * 0.625*0.625;
	kern[5] = vec2(-0.7071, -0.7071) * 0.750*0.750;
	kern[6] = vec2(-0.7071, 0.7071) * 0.875*0.875;
	kern[7] = vec2(0.7071, -0.7071) * 1.000*1.000;
	
	return kern[i];
}

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm)
{
	float ret = 1.0;

	vec2 pos_screen = vary_fragcoord.xy;
	vec3 pos_world = pos.xyz;
	vec2 noise_reflect = texture2D(noiseMap, vary_fragcoord.xy/128.0).xy;
		
	float angle_hidden = 0.0;
	float points = 0;
		
	float scale = min(ssao_radius / -pos_world.z, ssao_max_radius);
	
	// it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations (unrolling?)
	for (int i = 0; i < 8; i++)
	{
		vec2 samppos_screen = pos_screen + scale * reflect(getKern(i), noise_reflect);
		vec3 samppos_world = getPosition(samppos_screen).xyz; 
		
		vec3 diff = pos_world - samppos_world;
		float dist2 = dot(diff, diff);
			
		// assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
		// --> solid angle shrinking by the square of distance
		//radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
		//(k should vary inversely with # of samples, but this is taken care of later)
		
		float funky_val = (dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) ? 1.0 : 0.0;
		angle_hidden = angle_hidden + funky_val * min(1.0/dist2, ssao_factor_inv);
			
		// 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion" 
		float diffz_val = (diff.z > -1.0) ? 1.0 : 0.0;
		points = points + diffz_val;
	}
		
	angle_hidden = min(ssao_factor*angle_hidden/points, 1.0);
	
	float points_val = (points > 0.0) ? 1.0 : 0.0;
	ret = (1.0 - (points_val * angle_hidden));

	ret = max(ret, 0.0);
	return min(ret, 1.0);
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

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += spot_shadow_bias*scl;
	
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
	nmap4 = vec4((nmap4.xyz-0.5)*2.0,nmap4.w); // unpack norm
	float displace = nmap4.w;
	vec3 norm = nmap4.xyz;
	
	/*if (pos.z == 0.0) // do nothing for sky *FIX: REMOVE THIS IF/WHEN THE POSITION MAP IS BEING USED AS A STENCIL
	{
		gl_FragColor = vec4(0.0); // doesn't matter
		return;
	}*/
	
	float shadow = 1.0;
	float dp_directional_light = max(0.0, dot(norm, sun_dir.xyz));

	vec3 shadow_pos = pos.xyz + displace*norm;
	vec3 offset = sun_dir.xyz * (1.0-dp_directional_light);
	
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
	gl_FragColor[1] = calcAmbientOcclusion(pos, norm);
	
	spos = vec4(shadow_pos+norm*spot_shadow_offset, 1.0);
	
	//spotlight shadow 1
	vec4 lpos = shadow_matrix[4]*spos;
	gl_FragColor[2] = pcfShadow(shadowMap4, lpos, 0.8); 
	
	//spotlight shadow 2
	lpos = shadow_matrix[5]*spos;
	gl_FragColor[3] = pcfShadow(shadowMap5, lpos, 0.8); 

	//gl_FragColor.rgb = pos.xyz;
	//gl_FragColor.b = shadow;
}
