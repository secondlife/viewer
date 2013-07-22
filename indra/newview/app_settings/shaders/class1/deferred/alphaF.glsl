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

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

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

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

VARYING vec3 vary_ambient;
VARYING vec3 vary_directional;
VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec3 vary_pointlight_col;
VARYING vec3 vary_pointlight_col_linear;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

uniform vec2 screen_res;

vec3 calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	a = pow(a, 1.0/1.3);
	return vec3(a,a,a);
}

vec3 calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	float da = 0.0;

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist = d/la;
		da = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
		da *= da;
		da *= 2.0;
	

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= max(dot(n, lv), 0.0);		
	}

	return vec3(da,da,da);	
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

#ifdef WATER_FOG
uniform vec4 waterPlane;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;

vec4 applyWaterFogDeferred(vec3 pos, vec4 color)
{
	//normalize view vector
	vec3 view = normalize(pos);
	float es = -(dot(view, waterPlane.xyz));

	//find intersection point with water plane and eye vector
	
	//get eye depth
	float e0 = max(-waterPlane.w, 0.0);
	
	vec3 int_v = waterPlane.w > 0.0 ? view * waterPlane.w/es : vec3(0.0, 0.0, 0.0);
	
	//get object depth
	float depth = length(pos - int_v);
		
	//get "thickness" of water
	float l = max(depth, 0.1);

	float kd = waterFogDensity;
	float ks = waterFogKS;
	vec4 kc = waterFogColor;
	
	float F = 0.98;
	
	float t1 = -kd * pow(F, ks * e0);
	float t2 = kd + ks * es;
	float t3 = pow(F, t2*l) - 1.0;
	
	float L = min(t1/t2*t3, 1.0);
	
	float D = pow(0.98, l*kd);
	
	color.rgb = color.rgb * D + kc.rgb * L;
	color.a = kc.a + color.a;
	
	return color;
}
#endif

vec3 srgb_to_linear(vec3 cs)
{
	
/*        {  cs / 12.92,                 cs <= 0.04045
    cl = {
        {  ((cs + 0.055)/1.055)^2.4,   cs >  0.04045*/

	return pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
}

vec3 linear_to_srgb(vec3 cl)
{
	    /*{  0.0,                          0         <= cl
            {  12.92 * c,                    0         <  cl < 0.0031308
    cs = {  1.055 * cl^0.41666 - 0.055,   0.0031308 <= cl < 1
            {  1.0,                                       cl >= 1*/

	return 1.055 * pow(cl, vec3(0.41666)) - 0.055;
}


void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	vec4 pos = vec4(vary_position, 1.0);
	

#if HAS_SHADOW
	float shadow = 0.0;
	vec4 spos = pos;
		
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
	vec4 gamma_diff = diff;

	diff.rgb = srgb_to_linear(diff.rgb);
	
#ifdef USE_VERTEX_COLOR
	float vertex_color_alpha = diff.a * vertex_color.a;	
#else
	float vertex_color_alpha = 1.0;
#endif
	
	vec3 normal = vary_norm; 
	
	vec3 l = light_position[0].xyz;
	vec3 dlight = calcDirectionalLight(normal, l);
	dlight = dlight * vary_directional.rgb * vary_pointlight_col;

#if HAS_SHADOW
	vec4 col = vec4(vary_ambient + dlight * shadow, vertex_color_alpha);
#else
	vec4 col = vec4(vary_ambient + dlight, vertex_color_alpha);
#endif

	vec4 color = col;
	color.rgb *= gamma_diff.rgb;

	color.rgb = atmosLighting(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	color.rgb = srgb_to_linear(color.rgb);
	
	col = vec4(0,0,0,0);
	
   #define LIGHT_LOOP(i) col.rgb += light_diffuse[i].rgb * calcPointLightOrSpotLight(pos.xyz, normal, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z);

	LIGHT_LOOP(1)
	LIGHT_LOOP(2)
	LIGHT_LOOP(3)
	LIGHT_LOOP(4)
	LIGHT_LOOP(5)
	LIGHT_LOOP(6)
	LIGHT_LOOP(7)

	color.rgb += diff.rgb * vary_pointlight_col_linear * col.rgb;

	color.rgb = linear_to_srgb(color.rgb);

#ifdef WATER_FOG
	color = applyWaterFogDeferred(pos.xyz, color);
#endif

	frag_color = color;
}

