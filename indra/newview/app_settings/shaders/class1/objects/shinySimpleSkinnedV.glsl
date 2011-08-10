/** 
 * @file shinySimpleSkinnedV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

attribute vec3 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);
mat4 getObjectSkinnedTransform();

void main()
{
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	vec4 norm = vec4(position.xyz, 1.0);
	norm.xyz += normal.xyz;
	norm.xyz = (mat*norm).xyz;
	norm.xyz = normalize(norm.xyz-pos.xyz);
		
	vec3 ref = reflect(pos.xyz, -norm.xyz);

	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	gl_TexCoord[1] = gl_TextureMatrix[1]*vec4(ref,1.0);

	calcAtmospherics(pos.xyz);

	vec4 color = calcLighting(pos.xyz, norm.xyz, diffuse_color, vec4(0.));
	gl_FrontColor = color;
	
	gl_Position = gl_ProjectionMatrix*vec4(pos, 1.0);
}
