/**
 * @file class3/deferred/screenSpaceReflTraceF.glsl
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

in vec2 vary_fragcoord;
in vec3 camera_ray;

uniform sampler2D sceneMap;

GBufferInfo getGBuffer(vec2 screenpos);
float getDepth(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);

void main()
{
    vec2 tc = vary_fragcoord.xy;
    float depth = getDepth(tc);

    // skip sky pixels
    if (depth >= 1.0)
    {
        frag_color = vec4(0.0);
        return;
    }

    GBufferInfo gb = getGBuffer(tc);
    vec3 pos = getPositionWithDepth(tc, depth).xyz;

    float glossiness;
    if (GET_GBUFFER_FLAG(gb.gbufferFlag, GBUFFER_FLAG_HAS_PBR))
        glossiness = 1.0 - gb.specular.a;   // RT1.a = perceptualRoughness
    else
        glossiness = gb.specular.a;          // Legacy: a = glossiness

    vec4 ssrColor = vec4(0.0);
    tapScreenSpaceReflection(1, tc, pos, gb.normal, ssrColor, sceneMap, glossiness);
    frag_color = ssrColor;
}
