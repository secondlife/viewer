/** 
 * @file avatarSkinV.glsl
 *
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
 

ATTRIBUTE vec4 weight;

uniform vec4 matrixPalette[45];

mat4 getSkinnedTransform()
{
	mat4 ret;
	int i = int(floor(weight.x));
	float x = fract(weight.x);
		
	ret[0] = mix(matrixPalette[i+0], matrixPalette[i+1], x);
	ret[1] = mix(matrixPalette[i+15],matrixPalette[i+16], x);
	ret[2] = mix(matrixPalette[i+30],matrixPalette[i+31], x);
	ret[3] = vec4(0,0,0,1);

	return ret;
}
