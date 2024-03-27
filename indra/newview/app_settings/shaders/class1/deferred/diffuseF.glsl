/** 
 * @file diffuseF.glsl
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

/*[EXTRA_CODE_HERE]*/
 
out vec4 frag_data[4];

uniform sampler2D diffuseMap;

in vec3 vary_normal;
in vec4 vertex_color;
in vec2 vary_texcoord0;

vec2 encode_normal(vec3 n);

void main() 
{
	vec3 col = vertex_color.rgb * texture(diffuseMap, vary_texcoord0.xy).rgb;
	frag_data[0] = vec4(col, 0.0);
	frag_data[1] = vertex_color.aaaa; // spec
	//frag_data[1] = vec4(vec3(vertex_color.a), vertex_color.a+(1.0-vertex_color.a)*vertex_color.a); // spec - from former class3 - maybe better, but not so well tested
	vec3 nvn = normalize(vary_normal);
	frag_data[2] = vec4(encode_normal(nvn.xyz), vertex_color.a, GBUFFER_FLAG_HAS_ATMOS);
    frag_data[3] = vec4(0);
}

