/** 
 * @file avatarF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2D diffuseMap;

varying vec3 vary_normal;
varying vec4 vary_position;

void main() 
{
	gl_FragData[0] = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragData[1] = vec4(0,0,0,0);
	gl_FragData[2] = vec4(normalize(vary_normal), 0.0);
	gl_FragData[3] = vary_position;
}

