/**
 * @file class3/deferred/screenSpaceReflTraceF.glsl
 * @brief Screen Space Reflection ray tracing pass
 * Traces reflection rays and outputs to SSR buffer for post-process blur
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform mat4 inv_proj;
uniform float zNear;
uniform float zFar;

in vec2 vary_fragcoord;

uniform sampler2D normalMap;     // G-buffer normal for reflection direction
uniform sampler2D depthMap;      // G-buffer depth for ray origin
uniform sampler2D sceneMap;      // Scene color for SSR sampling
uniform sampler2D sceneDepth;    // Scene depth for ray marching

vec4 getNorm(vec2 screenpos);
float getDepth(vec2 pos_screen);
float linearDepth(float d, float znear, float zfar);
float linearDepth01(float d, float znear, float zfar);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

// Forward declaration - implemented in screenSpaceReflUtil.glsl
float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);

void main()
{
    vec2 tc = vary_fragcoord.xy;

    // Sample G-buffer
    vec4 norm = getNorm(tc);
    vec3 pos = getPositionWithDepth(tc, getDepth(tc)).xyz;

    // Initialize output
    frag_color = vec4(0.0);

    // Trace SSR with perfect reflections (glossiness=1.0)
    // Roughness will be handled during sampling via mipmap selection
    vec4 ssrColor = vec4(0.0);
    float hitWeight = tapScreenSpaceReflection(1, tc, pos, norm.xyz, ssrColor, sceneMap, 1.0);

    if (hitWeight > 0.0)
    {
        // DEBUG: Show ray hits (cyan = hit detected)
        //frag_color = vec4(0.0, 1.0, 1.0, 1.0);

        // Store raw SSR result for sampling pass
        // Alpha contains edge fade for proper blending
        frag_color = ssrColor;
    }
    else
    {
        // DEBUG: Show surfaces that attempted SSR but missed (red)
        frag_color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
