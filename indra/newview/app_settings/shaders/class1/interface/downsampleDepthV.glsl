/** 
 * @file debugV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;

uniform vec2 screen_res;

uniform vec2 delta;

VARYING vec2 tc0;
VARYING vec2 tc1;
VARYING vec2 tc2;
VARYING vec2 tc3;
VARYING vec2 tc4;
VARYING vec2 tc5;
VARYING vec2 tc6;
VARYING vec2 tc7;
VARYING vec2 tc8;

void main()
{
	gl_Position = vec4(position, 1.0); 
	
	vec2 tc = (position.xy*0.5+0.5)*screen_res;
	tc0 = tc+vec2(-delta.x,-delta.y);
	tc1 = tc+vec2(0,-delta.y);
	tc2 = tc+vec2(delta.x,-delta.y);
	tc3 = tc+vec2(-delta.x,0);
	tc4 = tc+vec2(0,0);
	tc5 = tc+vec2(delta.x,0);
	tc6 = tc+vec2(-delta.x,delta.y);
	tc7 = tc+vec2(0,delta.y);
	tc8 = tc+vec2(delta.x,delta.y);
}

