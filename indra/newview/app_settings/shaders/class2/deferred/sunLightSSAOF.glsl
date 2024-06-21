/**
 * @file class2/deferred/sunLightSSAOF.glsl
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

//class 2 -- shadows and SSAO

// Inputs
in vec2 vary_fragcoord;

vec4 getPosition(vec2 pos_screen);
vec4 getNorm(vec2 pos_screen);

float sampleDirectionalShadow(vec3 shadow_pos, vec3 norm, vec2 pos_screen);
float sampleSpotShadow(vec3 shadow_pos, vec3 norm, int index, vec2 pos_screen);
float calcAmbientOcclusion(vec4 pos, vec3 norm, vec2 pos_screen);

void main()
{
    vec2 pos_screen = vary_fragcoord.xy;
    vec4 pos  = getPosition(pos_screen);
    vec4 norm = getNorm(pos_screen);

    vec4 col;
    col.r = sampleDirectionalShadow(pos.xyz, norm.xyz, pos_screen);
    col.g = calcAmbientOcclusion(pos, norm.xyz, pos_screen);
    col.b = sampleSpotShadow(pos.xyz, norm.xyz, 0, pos_screen);
    col.a = sampleSpotShadow(pos.xyz, norm.xyz, 1, pos_screen);

    frag_color = clamp(col, vec4(0), vec4(1));
}
