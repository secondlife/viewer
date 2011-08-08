/** 
 * @file bumpV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec3 normal;
attribute vec2 texcoord0;
attribute vec2 texcoord2;

varying vec3 vary_mat0;
varying vec3 vary_mat1;
varying vec3 vary_mat2;

mat4 getObjectSkinnedTransform();

void main()
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	
	vec3 n = normalize((mat * vec4(normal.xyz+position.xyz, 1.0)).xyz-pos.xyz);
	vec3 b = normalize((mat * vec4(vec4(texcoord2,0,1).xyz+position.xyz, 1.0)).xyz-pos.xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
	
	gl_Position = gl_ProjectionMatrix*vec4(pos, 1.0);
	gl_FrontColor = diffuse_color;
}
