/** 
 * @file sunDiscF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

vec3 srgb_to_linear(vec3 c);
vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

uniform sampler2D diffuseMap;
uniform sampler2D altDiffuseMap;
uniform float blend_factor; // interp factor between sunDisc A/B
VARYING vec2 vary_texcoord0;
VARYING float sun_fade;

void main() 
{
    vec4 sunDiscA = texture2D(diffuseMap, vary_texcoord0.xy);
    vec4 sunDiscB = texture2D(altDiffuseMap, vary_texcoord0.xy);
    vec4 c     = mix(sunDiscA, sunDiscB, blend_factor);

    c.rgb = srgb_to_linear(c.rgb);
    c.rgb = clamp(c.rgb, vec3(0), vec3(1));
    c.rgb = pow(c.rgb, vec3(0.7f));

    //c.rgb = fullbrightAtmosTransport(c.rgb);
    c.rgb = fullbrightScaleSoftClip(c.rgb);

    // SL-9806 stars poke through
    //c.a *= sun_fade;

    frag_data[0] = c;
    frag_data[1] = vec4(0.0f);
    frag_data[2] = vec4(0.0, 1.0, 0.0, 1.0);

    gl_FragDepth = 0.999988f;
}

