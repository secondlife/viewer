/** 
 * @file shinyV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;

void main()
{
	//transform vertex
	gl_Position = ftransform(); //gl_ModelViewProjectionMatrix * gl_Vertex;
	
	vec4 pos = (gl_ModelViewMatrix * gl_Vertex);
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);

	calcAtmospherics(pos.xyz);
	
	gl_FrontColor = gl_Color;
	
	vec3 ref = reflect(pos.xyz, -norm);
	
	gl_TexCoord[0] = gl_TextureMatrix[0]*vec4(ref,1.0);
	
	gl_FogFragCoord = pos.z;
}

