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

// Ray march parameters wired to uniforms (.x component of each)
//   rayStep.x              = step size (default 0.5)
//   iterationCount.x       = max steps (default 96)
//   adaptiveStepMultiplier.x = step growth rate (default 1.03)
//   distanceBias.x         = max step size cap (default 5.0)
//   depthRejectBias.x      = max thickness for hit validation (default 1.0)
#define STEP_SIZE       rayStep.x
#define STEP_GROWTH     adaptiveStepMultiplier.x
#define MAX_STEP_SIZE   distanceBias.x
#define MAX_THICKNESS   depthRejectBias.x
#define DEPTH_BIAS      depthRejectBias.y
const int   BINARY_STEPS    = 8;

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
    float initialStepLen = length(dir);

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

    // After 8 binary steps, precision is initialStep / 256.
    // Scale acceptance with depth — precision degrades at distance.
    float depthScale = max(1.0, depth * 0.01);
    float maxError = max(initialStepLen * 0.1, 0.05) * depthScale;
    if (abs(dDepth) > maxError)
        return vec3(-1.0, -1.0, depth);

    return vec3(projectedCoord, depth);
}

vec3 rayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth, float startDepth)
{
    dir *= STEP_SIZE;

    for (int i = 0; i < int(iterationCount.x); i++)
    {
        hitCoord += dir;

        vec2 projectedCoord = generateProjectedPosition(hitCoord);

        if (projectedCoord.x < 0.0 || projectedCoord.x > 1.0 ||
            projectedCoord.y < 0.0 || projectedCoord.y > 1.0)
            return vec3(-1.0);

        float depth = getLinearDepth(projectedCoord);
        dDepth = abs(hitCoord.z) - depth;

        if (i < 1)
            continue;

        if (depth > maxZDepth)
            return vec3(-1.0);

        if (dDepth > 0.0)
        {
            float stepLen = length(dir);
            float thickness = max(MAX_THICKNESS, stepLen * 1.5);
            if (dDepth > thickness)
            {
                dir = normalize(dir) * min(stepLen * STEP_GROWTH, MAX_STEP_SIZE);
                continue;
            }
            return binarySearch(dir, hitCoord, dDepth);
        }

        // Grow step but cap at max to avoid skipping geometry
        dir = normalize(dir) * min(length(dir) * STEP_GROWTH, MAX_STEP_SIZE);
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

    float roughness = 1.0 - glossiness;

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

    // Bias the ray origin along the normal, scaled by distance.
    // Prevents grazing-angle rays from scraping the originating surface
    // at distance where depth precision breaks down.
    float depthBias = max(0.01, -viewPos.z * DEPTH_BIAS);
    vec3 biasedPos = viewPos - normal * depthBias;

    vec3 transformedPos = (inv_modelview_delta * vec4(biasedPos, 1.0)).xyz;
    float startDepth = -transformedPos.z;

    if (startDepth > maxZDepth)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 perfectReflDir = normalize(reflect(viewDir, normal));

    int numSamples = max(1, int(glossySampleCount));
    vec3 accumColor = vec3(0.0);
    float accumFade = 0.0;
    int hits = 0;

    for (int s = 0; s < numSamples; s++)
    {
        vec3 reflectDir = perfectReflDir;

        // Jitter reflection direction based on roughness (importance-sampled GGX)
        if (roughness > 0.001)
        {
            float alpha = roughness * roughness;
            float u1 = random(tc * screen_res + noiseSine + float(s) * 0.123);
            float u2 = random(tc * screen_res * 1.7 + noiseSine + float(s) * 0.456 + 0.5);

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
            continue;

        // Jitter ray origin along the surface normal (outward only) to break up step-boundary striations.
        // Each pixel gets a different offset, so concentric banding from discrete steps dissolves into noise.
        float normalJitter = random(tc * screen_res + float(s) * 0.789) * max(STEP_SIZE, -viewPos.z * 0.005);
        vec3 jitteredPos = biasedPos + normal * normalJitter;
        vec3 transformedJitteredPos = (inv_modelview_delta * vec4(jitteredPos, 1.0)).xyz;
        vec3 hitCoord = transformedJitteredPos;
        float dDepth;

        vec3 result = rayMarch(transformedReflDir, hitCoord, dDepth, startDepth);

        if (result.x < 0.0)
            continue;

        vec2 hitTC = result.xy;
        float hitDepth = result.z;

        float edgeFade = calculateEdgeFade(hitTC);

        float zFadeStart = maxZDepth * 0.8;
        float zFade = 1.0 - smoothstep(zFadeStart, maxZDepth, hitDepth);

        float rayLength = length(hitCoord - transformedJitteredPos);
        float maxMipLevels = floor(log2(max(screen_res.x, screen_res.y)));
        float distanceFactor = clamp(rayLength / maxZDepth, 0.0, 1.0);
        float effectiveRoughness = clamp(roughness + distanceFactor * roughness, 0.0, 1.0);
        float mipLevel = maxMipLevels * effectiveRoughness;
        vec4 sampledColor = textureLod(source, hitTC, mipLevel);

        float rayFade = 1.0 - smoothstep(maxZDepth * 0.6, maxZDepth, rayLength);
        float sampleFade = edgeFade * zFade * rayFade;

        accumColor += sampledColor.rgb;
        accumFade += sampleFade;
        hits++;
    }

    if (hits == 0)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    accumColor /= float(numSamples);
    accumFade /= float(numSamples);

    float remappedRoughness = clamp((roughness - (maxRoughness * 0.6)) / (maxRoughness - (maxRoughness * 0.6)), 0.0, 1.0);
    float roughnessFade = 1.0 - remappedRoughness;

    float combinedFade = accumFade * roughnessFade * baseEdgeFade;

    collectedColor = vec4(accumColor, combinedFade);
    return 1.0;
}
