/** 
 * @file class1/environment/moonF.glsl
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2D tex0;

VARYING vec2 vary_texcoord0;

// See:
// AS  off: class1/environment/moonF.glsl
// ALM off: class1/windlight/moonF.glsl
// ALM on : class1/deferred/moonF.glsl
void main() 
{
    vec4 color = texture2D(tex0, vary_texcoord0.xy);

    // SL-14113 Don't write to depth; prevent moon's quad from hiding stars which should be visible
    // Moon texture has transparent pixels <0x55,0x55,0x55,0x00>
    if (color.a <= 2./255.) // 0.00784
        discard;

    frag_color = color;
    gl_FragDepth = LL_SHADER_CONST_MOON_DEPTH; // SL-14113 Moon is infront of stars
}
