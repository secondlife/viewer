/** 
 * @file shinyV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;

void main()
{
	//transform vertex
	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	
	vec3 norm = normalize(gl_NormalMatrix * normal);

	calcAtmospherics(pos.xyz);
	
	gl_FrontColor = diffuse_color;
	
	vec3 ref = reflect(pos.xyz, -norm);
	
	gl_TexCoord[0] = gl_TextureMatrix[0]*vec4(ref,1.0);
	
	gl_FogFragCoord = pos.z;
}

