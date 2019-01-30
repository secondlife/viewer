/** 
 * @file class1/deferred/shadowUtil.glsl
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
uniform vec3 moon_dir;
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

float pcfShadowLegacy(sampler2DShadow shadowMap, vec4 stc)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias;
		
	stc.x = floor(stc.x*shadow_res.x + fract(stc.y*shadow_res.y*12345))/shadow_res.x; // add some chaotic jitter to X sample pos according to Y to disguise the snapping going on here
	
	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;
	
    shadow += shadow2D(shadowMap, stc.xyz+vec3(2.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(1.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-1.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-2.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
                       
    return shadow*0.2;
}

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen)
{
    stc.xyz /= stc.w;
    stc.z += shadow_bias * bias_scale;
 
    stc.x = floor(stc.x*pos_screen.x + fract(stc.y*pos_screen.y*0.666666666))/shadow_res.x; // add some chaotic jitter to X sample pos according to Y to disguise the snapping going on here
    
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
    float dp_sun = max(0.0, dot(sun_dir.xyz, norm));
    float dp_moon = max(0.0, dot(moon_dir.xyz, norm));
    float dp_directional_light = max(dp_sun,dp_moon);
          dp_directional_light = clamp(dp_directional_light, 0.0, 1.0);

    vec3 light_dir = (dp_moon > dp_sun) ? moon_dir : sun_dir;
    vec3 offset = light_dir * (1.0-dp_directional_light);
    vec3 shadow_pos = pos.xyz + (offset * shadow_bias);

    float shadow = 0.0f;
    vec4 spos = vec4(shadow_pos,1.0);

    // if we know this point is facing away from the sun then we know it's in shadow without having to do a squirrelly shadow-map lookup
    if (dp_directional_light <= 0.0)
    {
        return 0.0;
    }

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
            shadow += pcfShadowLegacy(shadowMap3, lpos)*w;
            weight += w;
            shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
        }

        if (spos.z < near_split.y && spos.z > far_split.z)
        {
            lpos = shadow_matrix[2]*spos;
            
            float w = 1.0;
            w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
            w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
            shadow += pcfShadowLegacy(shadowMap2, lpos)*w;
            weight += w;
        }

        if (spos.z < near_split.x && spos.z > far_split.y)
        {
            lpos = shadow_matrix[1]*spos;
            
            float w = 1.0;
            w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
            w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
            shadow += pcfShadowLegacy(shadowMap1, lpos)*w;
            weight += w;
        }

        if (spos.z > far_split.x)
        {
            lpos = shadow_matrix[0]*spos;
                            
            float w = 1.0;
            w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
                
            shadow += pcfShadowLegacy(shadowMap0, lpos)*w;
            weight += w;
        }

        shadow /= weight;
    }

    return min(dp_directional_light, shadow);
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

