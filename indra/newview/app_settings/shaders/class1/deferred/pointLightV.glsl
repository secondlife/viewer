/** 
 * @file pointLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

varying vec4 vary_light;
varying vec4 vary_fragcoord;

uniform vec2 screen_res;
uniform float near_clip;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 

	vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord = pos;
		
	vec4 tex = gl_MultiTexCoord0;
	tex.w = 1.0;
	
	vary_light = gl_MultiTexCoord0;
		
	gl_FrontColor = gl_Color;
}
