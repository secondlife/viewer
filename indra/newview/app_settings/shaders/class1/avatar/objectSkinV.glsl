/** 
 * @file objectSkinV.glsl
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */



ATTRIBUTE vec4 weight4;  

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
