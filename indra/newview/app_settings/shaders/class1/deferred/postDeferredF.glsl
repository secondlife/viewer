/** 
 * @file postDeferredF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect localLightMap;
uniform sampler2DRect sunLightMap;
uniform sampler2DRect giLightMap;
uniform sampler2D	  luminanceMap;
uniform sampler2DRect lightMap;

uniform vec3 lum_quad;
uniform float lum_lod;
uniform vec4 ambient;

uniform vec3 gi_quad;

uniform vec2 screen_res;
varying vec2 vary_fragcoord;

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	vec3 lum = texture2DLod(luminanceMap, tc/screen_res, lum_lod).rgb;
	float luminance = lum.r;
	luminance = luminance*lum_quad.y+lum_quad.z;

	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);

	float ambocc = texture2DRect(lightMap, vary_fragcoord.xy).g;
			
	vec3 gi_col = texture2DRect(giLightMap, vary_fragcoord.xy).rgb;
	gi_col = gi_col*gi_col*gi_quad.x + gi_col*gi_quad.y+gi_quad.z*ambocc*ambient.rgb;
	gi_col *= diff;
	
	vec4 sun_col =	texture2DRect(sunLightMap, vary_fragcoord.xy);
	
	vec3 local_col = texture2DRect(localLightMap, vary_fragcoord.xy).rgb;
		
		
	sun_col *= 1.0/min(luminance, 1.0);
	gi_col *= 1.0/luminance;
		
	vec3 col = sun_col.rgb+gi_col+local_col;
	
	gl_FragColor.rgb = col.rgb;
	col.rgb = max(col.rgb-vec3(1.0,1.0,1.0), vec3(0.0, 0.0, 0.0)); 
	
	gl_FragColor.a = 0.0; // max(dot(col.rgb,col.rgb)*lum_quad.x, sun_col.a);
	
	//gl_FragColor.rgb = vec3(lum_lod);
}
