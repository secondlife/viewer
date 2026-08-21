/**
 * @file ssrDownsampleF.glsl
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

// dual-filter downsample: 4 diagonal taps + weighted center, one pass per mip

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform vec2 texelStep;

in vec2 vary_texcoord0;

void main()
{
    vec4 sum = texture(diffuseRect, vary_texcoord0) * 4.0;
    sum += texture(diffuseRect, vary_texcoord0 + vec2(-texelStep.x, -texelStep.y));
    sum += texture(diffuseRect, vary_texcoord0 + vec2( texelStep.x, -texelStep.y));
    sum += texture(diffuseRect, vary_texcoord0 + vec2(-texelStep.x,  texelStep.y));
    sum += texture(diffuseRect, vary_texcoord0 + vec2( texelStep.x,  texelStep.y));
    frag_color = sum * 0.125;
}
