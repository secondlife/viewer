/** 
 * @file edgeF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;

uniform float gi_dist_cutoff;

varying vec2 vary_fragcoord;

uniform float depth_cutoff;
uniform float norm_cutoff;

uniform mat4 inv_proj;
uniform vec2 screen_res;

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
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz*2.0-1.0;
	float depth = getDepth(vary_fragcoord.xy);
	
	vec2 tc = vary_fragcoord.xy;
	
	float sc = 0.75;
	
	vec2 de;
	de.x = (depth-getDepth(tc+vec2(sc, sc))) + (depth-getDepth(tc+vec2(-sc, -sc)));
	de.y = (depth-getDepth(tc+vec2(-sc, sc))) + (depth-getDepth(tc+vec2(sc, -sc)));
	de /= depth;
	de *= de;
	de = step(depth_cutoff, de);
	
	vec2 ne;
	ne.x = dot(texture2DRect(normalMap, tc+vec2(-sc,-sc)).rgb*2.0-1.0, norm);
	ne.y = dot(texture2DRect(normalMap, tc+vec2(sc,sc)).rgb*2.0-1.0, norm);
	
	ne = 1.0-ne;
	
	ne = step(norm_cutoff, ne);
	
	gl_FragColor.a = dot(de,de)+dot(ne,ne);
	//gl_FragColor.a = dot(de,de);
}
