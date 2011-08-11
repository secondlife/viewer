/**
 * @file fullbrightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	


	calcAtmospherics(pos.xyz);

	gl_FrontColor = diffuse_color;

	gl_FogFragCoord = pos.z;
}
