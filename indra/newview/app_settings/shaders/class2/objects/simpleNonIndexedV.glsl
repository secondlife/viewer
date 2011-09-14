/** 
 * @file simpleNonIndexedV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


attribute vec3 position;
attribute vec2 texcoord0;
attribute vec3 normal;
attribute vec4 diffuse_color;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	vec4 vert = vec4(position.xyz,1.0);
	
	gl_Position = gl_ModelViewProjectionMatrix*vert;
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0, 0, 1);
	
	vec4 pos = (gl_ModelViewMatrix * vert);
	
	vec3 norm = normalize(gl_NormalMatrix * normal);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm, diffuse_color, vec4(0.));
	gl_FrontColor = color;

	gl_FogFragCoord = pos.z;
}
