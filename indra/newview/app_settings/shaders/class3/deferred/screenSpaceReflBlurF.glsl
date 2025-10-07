/**
 * @file class3/deferred/screenSpaceReflBlurF.glsl
 * @brief SSR blur and composite pass
 * Applies roughness-based blur to SSR results via mip sampling
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform vec2 screen_res;

in vec2 vary_fragcoord;

uniform sampler2D ssrBuffer;      // Raw SSR results with mipmaps
uniform sampler2D specularRect;   // G-buffer specular/roughness
uniform sampler2D normalMap;      // G-buffer normals

vec4 getNorm(vec2 screenpos);

void main()
{
    vec2 tc = vary_fragcoord.xy;

    // Sample G-buffer to get roughness
    vec4 norm = getNorm(tc);
    vec4 spec = texture(specularRect, tc);

    // Initialize output as transparent
    frag_color = vec4(0.0);

    // Check if this pixel has PBR material
    if (GET_GBUFFER_FLAG(norm.w, GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = spec.rgb;
        float roughness = orm.g;

        // Calculate mip level based on roughness
        // Higher roughness = higher mip level = more blur
        // Assuming typical mip chain (log2 of max resolution)
        float maxDim = max(screen_res.x, screen_res.y);
        float maxMipLevel = floor(log2(maxDim));
        float mipLevel = roughness * roughness * maxMipLevel; // Square for better distribution

        // Sample SSR buffer with mip-based blur
        // This effectively creates a convolution blur based on roughness
        vec4 ssrColor = textureLod(ssrBuffer, tc, mipLevel);

        // Output blurred SSR
        // Alpha contains the edge fade/hit mask from tracing
        frag_color = ssrColor;
    }
}
