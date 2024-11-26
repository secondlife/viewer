/**
 * @file starsF.glsl
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

in vec4 vertex_color;
in vec2 vary_texcoord0;
in vec2 screenpos;

uniform sampler2D diffuseMap;
uniform float blend_factor;
uniform float custom_alpha;
uniform float time;

float twinkle(){
    float d = fract(screenpos.x + screenpos.y);
    return abs(d);
}

// See:
// ALM off: class1/environment/starsF.glsl
// ALM on : class1/deferred/starsF.glsl
void main()
{
    // camera above water: class1\deferred\starsF.glsl
    // camera below water: class1\environment\starsF.glsl
    vec4 col_a = texture(diffuseMap, vary_texcoord0.xy);
    vec4 col_b = texture(diffuseMap, vary_texcoord0.xy);
    vec4 col = mix(col_b, col_a, blend_factor);
    col.rgb *= vertex_color.rgb;

    float factor = smoothstep(0.0f, 0.9f, custom_alpha);

    col.a = (col.a * factor) * 32.0f;
    col.a *= twinkle();

    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = col;
#else
    frag_data[0] = col;
#endif
}

