/** 
 * @file diffuseSkinnedV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */



attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec3 normal;
attribute vec2 texcoord0;

varying vec3 vary_normal;

mat4 getObjectSkinnedTransform();

void main()
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	vec4 norm = vec4(position.xyz, 1.0);
	norm.xyz += normal.xyz;
	norm.xyz = (mat*norm).xyz;
	norm.xyz = normalize(norm.xyz-pos.xyz);

	vary_normal = norm.xyz;
			
	gl_FrontColor = diffuse_color;
	
	gl_Position = gl_ProjectionMatrix*vec4(pos, 1.0);
}
