/** 
 * @file highlightV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

void main()
{
	//transform vertex
	gl_Position = ftransform();
	vec3 pos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	pos = normalize(pos);
	float d = dot(pos, normalize(gl_NormalMatrix * gl_Normal));
	d *= d;
	d = 1.0 - d;
	d *= d;
		
	d = min(d, gl_Color.a*2.0);
			
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor.rgb = gl_Color.rgb;
	gl_FrontColor.a = max(d, gl_Color.a);
}

