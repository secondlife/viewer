/**
 * @file diffuseAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_data[4];

uniform float minimum_alpha;

uniform sampler2D diffuseMap;

in vec3 vary_position;

in vec3 vary_normal;
in vec4 vertex_color;
in vec2 vary_texcoord0;

void mirrorClip(vec3 pos);

vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

void main()
{
    mirrorClip(vary_position);

    vec4 col = texture(diffuseMap, vary_texcoord0.xy) * vertex_color;

    if (col.a < minimum_alpha)
    {
        discard;
    }

    frag_data[0] = vec4(col.rgb, 0.0);
    frag_data[1] = vec4(0,0,0,0); // spec
    vec3 nvn = normalize(vary_normal);
    frag_data[2] = encodeNormal(nvn.xyz, 0, GBUFFER_FLAG_HAS_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(0);
#endif
}

