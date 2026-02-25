/**
 * @file motionBlurF.glsl
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

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D velocityMap;
uniform vec2 screen_res;
uniform int motion_blur_strength;

in vec2 vary_fragcoord;

void main()
{
    vec2 uv = vary_fragcoord;
    vec2 vel = texture(velocityMap, uv).rg;

    // NDC velocity to pixel velocity
    vec2 pixel_vel = vel * screen_res * 0.5;
    float speed = length(pixel_vel);

    // Early out for negligible motion
    if (speed < 0.5)
    {
        frag_color = texture(diffuseRect, uv);
        return;
    }

    // Clamp to max blur length
    float max_blur = float(motion_blur_strength);
    if (speed > max_blur)
    {
        pixel_vel *= max_blur / speed;
    }

    // Step size in UV space per iteration
    vec2 step_uv = (pixel_vel / screen_res) * (2.0 / 32.0);

    // Start sampling ahead of center
    vec2 sample_uv = uv + step_uv * 16.0;

    // 32-sample triangle-weighted blur
    vec3 color = vec3(0.0);
    float total = 0.0;

    for (int i = 0; i < 32; ++i)
    {
        float w = 32.0 - abs(float(i) - 16.0);
        total += w;
        color += texture(diffuseRect, sample_uv).rgb * w;
        sample_uv -= step_uv;
    }

    frag_color = vec4(color / total, 1.0);
}
