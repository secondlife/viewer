/** 
 * @file impostorV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

varying vec4 vary_position;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vary_position = gl_ModelViewMatrix * gl_Vertex;

	gl_FrontColor = gl_Color;
}
