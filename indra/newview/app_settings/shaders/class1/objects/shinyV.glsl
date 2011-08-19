/** 
 * @file shinyV.glsl
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
 
attribute vec3 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

void calcAtmospherics(vec3 inPositionEye);

uniform vec4 origin;

void main()
{
	//transform vertex
	vec4 pos = (gl_ModelViewMatrix * vec4(position.xyz, 1.0));
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	
	vec3 norm = normalize(gl_NormalMatrix * normal);

	calcAtmospherics(pos.xyz);
	
	gl_FrontColor = diffuse_color;
	
	vec3 ref = reflect(pos.xyz, -norm);
	
	gl_TexCoord[0] = gl_TextureMatrix[0]*vec4(ref,1.0);
	
	gl_FogFragCoord = pos.z;
}

