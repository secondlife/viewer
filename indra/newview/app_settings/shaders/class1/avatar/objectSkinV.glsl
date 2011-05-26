/** 
 * @file objectSkinV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#version 120

attribute vec4 object_weight;  

uniform mat4 matrixPalette[32];

mat4 getObjectSkinnedTransform()
{
	int i; 
	
	vec4 w = fract(object_weight);
	vec4 index = floor(object_weight);
	
	float scale = 1.0/(w.x+w.y+w.z+w.w);
	w *= scale;
	
	mat4 mat = matrixPalette[int(index.x)]*w.x;
	mat += matrixPalette[int(index.y)]*w.y;
	mat += matrixPalette[int(index.z)]*w.z;
	mat += matrixPalette[int(index.w)]*w.w;
		
	return mat;
}
