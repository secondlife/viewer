/** 
 * @file shadowV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix*gl_Vertex;
	//smash geometry against the near clip plane (great for ortho projections)
	pos.z = max(pos.z, -1.0);
	gl_Position = pos;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
