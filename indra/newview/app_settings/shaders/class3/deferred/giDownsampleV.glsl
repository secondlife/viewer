/** 
 * @file postgiV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

varying vec2 vary_fragcoord;
uniform vec2 screen_res;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord = (pos.xy*0.5+0.5)*screen_res;
}
