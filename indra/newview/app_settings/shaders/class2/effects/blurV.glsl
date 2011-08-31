/**
 * @file blurV.glsl
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
 


uniform vec2 texelSize;
uniform vec2 blurDirection;
uniform float blurWidth;

void main(void)
{
	// Transform vertex
	gl_Position = ftransform();
	
	vec2 blurDelta = texelSize * blurDirection * vec2(blurWidth, blurWidth);
	vec2 s = gl_MultiTexCoord0.st - (blurDelta * 3.0);
	
	// for (int i = 0; i < 7; i++) {
		// gl_TexCoord[i].st = s + (i * blurDelta);
	// }

	// MANUALLY UNROLL
	gl_TexCoord[0].st = s;
	gl_TexCoord[1].st = s + blurDelta;
	gl_TexCoord[2].st = s + (2. * blurDelta);
	gl_TexCoord[3].st = s + (3. * blurDelta);
	gl_TexCoord[4].st = s + (4. * blurDelta);
	gl_TexCoord[5].st = s + (5. * blurDelta);
	gl_TexCoord[6].st = s + (6. * blurDelta);

	// gl_TexCoord[0].st = s;
	// gl_TexCoord[1].st = blurDelta;
}
