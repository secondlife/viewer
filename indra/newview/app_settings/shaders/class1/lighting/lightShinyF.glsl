/** 
 * @file lightShinyF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


uniform sampler2D diffuseMap;
uniform samplerCube environmentMap;

void shiny_lighting() 
{
	vec4 color = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragColor = color;
}

