/** 
 * @file avatarShadowV.glsl
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
 


mat4 getSkinnedTransform();

attribute vec4 weight;

varying vec4 post_pos;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
				
	vec4 pos;
	vec3 norm;
	
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], gl_Vertex);
	pos.y = dot(trans[1], gl_Vertex);
	pos.z = dot(trans[2], gl_Vertex);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, gl_Normal);
	norm.y = dot(trans[1].xyz, gl_Normal);
	norm.z = dot(trans[2].xyz, gl_Normal);
	norm = normalize(norm);
	
	pos = gl_ProjectionMatrix * pos;
	post_pos = pos;

	gl_Position = vec4(pos.x, pos.y, pos.w*0.5, pos.w);
	
	gl_FrontColor = gl_Color;
}


