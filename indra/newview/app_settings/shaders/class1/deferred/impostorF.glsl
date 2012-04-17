/** 
 * @file impostorF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform float minimum_alpha;


uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

VARYING vec2 vary_texcoord0;

void main() 
{
	vec4 col = texture2D(diffuseMap, vary_texcoord0.xy);

	if (col.a < minimum_alpha)
	{
		discard;
	}

	frag_data[0] = vec4(col.rgb, col.a * 0.005);
	frag_data[1] = texture2D(specularMap, vary_texcoord0.xy);
	frag_data[2] = vec4(texture2D(normalMap, vary_texcoord0.xy).xyz, 0.0);
}
