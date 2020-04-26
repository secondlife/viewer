/** 
 * @file class3/deferred/shVisF.glsl
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

#ifdef DEFINE_GL_FRAGCOLOR
    out vec4 frag_color;
#else
    #define frag_color gl_FragColor
#endif

/////////////////////////////////////////////////////////////////////////
// Fragment shader for L1 SH debug rendering
/////////////////////////////////////////////////////////////////////////

uniform sampler2D sh_input_r;
uniform sampler2D sh_input_g;
uniform sampler2D sh_input_b;

uniform mat3 inv_modelviewprojection;

VARYING vec4 vary_pos;

void main(void) 
{
    vec2 coord = vary_pos.xy + vec2(0.5,0.5);

    coord.x *= (1.6/0.9);

    if (dot(coord, coord) > 0.25)
    {
        discard;
    }

    vec4 n = vec4(coord*2.0, 0.0, 1);
    //n.y = -n.y;
    n.z = sqrt(max(1.0-n.x*n.x-n.y*n.y, 0.0));
    //n.xyz = inv_modelviewprojection * n.xyz;

    vec4 l1tap = vec4(1.0/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265));
    vec4 l1r = texture2D(sh_input_r, vec2(0,0));
    vec4 l1g = texture2D(sh_input_g, vec2(0,0));
    vec4 l1b = texture2D(sh_input_b, vec2(0,0));
    vec3 indirect = vec3(
                      dot(l1r, l1tap * n),
                      dot(l1g, l1tap * n),
                      dot(l1b, l1tap * n));

    //indirect = pow(indirect, vec3(0.45));
    indirect *= 3.0;

	frag_color = vec4(indirect, 1.0);
}
