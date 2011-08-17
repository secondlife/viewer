/** 
 * @file highlightV.glsl
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

