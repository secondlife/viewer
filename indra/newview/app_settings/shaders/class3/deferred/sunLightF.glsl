/** 
 * @file class3\deferred\sunLightF.glsl
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

#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

//class 2, shadows, no SSAO

// Inputs
VARYING vec2 vary_fragcoord;

vec4 getPosition(vec2 pos_screen);
vec3 getNorm(vec2 pos_screen);

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen); 
float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen); 

void main() 
{
	vec2 pos_screen = vary_fragcoord.xy;
	vec4 pos = getPosition(pos_screen);
	vec3 norm = getNorm(pos_screen);

	frag_color.r = sampleDirectionalShadow(pos.xyz, norm, pos_screen);
	frag_color.g = 1.0f;
    frag_color.b = sampleSpotShadow(pos.xyz, norm, 0, pos_screen);
    frag_color.a = sampleSpotShadow(pos.xyz, norm, 1, pos_screen);
}
