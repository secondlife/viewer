/** 
 * @file impostorF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

void main() 
{
	gl_FragData[0] = texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragData[1] = texture2D(specularMap, gl_TexCoord[0].xy);
	gl_FragData[2] = vec4(texture2D(normalMap, gl_TexCoord[0].xy).xyz, 0.0);
}
