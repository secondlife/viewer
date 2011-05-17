/**
 * @file fullbrightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec4 pos = (gl_ModelViewMatrix * gl_Vertex);

	calcAtmospherics(pos.xyz);

	gl_FrontColor = gl_Color;

	gl_FogFragCoord = pos.z;
}
