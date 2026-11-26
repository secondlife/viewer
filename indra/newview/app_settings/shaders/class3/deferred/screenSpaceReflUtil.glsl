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

uniform int iterationCount;
uniform float maxThickness;
uniform float depthBias;
uniform int ssrDebug; // TEMP: hit classification view
uniform float glossySampleCount;
uniform float noiseSine;
uniform float maxZDepth;
uniform float maxRoughness;
uniform vec2 ssrJitterOffset;
uniform int hizMipCount;
uniform int ssrConeMips;
uniform float ssrConeScale;
uniform int sceneColorMipCount;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

// sin-hash breaks down at pixel-scale args and correlates across seeds
uvec2 pcg2d(uvec2 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    return v;
}

// interleaved gradient noise, temporally rotated by golden-ratio phase
float ssrIGN(vec2 p)
{
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))) + noiseSine * 0.61803398875);
}

vec2 generateProjectedPosition(vec3 pos)
{
    vec4 samplePosition = projection_matrix * vec4(pos, 1.0);
    samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
    // Compensate for SMAA T2x jitter difference between current and previous frame.
    // mSceneMap was rendered with the previous frame's jittered projection;
    // offset the UV so depth/color lookups hit the correct texels.
    samplePosition.xy += ssrJitterOffset;
    return samplePosition.xy;
}

float getLinearDepth(vec2 tc)
{
    // Force LOD 0 — sceneDepth now carries the Hi-Z mip chain,
    // and reflected UVs have chaotic derivatives that could cause
    // auto-LOD selection to sample coarse Hi-Z levels.
    float depth = textureLod(sceneDepth, tc, 0).r;
    vec4 pos = getPositionWithDepth(tc, depth);
    return -pos.z;
}

float projectDepth(vec3 viewPos)
{
    vec4 clip = projection_matrix * vec4(viewPos, 1.0);
    return (clip.z / clip.w) * 0.5 + 0.5;
}

// Hi-Z hierarchical screen-space ray trace (Godot parametric-T approach).
// origin/dir are in UV+depth space: (u, v, rawDepth) where depth is [0,1].
// Returns hit position (uv + raw depth) or vec3(-1) on miss.
//
// Uses parametric T along the ray for all comparisons, avoiding the AMD
// absolute-depth comparison that breaks for camera-facing rays (dir.z < 0).
// Z is normalized so dir.z = ±1, making depth_t = (cellDepth - origin.z) / dir.z
// trivially comparable with the edge_t from cell boundary intersections.
//
// Our Hi-Z stores MIN depth (standard GL: 0=near, 1=far), so min = closest.
// Godot uses reversed-Z with MAX depth, but the logic maps cleanly:
//   Godot: max(a,b,c,d) with reversed-Z → closest surface
//   Ours:  min(a,b,c,d) with standard-Z → closest surface
// The comparison flips accordingly.
//
// References:
//   Godot Engine SSR: screen_space_reflection.glsl (MIT license)
//   Sugulee/GPU Pro 5: Hi-Z Screen-Space Cone-Traced Reflections
// Returns vec4: xyz = hit position (UV + raw depth), w = overshoot behind the surface in cell allowances (<= 1 is a clean hit).
// Returns vec4(-1) on miss (ray exits screen or runs out of iterations).
// Thickness-rejected surfaces are skipped (mip-0 march) but don't kill the trace.
vec4 hiZTrace(vec3 origin, vec3 dir, int maxIterations)
{
    int maxLevel = hizMipCount - 1;

    // Guard near-zero Z direction — treat as Z+ epsilon.
    if (abs(dir.z) < 1e-7)
        dir.z = 1e-7;

    // Normalize direction so |dir.z| = 1.
    // This makes depth_t = (cellDepth - origin.z) * zDir directly comparable
    // with edge_t values along the XY axes.
    // Reference: https://hacksoflife.blogspot.com/2020/10/a-tip-for-hiz-ssr-parametric-t-tracing.html
    vec3 rayDir = dir / abs(dir.z);

    // Standard GL: 0=near, 1=far. Camera-facing rays move toward 0 (dir.z < 0).
    // Godot (reversed-Z): facing_camera = rayDir.z >= 0 (toward near=1.0).
    // For standard GL: facing_camera = dir.z < 0 (toward near=0.0).
    bool facingCamera = dir.z < 0.0;
    float zDir = rayDir.z;  // ±1 after normalization

    // Cell step direction: +1 or -1 per axis (matches Godot cell_step).
    vec2 cellStep = vec2(rayDir.x < 0.0 ? -1.0 : 1.0,
                         rayDir.y < 0.0 ? -1.0 : 1.0);

    const int SSR_MAX_TUNNEL = 2;
    int tunnels = 0;

    int curLevel = 0;
    float t = 0.0;

    // Compute t_max: parametric T to the screen edge where we stop tracing.
    vec2 t0 = (vec2(0.0) - origin.xy) / rayDir.xy;
    vec2 t1 = (vec2(1.0) - origin.xy) / rayDir.xy;
    vec2 t2 = max(t0, t1);
    float tMax = min(t2.x, t2.y);

    // Initial advance: push past origin cell to avoid self-intersection.
    // Matches Godot's initial advance exactly.
    {
        vec2 cellIndex = floor(origin.xy * screen_res);
        vec2 newCellIndex = cellIndex + clamp(cellStep, vec2(0.0), vec2(1.0));
        vec2 newCellPos = (newCellIndex / screen_res) + cellStep * 0.000001;
        vec2 posT = (newCellPos - origin.xy) / rayDir.xy;
        t = min(posT.x, posT.y);

        // IGN-jittered start: stair-step banding becomes noise the temporal resolve eats
        vec2 cellT = 1.0 / (screen_res * max(abs(rayDir.xy), vec2(1e-6)));
        t += ssrIGN(gl_FragCoord.xy) * min(cellT.x, cellT.y);
    }

    float overshoot = 0.0;

    for (int i = 0; i < maxIterations && curLevel >= 0 && t < tMax; i++)
    {
        vec3 pos = origin + rayDir * t;

        // Cell lookup at current mip level.
        vec2 cellCount = vec2(max(1, int(screen_res.x) >> curLevel),
                              max(1, int(screen_res.y) >> curLevel));
        ivec2 cellIndex = ivec2(floor(pos.xy * cellCount));
        cellIndex = clamp(cellIndex, ivec2(0), ivec2(cellCount) - 1);

        // Min depth in this cell (closest surface, standard GL).
        float cellDepth = texelFetch(sceneDepth, cellIndex, curLevel).r;

        // Parametric T to the depth surface.
        // Since rayDir.z = ±1, this is (cellDepth - origin.z) * (±1).
        float depthT = (cellDepth - origin.z) * zDir;

        // Parametric T to the nearest cell boundary in XY.
        vec2 newCellIndex = vec2(cellIndex) + clamp(cellStep, vec2(0.0), vec2(1.0));
        vec2 newCellPos = (newCellIndex / cellCount) + cellStep * 0.000001;
        vec2 posT = (newCellPos - origin.xy) / rayDir.xy;
        float edgeT = min(posT.x, posT.y);

        // Hit detection (matches Godot exactly):
        // Forward rays: hit if depth surface is reached before cell boundary.
        // Camera-facing rays: hit if ray hasn't traveled past the depth surface.
        bool hit = facingCamera ? (t <= depthT) : (depthT <= edgeT);

        int mipOffset = hit ? -1 : 1;

        // Thickness gate at mip 0: surfaces are assumed maxThickness * view depth thick (a fraction of distance)
        if (curLevel == 0 && hit)
        {
            float z0 = getPositionWithDepth(pos.xy, cellDepth).z;
            float z1 = getPositionWithDepth(pos.xy, pos.z).z;
            // a grazing ray moves further in depth per cell than the slab; it may have crossed within that step
            vec3 exitPos = origin + rayDir * edgeT;
            float rayStepZ = abs(getPositionWithDepth(exitPos.xy, exitPos.z).z - z1);
            float thickness = max(maxThickness * abs(z0), rayStepZ);
            float cellOvershoot = max(z0 - z1, 0.0) / max(thickness, 1e-4);

            if (cellOvershoot > 2.0)
            {
                // passed behind this surface: keep tracing for what is really there
                tunnels++;
                if (tunnels > SSR_MAX_TUNNEL)
                    return vec4(-2.0); // TEMP: -2 marks a tunnel-budget miss for the debug view
                hit = false;
                mipOffset = 1;  // skip cell and re-coarsen; Hi-Z re-descends if the surface continues
            }
            else
            {
                overshoot = cellOvershoot;
            }
        }

        // Advance parametric T (matches Godot exactly):
        // Only advance to depthT for non-facing-camera hits.
        // For facing-camera hits, descend without advancing.
        if (hit)
        {
            if (!facingCamera)
                t = max(t, depthT);
        }
        else
        {
            t = edgeT;
        }

        curLevel = min(curLevel + mipOffset, maxLevel);
    }

    vec3 hitPos = origin + rayDir * t;

    // Final bounds check.
    if (hitPos.x < 0.0 || hitPos.x > 1.0 ||
        hitPos.y < 0.0 || hitPos.y > 1.0 ||
        t >= tMax)
        return vec4(-1.0);

    // TEMP: tunnel count rides above the overshoot (overshoot <= 2) for the debug view
    return vec4(hitPos, overshoot + 4.0 * float(tunnels));
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

    // the reflected ray starts on the surface itself; any normal offset shifts the whole reflection
    vec3 transformedPos = (inv_modelview_delta * vec4(viewPos, 1.0)).xyz;
    float startDepth = -transformedPos.z;

    if (startDepth > maxZDepth)
    {
        collectedColor = vec4(0.0);
        return 0.0;
    }

    vec3 perfectReflDir = normalize(reflect(viewDir, normal));

    int numSamples = max(1, int(glossySampleCount));
    if (roughness <= 0.001)
    { // jitter is skipped below this roughness, so extra rays are identical — trace once
        numSamples = 1;
    }
    vec3 accumColor = vec3(0.0);
    float accumFade = 0.0;
    int hits = 0;
    bool budgetMiss = false; // TEMP: debug
    bool validationMiss = false; // TEMP: debug
    bool facingMiss = false; // TEMP: debug

    for (int s = 0; s < numSamples; s++)
    {
        vec3 reflectDir = perfectReflDir;

        // Jitter reflection direction based on roughness (importance-sampled GGX)
        if (roughness > 0.001)
        {
            float alpha = roughness * roughness;
            uvec2 hash = pcg2d(uvec2(gl_FragCoord.xy)
                             + uvec2(uint(noiseSine) * 7919u + uint(s) * 104729u,
                                     uint(noiseSine) * 6271u + uint(s) * 15013u));
            float u1 = float(hash.x) * 2.3283064365386963e-10;
            float u2 = float(hash.y) * 2.3283064365386963e-10;

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
            facingMiss = true;
            continue;
        }

        // Fade rays pointing back toward camera to avoid sharp SSR boundary.
        float cameraFacingFade = 1.0 - smoothstep(0.45, 0.5, transformedReflDir.z);

        // Project ray origin and a nearby point to screen space (UV + raw depth).
        // Use a short step (fraction of distance-to-camera) to ensure the end
        // point stays in front of the camera — projecting at maxZDepth can wrap
        // behind the camera for reflections going toward the viewer.
        // nudge the start just off the surface along its normal against reprojection noise; depth-scaled and small
        // enough that the reflection shifts by about a pixel at most
        float nudge = max(0.001, -viewPos.z * 0.001);
        vec3 transformedStart = (inv_modelview_delta * vec4(viewPos + normal * nudge, 1.0)).xyz;

        vec3 ssOrigin = vec3(generateProjectedPosition(transformedStart),
                             projectDepth(transformedStart));
        float stepDist = max(0.1, -transformedStart.z * 0.1);
        vec3 transformedEnd = transformedStart + transformedReflDir * stepDist;
        vec3 ssFar = vec3(generateProjectedPosition(transformedEnd),
                          projectDepth(transformedEnd));
        vec3 ssDir = ssFar - ssOrigin;

        // the trace reads last frame's depth; under camera motion the reprojected start can land a hair behind that
        // surface and read as an immediate self-hit, so pull its depth onto the surface (direction unchanged)
        ivec2 originPixel = clamp(ivec2(ssOrigin.xy * screen_res), ivec2(0), ivec2(screen_res) - 1);
        ssOrigin.z = min(ssOrigin.z, texelFetch(sceneDepth, originPixel, 0).r);

        vec4 result = hiZTrace(ssOrigin, ssDir, iterationCount);

        if (result.x < 0.0)
        {
            if (result.x < -1.5)
            {
                budgetMiss = true;
            }

            continue;
        }

        vec2 hitTC = result.xy;

        // Read actual surface depth at the hit pixel (mip 0, exact pixel).
        ivec2 hitPixel = ivec2(hitTC * screen_res);
        hitPixel = clamp(hitPixel, ivec2(0), ivec2(screen_res) - 1);
        float hitSurfaceDepth = texelFetch(sceneDepth, hitPixel, 0).r;

        // Reject sky / far-plane hits (forward-Z: 1.0 = far).
        float hitDepth = -getPositionWithDepth(hitTC, hitSurfaceDepth).z;
        if (hitDepth > maxZDepth)
            continue;

        vec3 viewSpaceSurface = getPositionWithDepth(hitTC, hitSurfaceDepth).xyz;

        // self-hit: a hit lying in the origin surface's own plane is that surface finding itself
        vec3 originNormal = normalize(mat3(inv_modelview_delta) * normal);
        if (dot(viewSpaceSurface - transformedPos, originNormal) < max(0.005, -transformedPos.z * 0.002))
        {
            continue;
        }

        // end-depth validation (rejects camera-facing false hits, which don't snap onto the surface); a ramp,
        // so near-misses around silhouettes fade instead of dropping out in blocks
        vec3 viewSpaceHit = getPositionWithDepth(hitTC, result.z).xyz;
        float endThickness = maxThickness * abs(viewSpaceSurface.z);
        float endError = length(viewSpaceSurface - viewSpaceHit);

        if (endError > endThickness)
        {
            validationMiss = true;
            continue;
        }

        float endValidation = 1.0 - smoothstep(0.1 * endThickness, endThickness, endError);

        // full weight within one cell's allowance, gone by two; rays that slid behind a silhouette fade out
        float hit_overshoot = mod(result.w, 4.0);
        float confidence = (1.0 - smoothstep(1.0, 2.0, hit_overshoot)) * endValidation;

        // Short-ray back-face check (Godot-style):
        // For rays that traveled < 3 pixels, compute a geometric normal at the
        // hit point from depth buffer cross-derivatives. Reject if the reflected
        // ray exits the surface (dot >= 0), which indicates self-intersection.
        float rayLen = length(result.xy - ssOrigin.xy);
        float rayPixelLen = rayLen * max(screen_res.x, screen_res.y);
        if (rayPixelLen < 3.0)
        {
            float dR = texelFetch(sceneDepth, hitPixel + ivec2(1, 0), 0).r;
            float dD = texelFetch(sceneDepth, hitPixel + ivec2(0, 1), 0).r;
            vec3 posR = getPositionWithDepth((vec2(hitPixel) + vec2(1.5, 0.5)) / screen_res, dR).xyz;
            vec3 posD = getPositionWithDepth((vec2(hitPixel) + vec2(0.5, 1.5)) / screen_res, dD).xyz;
            vec3 hitGeomNormal = cross(posR - viewSpaceSurface, posD - viewSpaceSurface);
            // Ensure normal faces toward camera (positive Z in view space).
            if (hitGeomNormal.z < 0.0) hitGeomNormal = -hitGeomNormal;
            hitGeomNormal = normalize(hitGeomNormal);

            if (dot(reflectDir, hitGeomNormal) >= 0.0)
                continue;
        }

        // Fringe and skim: the hit pixel's 4 neighbors in view space
        ivec2 maxPixel = ivec2(screen_res) - 1;
        ivec2 pxL = clamp(hitPixel - ivec2(1, 0), ivec2(0), maxPixel);
        ivec2 pxR = clamp(hitPixel + ivec2(1, 0), ivec2(0), maxPixel);
        ivec2 pxD = clamp(hitPixel - ivec2(0, 1), ivec2(0), maxPixel);
        ivec2 pxU = clamp(hitPixel + ivec2(0, 1), ivec2(0), maxPixel);
        vec3 nbL = getPositionWithDepth((vec2(pxL) + 0.5) / screen_res, texelFetch(sceneDepth, pxL, 0).r).xyz;
        vec3 nbR = getPositionWithDepth((vec2(pxR) + 0.5) / screen_res, texelFetch(sceneDepth, pxR, 0).r).xyz;
        vec3 nbD = getPositionWithDepth((vec2(pxD) + 0.5) / screen_res, texelFetch(sceneDepth, pxD, 0).r).xyz;
        vec3 nbU = getPositionWithDepth((vec2(pxU) + 0.5) / screen_res, texelFetch(sceneDepth, pxU, 0).r).xyz;

        // fringe: second difference of 1/z is zero on any plane at any slope and rises where the pixel straddles an edge
        float invC = 1.0 / max(-viewSpaceSurface.z, 1e-4);
        float bendX = abs(1.0 / max(-nbL.z, 1e-4) + 1.0 / max(-nbR.z, 1e-4) - 2.0 * invC);
        float bendY = abs(1.0 / max(-nbD.z, 1e-4) + 1.0 / max(-nbU.z, 1e-4) - 2.0 * invC);
        float fringe = 1.0 - smoothstep(maxThickness, 2.0 * maxThickness, max(bendX, bendY) / invC);

        // skim: a ray meeting the surface near edge-on reads a sliver of it
        vec3 hitSurfaceNormal = normalize(cross(nbR - nbL, nbU - nbD));
        float skim = smoothstep(0.05, 0.2, abs(dot(transformedReflDir, hitSurfaceNormal)));

        confidence *= fringe * skim;

        float edgeFade = calculateEdgeFade(hitTC);

        // screen room: how far the ray could travel before leaving the screen; hits that used most of it
        // fade gradually, so coverage approaches the screen-exit cut softly instead of stopping on a line
        vec2 roomDir = normalize(ssDir.xy + vec2(1e-7));
        vec2 roomT = max((vec2(0.0) - ssOrigin.xy) / roomDir, (vec2(1.0) - ssOrigin.xy) / roomDir);
        float roomLen = max(min(roomT.x, roomT.y), 1e-4);
        edgeFade *= 1.0 - smoothstep(0.5, 1.0, rayLen / roomLen);

        float zFadeStart = maxZDepth * 0.8;
        float zFade = 1.0 - smoothstep(zFadeStart, maxZDepth, hitDepth);

        // Cone-matched blur: prefiltered-pyramid mip from the GGX cone footprint at the hit
        float coneMip = 0.0;
        if (ssrConeMips > 0)
        {
            float hitDist = length(viewSpaceSurface - transformedPos);
            float coneTan = roughness * roughness * ssrConeScale;
            float footprint = coneTan * hitDist / max(-viewSpaceSurface.z, 0.01)
                            * screen_res.y * 0.5 * projection_matrix[1][1];
            coneMip = clamp(log2(max(footprint, 1.0)), 0.0, float(sceneColorMipCount - 1));
        }

        vec4 sampledColor = textureLod(source, hitTC, coneMip);

        // Tone map hit color (Reinhard on luminance) to compress brights
        // before mip chain averaging — prevents fireflies.
        float luma = dot(sampledColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        sampledColor.rgb /= (1.0 + luma);

        // Fade-in: suppress near-origin artifacts from self-intersection.
        float fadeIn = smoothstep(0.0, 0.01, rayLen);
        float rayFade = 1.0 - smoothstep(0.6, 1.0, rayLen);

        float sampleFade = edgeFade * zFade * fadeIn * rayFade * confidence * cameraFacingFade;

        // TEMP: hit classification view, full weight; pre-compressed so the composite's recovery shows these exact colors
        if (ssrDebug == 1)
        {
            vec3 debug_color = vec3(1.0 - confidence, clamp(rayPixelLen / 64.0, 0.0, 1.0), step(4.0, result.w));
            float debug_luma = dot(debug_color, vec3(0.2126, 0.7152, 0.0722));
            sampledColor.rgb = debug_color / (1.0 + debug_luma);
            sampleFade = 1.0;
        }

        accumColor += sampledColor.rgb * sampleFade;
        accumFade += sampleFade;
        hits++;
    }

    if (hits == 0)
    {
        collectedColor = vec4(0.0);

        // TEMP: misses that ran out of tunnel budget show orange
        if (ssrDebug == 1 && (budgetMiss || validationMiss || facingMiss))
        {
            // TEMP: orange = tunnel budget, cyan = end-depth validation, white = camera-facing cutoff
            vec3 debug_color = vec3(1.0, 0.5, 0.0);

            if (validationMiss)
            {
                debug_color = vec3(0.0, 1.0, 1.0);
            }

            if (facingMiss)
            {
                debug_color = vec3(1.0);
            }

            collectedColor = vec4(debug_color / (1.0 + dot(debug_color, vec3(0.2126, 0.7152, 0.0722))), 1.0);
            return 1.0;
        }

        return 0.0;
    }

    accumColor /= float(numSamples);
    accumFade /= float(numSamples);

    float combinedFade = accumFade * baseEdgeFade;

    collectedColor = vec4(accumColor * baseEdgeFade, combinedFade);
    return 1.0;
}
