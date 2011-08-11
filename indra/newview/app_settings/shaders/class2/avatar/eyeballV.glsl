/** 
 * @file eyeballV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec3 normal;
attribute vec2 texcoord0;

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

void main()
{
	//transform vertex
	vec3 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0)).xyz;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
		
	vec3 norm = normalize(gl_NormalMatrix * normal);

	calcAtmospherics(pos.xyz);
	
	// vec4 specular = specularColor;
	vec4 specular = vec4(1.0);
	vec4 color = calcLightingSpecular(pos, norm, diffuse_color, specular, vec4(0.0));
			
	gl_FrontColor = color;
	gl_FogFragCoord = pos.z;

}

