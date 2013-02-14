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
	
	float w0 = fract(weight4.x);
	float w1 = fract(weight4.y);
	float w2 = fract(weight4.z);
	float w3 = fract(weight4.w);
			
	int i0 = int(floor(weight4.x));
	int i1 = int(floor(weight4.y));
	int i2 = int(floor(weight4.z));
	int i3 = int(floor(weight4.w));

	//float scale = 1.0/(w.x+w.y+w.z+w.w);
	//w *= scale;
	
	mat4 mat = matrixPalette[i0]*w0;
	mat += matrixPalette[i1]*w1;
	mat += matrixPalette[i2]*w2;
	mat += matrixPalette[i3]*w3;
		
	return mat;
}
