/** 
 * @file pointLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

varying vec4 vary_light;
varying vec3 vary_fragcoord;

uniform vec2 screen_res;
uniform float near_clip;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 

	vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
		
	vec4 tex = gl_MultiTexCoord0;
	tex.w = 1.0;
	
	vary_light = gl_MultiTexCoord0;
		
	gl_FrontColor = gl_Color;
}
