/** 
 * @file bumpV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */



varying vec3 vary_mat0;
varying vec3 vary_mat1;
varying vec3 vary_mat2;

mat4 getObjectSkinnedTransform();

void main()
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	
	vec3 pos = (mat*gl_Vertex).xyz;
	
	
	vec3 n = normalize((mat * vec4(gl_Normal.xyz+gl_Vertex.xyz, 1.0)).xyz-pos.xyz);
	vec3 b = normalize((mat * vec4(gl_MultiTexCoord2.xyz+gl_Vertex.xyz, 1.0)).xyz-pos.xyz);
	vec3 t = cross(b, n);
	
	vary_mat0 = vec3(t.x, b.x, n.x);
	vary_mat1 = vec3(t.y, b.y, n.y);
	vary_mat2 = vec3(t.z, b.z, n.z);
	
	gl_Position = gl_ProjectionMatrix*vec4(pos, 1.0);
	gl_FrontColor = gl_Color;
}
