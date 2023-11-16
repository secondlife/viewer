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
in vec2 vary_fragcoord;

uniform sampler2D normalMap;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
float getDepth(vec2 pos_screen);

vec4 getWaterFogView(vec3 pos);

void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = getDepth(tc.xy);
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture(normalMap, tc);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
    }
    else if (!GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_ATMOS))
    {
        //should only be true of WL sky, just port over base color value
        discard;
    }

    vec4 fogged = getWaterFogView(pos.xyz);

    frag_color.rgb = max(fogged.rgb, vec3(0)); //output linear since local lights will be added to this shader's results
    frag_color.a = fogged.a;
}
