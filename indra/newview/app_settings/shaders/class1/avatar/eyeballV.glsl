/** 
 * @file eyeballV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
		
	calcAtmospherics(pos.xyz);

	vec4 specular = vec4(1.0);
	vec4 color = calcLightingSpecular(pos, norm, gl_Color, specular, vec4(0.0));	
	gl_FrontColor = color;

}

