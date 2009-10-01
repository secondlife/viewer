/** 
 * @file luminanceF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2DRect diffuseMap;

varying vec2 vary_fragcoord;

void main() 
{
	gl_FragColor = texture2DRect(diffuseMap, vary_fragcoord.xy);
}
