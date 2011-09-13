/**
 * @file fullbrightNoColorV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
  
attribute vec3 position;
attribute vec2 texcoord0;
attribute vec3 normal;

void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	vec4 vert = vec4(position.xyz,1.0);
	vec4 pos = (gl_ModelViewMatrix * vert);
	gl_Position = gl_ModelViewProjectionMatrix*vec4(position.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);

	calcAtmospherics(pos.xyz);

	gl_FrontColor = vec4(1,1,1,1);

	gl_FogFragCoord = pos.z;
}
