/** 
 * @file alphaF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform float display_gamma;
uniform vec4 gamma;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;

#if HAS_SHADOW
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;

uniform vec2 shadow_res;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;

#endif

#ifdef USE_DIFFUSE_TEX
uniform sampler2D diffuseMap;
#endif

VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cl);
vec2 encode_normal (vec3 n);
vec3 decode_normal (vec2 enc);

vec3 scaleSoftClip(vec3 l);
vec3 atmosFragAmbient(vec3 light, vec3 sunlit);
vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten);
vec3 atmosFragAffectDirectionalLight(float light, vec3 sunlit);
void calcFragAtmospherics(vec3 inPositionEye, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	float da = 1.0;

	vec3 col = vec3(0);

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist = d/la;
		float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
		dist_atten *= dist_atten;
		dist_atten *= 2.0;

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= max(dot(n, lv), 0.0);		

		float lit = max(da * dist_atten,0.0);

		col = light_col * lit * diffuse;

		// no spec for alpha shader...
	}

	return max(col, vec3(0.0,0.0,0.0));
}

#if HAS_SHADOW
float pcfShadow(sampler2DShadow shadowMap, vec4 stc)
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
#endif


void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	vec4 pos = vec4(vary_position, 1.0);
	
	float shadow = 1.0;

#if HAS_SHADOW
	vec4 spos = pos;
		
	if (spos.z > -shadow_clip.w)
	{	
		shadow = 0.0;

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
			shadow += pcfShadow(shadowMap3, lpos)*w;
			weight += w;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}

		if (spos.z < near_split.y && spos.z > far_split.z)
		{
			lpos = shadow_matrix[2]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
			w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap2, lpos)*w;
			weight += w;
		}

		if (spos.z < near_split.x && spos.z > far_split.y)
		{
			lpos = shadow_matrix[1]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
			w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
			shadow += pcfShadow(shadowMap1, lpos)*w;
			weight += w;
		}

		if (spos.z > far_split.x)
		{
			lpos = shadow_matrix[0]*spos;
							
			float w = 1.0;
			w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
				
			shadow += pcfShadow(shadowMap0, lpos)*w;
			weight += w;
		}
		

		shadow /= weight;
	}
	else
	{
		shadow = 1.0;
	}
#endif

#ifdef USE_INDEXED_TEX
	vec4 diff = diffuseLookup(vary_texcoord0.xy);
#else
	vec4 diff = texture2D(diffuseMap,vary_texcoord0.xy);
#endif

#ifdef FOR_IMPOSTOR
	vec4 color;
	color.rgb = diff.rgb;
	color.a = 1.0;

#ifdef USE_VERTEX_COLOR
	float final_alpha = diff.a * vertex_color.a;
	diff.rgb *= vertex_color.rgb;
#else
	float final_alpha = diff.a;
#endif
	
	// Insure we don't pollute depth with invis pixels in impostor rendering
	//
	if (final_alpha < 0.01)
	{
		discard;
	}
#else
	
#ifdef USE_VERTEX_COLOR
	float final_alpha = diff.a * vertex_color.a;
	diff.rgb *= vertex_color.rgb;
#else
	float final_alpha = diff.a;
#endif


	vec4 gamma_diff = diff;	
	diff.rgb = srgb_to_linear(diff.rgb);

	vec3 norm = vary_norm; 

        vec3 sunlit;
        vec3 amblit;
        vec3 additive;
        vec3 atten;

	calcFragAtmospherics(pos.xyz, 1.0, sunlit, amblit, additive, atten);

	vec2 abnormal	= encode_normal(norm.xyz);
		 norm.xyz   = decode_normal(abnormal.xy);

	float sun_da  = dot(norm.xyz, sun_dir.xyz);
	float moon_da = dot(norm.xyz, moon_dir.xyz);

    float final_da = max(sun_da, moon_da);
          final_da = min(final_da, shadow);
          final_da = clamp(final_da, 0.0f, 1.0f);
	  final_da = pow(final_da, 1.0/1.3);

	vec4 color = vec4(0,0,0,0);

	color.rgb = atmosFragAmbient(color.rgb, amblit);
	color.a   = final_alpha;

	float ambient = abs(final_da);
	ambient *= 0.5;
	ambient *= ambient;
	ambient = (1.0-ambient);

	color.rgb *= ambient;
	color.rgb += atmosFragAffectDirectionalLight(final_da, sunlit);
	color.rgb *= gamma_diff.rgb;

	//color.rgb = mix(diff.rgb, color.rgb, final_alpha);
	
	color.rgb = atmosFragLighting(color.rgb, additive, atten);
	color.rgb = scaleSoftClip(color.rgb);

	vec4 light = vec4(0,0,0,0);

	color.rgb = srgb_to_linear(color.rgb);
	
   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diff.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z);

	LIGHT_LOOP(1)
	LIGHT_LOOP(2)
	LIGHT_LOOP(3)
	LIGHT_LOOP(4)
	LIGHT_LOOP(5)
	LIGHT_LOOP(6)
	LIGHT_LOOP(7)

	// keep it linear
	//
	color.rgb += light.rgb;

	// straight to display gamma, we're post-deferred
	//
	color.rgb = linear_to_srgb(color.rgb);

#ifdef WATER_FOG
	color = applyWaterFogView(pos.xyz, color);
#endif
#endif

	frag_color = color;
}

