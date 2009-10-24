/** 
 * @file blurLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2DRect edgeMap;

uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;

varying vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

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
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz*2.0-1.0;
	vec3 pos = getPosition(vary_fragcoord.xy).xyz;
	vec4 ccol = texture2DRect(lightMap, vary_fragcoord.xy).rgba;
	
	vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
	
	dlt /= max(-pos.z*dist_factor, 1.0);
	
	vec2 defined_weight = kern[0].xy; // special case the kern[0] (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
	vec4 col = defined_weight.xyxx * ccol;

	float center_e = 1.0 - (texture2DRect(edgeMap, vary_fragcoord.xy).a+
		      texture2DRect(edgeMap, vary_fragcoord.xy+dlt*0.333).a+
	              texture2DRect(edgeMap, vary_fragcoord.xy-dlt*0.333).a);
	
	float e = center_e;
	for (int i = 1; i < 4; i++)
	{
		vec2 tc = vary_fragcoord.xy + kern[i].z*dlt;
		
		e = max(e, 0.0);
		
		vec2 wght = kern[i].xy*e;
		
		col += texture2DRect(lightMap, tc)*wght.xyxx;
		defined_weight += wght;
		
		e *= e;
		e -= texture2DRect(edgeMap, tc.xy).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.333).a+
			texture2DRect(edgeMap, tc.xy-dlt*0.333).a;
	}

	e = center_e;
	for (int i = 1; i < 4; i++)
	{
		vec2 tc = vary_fragcoord.xy - kern[i].z*dlt;
		
		e = max(e, 0.0);
		
		vec2 wght = kern[i].xy*e;
		
		col += texture2DRect(lightMap, tc)*wght.xyxx;
		defined_weight += wght;
		
		e *= e;
		e -= texture2DRect(edgeMap, tc.xy).a+
			texture2DRect(edgeMap, tc.xy+dlt*0.333).a+
			texture2DRect(edgeMap, tc.xy-dlt*0.333).a;
	}


	col /= defined_weight.xyxx;
	
	gl_FragColor = col;
	
	//gl_FragColor = ccol;
}
