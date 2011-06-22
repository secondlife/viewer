/** 
 * @file attachmentShadowV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */



mat4 getObjectSkinnedTransform();

void main()
{
	//transform vertex
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	mat4 mat = getObjectSkinnedTransform();
	
	mat = gl_ModelViewMatrix * mat;
	vec3 pos = (mat*gl_Vertex).xyz;
	
	gl_FrontColor = gl_Color;
	
	vec4 p = gl_ProjectionMatrix * vec4(pos, 1.0);
	p.z = max(p.z, -p.w+0.01);
	gl_Position = p;
}
