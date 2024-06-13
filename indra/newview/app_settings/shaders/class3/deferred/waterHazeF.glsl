/**
 * @file class3/deferred/waterHazeF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

out vec4 frag_color;

// Inputs
in vec4 vary_fragcoord;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
float getDepth(vec2 pos_screen);

vec4 getWaterFogView(vec3 pos);

uniform int above_water;

void main()
{
    vec2  tc           = vary_fragcoord.xy/vary_fragcoord.w*0.5+0.5;
    float depth        = getDepth(tc.xy);

    if (above_water > 0)
    {
        // we want to depth test when the camera is above water, but some GPUs have a hard time
        // with depth testing against render targets that are bound for sampling in the same shader
        // so we do it manually here

        float cur_depth = vary_fragcoord.z/vary_fragcoord.w*0.5+0.5;
        if (cur_depth > depth)
        {
            discard;
        }
    }

    vec4  pos          = getPositionWithDepth(tc, depth);

    vec4 fogged = getWaterFogView(pos.xyz);

    frag_color = max(fogged, vec4(0)); //output linear since local lights will be added to this shader's results

}
