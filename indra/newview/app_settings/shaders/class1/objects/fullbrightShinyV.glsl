/**
 * @file fullbrightShinyV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec3 normal;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	
	vec3 norm = normalize(gl_NormalMatrix * normal);
	vec3 ref = reflect(pos.xyz, -norm);

	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	gl_TexCoord[1] = gl_TextureMatrix[1]*vec4(ref,1.0);

	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	calcAtmospherics(pos.xyz);

	gl_FrontColor = diffuse_color;

	gl_FogFragCoord = pos.z;
}
