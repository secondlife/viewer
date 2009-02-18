/** 
 * @file blurLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect positionMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;

uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[32];
uniform int kern_length;
uniform float kern_scale;

varying vec2 vary_fragcoord;

void main() 
{
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	vec3 pos = texture2DRect(positionMap, vary_fragcoord.xy).xyz;
	vec2 ccol = texture2DRect(lightMap, vary_fragcoord.xy).rg;
	
	vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
	
	vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
	vec2 col = defined_weight * ccol;
	
	for (int i = 1; i < kern_length; i++)
	{
		vec2 tc = vary_fragcoord.xy + kern[i].z*dlt;
	        vec3 samppos = texture2DRect(positionMap, tc).xyz;
		float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
		if (d*d <= 0.003)
		{
			col += texture2DRect(lightMap, tc).rg*kern[i].xy;
			defined_weight += kern[i].xy;
		}
	}

	col /= defined_weight;

	gl_FragColor = vec4(col.r, col.g, 0.0, 1.0);
}
