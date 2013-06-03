/** 
 * @file multiPointLightF.glsl
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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect depthMap;
uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform samplerCube environmentMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;


uniform vec3 env_mat[3];
uniform float sun_wash;

uniform int light_count;

#define MAX_LIGHT_COUNT		16
uniform vec4 light[MAX_LIGHT_COUNT];
uniform vec4 light_col[MAX_LIGHT_COUNT];

VARYING vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform float far_z;

uniform mat4 inv_proj;

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

void main() 
{
	vec2 frag = (vary_fragcoord.xy*0.5+0.5)*screen_res;
	vec3 pos = getPosition(frag.xy).xyz;
	if (pos.z < far_z)
	{
		discard;
	}
	
	vec3 norm = texture2DRect(normalMap, frag.xy).xyz;
	norm = (norm.xyz-0.5)*2.0; // unpack norm
	norm = normalize(norm);
	vec4 spec = texture2DRect(specularRect, frag.xy);
	vec3 diff = texture2DRect(diffuseRect, frag.xy).rgb;
	float noise = texture2D(noiseMap, frag.xy/128.0).b;
	vec3 out_col = vec3(0,0,0);
	vec3 npos = normalize(-pos);

	// As of OSX 10.6.7 ATI Apple's crash when using a variable size loop
	for (int i = 0; i < MAX_LIGHT_COUNT; ++i)
	{
		bool light_contrib = (i < light_count);
		
		vec3 lv = light[i].xyz-pos;
		float dist2 = dot(lv,lv);
		dist2 /= light[i].w;
		if (dist2 > 1.0)
		{
			light_contrib = false;
		}
		
		float da = dot(norm, lv);
		if (da < 0.0)
		{
			light_contrib = false;
		}
		
		if (light_contrib)
		{
			lv = normalize(lv);
			da = dot(norm, lv);
					
			float fa = light_col[i].a+1.0;
			float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);
			dist_atten *= noise;

			float lit = da * dist_atten;
			
			vec3 col = light_col[i].rgb*lit*diff;
			//vec3 col = vec3(dist2, light_col[i].a, lit);
			
			if (spec.a > 0.0)
			{
				//vec3 ref = dot(pos+lv, norm);
				
				float sa = dot(normalize(lv+npos),norm);
				
				if (sa > 0.0)
				{
					sa = 6 * texture2D(lightFunc, vec2(sa, spec.a)).r * min(dist_atten*4.0, 1.0);
					sa *= noise;
					col += da*sa*light_col[i].rgb*spec.rgb;
				}
			}
			
			out_col += col;
		}
	}
	
	if (dot(out_col, out_col) <= 0.0)
	{
		discard;
	}
	
	frag_color.rgb = out_col;
	frag_color.a = 0.0;
}
