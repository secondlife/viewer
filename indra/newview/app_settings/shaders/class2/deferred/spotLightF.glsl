/** 
 * @file spotLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform samplerCube environmentMap;
uniform sampler2DRect lightMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;
uniform sampler2D projectionMap;

uniform mat4 proj_mat; //screen space to light space
uniform float proj_near; //near clip for projection
uniform vec3 proj_p; //plane projection is emitting from (in screen space)
uniform vec3 proj_n;
uniform float proj_focus; //distance from plane to begin blurring
uniform float proj_lod;  //(number of mips in proj map)
uniform float proj_range; //range between near clip and far clip plane of projection
uniform float proj_ambiance;
uniform float near_clip;
uniform float far_clip;

uniform vec3 proj_origin; //origin of projection to be used for angular attenuation
uniform float sun_wash;
uniform int proj_shadow_idx;
uniform float shadow_fade;

varying vec4 vary_light;

varying vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform mat4 inv_proj;

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

void main() 
{
	vec4 frag = vary_fragcoord;
	frag.xyz /= frag.w;
	frag.xyz = frag.xyz*0.5+0.5;
	frag.xy *= screen_res;
	
	float shadow = 1.0;
	
	if (proj_shadow_idx >= 0)
	{
		vec4 shd = texture2DRect(lightMap, frag.xy);
		float sh[2];
		sh[0] = shd.b;
		sh[1] = shd.a;
		shadow = min(sh[proj_shadow_idx]+shadow_fade, 1.0);
	}
	
	vec3 pos = getPosition(frag.xy).xyz;
	vec3 lv = vary_light.xyz-pos.xyz;
	float dist2 = dot(lv,lv);
	dist2 /= vary_light.w;
	if (dist2 > 1.0)
	{
		discard;
	}
	
	vec3 norm = texture2DRect(normalMap, frag.xy).xyz*2.0-1.0;
	
	norm = normalize(norm);
	float l_dist = -dot(lv, proj_n);
	
	vec4 proj_tc = (proj_mat * vec4(pos.xyz, 1.0));
	if (proj_tc.z < 0.0)
	{
		discard;
	}
	
	proj_tc.xyz /= proj_tc.w;
	
	float fa = gl_Color.a+1.0;
	float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);
	
	lv = proj_origin-pos.xyz;
	lv = normalize(lv);
	float da = dot(norm, lv);
		
	vec3 col = vec3(0,0,0);
		
	vec3 diff_tex = texture2DRect(diffuseRect, frag.xy).rgb;
		
	float noise = texture2D(noiseMap, frag.xy/128.0).b;
	if (proj_tc.z > 0.0 &&
		proj_tc.x < 1.0 &&
		proj_tc.y < 1.0 &&
		proj_tc.x > 0.0 &&
		proj_tc.y > 0.0)
	{
		float lit = 0.0;
		if (da > 0.0)
		{
			float diff = clamp((l_dist-proj_focus)/proj_range, 0.0, 1.0);
			float lod = diff * proj_lod;
			
			vec4 plcol = texture2DLod(projectionMap, proj_tc.xy, lod);
		
			vec3 lcol = gl_Color.rgb * plcol.rgb * plcol.a;
			
			lit = da * dist_atten * noise;
			
			col = lcol*lit*diff_tex*shadow;
		}
		
		float diff = clamp((proj_range-proj_focus)/proj_range, 0.0, 1.0);
		float lod = diff * proj_lod;
		vec4 amb_plcol = texture2DLod(projectionMap, proj_tc.xy, lod);
		//float amb_da = mix(proj_ambiance, proj_ambiance*max(-da, 0.0), max(da, 0.0));
		float amb_da = proj_ambiance;
		if (da > 0.0)
		{
			amb_da += (da*0.5)*(1.0-shadow)*proj_ambiance;
		}
		
		amb_da += (da*da*0.5+0.5)*proj_ambiance;
			
		amb_da *= dist_atten * noise;
		
		amb_da = min(amb_da, 1.0-lit);
		
		col += amb_da*gl_Color.rgb*diff_tex.rgb*amb_plcol.rgb*amb_plcol.a;
	}
	
	
	vec4 spec = texture2DRect(specularRect, frag.xy);
	if (spec.a > 0.0)
	{
		vec3 ref = reflect(normalize(pos), norm);
		
		//project from point pos in direction ref to plane proj_p, proj_n
		vec3 pdelta = proj_p-pos;
		float ds = dot(ref, proj_n);
		
		if (ds < 0.0)
		{
			vec3 pfinal = pos + ref * dot(pdelta, proj_n)/ds;
			
			vec3 stc = (proj_mat * vec4(pfinal.xyz, 1.0)).xyz;

			if (stc.z > 0.0)
			{
				stc.xy /= stc.z+proj_near;
					
				if (stc.x < 1.0 &&
					stc.y < 1.0 &&
					stc.x > 0.0 &&
					stc.y > 0.0)
				{
					vec4 scol = texture2DLod(projectionMap, stc.xy, proj_lod-spec.a*proj_lod);
					col += dist_atten*scol.rgb*gl_Color.rgb*scol.a*spec.rgb*shadow;
				}
			}
		}
	}
	
	gl_FragColor.rgb = col;	
	gl_FragColor.a = 0.0;
}
