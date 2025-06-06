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

/** @brief Sampler for the previous frame's scene color. Potentially used for temporal reprojection or other effects. */
uniform sampler2D prevSceneMap;
/** @brief Sampler for the current frame's scene color. This is what reflections will be sampled from. */
uniform sampler2D sceneMap;
/** @brief Sampler for the current frame's scene depth. Used to find intersections during ray marching. */
uniform sampler2D sceneDepth;

/** @brief Resolution of the screen in pixels (width, height). */
uniform vec2 screen_res;
/** @brief The current view's projection matrix. Transforms view space coordinates to clip space. */
uniform mat4 projection_matrix;
//uniform float zNear; // Near clipping plane distance (commented out, likely unused or implicitly handled)
/** @brief Far clipping plane distance. Used in depth calculations. */
uniform float zFar;
/** @brief The inverse of the projection matrix. Transforms clip space coordinates back to view space. */
uniform mat4 inv_proj;
/** @brief Transformation matrix from the last frame's camera space to the current frame's camera space. Used for temporal reprojection or motion vector calculations. */
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

// --- SSR Algorithm Configuration Flags ---
// These booleans control various optimizations and features of the SSR algorithm.

/** @brief If true, a binary search step is performed to refine the hit point after an initial overshoot. */
bool isBinarySearchEnabled = true;
/** @brief If true, the ray marching step size is adapted based on the distance to the nearest surface. */
bool isAdaptiveStepEnabled = true;
/** @brief If true, the base ray marching step size increases exponentially with each step. */
bool isExponentialStepEnabled = true;
/** @brief If true, debug colors are used to visualize aspects of the ray marching process (e.g., delta values). */
bool debugDraw = false;

// --- SSR Algorithm Parameters (controlled by debug settings) ---

/** @brief Maximum number of iterations for the ray marching loops. Debug setting: RenderScreenSpaceReflectionIterations */
uniform float iterationCount;
/** @brief Initial base step size for ray marching. Debug setting: RenderScreenSpaceReflectionRayStep */
uniform float rayStep;
/** @brief Threshold for considering a ray hit. If the distance between the ray and the scene surface is less than this, it's a hit. Debug setting: RenderScreenSpaceReflectionDistanceBias */
uniform float distanceBias;
/** @brief Bias to prevent self-reflection or reflections from surfaces very close to the ray origin. Rays originating from surfaces shallower than this bias are rejected. Debug setting: RenderScreenSpaceReflectionDepthRejectionBias */
uniform float depthRejectBias;
/** @brief Number of samples to take for glossy reflections. Higher values give smoother but more expensive reflections. Debug setting: RenderScreenSpaceReflectionGlossySamples */
uniform float glossySampleCount;
/** @brief Multiplier for the adaptive step size calculation. Debug setting: RenderScreenSpaceReflectionAdaptiveStepMultiplier */
uniform float adaptiveStepMultiplier;
/** @brief A time-varying sine value, likely used to introduce temporal variation in noise patterns (e.g., for Poisson disk sampling). */
uniform float noiseSine;

/** @brief A small constant value used for floating-point comparisons, typically to avoid issues with precision. Used in self-intersection checks. */
float epsilon = 0.1;

/**
 * @brief Samples the scene depth texture and converts it to linear view space Z depth.
 * @param tc Texture coordinates (screen space, 0-1 range) at which to sample the depth.
 * @return float The linear depth value (typically negative, representing distance from camera in view space).
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

/** @brief Predefined array of 128 3D Poisson disk sample offsets. Used for generating distributed samples, e.g., for glossy reflections. Values are in [0,1] range. */
uniform vec3 POISSON3D_SAMPLES[128] = vec3[128](
    // ... (array values as provided) ...
    vec3(0.5433144, 0.1122154, 0.2501391),
    vec3(0.6575254, 0.721409, 0.16286),
    vec3(0.02888453, 0.05170321, 0.7573566),
    vec3(0.06635678, 0.8286457, 0.07157445),
    vec3(0.8957489, 0.4005505, 0.7916042),
    vec3(0.3423355, 0.5053263, 0.9193521),
    vec3(0.9694794, 0.9461077, 0.5406441),
    vec3(0.9975473, 0.02789414, 0.7320132),
    vec3(0.07781899, 0.3862341, 0.918594),
    vec3(0.4439073, 0.9686955, 0.4055861),
    vec3(0.9657035, 0.6624081, 0.7082613),
    vec3(0.7712346, 0.07273269, 0.3292839),
    vec3(0.2489169, 0.2550394, 0.1950516),
    vec3(0.7249326, 0.9328285, 0.3352458),
    vec3(0.6028461, 0.4424961, 0.5393377),
    vec3(0.2879795, 0.7427881, 0.6619173),
    vec3(0.3193627, 0.0486145, 0.08109283),
    vec3(0.1233155, 0.602641, 0.4378719),
    vec3(0.9800708, 0.211729, 0.6771586),
    vec3(0.4894537, 0.3319927, 0.8087631),
    vec3(0.4802743, 0.6358885, 0.814935),
    vec3(0.2692913, 0.9911493, 0.9934899),
    vec3(0.5648789, 0.8553897, 0.7784553),
    vec3(0.8497344, 0.7870212, 0.02065313),
    vec3(0.7503014, 0.2826185, 0.05412734),
    vec3(0.8045461, 0.6167251, 0.9532926),
    vec3(0.04225039, 0.2141281, 0.8678675),
    vec3(0.07116079, 0.9971236, 0.3396397),
    vec3(0.464099, 0.480959, 0.2775862),
    vec3(0.6346927, 0.31871, 0.6588384),
    vec3(0.449012, 0.8189669, 0.2736875),
    vec3(0.452929, 0.2119148, 0.672004),
    vec3(0.01506042, 0.7102436, 0.9800494),
    vec3(0.1970513, 0.4713539, 0.4644522),
    vec3(0.13715, 0.7253224, 0.5056525),
    vec3(0.9006432, 0.5335414, 0.02206874),
    vec3(0.9960898, 0.7961011, 0.01468861),
    vec3(0.3386469, 0.6337739, 0.9310676),
    vec3(0.1745718, 0.9114985, 0.1728188),
    vec3(0.6342545, 0.5721557, 0.4553517),
    vec3(0.1347412, 0.1137158, 0.7793725),
    vec3(0.3574478, 0.3448052, 0.08741581),
    vec3(0.7283059, 0.4753885, 0.2240275),
    vec3(0.8293507, 0.9971212, 0.2747005),
    vec3(0.6501846, 0.000688076, 0.7795712),
    vec3(0.01149416, 0.4930083, 0.792608),
    vec3(0.666189, 0.1875442, 0.7256873),
    vec3(0.8538797, 0.2107637, 0.1547532),
    vec3(0.5826825, 0.9750752, 0.9105834),
    vec3(0.8914346, 0.08266425, 0.5484225),
    vec3(0.4374518, 0.02987111, 0.7810078),
    vec3(0.2287418, 0.1443802, 0.1176908),
    vec3(0.2671157, 0.8929081, 0.8989366),
    vec3(0.5425819, 0.5524959, 0.6963879),
    vec3(0.3515188, 0.8304397, 0.0502702),
    vec3(0.3354864, 0.2130747, 0.141169),
    vec3(0.9729427, 0.3509927, 0.6098799),
    vec3(0.7585629, 0.7115368, 0.9099342),
    vec3(0.0140543, 0.6072157, 0.9436461),
    vec3(0.9190664, 0.8497264, 0.1643751),
    vec3(0.1538157, 0.3219983, 0.2984214),
    vec3(0.8854713, 0.2968667, 0.8511457),
    vec3(0.1910622, 0.03047311, 0.3571215),
    vec3(0.2456353, 0.5568692, 0.3530164),
    vec3(0.6927255, 0.8073994, 0.5808484),
    vec3(0.8089353, 0.8969175, 0.3427134),
    vec3(0.194477, 0.7985603, 0.8712182),
    vec3(0.7256182, 0.5653068, 0.3985921),
    vec3(0.9889427, 0.4584851, 0.8363391),
    vec3(0.5718582, 0.2127113, 0.2950557),
    vec3(0.5480209, 0.0193435, 0.2992659),
    vec3(0.6598953, 0.09478426, 0.92187),
    vec3(0.1385615, 0.2193868, 0.205245),
    vec3(0.7623423, 0.1790726, 0.1508465),
    vec3(0.7569032, 0.3773386, 0.4393887),
    vec3(0.5842971, 0.6538072, 0.5224424),
    vec3(0.9954313, 0.5763943, 0.9169143),
    vec3(0.001311183, 0.340363, 0.1488652),
    vec3(0.8167927, 0.4947158, 0.4454727),
    vec3(0.3978434, 0.7106082, 0.002727509),
    vec3(0.5459411, 0.7473233, 0.7062873),
    vec3(0.4151598, 0.5614617, 0.4748358),
    vec3(0.4440694, 0.1195122, 0.9624678),
    vec3(0.1081301, 0.4813806, 0.07047641),
    vec3(0.2402785, 0.3633997, 0.3898734),
    vec3(0.2317942, 0.6488295, 0.4221864),
    vec3(0.01145542, 0.9304277, 0.4105759),
    vec3(0.3563728, 0.9228861, 0.3282344),
    vec3(0.855314, 0.6949819, 0.3175117),
    vec3(0.730832, 0.01478493, 0.5728671),
    vec3(0.9304829, 0.02653277, 0.712552),
    vec3(0.4132186, 0.4127623, 0.6084146),
    vec3(0.7517329, 0.9978395, 0.1330464),
    vec3(0.5210338, 0.4318751, 0.9721575),
    vec3(0.02953994, 0.1375937, 0.9458942),
    vec3(0.1835506, 0.9896691, 0.7919457),
    vec3(0.3857062, 0.2682322, 0.1264563),
    vec3(0.6319699, 0.8735335, 0.04390657),
    vec3(0.5630485, 0.3339024, 0.993995),
    vec3(0.90701, 0.1512893, 0.8970422),
    vec3(0.3027443, 0.1144253, 0.1488708),
    vec3(0.9149003, 0.7382028, 0.7914025),
    vec3(0.07979286, 0.6892691, 0.2866171),
    vec3(0.7743186, 0.8046008, 0.4399814),
    vec3(0.3128662, 0.4362317, 0.6030678),
    vec3(0.1133721, 0.01605821, 0.391872),
    vec3(0.5185481, 0.9210006, 0.7889017),
    vec3(0.8217013, 0.325305, 0.1668191),
    vec3(0.8358996, 0.1449739, 0.3668382),
    vec3(0.1778213, 0.5599256, 0.1327691),
    vec3(0.06690693, 0.5508637, 0.07212365),
    vec3(0.9750564, 0.284066, 0.5727578),
    vec3(0.4350255, 0.8949825, 0.03574753),
    vec3(0.8931149, 0.9177974, 0.8123496),
    vec3(0.9055127, 0.989903, 0.813235),
    vec3(0.2897243, 0.3123978, 0.5083504),
    vec3(0.1519223, 0.3958645, 0.2640327),
    vec3(0.6840154, 0.6463035, 0.2346607),
    vec3(0.986473, 0.8714055, 0.3960275),
    vec3(0.6819352, 0.4169535, 0.8379834),
    vec3(0.9147297, 0.6144146, 0.7313942),
    vec3(0.6554981, 0.5014008, 0.9748477),
    vec3(0.9805915, 0.1318207, 0.2371372),
    vec3(0.5980836, 0.06796348, 0.9941338),
    vec3(0.6836596, 0.9917196, 0.2319056),
    vec3(0.5276511, 0.2745509, 0.5422578),
    vec3(0.829482, 0.03758276, 0.1240466),
    vec3(0.2698198, 0.0002266169, 0.3449324)
);

/**
 * @brief Retrieves a Poisson disk sample from the predefined array and maps it to the [-1, 1] range.
 * @param i The index of the sample to retrieve (should be within the bounds of POISSON3D_SAMPLES).
 * @return vec3 The 3D sample vector, with components in the range [-1, 1].
 */
vec3 getPoissonSample(int i) {
    // Samples are stored in [0,1], scale and shift to [-1,1].
    return POISSON3D_SAMPLES[i] * 2.0 - 1.0;
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
    out float outEdgeFactor)
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
    if (useAdaptiveStepping) {
        if (deltaFromSurface < 0.0f) { // Ray is in front of the surface
            vec3 stepDir = normalize(currentBaseStepVec);
            if (abs(stepDir.z) > 0.0001f) { // Avoid division by zero if ray is mostly horizontal
                // Project the Z-difference onto the ray's direction to estimate distance to intersection.
                float distToPotentialIntersection = abs(deltaFromSurface) / abs(stepDir.z);
                // Determine adaptive step length:
                // - At least minStepLenScalar.
                // - Try to step a fraction (e.g., 75%) towards potential intersection.
                // - No more than the current length of currentBaseStepVec.
                float adaptiveLength = clamp(distToPotentialIntersection * 0.75f,
                                             minStepLenScalar,
                                             length(currentBaseStepVec));
                actualMarchingVector = stepDir * adaptiveLength;
            } else { // Ray is mostly horizontal, use a conservative step.
                actualMarchingVector = stepDir * max(length(currentBaseStepVec) * 0.5f, minStepLenScalar);
            }
        } else { // deltaFromSurface >= 0.0f (Ray is at or behind the surface, but not yet a 'hit' or triggering binary search)
                 // This typically involves a retreat if the ray overshot.
            float directionSign = sign(deltaFromSurface); // 0 if at surface, 1 if behind.
            // Form a retreat vector.
            vec3 baseStepForRetreat = currentBaseStepVec * (1.0f - minStepLenScalar * max(directionSign, 0.0f));
            actualMarchingVector = baseStepForRetreat * (-directionSign); // Retreat if delta > 0.
        }
    } else { // Not adaptive stepping, use the current base step vector.
        actualMarchingVector = currentBaseStepVec;
    }
    
    currentMarchingPos += actualMarchingVector; // Advance the ray position.

    if (useExponentialStepping) {
        // If exponential stepping is enabled, increase the magnitude of the base step vector for subsequent iterations.
        currentBaseStepVec *= expStepMultiplier;
    }
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
    float roughness)
{
    // Transform initialPosition and the target of initialReflection vector from current camera space to previous camera space.
    // This is crucial if 'source' texture and 'sceneDepth' are from the previous frame's perspective,
    // or if a consistent coordinate system is needed for ray marching against 'inv_modelview_delta'-transformed geometry.
    vec3 reflectionTargetPoint = initialPosition + initialReflection;
    vec3 currentPosition_transformed = (inv_modelview_delta * vec4(initialPosition, 1.0)).xyz;
    vec3 reflectionTarget_transformed = (inv_modelview_delta * vec4(reflectionTargetPoint, 1.0)).xyz;
    vec3 reflectionVector_transformed = reflectionTarget_transformed - currentPosition_transformed;

    // Depth of the reflecting surface in the transformed view space.
    float reflectingSurfaceViewDepth = -currentPosition_transformed.z;

    // Initialize ray marching variables.
    // 'baseStepVector' starts with 'rayStep' magnitude in the direction of the (transformed) reflection.
    vec3 baseStepVector = rayStep * reflectionVector_transformed;
    vec3 marchingPosition = currentPosition_transformed + baseStepVector; // First step from origin.
    
    float delta; // Difference between ray's Z depth and scene's Z depth at the projected screen position.
    vec2 screenPosition; // Ray's current projected 2D screen position.
    bool hit = false;    // Flag to indicate if an intersection was found.
    hitColor = vec4(0.0); // Initialize output hit color.
    edgeFade = 1.0;       // Initialize output edge fade.
    float depthFromScreen = 0.0; // Linear depth sampled from the sceneDepth texture.

    int i = 0; // Iteration counter.
    // Only trace if the reflecting surface itself isn't too shallow (depthRejectBias).
    if (reflectingSurfaceViewDepth > depthRejectBias) {
        // --- Main Ray Marching Loop ---
        for (; i < int(iterationCount) && !hit; i++) {
            // Project current 3D marching position to 2D screen space.
            screenPosition = generateProjectedPosition(marchingPosition);
            
            // Check if ray is off-screen. If so, update edgeFade and break.
            if (checkAndUpdateOffScreen(screenPosition, edgeFade)) {
                hit = false; // Ensure hit is false as ray went out of bounds.
                break;
            }
            
            // Get linear depth of the scene at the ray's projected screen position.
            depthFromScreen = getLinearDepth(screenPosition);
            // Calculate delta: difference between ray's absolute Z and scene's depth.
            // Assumes marchingPosition.z and depthFromScreen are comparable (e.g., both positive view depths or both negative view Z).
            // Original code uses abs(marchingPosition.z), implying positive view depth convention.
            delta = abs(marchingPosition.z) - depthFromScreen;

            // Check for self-intersection (ray hitting the surface it originated from).
            if (checkSelfIntersection(reflectingSurfaceViewDepth, depthFromScreen, epsilon)) {
                edgeFade = calculateEdgeFade(screenPosition); // Update edge fade even on self-intersection.
                hit = false; // This is not a valid reflection hit.
                break;
            }

            // Check for intersection: if absolute delta is within distanceBias.
            if (abs(delta) < distanceBias) {
                processRayIntersection(screenPosition, depthFromScreen, delta, source, roughness,
                                       hitColor, hitDepth, edgeFade);
                hit = true;
                break; // Found a hit.
            }
            
            // If ray overshot (delta > 0) and binary search is enabled, break to start binary search.
            if (isBinarySearchEnabled && delta > 0.0) {
                break;
            }

            // Advance the ray: calculate next step and update marchingPosition & baseStepVector.
            advanceRayMarch(delta, marchingPosition, baseStepVector,
                            rayStep, isAdaptiveStepEnabled, isExponentialStepEnabled, adaptiveStepMultiplier);
        }
        
        // --- Binary Search Refinement Loop ---
        // Perform binary search if enabled, the main loop overshot (delta > 0), and no hit was found yet.
        // 'delta' and 'baseStepVector' hold their values from the last iteration of the main loop.
        if (isBinarySearchEnabled && delta > 0.0 && !hit) {
            vec3 binarySearchBaseStep = baseStepVector; // Start with the step that caused the overshoot.
            
            // Continue iteration count from where the main loop left off.
            for (; i < int(iterationCount) && !hit; i++) {
                binarySearchBaseStep *= 0.5; // Halve the step size for refinement.
                // Move back or forward along the ray based on the sign of the last known delta.
                // If delta > 0 (overshot), sign(delta) is 1, so we subtract (move back).
                marchingPosition = marchingPosition - binarySearchBaseStep * sign(delta);

                screenPosition = generateProjectedPosition(marchingPosition);
                
                if (checkAndUpdateOffScreen(screenPosition, edgeFade)) {
                    hitColor.a = 0.0; // Indicate no valid color if off-screen.
                    hit = false;
                    break;
                }
                
                depthFromScreen = getLinearDepth(screenPosition);
                // Recalculate delta for this binary search iteration.
                float currentBinarySearchDelta = abs(marchingPosition.z) - depthFromScreen;

                if (checkSelfIntersection(reflectingSurfaceViewDepth, depthFromScreen, epsilon)) {
                    edgeFade = calculateEdgeFade(screenPosition);
                    hit = false;
                    break;
                }

                // Check for hit using the refined delta.
                // The condition `depthFromScreen != (reflectingSurfaceViewDepth - distanceBias)` seems specific
                // and might be trying to avoid certain self-reflection scenarios or floating point issues.
                if (abs(currentBinarySearchDelta) < distanceBias &&
                    depthFromScreen != (reflectingSurfaceViewDepth - distanceBias)) {
                    processRayIntersection(screenPosition, depthFromScreen, currentBinarySearchDelta, source, roughness,
                                           hitColor, hitDepth, edgeFade);
                    hit = true;
                    break;
                }
                delta = currentBinarySearchDelta; // Update delta for the next binary search refinement step.
            }
        }
    }

    return hit; // Return whether a valid intersection was found.
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
 * @param glossiness The glossiness of the surface (0.0 for rough, 1.0 for perfectly smooth/mirror).
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
    // Early out for transparent surfaces if this define is active.
    // Sets a magenta color for debugging.
    collectedColor = vec4(1, 0, 1, 1);
    return 0; // No hits for transparent surfaces.
#endif

    // Convert glossiness to roughness. Squaring roughness provides a more perceptually linear response.
    float roughness = 1.0 - glossiness;
    roughness = roughness * roughness;

    // Only proceed if the surface is not perfectly rough.
    if (roughness < 1.0) { // Max roughness might be 1.0, so < 1.0 means some reflectivity.
        collectedColor = vec4(0.0); // Initialize accumulated color.
        int hits = 0;               // Counter for successful ray hits.
        float cumulativeFade = 0.0; // Accumulator for edge fade factors from successful rays.
        float averageHitDepth = 0.0;  // Accumulator for hit depths.

        float currentPixelViewDepth = -viewPos.z; // View space depth of the reflecting pixel.
        
        // Ensure the normal is facing the camera.
        if (dot(n, -viewPos) < 0.0) {
            n = -n;
        }

        // Calculate the perfect reflection direction.
        vec3 rayDirection = normalize(reflect(normalize(viewPos), normalize(n)));

        // Calculate a base edge fade for the current pixel's screen position.
        // This fade is applied regardless of where the reflection rays go.
        vec2 distFromCenter = abs(tc * 2.0 - 1.0);
        float baseEdgeFade = 1.0 - smoothstep(0.85, 1.0, max(distFromCenter.x, distFromCenter.y));

        // Attenuation based on viewing angle (Fresnel-like effect).
        // angleFactor is close to 1 for direct view, close to 0 for grazing angles.
        float angleFactor = abs(dot(normalize(-viewPos), normalize(n)));
        float angleFactorSq = angleFactor * angleFactor; // Square to make falloff more pronounced.

        // Attenuation based on distance from the camera.
        float distFadeDiv = 128.0; // Denominator for distance fade calculation.
        float distanceFactor = clamp((1.0 + (viewPos.z / distFadeDiv)) * 4.0, 0.0, 1.0);

        // Combine angle and distance factors for fading.
        // The commented-out 'combinedFade' logic suggests experimentation with how these factors interact.
        // The final used 'combinedFade' is simply 'distanceFactor'.
        float combinedFade = distanceFactor;


        // Skip reflection calculation entirely if the pixel is too close to the screen edge.
        if (baseEdgeFade > 0.001) {
            // Adjust the number of samples based on roughness and base edge fade.
            // Smoother surfaces or surfaces away from edges get more samples.
            totalSamples = int(max(float(glossySampleCount), float(glossySampleCount) * (1.0 - roughness) * baseEdgeFade));
            totalSamples = max(totalSamples, 1); // Ensure at least one sample.
                
            for (int i = 0; i < totalSamples; i++) {
                // Generate a perturbed reflection direction for glossy effects using Poisson disk samples.
                // 'noiseSine' and 'random' calls add temporal and spatial variation to the sampling pattern.
                float temporalOffset = fract(noiseSine * 0.1) * 127.0; // Temporal offset for Poisson index.
                int poissonIndex = int(mod(float(i) + temporalOffset + random(tc * noiseSine) * 64.0, 128.0));
                
                // Create an orthonormal basis around the main ray direction.
                vec3 firstBasis = normalize(cross(getPoissonSample(poissonIndex), rayDirection));
                vec3 secondBasis = normalize(cross(rayDirection, firstBasis));
                // Generate random coefficients to offset the ray within the cone defined by roughness.
                vec2 coeffs = vec2(random(tc + vec2(0, i)) + random(tc + vec2(i, 0)));
                // Cubic roughness term to make glossy reflections spread more at higher roughness values.
                vec3 reflectionDirectionRandomized = rayDirection + ((firstBasis * coeffs.x + secondBasis * coeffs.y) * (roughness * roughness * roughness));

                float hitDepthVal;    // Stores depth of the hit for this ray.
                vec4 hitpointColor; // Stores color of the hit for this ray.
                float rayEdgeFadeVal; // Stores edge fade for this ray.

                // Trace the individual (potentially perturbed) reflection ray.
                // Roughness is multiplied by 2 here, potentially to increase blur for glossy rays.
                bool hit = traceScreenRay(viewPos, reflectionDirectionRandomized, hitpointColor, hitDepthVal, source, rayEdgeFadeVal, roughness * 2.0);
                
                if (hit) {
                    ++hits;
                    collectedColor.rgb += hitpointColor.rgb; // Accumulate color.
                    collectedColor.a += 1.0;                 // Using alpha to count hits for averaging, or for intensity.
                    cumulativeFade += rayEdgeFadeVal;        // Accumulate edge fade.
                    averageHitDepth += hitDepthVal;          // Accumulate hit depth.
                }
            }

            if (hits > 0) {
                // Average the accumulated values if any rays hit.
                collectedColor /= float(hits); // Average color. Note: alpha also gets divided.
                cumulativeFade /= float(hits);   // Average edge fade.
                averageHitDepth /= float(hits);  // Average hit depth.
            } else {
                // No hits, result is black and no fade.
                collectedColor = vec4(0.0);
                cumulativeFade = 0.0;
            }
        } else { // Pixel is too close to the edge (baseEdgeFade is ~0).
            cumulativeFade = 0.0; // No reflection, so no cumulative fade.
        }
        
        // Apply final fading to the reflection.
        // 'cumulativeFade' is the average edge fade of successful rays.
        float finalEdgeFade = min(cumulativeFade * 1.0, 1.0); // Clamp fade.

        // Apply the combined distance/angle fade.
        finalEdgeFade *= combinedFade;
        
        // Set the alpha of the collected color to the final combined fade factor.
        // This alpha value will likely be used to blend the reflection with the underlying surface color.
        // Commented-out lines suggest alternative ways to use roughness or fade for the final alpha.
        collectedColor.a = finalEdgeFade;
        return float(hits); // Return the number of successful ray hits.
    }

    return 0; // Surface is perfectly rough, no reflections calculated.
}
