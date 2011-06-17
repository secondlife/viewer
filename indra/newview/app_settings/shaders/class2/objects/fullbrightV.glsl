/**
 * @file fullbrightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


void calcAtmospherics(vec3 inPositionEye);

varying float vary_texture_index;

void main()
{
	//transform vertex
	vec4 vert = vec4(gl_Vertex.xyz,1.0);
	vary_texture_index = gl_Vertex.w;
	gl_Position = gl_ModelViewProjectionMatrix*vert;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec4 pos = (gl_ModelViewMatrix * vert);

	calcAtmospherics(pos.xyz);

	gl_FrontColor = gl_Color;

	gl_FogFragCoord = pos.z;
}
