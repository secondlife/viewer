/** 
 * @file luminanceF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect lightMap;
uniform sampler2DRect diffuseRect;

varying vec2 vary_fragcoord;
void main() 
{
	float i = texture2DRect(lightMap, vary_fragcoord.xy).r;
	gl_FragColor.rgb = vec3(i);
	gl_FragColor.a = 1.0;
}
