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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_data[4];

uniform float minimum_alpha;


uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

in vec2 vary_texcoord0;

vec3 linear_to_srgb(vec3 c);

vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

void main()
{
    vec4 col = texture(diffuseMap, vary_texcoord0.xy);

    if (col.a < minimum_alpha)
    {
        discard;
    }

    vec4 norm = texture(normalMap,   vary_texcoord0.xy);
    vec4 spec = texture(specularMap, vary_texcoord0.xy);

    frag_data[0] = vec4(col.rgb, 0.0);
    frag_data[1] = spec;
    frag_data[2] = vec4(norm.xyz, GBUFFER_FLAG_HAS_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(0);
#endif
}
