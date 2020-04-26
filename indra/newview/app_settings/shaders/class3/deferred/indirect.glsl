/** 
 * @file class3/deferred/indirect.glsl
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

uniform sampler2D sh_input_r;
uniform sampler2D sh_input_g;
uniform sampler2D sh_input_b;

vec3 GetIndirect(vec3 norm)
{
    vec4 l1tap = vec4(1.0/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265));
    vec4 l1r = texture2D(sh_input_r, vec2(0,0));
    vec4 l1g = texture2D(sh_input_g, vec2(0,0));
    vec4 l1b = texture2D(sh_input_b, vec2(0,0));
    vec3 indirect = vec3(dot(l1r, l1tap * vec4(1, norm.xyz)),
                         dot(l1g, l1tap * vec4(1, norm.xyz)),
                         dot(l1b, l1tap * vec4(1, norm.xyz)));
    indirect = clamp(indirect, vec3(0), vec3(1.0));
    return indirect;
}

