/**
 * @file class3/deferred/screenSpaceReflUtil.glsl
 * @brief Utility functions for implementing Screen Space Reflections (SSR).
 *
 * This file contains the core logic for ray marching in screen space to find reflections,
 * including adaptive step sizes, binary search refinement, and handling of glossy reflections.
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

// --- Uniforms ---
// These are variables passed from the CPU to the shader.

/** @brief Sampler for the current frame's scene color. This is what reflections will be sampled from. */
uniform sampler2D sceneMap;
/** @brief Sampler for the current frame's scene depth. Used to find intersections during ray marching. */
uniform sampler2D sceneDepth;

/** @brief Resolution of the screen in pixels (width, height). */
uniform vec2 screen_res;
/** @brief The current view's projection matrix. Transforms view space coordinates to clip space. */
uniform mat4 projection_matrix;
/** @brief The inverse of the projection matrix. Transforms clip space coordinates back to view space. */
uniform mat4 inv_proj;
/** @brief Transformation matrix from the last frame's camera space to the current frame's camera space. Used for temporal reprojection.*/
uniform mat4 modelview_delta;
/** @brief Inverse of the modelview_delta matrix. Transforms current camera space back to last frame's camera space. */
uniform mat4 inv_modelview_delta;

// --- Forward declaration for a function defined elsewhere or later in the file ---
/**
 * @brief Reconstructs view space position from screen coordinates and depth.
 * @param pos_screen Screen space texture coordinates (0-1 range).
 * @param depth Depth value sampled from the depth buffer (typically non-linear).
 * @return vec4 The reconstructed position in view space. The .xyz components are position, .w is typically 1.0.
 */
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

/**
 * @brief Generates a pseudo-random float value based on a 2D input vector.
 * @param uv A 2D vector used as a seed for the random number generation.
 * @return float A pseudo-random value in the range [0, 1).
 */
float random (vec2 uv)
{
    // A common simple hash function to generate pseudo-random numbers in GLSL.
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Based off of https://github.com/RoundedGlint585/ScreenSpaceReflection/
// A few tweaks here and there to suit our needs.

/**
 * @brief Projects a 3D position (typically in view space) to 2D screen space coordinates.
 * @param pos The 3D position to project (assumed to be in view space).
 * @return vec2 The 2D screen space coordinates, in the range [0, 1] for x and y.
 */
vec2 generateProjectedPosition(vec3 pos)
{
    // Project the 3D position to clip space.
    vec4 samplePosition = projection_matrix * vec4(pos, 1.f);
    // Perform perspective divide and map to [0, 1] texture coordinate range.
    samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
    return samplePosition.xy;
}

// These booleans control various optimizations and features of the SSR algorithm.
/** @brief If true, a binary search step is performed to refine the hit point after an initial overshoot. */
bool isBinarySearchEnabled = true;
/** @brief If true, the ray marching step size is adapted based on the distance to the nearest surface. */
bool isAdaptiveStepEnabled = true;
/** @brief If true, the base ray marching step size increases exponentially with each step. */
bool isExponentialStepEnabled = true;
/** @brief If true, debug colors are used to visualize aspects of the ray marching process (e.g., delta values). */
bool debugDraw = false;

// Near Pass Uniforms
uniform vec3 iterationCount;
uniform vec3 rayStep;
uniform vec3 distanceBias;
uniform vec3 depthRejectBias;
uniform vec3 adaptiveStepMultiplier;
uniform vec3 splitParamsStart;
uniform vec3 splitParamsEnd;

float blurStepY = 0.01;
float blurStepX = 0.01;
float blurIterationX = 2;
float blurIterationY = 4;

/** @brief Number of samples to take for glossy reflections. Higher values give smoother but more expensive reflections. Debug setting: RenderScreenSpaceReflectionGlossySamples */
uniform float glossySampleCount;
/** @brief A time-varying sine value, used to introduce temporal variation with the poisson sphere sampling. */
uniform float noiseSine;

/** @brief A small constant value used for floating-point comparisons, typically to avoid issues with precision. Used in self-intersection checks. */
float epsilon = 0.000001;

/**
 * @brief Samples the scene depth texture and converts it to linear view space Z depth.
 * @param tc Texture coordinates (screen space, 0-1 range) at which to sample the depth.
 * @return float The linear depth value.
 */
float getLinearDepth(vec2 tc)
{
    // Sample the raw depth value from the depth texture.
    float depth = texture(sceneDepth, tc).r;
    // Reconstruct the view space position using the sampled depth.
    vec4 pos = getPositionWithDepth(tc, depth);
    // Return the Z component of the view space position, which is the linear depth.
    return -pos.z; // Negated as view space Z is typically negative.
}

/**
 * @brief Calculates a fade factor based on the proximity of a screen position to the screen edges.
 * The fade increases as the position gets closer to any edge.
 * @param screenPos The screen position in normalized device coordinates (0-1 range).
 * @return float The edge fade factor, ranging from 1.0 (no fade, away from edges) to 0.0 (full fade, at an edge).
 */
float calculateEdgeFade(vec2 screenPos) {
    // Defines how close to the edge the fade effect starts.
    // 0.9 means fading begins when the point is 10% away from the screen edge.
    const float edgeFadeStart = 0.9;
    
    // Convert screen position from [0,1] to [-1,1] range and take absolute value.
    // This gives distance from the center: 0 at center, 1 at edges.
    vec2 distFromCenter = abs(screenPos * 2.0 - 1.0);
    
    // Calculate a smooth fade for each axis using smoothstep.
    // smoothstep(edgeFadeStart, 1.0, x) will be:
    // - 0 if x < edgeFadeStart
    // - 1 if x > 1.0
    // - A smooth transition between 0 and 1 if edgeFadeStart <= x <= 1.0
    vec2 fade = smoothstep(edgeFadeStart, 1.0, distFromCenter);
    
    // Use the maximum fade value between X and Y axes.
    // We want to fade if the point is close to ANY edge.
    // If fade.x is high (close to X-edge) or fade.y is high (close to Y-edge),
    // max(fade.x, fade.y) will be high.
    // 1.0 - max(...) inverts this, so it's 1.0 (no fade) in the center and 0.0 (full fade) at edges.
    return 1.0 - max(fade.x, fade.y);
}

/**
 * @brief Calculates the approximate number of mipmap levels for a texture of a given resolution.
 * This can be used to determine a suitable blur radius or LOD for texture sampling.
 * @param resolution The 2D resolution of the texture (width, height).
 * @return float The estimated number of mipmap levels.
 */
float calculateMipmapLevels(vec2 resolution) {
    // Find the larger dimension of the texture.
    float maxDimension = max(resolution.x, resolution.y);
    // The number of mip levels is related to log2 of the largest dimension.
    // Adding 1.0 accounts for the base level (mip 0).
    return floor(log2(maxDimension)) + 1.0;
}

/**
 * @brief Processes a confirmed ray intersection, calculating hit color, depth, and edge fade.
 * This function encapsulates the logic for sampling the source texture and applying debug visualization.
 * @param screenPos Current screen position (texture coordinates) of the hit.
 * @param hitScreenDepth Depth value from the scene's depth buffer at the hit point (linear view space Z).
 * @param signedDelta Signed difference between the ray's current depth and the scene's depth. Used for debug coloring.
 * @param texSource The texture (e.g., scene color) to sample for the reflection color.
 * @param reflRoughness Roughness of the reflecting surface, used to adjust mip level for blurring.
 * @param outHitColor Output: The calculated color of the reflection.
 * @param outHitDepthValue Output: The depth of the reflection hit.
 * @param outEdgeFactor Output: The edge fade factor calculated at the hit position.
 */
void processRayIntersection(
    vec2 screenPos,
    float hitScreenDepth,
    float signedDelta,
    sampler2D texSource,
    float reflRoughness,
    out vec4 outHitColor,
    out float outHitDepthValue,
    out float outEdgeFactor
)
{
    vec4 color = vec4(1.0); // Default color multiplier.
    if(debugDraw) {
        // If debug drawing is enabled, colorize based on the sign of delta.
        // Helps visualize if the ray hit in front of or behind the surface.
        color = vec4(0.5 + sign(signedDelta) / 2.0, 0.3, 0.5 - sign(signedDelta) / 2.0, 0.0);
    }
        
    // Calculate mip level for texture sampling based on screen resolution and roughness.
    // Higher roughness leads to sampling from higher (blurrier) mip levels.
    float mipLevel = calculateMipmapLevels(screen_res) * reflRoughness;
    
    // Sample the source texture at the hit position using the calculated mip level.
    outHitColor = textureLod(texSource, screenPos, mipLevel) * color;
    outHitColor.a = 1.0; // Ensure full opacity for the hit.
    outHitDepthValue = hitScreenDepth; // Store the depth of the hit.
    outEdgeFactor = calculateEdgeFade(screenPos); // Calculate edge fade at the hit position.
}

/**
 * @brief Checks if the ray's projected screen position is outside the [0,1] screen bounds.
 * If off-screen, it calculates and updates the edge fade factor.
 * @param screenPos The ray's current projected screen position (texture coordinates).
 * @param currentEdgeFade Input/Output: The current edge fade value, which will be updated if the ray is off-screen.
 * @return bool True if the ray is off-screen, false otherwise.
 */
bool checkAndUpdateOffScreen(vec2 screenPos, inout float currentEdgeFade)
{
    if (screenPos.x > 1.0 || screenPos.x < 0.0 ||
        screenPos.y > 1.0 || screenPos.y < 0.0)
    {
        // If off-screen, clamp the position to the edges and calculate edge fade.
        vec2 clampedPos = clamp(screenPos, 0.0, 1.0);
        currentEdgeFade = calculateEdgeFade(clampedPos);
        return true; // Is off-screen
    }
    return false; // Is on-screen
}

/**
 * @brief Checks if the ray is intersecting too close to the surface it originated from (self-intersection).
 * This helps prevent artifacts where a reflection ray immediately hits its own surface.
 * @param reflectingSurfaceDepth The view space Z depth of the surface from which the ray originates.
 * @param currentRayMarchDepth The view space Z depth of the scene surface at the ray's current projected position.
 * @param depthEpsilon A small tolerance value for the depth comparison.
 * @return bool True if a self-intersection is detected, false otherwise.
 */
bool checkSelfIntersection(float reflectingSurfaceDepth, float currentRayMarchDepth, float depthEpsilon)
{
    // Check if the ray's current depth is within epsilon distance of the original surface's depth.
    return reflectingSurfaceDepth < currentRayMarchDepth + depthEpsilon &&
           reflectingSurfaceDepth > currentRayMarchDepth - depthEpsilon;
}

/**
 * @brief Advances the ray marching position based on adaptive and/or exponential stepping logic.
 * @param deltaFromSurface Current difference between the ray's depth and the scene depth at the projected point.
 *                         Negative means ray is in front of surface, positive means behind.
 * @param currentMarchingPos Input/Output: The current 3D position of the ray in view space. Will be updated.
 * @param currentBaseStepVec Input/Output: The base step vector. Its direction is the ray direction. Its magnitude
 *                           can be scaled by exponential stepping and provides a reference for adaptive steps.
 * @param minStepLenScalar The minimum step length ('rayStep' uniform), a scalar.
 * @param useAdaptiveStepping Boolean flag to enable adaptive stepping.
 * @param useExponentialStepping Boolean flag to enable exponential step scaling.
 * @param expStepMultiplier Multiplier for exponential step scaling ('adaptiveStepMultiplier' uniform).
 */
void advanceRayMarch(
    float deltaFromSurface,
    inout vec3 currentMarchingPos,
    inout vec3 currentBaseStepVec,
    float minStepLenScalar,
    bool useAdaptiveStepping,
    bool useExponentialStepping,
    float expStepMultiplier
)
{
    vec3 actualMarchingVector;
    
    // Calculate a minimum step size based on the current position's depth
    // This prevents steps that are smaller than the depth buffer's precision
    float currentDepth = abs(currentMarchingPos.z);
    float depthBasedMinStep = max(minStepLenScalar, currentDepth * 0.001); // 0.1% of current depth
    
    if (useAdaptiveStepping) {
        if (deltaFromSurface < 0.0f) { // Ray is in front of the surface
            vec3 stepDir = normalize(currentBaseStepVec);
            if (abs(stepDir.z) > 0.0001f) { // Avoid division by zero if ray is mostly horizontal
                // Project the Z-difference onto the ray's direction to estimate distance to intersection.
                float distToPotentialIntersection = abs(deltaFromSurface) / abs(stepDir.z);
                // Determine adaptive step length with enhanced minimum:
                float adaptiveLength = clamp(distToPotentialIntersection * 0.75f,
                                             depthBasedMinStep,
                                             length(currentBaseStepVec));
                actualMarchingVector = stepDir * adaptiveLength;
            } else { // Ray is mostly horizontal, use a conservative step.
                float stepLength = max(length(currentBaseStepVec) * 0.5f, depthBasedMinStep);
                actualMarchingVector = stepDir * stepLength;
            }
        } else { // deltaFromSurface >= 0.0f (Ray is at or behind the surface)
            float directionSign = sign(deltaFromSurface);
            // Ensure retreat step is also not too small
            vec3 baseStepForRetreat = currentBaseStepVec * max(0.5f, 1.0f - minStepLenScalar * max(directionSign, 0.0f));
            actualMarchingVector = baseStepForRetreat * (-directionSign);
            
            // Ensure minimum retreat distance
            if (length(actualMarchingVector) < depthBasedMinStep) {
                actualMarchingVector = normalize(actualMarchingVector) * depthBasedMinStep;
            }
        }
    } else { // Not adaptive stepping, use the current base step vector.
        actualMarchingVector = currentBaseStepVec;
        // But still enforce minimum step size
        if (length(actualMarchingVector) < depthBasedMinStep) {
            actualMarchingVector = normalize(actualMarchingVector) * depthBasedMinStep;
        }
    }
    
    currentMarchingPos += actualMarchingVector; // Advance the ray position.

    if (useExponentialStepping) {
        // If exponential stepping is enabled, increase the magnitude of the base step vector for subsequent iterations.
        currentBaseStepVec *= expStepMultiplier;
    }
}

const float MAX_Z_DEPTH = 256;
const float MAX_ROUGHNESS = 0.45;

/**
 * @brief Checks if a hit point is within the specified distance range from the reflector.
 * @param reflectorDepth The depth of the reflecting surface.
 * @param hitDepth The depth of the hit point.
 * @param rangeStart The start of the valid range (minimum distance from reflector).
 * @param rangeEnd The end of the valid range (maximum distance from reflector).
 * @return bool True if the hit is within the valid range, false otherwise.
 */
bool isWithinReflectionRange(float reflectorDepth, float hitDepth, float rangeStart, float rangeEnd)
{
    float distanceFromReflector = abs(hitDepth - reflectorDepth);
    return distanceFromReflector >= rangeStart && distanceFromReflector <= rangeEnd;
}

/**
 * @brief Traces a single reflection ray through the scene using screen-space ray marching.
 * It attempts to find an intersection with the scene geometry represented by a depth buffer.
 * Features include adaptive step size, exponential step increase, binary search refinement,
 * and edge fading.
 *
 * @param initialPosition The starting position of the ray in current view space.
 * @param initialReflection The initial reflection vector (direction) in current view space.
 * @param hitColor Output: If an intersection is found, this will be the color sampled from the 'source' texture at the hit point. Otherwise, it's (0,0,0,0).
 * @param hitDepth Output: If an intersection is found, this will be the linear view space Z depth of the hit point.
 * @param source The sampler2D (e.g., current frame's color buffer) to fetch reflection color from upon a hit.
 * @param edgeFade Output: A factor (0-1) indicating how close the ray path or hit point is to the screen edges. 1 means no fade.
 * @param roughness Surface roughness (0-1), used to adjust mip level for texture sampling to simulate blurriness.
 * @return bool True if the ray hits a surface, false otherwise.
 */
bool traceScreenRay(
    vec3 initialPosition,
    vec3 initialReflection,
    out vec4 hitColor,
    out float hitDepth,
    sampler2D source,
    out float edgeFade,
    float roughness,
    int passIterationCount,
    float passRayStep,
    float passDistanceBias,
    float passDepthRejectBias,
    float passAdaptiveStepMultiplier,
    float passDepthScaleExponent,
    vec2 distanceParams
)
{
    // Transform initialPosition and the target of initialReflection vector from current camera space to previous camera space.
    vec3 reflectionTargetPoint = initialPosition + initialReflection;
    vec3 currentPosition_transformed = (inv_modelview_delta * vec4(initialPosition, 1.0)).xyz;

    vec2 initialScreenPos = generateProjectedPosition(currentPosition_transformed);
    if (initialScreenPos.x < 0.0 || initialScreenPos.x > 1.0 ||
        initialScreenPos.y < 0.0 || initialScreenPos.y > 1.0) {
        hitColor = vec4(0.0);
        edgeFade = 0.0;
        hitDepth = 0.0;
        return false;
    }

    vec3 reflectionTarget_transformed = (inv_modelview_delta * vec4(reflectionTargetPoint, 1.0)).xyz;
    vec3 reflectionVector_transformed = reflectionTarget_transformed - currentPosition_transformed;

    // Depth of the reflecting surface in the transformed view space.
    float reflectingSurfaceViewDepth = -currentPosition_transformed.z;

    if (reflectingSurfaceViewDepth > MAX_Z_DEPTH) {
        // Do a sanity check: if the reflecting surface is too far away, skip ray tracing.
        hitColor = vec4(0.0);
        edgeFade = 0.0;
        hitDepth = 0.0;
        return false;
    }

    // Extract range parameters
    float rangeStart = distanceParams.x;
    float rangeEnd = distanceParams.y;

    // Initialize ray marching variables - NO SCALING
    vec3 normalizedReflection = normalize(reflectionVector_transformed);

    if (normalizedReflection.z >= 0.5) {
        hitColor = vec4(0.0);
        edgeFade = 0.0;
        hitDepth = 0.0;
        return false;
    }

    float depthScale = pow(reflectingSurfaceViewDepth, passDepthScaleExponent);
    vec3 baseStepVector = passRayStep * max(1.0, depthScale) * normalizedReflection;
    vec3 marchingPosition = currentPosition_transformed + baseStepVector; // First step from origin.
    
    float delta; // Difference between ray's Z depth and scene's Z depth at the projected screen position.
    vec2 screenPosition; // Ray's current projected 2D screen position.
    bool hit = false;    // Flag to indicate if an intersection was found.
    hitColor = vec4(0.0); // Initialize output hit color.
    edgeFade = 1.0;       // Initialize output edge fade.
    float depthFromScreen = 0.0; // Linear depth sampled from the sceneDepth texture.
    float furthestValidDepth = 0.0; // The furthest valid depth encountered during ray marching.
    int i = 0; // Iteration counter.
    // Only trace if the reflecting surface itself isn't too shallow (depthRejectBias).
    if (reflectingSurfaceViewDepth > passDepthRejectBias) {
        // --- Main Ray Marching Loop ---
        for (; i < int(passIterationCount) && !hit; i++) {
            // Project current 3D marching position to 2D screen space.
            screenPosition = generateProjectedPosition(marchingPosition);
            
            // Check if ray is off-screen. If so, update edgeFade and break.
            if (checkAndUpdateOffScreen(screenPosition, edgeFade)) {
                hit = false; // Ensure hit is false as ray went out of bounds.
                break;
            }
            
            // Get linear depth of the scene at the ray's projected screen position.
            depthFromScreen = getLinearDepth(screenPosition);

            // We store a max Z depth, and just kind of assume that the ray is intersecting with the sky if we hit it.
            if (depthFromScreen >= MAX_Z_DEPTH)
            {
                hit = false;
                break;
            }
            
            // Early termination if ray has traveled too far from the reflector
            float rayTravelDistance = abs(abs(marchingPosition.z) - reflectingSurfaceViewDepth);
            if (rayTravelDistance > rangeEnd) {
                hit = false;
                break;
            }

            // Calculate delta: difference between ray's absolute Z and scene's depth.
            delta = abs(marchingPosition.z) - depthFromScreen;

            // Check for self-intersection (ray hitting the surface it originated from).
            if (checkSelfIntersection(reflectingSurfaceViewDepth, depthFromScreen, epsilon)) {
                edgeFade = calculateEdgeFade(screenPosition); // Update edge fade even on self-intersection.
                hit = false; // This is not a valid reflection hit.
                break;
            }

            // Check for intersection: if absolute delta is within distanceBias.
            if (abs(delta) < passDistanceBias) {
                if (isWithinReflectionRange(reflectingSurfaceViewDepth, depthFromScreen, rangeStart, rangeEnd)) {
                    processRayIntersection(screenPosition, depthFromScreen, delta, source, roughness,
                                        hitColor, hitDepth, edgeFade);
                    if (hitDepth > furthestValidDepth) {
                        furthestValidDepth = hitDepth; // Update the furthest valid depth.
                    }
                    hit = true;
                    break; // Found a hit.
                }
            }
            
            // If ray overshot (delta > 0) and binary search is enabled, break to start binary search.
            if (isBinarySearchEnabled && delta > 0.0) {
                break;
            }

            // Advance the ray: calculate next step and update marchingPosition & baseStepVector.
            advanceRayMarch(delta, marchingPosition, baseStepVector,
                            passRayStep, isAdaptiveStepEnabled, isExponentialStepEnabled, passAdaptiveStepMultiplier);
        }
        
        // --- Binary Search Refinement Loop ---
        // Perform binary search if enabled, the main loop overshot (delta > 0), and no hit was found yet.
        if (isBinarySearchEnabled && delta > 0.0 && !hit) {
            vec3 binarySearchBaseStep = baseStepVector; // Start with the step that caused the overshoot.
            
            // Continue iteration count from where the main loop left off.
            for (; i < int(passIterationCount) && !hit; i++) {
                binarySearchBaseStep *= 0.5; // Halve the step size for refinement.
                // Move back or forward along the ray based on the sign of the last known delta.
                marchingPosition = marchingPosition - binarySearchBaseStep * sign(delta);

                screenPosition = generateProjectedPosition(marchingPosition);
                if (checkAndUpdateOffScreen(screenPosition, edgeFade)) {
                    hitColor.a = 0.0; // Indicate no valid color if off-screen.
                    hit = false;
                    break;
                }
                
                depthFromScreen = getLinearDepth(screenPosition);

                if (depthFromScreen >= MAX_Z_DEPTH)
                {
                    hit = false;
                    break;
                }
                
                // Check if we've traveled too far
                float rayTravelDistance = abs(abs(marchingPosition.z) - reflectingSurfaceViewDepth);
                if (rayTravelDistance > rangeEnd) {
                    hit = false;
                    break;
                }
                
                // Recalculate delta for this binary search iteration.
                float currentBinarySearchDelta = abs(marchingPosition.z) - depthFromScreen;

                if (checkSelfIntersection(reflectingSurfaceViewDepth, depthFromScreen, epsilon)) {
                    edgeFade = calculateEdgeFade(screenPosition);
                    hit = false;
                    break;
                }

                // Check for hit using the refined delta.
                if (abs(currentBinarySearchDelta) < passDistanceBias &&
                    depthFromScreen != (reflectingSurfaceViewDepth - passDistanceBias)) {
                    if (isWithinReflectionRange(reflectingSurfaceViewDepth, depthFromScreen, rangeStart, rangeEnd)) {
                        processRayIntersection(screenPosition, depthFromScreen, currentBinarySearchDelta, source, roughness,
                                               hitColor, hitDepth, edgeFade);
                        
                        if (hitDepth > furthestValidDepth) {
                            furthestValidDepth = hitDepth; // Update the furthest valid depth.
                        }
                        hit = true;
                        break;
                    }
                }
                delta = currentBinarySearchDelta; // Update delta for the next binary search refinement step.
            }
        }
    }

    // Do a bit of distance fading if we have a hit.  Use it to fade out far off objects that just don't look right.
    if (hit) {
        float zFadeStart = MAX_Z_DEPTH * 0.8;
        float zFade = 1.0 - smoothstep(zFadeStart, MAX_Z_DEPTH, furthestValidDepth);

        // Combine both fades (multiply for stronger effect)
        edgeFade *= zFade;

    }

    return hit; // Return whether a valid intersection was found.
}

/**
 * @brief Handles the actual tracing logic for screen space reflections, including multi-sample glossy reflections.
 *
 * @param viewPos The view space position of the current pixel/surface.
 * @param rayDirection The perfect reflection direction (already calculated).
 * @param tc The texture coordinates of the current pixel being processed (screen space, 0-1).
 * @param tracedColor Output: The accumulated reflection color. Alpha component is used for final fade/intensity.
 * @param source The sampler2D (e.g., sceneMap) from which to sample reflection colors.
 * @param roughness The roughness of the surface (0.0 for smooth, 1.0 for rough).
 * @param iterations The number of ray marching iterations to perform.
 * @param passRayStep The step size for ray marching.
 * @param passDistanceBias The distance bias for intersection detection.
 * @param passDepthRejectBias The depth rejection bias.
 * @param passAdaptiveStepMultiplier The multiplier for adaptive stepping.
 * @return bool True if at least one ray hit was successful, false otherwise.
 */
bool tracePass(vec3 viewPos, vec3 rayDirection, vec2 tc, inout vec4 tracedColor, sampler2D source,
               float roughness, int iterations, float passRayStep, float passDistanceBias,
               float passDepthRejectBias, float passAdaptiveStepMultiplier, float passDepthScaleExponent, vec2 distanceParams)
{
    tracedColor = vec4(0.0); // Initialize accumulated color.
    int hits = 0;            // Counter for successful ray hits.
    float cumulativeFade = 0.0; // Accumulator for edge fade factors from successful rays.
    float averageHitDepth = 0.0;  // Accumulator for hit depths.

    // Adjust the number of samples based on roughness.
    // Smoother surfaces get more samples.
    int totalSamples = int(max(float(glossySampleCount), float(glossySampleCount) * (1.0 - roughness)));
    totalSamples = max(totalSamples, 1); // Ensure at least one sample.
    vec2 blurOffset = vec2(0);
    int pixelSeed = int(mod(dot(tc * screen_res, vec2(37.0, 17.0)), 128.0));
    for (int i = 0; i < totalSamples; i++) {
        float temporalJitter = noiseSine; // Arbitrary multiplier for decorrelation

        float u1 = random(tc + vec2(i, 0.123) + temporalJitter);
        float u2 = random(tc + vec2(0.456, i) + temporalJitter);
        float jitter = random(tc + vec2(i * 0.777, 0.888) + temporalJitter); // Additional random jitter

        float alpha = max(roughness * roughness, 0.0001); // Already using alpha^2, clamp to avoid div by zero
        float theta = atan(alpha * sqrt(u1) / sqrt(1.0 - u1));
        // Slightly jitter phi with a random offset, scaled by roughness for more effect on rough surfaces
        float phi = 2.0 * 3.14159265 * (u2 + jitter * 0.15 * roughness);

        // Build tangent space
        vec3 up = abs(rayDirection.y) < 0.999 ? vec3(0,1,0) : vec3(1,0,0);
        vec3 tangent = normalize(cross(up, rayDirection));
        vec3 bitangent = cross(rayDirection, tangent);

        // GGX half-vector in tangent space
        vec3 h = normalize(
            sin(theta) * cos(phi) * tangent +
            sin(theta) * sin(phi) * bitangent +
            cos(theta) * rayDirection
        );

        // Reflect view vector about GGX half-vector to get the final sample direction
        vec3 reflectionDirectionRandomized = normalize(reflect(-rayDirection, h));

        float hitDepthVal;    // Stores depth of the hit for this ray.
        vec4 hitpointColor; // Stores color of the hit for this ray.
        float rayEdgeFadeVal; // Stores edge fade for this ray.

        bool hit = traceScreenRay(viewPos, reflectionDirectionRandomized, hitpointColor,
                                hitDepthVal, source, rayEdgeFadeVal, roughness * 2.0,
                                iterations, passRayStep, passDistanceBias, passDepthRejectBias, passAdaptiveStepMultiplier, passDepthScaleExponent, distanceParams);
        
        if (hit) {
            ++hits;
            tracedColor.rgb += hitpointColor.rgb; // Accumulate color.
            tracedColor.a += 1.0;                 // Using alpha to count hits for averaging, or for intensity.
            cumulativeFade += rayEdgeFadeVal;        // Accumulate edge fade.
            averageHitDepth += hitDepthVal;          // Accumulate hit depth.
        }
    }

    if (hits > 0) {
        // Average the accumulated values if any rays hit.
        tracedColor /= float(hits); // Average color. Note: alpha also gets divided.
        cumulativeFade /= float(hits);   // Average edge fade.
        averageHitDepth /= float(hits);  // Average hit depth.
        // Store the average edge fade in the alpha channel for the caller to use.
        tracedColor.a = cumulativeFade;
    } else {
        // No hits, result is black and no fade.
        tracedColor = vec4(0.0);
    }
    
    return hits > 0; // Return true if at least one ray hit was successful.
}

/**
 * @brief Main function to compute screen space reflections for a given screen pixel.
 * It traces multiple rays for glossy reflections, accumulates results, and applies various fading effects.
 *
 * @param totalSamples The desired number of samples for glossy reflections (can be adjusted internally).
 * @param tc The texture coordinates of the current pixel being processed (screen space, 0-1).
 * @param viewPos The view space position of the current pixel/surface.
 * @param n The view space normal of the current pixel/surface.
 * @param collectedColor Output: The accumulated reflection color. Alpha component is used for final fade/intensity.
 * @param source The sampler2D (e.g., sceneMap) from which to sample reflection colors.
 * @param glossiness The glossiness of the surface (0.0 for rough, 1.0 for perfectly smooth/mirror).  Gets converted to roughness internally.
 * @return float The number of rays that successfully hit a surface.
 */
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
    
    if (roughness < MAX_ROUGHNESS) {

        float viewDotNormal = dot(normalize(-viewPos), normalize(n));
        if (viewDotNormal <= 0.0) {
            collectedColor = vec4(0.0);
            return 0.0;
        }

        float remappedRoughness = clamp((roughness - (MAX_ROUGHNESS * 0.6)) / (MAX_ROUGHNESS - (MAX_ROUGHNESS * 0.6)), 0.0, 1.0);
        float roughnessIntensityFade = 1.0 - remappedRoughness;

        float roughnessFade = roughnessIntensityFade;
        float currentPixelViewDepth = -viewPos.z;
        
        vec2 distFromCenter = abs(tc * 2.0 - 1.0);
        float baseEdgeFade = 1.0 - smoothstep(0.85, 1.0, max(distFromCenter.x, distFromCenter.y));
        
        if (baseEdgeFade > 0.001) {
            vec3 rayDirection = normalize(reflect(normalize(viewPos), normalize(n)));

            float angleFactor = viewDotNormal;
            float angleFactorSq = angleFactor * angleFactor;
            
            float combinedFade = roughnessFade;// distanceFactor;
            combinedFade *= 1 - angleFactorSq;
            
            vec4 nearColor = vec4(0.0);
            vec4 midColor = vec4(0.0);
            vec4 farColor = vec4(0.0);
            bool hasNearHits = false;
            bool hasMidHits = false;
            bool hasFarHits = false;
            
            float stepRoughnesMultiplier = mix(1.0, 5.0, roughness);

            // Near pass
            vec2 distanceParams = vec2(splitParamsStart.x, splitParamsEnd.x);
            hasNearHits = tracePass(viewPos, rayDirection, tc, nearColor, source, roughness,
                                    int(iterationCount.x), rayStep.x, distanceBias.x,
                                    depthRejectBias.x, adaptiveStepMultiplier.x, 0.6 * stepRoughnesMultiplier, distanceParams);
            
            // Mid pass
            distanceParams = vec2(splitParamsStart.y, splitParamsEnd.y);
            hasMidHits = tracePass(viewPos, rayDirection, tc, midColor, source, roughness,
                                   int(iterationCount.y), rayStep.y, distanceBias.y,
                                   depthRejectBias.y, adaptiveStepMultiplier.y, 0.35 * stepRoughnesMultiplier, distanceParams);
            
            // Far pass
            distanceParams = vec2(splitParamsStart.z, splitParamsEnd.z);
            hasFarHits = tracePass(viewPos, rayDirection, tc, farColor, source, roughness,
                                   int(iterationCount.z), rayStep.z, distanceBias.z,
                                   depthRejectBias.z, adaptiveStepMultiplier.z, 0.1 * stepRoughnesMultiplier, distanceParams);
            
            // Combine results from all three passes
            collectedColor = vec4(0.0);
            float totalWeight = 0.0;
            
            // Use a priority system: prefer near hits, then mid, then far
            if (hasNearHits) {
                collectedColor = nearColor;
                totalWeight = 1.0;
            } else if (hasMidHits) {
                collectedColor = midColor;
                totalWeight = 1.0;
            } else if (hasFarHits) {
                collectedColor = farColor;
                totalWeight = 1.0;
            }
            
            if (totalWeight > 0.0) {
                float finalEdgeFade = collectedColor.a * combinedFade * baseEdgeFade;
                collectedColor.a = finalEdgeFade;
                return 1.0;
            } else {
                collectedColor = vec4(0.0);
                return 0.0;
            }
        } else {
            collectedColor = vec4(0.0);
            return 0.0;
        }
    }

    return 0;
}
