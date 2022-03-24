/** 
 * @file class3/deferred/gatherSkyShF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

VARYING vec2 vary_frag;

uniform vec2 screen_res;
uniform sampler2D sh_input_r;
uniform sampler2D sh_input_g;
uniform sampler2D sh_input_b;

void main()
{
    vec2 offset	  = vec2(2.0) / screen_res;

    vec4 r = vec4(0);
    vec4 g = vec4(0);
    vec4 b = vec4(0);

    vec2 tc = vary_frag * 2.0;

	r += texture2D(sh_input_r, tc + vec2(0,        0));
	r += texture2D(sh_input_r, tc + vec2(offset.x, 0));
	r += texture2D(sh_input_r, tc + vec2(0,        offset.y));
	r += texture2D(sh_input_r, tc + vec2(offset.x, offset.y));
    r /= 4.0f;

	g += texture2D(sh_input_g, tc + vec2(0,        0));
	g += texture2D(sh_input_g, tc + vec2(offset.x, 0));
	g += texture2D(sh_input_g, tc + vec2(0,        offset.y));
	g += texture2D(sh_input_g, tc + vec2(offset.x, offset.y));
    g /= 4.0f;

	b += texture2D(sh_input_b, tc + vec2(0,        0));
	b += texture2D(sh_input_b, tc + vec2(offset.x, 0));
	b += texture2D(sh_input_b, tc + vec2(0,        offset.y));
	b += texture2D(sh_input_b, tc + vec2(offset.x, offset.y));
    b /= 4.0f;

    frag_data[0] = r;
    frag_data[1] = g;
    frag_data[2] = b;
}
