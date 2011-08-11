/** 
 * @file objectSkinV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */



attribute vec4 weight4;  

uniform mat4 matrixPalette[32];

mat4 getObjectSkinnedTransform()
{
	int i; 
	
	vec4 w = fract(weight4);
	vec4 index = floor(weight4);
	
	float scale = 1.0/(w.x+w.y+w.z+w.w);
	w *= scale;
	
	mat4 mat = matrixPalette[int(index.x)]*w.x;
	mat += matrixPalette[int(index.y)]*w.y;
	mat += matrixPalette[int(index.z)]*w.z;
	mat += matrixPalette[int(index.w)]*w.w;
		
	return mat;
}
