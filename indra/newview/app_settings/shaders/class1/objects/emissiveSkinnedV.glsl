/** 
 * @file emissiveSkinnedV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


attribute vec3 position;
attribute float emissive;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);
mat4 getObjectSkinnedTransform();

void main()
{
	//transform vertex
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	calcAtmospherics(pos.xyz);

	gl_FrontColor = vec4(0,0,0,emissive);
	
	gl_Position = gl_ProjectionMatrix*vec4(pos, 1.0);
		
	gl_FogFragCoord = pos.z;
}
