/** 
 * @file simpleV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
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
	
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm, gl_Color, vec4(0.));
	gl_FrontColor = color;

	gl_FogFragCoord = pos.z;
}
