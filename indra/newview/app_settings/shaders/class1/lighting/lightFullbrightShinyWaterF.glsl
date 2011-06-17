/** 
 * @file lightFullbrightShinyWaterF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

 


uniform sampler2D diffuseMap;
uniform samplerCube environmentMap;

void fullbright_shiny_lighting_water() 
{
	gl_FragColor = texture2D(diffuseMap, gl_TexCoord[0].xy);
}
