/** 
 * @file pickAvatarF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2D diffuseMap;

void main() 
{
	gl_FragColor = vec4(gl_Color.rgb, texture2D(diffuseMap, gl_TexCoord[0].xy).a);
}
