/** 
 * @file sunLightSSAOF.glsl
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
 

#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

//class 1 -- no shadow, SSAO only

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2D noiseMap;


// Inputs
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec3 decode_normal (vec2 enc);
vec4 getPosition(vec2 pos_screen);
vec3 getNorm(vec2 pos_screen);
float calcAmbientOcclusion(vec4 pos, vec3 norm, vec2 pos_screen);

void main() 
{
	vec2 pos_screen = vary_fragcoord.xy;
	
	//try doing an unproject here
	
	vec4 pos = getPosition(pos_screen);	
	vec3 norm = getNorm(pos_screen);
		
	frag_color.r = 1.0;
	frag_color.g = calcAmbientOcclusion(pos, norm, pos_screen);
	frag_color.b = 1.0; 
	frag_color.a = 1.0;
}
