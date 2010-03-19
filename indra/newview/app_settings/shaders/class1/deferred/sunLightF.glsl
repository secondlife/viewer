/** 
 * @file sunLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//class 1, no shadow, no SSAO, should never be called

#extension GL_ARB_texture_rectangle : enable

void main() 
{
	gl_FragColor = vec4(0,0,0,0);
}
