/** 
 * @file attachmentShadowV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

mat4 getObjectSkinnedTransform();

void main()
{
	//transform vertex
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	gl_FrontColor = diffuse_color;
	
	vec4 p = gl_ProjectionMatrix * vec4(pos, 1.0);
	p.z = max(p.z, -p.w+0.01);
	gl_Position = p;
}
