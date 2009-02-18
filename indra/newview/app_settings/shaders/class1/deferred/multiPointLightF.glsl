/** 
 * @file multiPointLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect positionMap;
uniform sampler2DRect normalMap;
uniform samplerCube environmentMap;
uniform sampler2DRect lightMap;
uniform sampler2D noiseMap;

uniform vec3 env_mat[3];
uniform float sun_wash;

uniform int light_count;

uniform vec4 light[16];
uniform vec4 light_col[16];

varying vec3 vary_fragcoord;
uniform vec2 screen_res;

void main() 
{
	vec2 frag = (vary_fragcoord.xy*0.5+0.5)*screen_res;
	vec3 pos = texture2DRect(positionMap, frag.xy).xyz;
	vec3 norm = normalize(texture2DRect(normalMap, frag.xy).xyz);
	vec4 spec = texture2DRect(specularRect, frag.xy);
	vec3 diff = texture2DRect(diffuseRect, frag.xy).rgb;
	float noise = texture2D(noiseMap, frag.xy/128.0).b;
	vec3 out_col = vec3(0,0,0);
	
	for (int i = 0; i < light_count; ++i)
	{
		vec3 lv = light[i].xyz-pos;
		float dist2 = dot(lv,lv);
		if (dist2 > light[i].w)
		{
			continue;
		}
		
		float da = dot(norm, lv);
		if (da < 0.0)
		{
			continue;
		}
				
		lv = normalize(lv);
		da = dot(norm, lv);
				
		float fa = light_col[i].a+1.0;
		float dist_atten = clamp(1.0-(dist2-light[i].w*(1.0-fa))/(light[i].w*fa), 0.0, 1.0);
		dist_atten *= noise;

		float lit = da * dist_atten;
		
		vec3 col = light_col[i].rgb*lit*diff;
		
		if (spec.a > 0.0)
		{
			vec3 ref = reflect(normalize(pos), norm);
			float sa = dot(ref,lv);
			sa = max(sa, 0.0);
			sa = pow(sa, 128.0 * spec.a*spec.a/dist_atten)*min(dist_atten*4.0, 1.0);
			sa *= noise;
			col += da*sa*light_col[i].rgb*spec.rgb;
		}
		
		out_col += col;	
	}
	
	//attenuate point light contribution by SSAO component
	out_col *= texture2DRect(lightMap, frag.xy).g;
	
	gl_FragColor.rgb = out_col;
	gl_FragColor.a = 0.0;
}
