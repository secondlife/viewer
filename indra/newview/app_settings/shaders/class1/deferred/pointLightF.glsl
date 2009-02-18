/** 
 * @file pointLightF.glsl
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

varying vec4 vary_light;

varying vec3 vary_fragcoord;
uniform vec2 screen_res;

void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;	
	vec3 pos = texture2DRect(positionMap, frag).xyz;
	vec3 lv = vary_light.xyz-pos;
	float dist2 = dot(lv,lv);
	if (dist2 > vary_light.w)
	{
		discard;
	}
	
	vec3 norm = texture2DRect(normalMap, frag).xyz;
	float da = dot(norm, lv);
	if (da < 0.0)
	{
		discard;
	}
	
	norm = normalize(norm);
	lv = normalize(lv);
	da = dot(norm, lv);
	
	float noise = texture2D(noiseMap, frag/128.0).b;
	
	vec3 col = texture2DRect(diffuseRect, frag).rgb;
	float fa = gl_Color.a+1.0;
	float dist_atten = clamp(1.0-(dist2-vary_light.w*(1.0-fa))/(vary_light.w*fa), 0.0, 1.0);
	float lit = da * dist_atten * noise;
	
	col = gl_Color.rgb*lit*col;

	vec4 spec = texture2DRect(specularRect, frag);
	if (spec.a > 0.0)
	{
		vec3 ref = reflect(normalize(pos), norm);
		float sa = dot(ref,lv);
		sa = max(sa, 0.0);
		sa = pow(sa, 128.0 * spec.a*spec.a/dist_atten)*min(dist_atten*4.0, 1.0);
		sa *= noise;
		col += da*sa*gl_Color.rgb*spec.rgb;
	}
	
	//attenuate point light contribution by SSAO component
	col *= texture2DRect(lightMap, frag.xy).g;
	

	gl_FragColor.rgb = col;	
	gl_FragColor.a = 0.0;
}
