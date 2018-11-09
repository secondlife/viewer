/** 
 * @file shadowUtil.glsl
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

uniform sampler2DRect   normalMap;
uniform sampler2DRect   depthMap;
uniform sampler2D       noiseMap;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;

uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

uniform vec3 sun_dir;
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;

uniform float spot_shadow_bias;
uniform float spot_shadow_offset;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec3 decode_normal(vec2 enc);

vec2 getScreenCoordinate(vec2 screenpos)
{
	vec2 sc = screenpos.xy * 2.0;
    if (screen_res.x > 0 && screen_res.y > 0)
    {
       sc /= screen_res;
    }
    return sc - vec2(1.0, 1.0);
}

vec3 getNorm(vec2 screenpos)
{
   vec2 enc_norm = texture2DRect(normalMap, screenpos.xy).xy;
   return decode_normal(enc_norm);
}

float getDepth(vec2 pos_screen)
{
    float depth = texture2DRect(depthMap, pos_screen).r;
	return depth;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
	vec2 sc = getScreenCoordinate(pos_screen);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

vec4 getPositionWithDepth(vec2 pos_screen, float depth)
{
	vec2 sc = getScreenCoordinate(pos_screen);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias * bias_scale;
		
	stc.x = floor(stc.x*pos_screen.x + fract(stc.y*shadow_res.y*12345))/shadow_res.x; // add some chaotic jitter to X sample pos according to Y to disguise the snapping going on here
	
	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(2.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(1.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-1.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-2.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
    return shadow*0.2;
}

float pcfSpotShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen)
{
	stc.xyz /= stc.w;
	stc.z += spot_shadow_bias * bias_scale;
	stc.x = floor(proj_shadow_res.x * stc.x + fract(pos_screen.y*0.666666666)) / proj_shadow_res.x; // snap

	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;

	vec2 off = 1.0/proj_shadow_res;
	off.y *= 1.5;
	
	shadow += shadow2D(shadowMap, stc.xyz+vec3(off.x*2.0, off.y, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(off.x, -off.y, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-off.x, off.y, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-off.x*2.0, -off.y, 0.0)).x;
    return shadow*0.2;
}

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen)
{
	float dp_directional_light = max(0.0, dot(sun_dir.xyz, norm));
	vec3 offset = sun_dir.xyz * (1.0-dp_directional_light);
	vec3 shadow_pos = pos.xyz + (offset * shadow_bias);

    float shadow = 0.0f;
	vec4 spos = vec4(shadow_pos,1.0);
	if (spos.z > -shadow_clip.w)
	{	
		vec4 lpos;
		vec4 near_split = shadow_clip*-0.75;
		vec4 far_split = shadow_clip*-1.25;
		vec4 transition_domain = near_split-far_split;
		float weight = 0.0;

		if (spos.z < near_split.z)
		{
			lpos = shadow_matrix[3]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap3, lpos, 0.5, pos_screen)*w;
			weight += w;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}

		if (spos.z < near_split.y && spos.z > far_split.z)
		{
			lpos = shadow_matrix[2]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
			w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap2, lpos, 0.75, pos_screen)*w;
			weight += w;
		}

		if (spos.z < near_split.x && spos.z > far_split.y)
		{
			lpos = shadow_matrix[1]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
			w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
			shadow += pcfShadow(shadowMap1, lpos, 0.88, pos_screen)*w;
			weight += w;
		}

		if (spos.z > far_split.x)
		{
			lpos = shadow_matrix[0]*spos;
							
			float w = 1.0;
			w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
				
			shadow += pcfShadow(shadowMap0, lpos, 1.0, pos_screen)*w;
			weight += w;
		}

		shadow /= weight;
	}
    return shadow;
}

float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen)
{
    float shadow = 0.0f;
    pos += norm * spot_shadow_offset;

	vec4 spos = vec4(pos,1.0);
	if (spos.z > -shadow_clip.w)
	{	
		vec4 lpos;
		
		vec4 near_split = shadow_clip*-0.75;
		vec4 far_split = shadow_clip*-1.25;
		vec4 transition_domain = near_split-far_split;
		float weight = 0.0;

        {
			lpos = shadow_matrix[4 + index]*spos;
			float w = 1.0;
			w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
		
			shadow += pcfSpotShadow((index == 0) ? shadowMap4 : shadowMap5, lpos, 0.8, spos.xy)*w;
			weight += w;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}

		shadow /= weight;
	}
    return shadow;
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
float calcAmbientOcclusion(vec4 pos, vec3 norm, vec2 pos_screen)
{
	float ret = 1.0;
	vec3 pos_world = pos.xyz;
	vec2 noise_reflect = texture2D(noiseMap, pos_screen.xy/128.0).xy;
		
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

