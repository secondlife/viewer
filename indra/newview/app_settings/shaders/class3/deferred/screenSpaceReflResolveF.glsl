/**
 * @file screenSpaceReflResolveF.glsl
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

// Temporal accumulation of 1-ray-per-frame SSR traces, in the trace's
// encoded space (fade-premultiplied, luma-compressed).

out vec4 frag_color;

in vec2 vary_fragcoord;

uniform sampler2D currentColorTex;   // this frame's raw trace (SSR res)
uniform sampler2D previousColorTex;  // accumulated history (SSR res)

uniform mat4 projection_matrix;
uniform mat4 inv_modelview_delta;    // current camera space -> previous camera space
uniform vec2 ssrJitterOffset;        // SMAA T2x (curJitter - prevJitter) / res
uniform float ssrHistoryBlend;       // 0.9, or 0.0 when history is invalid

// deferredUtil.glsl
float getDepth(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);
GBufferInfo getGBuffer(vec2 screenpos);

// single-scalar clip; per-channel clamps break premultiplied rgb/a ratios
vec4 clipToAABB(vec4 v, vec4 mn, vec4 mx)
{
    vec4 center = 0.5 * (mx + mn);
    vec4 extent = 0.5 * (mx - mn) + 1e-4;
    vec4 delta  = v - center;
    vec4 ts     = abs(delta) / extent;
    float t     = max(max(ts.x, ts.y), max(ts.z, ts.w));
    return (t > 1.0) ? (center + delta / t) : v;
}

// 5-tap Catmull-Rom; bilinear refetch compounds into blur over frames
vec4 sampleHistoryCatmullRom(vec2 uv, vec2 texSize)
{
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / w12;

    vec2 texPos0  = (texPos1 - 1.0) / texSize;
    vec2 texPos3  = (texPos1 + 2.0) / texSize;
    vec2 texPos12 = (texPos1 + offset12) / texSize;

    vec4 result = vec4(0.0);
    float wSum = 0.0;

    result += textureLod(previousColorTex, vec2(texPos12.x, texPos0.y), 0.0) * (w12.x * w0.y);
    wSum   += w12.x * w0.y;
    result += textureLod(previousColorTex, vec2(texPos0.x, texPos12.y), 0.0) * (w0.x * w12.y);
    wSum   += w0.x * w12.y;
    result += textureLod(previousColorTex, texPos12, 0.0) * (w12.x * w12.y);
    wSum   += w12.x * w12.y;
    result += textureLod(previousColorTex, vec2(texPos3.x, texPos12.y), 0.0) * (w3.x * w12.y);
    wSum   += w3.x * w12.y;
    result += textureLod(previousColorTex, vec2(texPos12.x, texPos3.y), 0.0) * (w12.x * w3.y);
    wSum   += w12.x * w3.y;

    return max(result / wSum, vec4(0.0));
}

void main()
{
    ivec2 px = ivec2(gl_FragCoord.xy);
    vec4 cur = texelFetch(currentColorTex, px, 0);

    float depth = getDepth(vary_fragcoord);
    if (depth >= 1.0)
    { // sky
        frag_color = cur;
        return;
    }

    GBufferInfo gb = getGBuffer(vary_fragcoord);
    float glossiness;
    if (GET_GBUFFER_FLAG(gb.gbufferFlag, GBUFFER_FLAG_HAS_PBR))
        glossiness = 1.0 - gb.specular.a;
    else
        glossiness = gb.specular.a;
    float roughness = 1.0 - glossiness;

    // 3x3 moments: mean doubles as spatial ray reuse, variance bounds history
    vec4 m1 = vec4(0.0);
    vec4 m2 = vec4(0.0);
    ivec2 sz = textureSize(currentColorTex, 0);
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec4 s = texelFetch(currentColorTex, clamp(px + ivec2(x, y), ivec2(0), sz - 1), 0);
            m1 += s;
            m2 += s * s;
        }
    }
    vec4 mu = m1 / 9.0;
    vec4 sigma = sqrt(max(m2 / 9.0 - mu * mu, vec4(0.0)));

    // reuse neighbors' rays where the GGX lobe is wide; mirrors keep their own sample
    vec4 filtered = mix(cur, mu, smoothstep(0.0, 0.2, roughness));

    if (ssrHistoryBlend <= 0.0)
    {
        frag_color = filtered;
        return;
    }

    // camera-only reprojection into last frame's screen space
    vec3 viewPos  = getPositionWithDepth(vary_fragcoord, depth).xyz;
    vec3 prevPos  = (inv_modelview_delta * vec4(viewPos, 1.0)).xyz;
    vec4 prevClip = projection_matrix * vec4(prevPos, 1.0);
    if (prevClip.w <= 0.0)
    {
        frag_color = filtered;
        return;
    }
    vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5 + ssrJitterOffset;

    float w = ssrHistoryBlend;
    if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
    {
        w = 0.0;
    }

    // Salvi variance bound; min/max would clamp accumulation to the noise extremes
    vec4 hist = clipToAABB(sampleHistoryCatmullRom(prevUV, vec2(textureSize(previousColorTex, 0))),
                           mu - sigma * 1.25, mu + sigma * 1.25);

    // alpha blends too, so accumulated confidence stays coherent with color
    frag_color = mix(filtered, hist, w);
}
