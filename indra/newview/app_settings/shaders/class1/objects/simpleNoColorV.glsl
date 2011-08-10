/** 
 * @file simpleNoColorV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec3 normal;
attribute vec2 texcoord0;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
		
	vec3 norm = normalize(gl_NormalMatrix * normal);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm, vec4(1,1,1,1), vec4(0.));
	gl_FrontColor = color;

	gl_FogFragCoord = pos.z;
}
