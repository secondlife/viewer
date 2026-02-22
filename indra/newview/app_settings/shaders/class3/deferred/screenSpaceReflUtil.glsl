/**
 * @file class3/deferred/screenSpaceReflUtil.glsl
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

// Based on https://imanolfotia.com/blog/1

uniform sampler2D sceneMap;
uniform sampler2D sceneDepth;

uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform mat4 inv_proj;
uniform mat4 modelview_delta;
uniform mat4 inv_modelview_delta;

// Declared to keep pipeline uniform setup happy
uniform vec3 iterationCount;
uniform vec3 rayStep;
uniform vec3 distanceBias;
uniform vec3 depthRejectBias;
uniform vec3 adaptiveStepMultiplier;
uniform vec3 splitParamsStart;
uniform vec3 splitParamsEnd;
uniform float glossySampleCount;
uniform float noiseSine;
uniform float maxZDepth;
uniform float maxRoughness;

// Hardcoded ray march parameters
const float STEP_SIZE       = 0.5;
const float STEP_GROWTH     = 1.05;
const int   MAX_STEPS       = 64;
const int   BINARY_STEPS    = 8;
const float MAX_THICKNESS   = 1.0;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

float random(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 generateProjectedPosition(vec3 pos)
{
    vec4 samplePosition = projection_matrix * vec4(pos, 1.0);
    samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
    return samplePosition.xy;
}

float getLinearDepth(vec2 tc)
{
    float depth = texture(sceneDepth, tc).r;
    vec4 pos = getPositionWithDepth(tc, depth);
    return -pos.z;
}

vec3 binarySearch(vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
    float depth;

    for (int i = 0; i < BINARY_STEPS; i++)
    {
        vec2 projectedCoord = generateProjectedPosition(hitCoord);
        depth = getLinearDepth(projectedCoord);
        dDepth = abs(hitCoord.z) - depth;

        dir *= 0.5;
        if (dDepth > 0.0)
            hitCoord -= dir;
        else
            hitCoord += dir;
    }

    vec2 projectedCoord = generateProjectedPosition(hitCoord);

    // Reject if the ray ended up too far behind the surface
    if (abs(dDepth) > MAX_THICKNESS)
        return vec3(-1.0, -1.0, depth);

    return vec3(projectedCoord, depth);
}

vec3 rayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth, float startDepth)
{
    dir *= STEP_SIZE;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        hitCoord += dir;

        vec2 projectedCoord = generateProjectedPosition(hitCoord);

        if (projectedCoord.x < 0.0 || projectedCoord.x > 1.0 ||
            projectedCoord.y < 0.0 || projectedCoord.y > 1.0)
            return vec3(-1.0);

        float depth = getLinearDepth(projectedCoord);
        dDepth = abs(hitCoord.z) - depth;

        if (abs(depth - startDepth) < 0.001)
            continue;

        if (depth > maxZDepth)
            return vec3(-1.0);

        if (dDepth > 0.0)
        {
            // If we overshot by more than the thickness limit, skip this surface
            if (dDepth > MAX_THICKNESS)
            {
                dir *= STEP_GROWTH;
                continue;
            }
            return binarySearch(dir, hitCoord, dDepth);
        }

        dir *= STEP_GROWTH;
    }

    return vec3(-1.0);
}

float calculateEdgeFade(vec2 screenPos)
{
    vec2 distFromCenter = abs(screenPos * 2.0 - 1.0);
    vec2 fade = smoothstep(0.85, 1.0, distFromCenter);
    return 1.0 - max(fade.x, fade.y);
}

float tapScreenSpaceReflection(
    int totalSamples,
    vec2 tc,
    vec3 viewPos,
    vec3 n,
    inout vec4 collectedColor,
    sampler2D source,
    float glossiness)
{
#ifdef TRANSPARENT_SURFACE
    collectedColor = vec4(1, 0, 1, 1);
    return 0;
#endif

    float roughness = (1.0 - glossiness) * 0.5;

    if (roughness >= maxRoughness)
        return 0.0;

    vec3 viewDir = normalize(viewPos);
    vec3 normal = normalize(n);

    float viewDotNormal = dot(-viewDir, normal);
    if (viewDotNormal <= 0.0)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec2 distFromCenter = abs(tc * 2.0 - 1.0);
    float baseEdgeFade = 1.0 - smoothstep(0.85, 1.0, max(distFromCenter.x, distFromCenter.y));
    if (baseEdgeFade <= 0.001)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 transformedPos = (inv_modelview_delta * vec4(viewPos, 1.0)).xyz;
    float startDepth = -transformedPos.z;

    if (startDepth > maxZDepth)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 reflectDir = normalize(reflect(viewDir, normal));

    // Jitter reflection direction based on roughness (importance-sampled GGX)
    if (roughness > 0.001)
    {
        float alpha = roughness * roughness;
        float u1 = random(tc * screen_res + noiseSine);
        float u2 = random(tc * screen_res * 1.7 + noiseSine + 0.5);

        float theta = atan(alpha * sqrt(u1) / sqrt(1.0 - u1));
        float phi = 2.0 * 3.14159265 * u2;

        vec3 up = abs(reflectDir.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        vec3 tangent = normalize(cross(up, reflectDir));
        vec3 bitangent = cross(reflectDir, tangent);

        vec3 h = normalize(
            sin(theta) * cos(phi) * tangent +
            sin(theta) * sin(phi) * bitangent +
            cos(theta) * reflectDir
        );

        reflectDir = normalize(reflect(-reflectDir, h));
    }

    vec3 reflTarget = viewPos + reflectDir;
    vec3 transformedTarget = (inv_modelview_delta * vec4(reflTarget, 1.0)).xyz;
    vec3 transformedReflDir = normalize(transformedTarget - transformedPos);

    if (transformedReflDir.z >= 0.5)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    // Jitter ray start by a fraction of the first step to break up banding
    float jitter = random(tc * screen_res) * STEP_SIZE * 0.5;
    vec3 hitCoord = transformedPos + transformedReflDir * jitter;
    float dDepth;

    vec3 result = rayMarch(transformedReflDir, hitCoord, dDepth, startDepth);

    if (result.x < 0.0)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec2 hitTC = result.xy;
    float hitDepth = result.z;

    float edgeFade = calculateEdgeFade(hitTC);

    float zFadeStart = maxZDepth * 0.8;
    float zFade = 1.0 - smoothstep(zFadeStart, maxZDepth, hitDepth);

    float remappedRoughness = clamp((roughness - (maxRoughness * 0.6)) / (maxRoughness - (maxRoughness * 0.6)), 0.0, 1.0);
    float roughnessFade = 1.0 - remappedRoughness;

    // Estimate effective roughness from surface roughness + ray travel distance.
    // A rough surface scatters reflected rays into a cone; the further the hit,
    // the larger that cone's footprint on screen, so we blur more.
    float rayLength = length(hitCoord - transformedPos);
    float maxMipLevels = floor(log2(max(screen_res.x, screen_res.y)));
    float distanceFactor = clamp(rayLength / maxZDepth, 0.0, 1.0);
    float effectiveRoughness = clamp(roughness + distanceFactor * roughness, 0.0, 1.0);
    float mipLevel = maxMipLevels * effectiveRoughness;
    vec4 sampledColor = textureLod(source, hitTC, mipLevel);

    // Fade out distant ray hits — long rays at grazing angles lack
    // depth buffer precision and produce unreliable results.
    float rayFade = 1.0 - smoothstep(maxZDepth * 0.3, maxZDepth * 0.7, rayLength);

    float combinedFade = edgeFade * zFade * roughnessFade * baseEdgeFade * rayFade;

    collectedColor = vec4(sampledColor.rgb, combinedFade);
    return 1.0;
}
