/** 
 * @file twotexturecompareF.glsl
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

out vec4 frag_color;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D dither_tex;
uniform float dither_scale;
uniform float dither_scale_s;
uniform float dither_scale_t;

in vec2 vary_texcoord0;
in vec2 vary_texcoord1;

void main() 
{
	frag_color = abs(texture(tex0, vary_texcoord0.xy) - texture(tex1, vary_texcoord0.xy));

	vec2 dither_coord;
	dither_coord[0] = vary_texcoord0[0] * dither_scale_s;
	dither_coord[1] = vary_texcoord0[1] * dither_scale_t;
	vec4 dither_vec = texture(dither_tex, dither_coord.xy);

	for(int i = 0; i < 3; i++)
	{
		if(frag_color[i] < dither_vec[i] * dither_scale)
		{
			frag_color[i] = 0.f;
		}
	}
}
