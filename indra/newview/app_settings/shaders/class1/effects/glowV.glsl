/** 
 * @file glowV.glsl
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
 


uniform vec2 glowDelta;

void main() 
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy + glowDelta*(-3.5);
	gl_TexCoord[1].xy = gl_MultiTexCoord0.xy + glowDelta*(-2.5);
	gl_TexCoord[2].xy = gl_MultiTexCoord0.xy + glowDelta*(-1.5);
	gl_TexCoord[3].xy = gl_MultiTexCoord0.xy + glowDelta*(-0.5);
	gl_TexCoord[0].zw = gl_MultiTexCoord0.xy + glowDelta*(0.5);
	gl_TexCoord[1].zw = gl_MultiTexCoord0.xy + glowDelta*(1.5);
	gl_TexCoord[2].zw = gl_MultiTexCoord0.xy + glowDelta*(2.5);
	gl_TexCoord[3].zw = gl_MultiTexCoord0.xy + glowDelta*(3.5);
}
