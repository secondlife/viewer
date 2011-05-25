/** 
 * @file starsV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120


void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
