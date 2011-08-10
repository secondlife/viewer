/**
 * @file fullbrightShinyV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;

varying float vary_texture_index;

attribute vec4 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

void main()
{
	//transform vertex
	vec4 vert = vec4(position.xyz,1.0);
	vary_texture_index = position.w;
	vec4 pos = (gl_ModelViewMatrix * vert);
	gl_Position = gl_ModelViewProjectionMatrix*vec4(position.xyz, 1.0);
	
	vec3 norm = normalize(gl_NormalMatrix * normal);
	vec3 ref = reflect(pos.xyz, -norm);

	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	gl_TexCoord[1] = gl_TextureMatrix[1]*vec4(ref,1.0);

	calcAtmospherics(pos.xyz);

	gl_FrontColor = diffuse_color;

	gl_FogFragCoord = pos.z;
}
