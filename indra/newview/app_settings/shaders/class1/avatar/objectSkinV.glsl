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

/* BENTO JOINT COUNT LIMITS
 * Note that the value in these two lines also needs to be updated to value-1 several places below.
 */
uniform mat3 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT];
uniform vec3 translationPalette[MAX_JOINTS_PER_MESH_OBJECT];

mat4 getObjectSkinnedTransform()
{
	int i;
	
	vec4 w = fract(weight4);
	vec4 index = floor(weight4);
	
		 index = min(index, vec4(MAX_JOINTS_PER_MESH_OBJECT-1));
		 index = max(index, vec4( 0.0));

    w *= 1.0/(w.x+w.y+w.z+w.w);
	
	int i1 = int(index.x);
	int i2 = int(index.y);
	int i3 = int(index.z);
	int i4 = int(index.w);
		
	mat3 mat  = matrixPalette[i1]*w.x;
		 mat += matrixPalette[i2]*w.y;
		 mat += matrixPalette[i3]*w.z;
		 mat += matrixPalette[i4]*w.w;

	vec3 trans = translationPalette[i1]*w.x;
	trans += translationPalette[i2]*w.y;
	trans += translationPalette[i3]*w.z;
	trans += translationPalette[i4]*w.w;

	mat4 ret;

	ret[0] = vec4(mat[0], 0);
	ret[1] = vec4(mat[1], 0);
	ret[2] = vec4(mat[2], 0);
	ret[3] = vec4(trans, 1.0);
				
	return ret;

#ifdef IS_AMD_CARD
   // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
   mat3 dummy1 = matrixPalette[0];
   vec3 dummy2 = translationPalette[0];
   mat3 dummy3 = matrixPalette[MAX_JOINTS_PER_MESH_OBJECT-1];
   vec3 dummy4 = translationPalette[MAX_JOINTS_PER_MESH_OBJECT-1];
#endif

}

