/** 
 * @file impostorV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	gl_FrontColor = gl_Color;
}
