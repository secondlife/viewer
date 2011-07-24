/** 
 * @file bumpV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
	gl_FrontColor = gl_Color;
}
