/** 
 * @file diffuseF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2D diffuseMap;

varying vec3 vary_normal;

void main() 
{
	gl_FragColor = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragColor.rgb = vary_normal*0.5+0.5;
}
