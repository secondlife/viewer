/** 
 * @file giDownsampleF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2DRect giLightMap;

uniform vec2 kern[32];
uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform int kern_length;
uniform float kern_scale;
uniform vec3 blur_quad;

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

float getDepth(vec2 pos_screen)
{
	float z = texture2DRect(depthMap, pos_screen.xy).a;
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

void main() 
{
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	float depth = getDepth(vary_fragcoord.xy);
		
	vec3 ccol = texture2DRect(giLightMap, vary_fragcoord.xy).rgb;
	vec2 dlt = kern_scale * delta/(vec2(1.0,1.0)+norm.xy*norm.xy);
	dlt /= clamp(-depth*blur_quad.x, 1.0, 3.0);
	float defined_weight = kern[0].x;
	vec3 col = ccol*kern[0].x;
	
	for (int i = 0; i < kern_length; i++)
	{
		vec2 tc = vary_fragcoord.xy + kern[i].y*dlt;
		vec3 sampNorm = texture2DRect(normalMap, tc.xy).xyz;
		sampNorm = vec3((sampNorm.xy-0.5)*2.0,sampNorm.z); // unpack norm
		
		float d = dot(norm.xyz, sampNorm);
		
		if (d > 0.5)
		{
			float sampdepth = getDepth(tc.xy);
			sampdepth -= depth;
			if (sampdepth*sampdepth < blur_quad.z)
			{
	    		col += texture2DRect(giLightMap, tc).rgb*kern[i].x;
				defined_weight += kern[i].x;
			}
		}
	}

	col /= defined_weight;
	
	//col = ccol;
	
	col = col*blur_quad.y;
	
	gl_FragData[0].xyz = col;
	
	//gl_FragColor = ccol;
}
