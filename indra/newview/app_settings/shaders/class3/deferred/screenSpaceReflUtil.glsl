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

uniform sampler2D sceneMap;
uniform sampler2D sceneDepth;

uniform vec2 screen_res;
uniform mat4 projection_matrix;
//uniform float zNear;
//uniform float zFar;
uniform mat4 inv_proj;
uniform mat4 modelview_delta;  // should be transform from last camera space to current camera space
uniform mat4 inv_modelview_delta;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

float random (vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

// Based off of https://github.com/RoundedGlint585/ScreenSpaceReflection/
// A few tweaks here and there to suit our needs.

vec2 generateProjectedPosition(vec3 pos)
{
    vec4 samplePosition = projection_matrix * vec4(pos, 1.f);
    samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
    return samplePosition.xy;
}

bool isBinarySearchEnabled = true;
bool isAdaptiveStepEnabled = true;
bool isExponentialStepEnabled = true;
bool debugDraw = false;

uniform float iterationCount;
uniform float rayStep;
uniform float distanceBias;
uniform float depthRejectBias;
uniform float glossySampleCount;
uniform float adaptiveStepMultiplier;
uniform float noiseSine;

float epsilon = 0.1;

float getLinearDepth(vec2 tc)
{
    float depth = texture(sceneDepth, tc).r;

    vec4 pos = getPositionWithDepth(tc, depth);

    return -pos.z;
}

// Add this function to your shader code to calculate edge fade based on screen position
float calculateEdgeFade(vec2 screenPos) {
    // Start fading when we're this close to the edge (0.9 = start fading at 10% from edge)
    const float edgeFadeStart = 0.9;
    
    // Convert 0-1 screen position to -1 to 1 range and take absolute value
    // This gives us how far we are from the center (0 = center, 1 = edge)
    vec2 distFromCenter = abs(screenPos * 2.0 - 1.0);
    
    // Calculate smooth fade for each axis
    vec2 fade = smoothstep(edgeFadeStart, 1.0, distFromCenter);
    
    // Use the maximum fade value between X and Y axes
    // (we want to fade if we're close to ANY edge)
    return 1.0 - max(fade.x, fade.y);
}

uniform vec3 POISSON3D_SAMPLES[128] = vec3[128](
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

vec3 getPoissonSample(int i) {
    return POISSON3D_SAMPLES[i] * 2 - 1;
}

float calculateMipmapLevels(vec2 resolution) {
    // Find the maximum dimension
    float maxDimension = max(resolution.x, resolution.y);
    
    // Calculate log2 of max dimension
    // (the number of times we can divide by 2 until we reach 1)
    return floor(log2(maxDimension)) + 1.0;
}

// Modified traceScreenRay function with improved fade
bool traceScreenRay(vec3 position, vec3 reflection, out vec4 hitColor, out float hitDepth, float depth, sampler2D source, out float edgeFade, float roughness) {
    // Transform position and reflection into same coordinate frame as the sceneMap and sceneDepth
    reflection += position;
    position = (inv_modelview_delta * vec4(position, 1)).xyz;
    reflection = (inv_modelview_delta * vec4(reflection, 1)).xyz;
    reflection -= position;

    depth = -position.z;

    vec3 step = rayStep * reflection;
    vec3 marchingPosition = position + step;
    float delta;
    float depthFromScreen;
    vec2 screenPosition;
    bool hit = false;
    hitColor = vec4(0);
    edgeFade = 1.0; // Default to no fade
    
    int i = 0;
    if (depth > depthRejectBias)
    {
        for (; i < iterationCount && !hit; i++)
        {
            screenPosition = generateProjectedPosition(marchingPosition);
            
            // Calculate edge fade when we hit screen boundaries
            if (screenPosition.x > 1 || screenPosition.x < 0 ||
                screenPosition.y > 1 || screenPosition.y < 0)
            {
                // Clamp screen position to 0-1 range for fade calculation
                vec2 clampedPos = clamp(screenPosition, 0.0, 1.0);
                vec2 distFromCenter = abs(clampedPos * 2.0 - 1.0);
                edgeFade = calculateEdgeFade(clampedPos);
                
                hitDepth = depthFromScreen;
                hit = false;
                break;
            }
            
            depthFromScreen = getLinearDepth(screenPosition);
            delta = abs(marchingPosition.z) - depthFromScreen;

            if (depth < depthFromScreen + epsilon && depth > depthFromScreen - epsilon)
            {
                // Calculate a fade factor based on how close we are to screen edges
                vec2 distFromCenter = abs(screenPosition * 2.0 - 1.0);
                edgeFade = calculateEdgeFade(screenPosition);
                break;
            }

            if (abs(delta) < distanceBias)
            {
                vec4 color = vec4(1);
                if(debugDraw)
                    color = vec4(0.5 + sign(delta)/2, 0.3, 0.5 - sign(delta)/2, 0);
                    
                float mipLevel = calculateMipmapLevels(screen_res) * roughness; // Maps 0-1 roughness to 0-4 mip levels
                vec2 screenCoord = screenPosition + (1 / screen_res * mipLevel );
                // Do a box sample.
                hitColor = textureLod(source, screenCoord, mipLevel) * color;
                hitColor += textureLod(source, -screenCoord, mipLevel) * color;
                hitColor += textureLod(source, vec2(-screenCoord.x, screenCoord.y), mipLevel) * color;
                hitColor += textureLod(source, vec2(screenCoord.x, -screenCoord.y), mipLevel) * color;
                hitColor *= 0.25;
                hitColor.a = 1;
                hitDepth = depthFromScreen;
                hit = true;
                
                // Calculate edge fade even for successful hits
                vec2 distFromCenter = abs(screenPosition * 2.0 - 1.0);
                edgeFade = calculateEdgeFade(screenPosition);
                
                // Add depth discontinuity detection for improved quality
                float depthGradient = 0.0;
                vec2 texelSize = 1.0 / screen_res;
                float depthRight = getLinearDepth(screenPosition + vec2(texelSize.x, 0));
                float depthBottom = getLinearDepth(screenPosition + vec2(0, texelSize.y));
                depthGradient = max(abs(depthRight - depthFromScreen), abs(depthBottom - depthFromScreen));
                
                break;
            }
            
            // Rest of your existing tracing code with adaptive steps
            if (isBinarySearchEnabled && delta > 0)
            {
                break;
            }
            if (isAdaptiveStepEnabled)
            {
                float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);
                step = step * (1.0 - rayStep * max(directionSign, 0.0));
                marchingPosition += step * (-directionSign);
            }
            else
            {
                marchingPosition += step;
            }

            if (isExponentialStepEnabled)
            {
                step *= adaptiveStepMultiplier;
            }
        }
        
        // Binary search refinement with edge fade calculation
        if(isBinarySearchEnabled)
        {
            for(; i < iterationCount && !hit; i++)
            {
                step *= 0.5;
                marchingPosition = marchingPosition - step * sign(delta);

                screenPosition = generateProjectedPosition(marchingPosition);
                if (screenPosition.x > 1 || screenPosition.x < 0 ||
                    screenPosition.y > 1 || screenPosition.y < 0)
                {
                    vec2 clampedPos = clamp(screenPosition, 0.0, 1.0);
                    vec2 distFromCenter = abs(clampedPos * 2.0 - 1.0);
                    edgeFade = calculateEdgeFade(screenPosition);
                    hitDepth = depthFromScreen;
                    hitColor.a = 0;
                    hit = false;
                    break;
                }
                
                depthFromScreen = getLinearDepth(screenPosition);
                delta = abs(marchingPosition.z) - depthFromScreen;

                if (depth < depthFromScreen + epsilon && depth > depthFromScreen - epsilon)
                {
                    vec2 distFromCenter = abs(screenPosition * 2.0 - 1.0);
                    edgeFade = calculateEdgeFade(screenPosition);
                    break;
                }

                if (abs(delta) < distanceBias && depthFromScreen != (depth - distanceBias))
                {
                    vec4 color = vec4(1);
                    if(debugDraw)
                        color = vec4(0.5 + sign(delta)/2, 0.3, 0.5 - sign(delta)/2, 0);
                        
                    float mipLevel = calculateMipmapLevels(screen_res) * roughness; // Maps 0-1 roughness to 0-4 mip levels
                    vec2 screenCoord = screenPosition + (1 / screen_res * mipLevel);
                    // Do a box sample.
                    hitColor = textureLod(source, screenCoord, mipLevel) * color;
                    hitColor += textureLod(source, -screenCoord, mipLevel) * color;
                    hitColor += textureLod(source, vec2(-screenCoord.x, screenCoord.y), mipLevel) * color;
                    hitColor += textureLod(source, vec2(screenCoord.x, -screenCoord.y), mipLevel) * color;
                    hitColor *= 0.25;
                    hitDepth = depthFromScreen;
                    hitColor.a = 1;
                    hit = true;
                    
                    vec2 distFromCenter = abs(screenPosition * 2.0 - 1.0);
                    edgeFade = calculateEdgeFade(screenPosition);
                    
                    // Add depth discontinuity detection for improved quality in binary search
                    float depthGradient = 0.0;
                    vec2 texelSize = 1.0 / screen_res;
                    float depthRight = getLinearDepth(screenPosition + vec2(texelSize.x, 0));
                    float depthBottom = getLinearDepth(screenPosition + vec2(0, texelSize.y));
                    depthGradient = max(abs(depthRight - depthFromScreen), abs(depthBottom - depthFromScreen));
                    
                    break;
                }
            }
        }
    }

    return hit;
}

// Enhanced tapScreenSpaceReflection function
float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness) {
#ifdef TRANSPARENT_SURFACE
    collectedColor = vec4(1, 0, 1, 1);
    return 0;
#endif
    collectedColor = vec4(0);
    int hits = 0;
    float cumulativeFade = 0.0;
    float averageHitDepth = 0.0;

    float depth = -viewPos.z;
    
    // Flip normal if needed
    if (dot(n, -viewPos) < 0.0) {
        n = -n;
    }

    vec3 rayDirection = normalize(reflect(normalize(viewPos), normalize(n)));

    vec2 uv2 = tc * screen_res;
    float c = (uv2.x + uv2.y) * 0.125;
    float jitter = mod(c, 1.0);

    // Calculate a base edge fade for the current screen position
    vec2 distFromCenter = abs(tc * 2.0 - 1.0);
    float baseEdgeFade = 1.0 - smoothstep(0.85, 1.0, max(distFromCenter.x, distFromCenter.y));
    
    float zFar = 250.0;
    // Distance-based vignette
    float distanceVignette = clamp((1.0+(viewPos.z/zFar)) * 2, 0.0, 1.0);

    // Improved roughness mapping
    float roughness = 1.0 - glossiness;
    //roughness = roughness * roughness; // More physically accurate roughness mapping

    // Skip calculation entirely if we're right at the edge
    if (baseEdgeFade > 0.01) {
        // Calculate sample count based on roughness and edge fade
        totalSamples = int(max(float(glossySampleCount), float(glossySampleCount) * (1.0 - roughness) * baseEdgeFade));
        totalSamples = max(totalSamples, 1);
        
        for (int i = 0; i < totalSamples; i++) {
            int poissonIndex = clamp(int(i + (random(tc) * (128 / totalSamples))), 0, 128);
            vec3 firstBasis = normalize(cross(getPoissonSample(poissonIndex), rayDirection));
            vec3 secondBasis = normalize(cross(rayDirection, firstBasis));
            vec2 coeffs = vec2(random(tc + vec2(0, i)) + random(tc + vec2(i, 0)));
            vec3 reflectionDirectionRandomized = rayDirection + ((firstBasis * coeffs.x + secondBasis * coeffs.y) * min((roughness * roughness * roughness * roughness), 0.001));

            float hitDepth;
            vec4 hitpoint;
            float rayEdgeFade;

            bool hit = traceScreenRay(viewPos, reflectionDirectionRandomized, hitpoint, hitDepth, depth, source, rayEdgeFade, roughness);
            
            if (hit) {
                ++hits;
                collectedColor.rgb += hitpoint.rgb;
                collectedColor.a += 1.0;
                cumulativeFade += rayEdgeFade;
                averageHitDepth += hitDepth;
            }
        }

        if (hits > 0) {
            collectedColor /= float(hits);
            cumulativeFade /= float(hits);
            averageHitDepth /= float(hits);
            
        } else {
            collectedColor = vec4(0);
            cumulativeFade = 0.0;
        }
    } else {
        cumulativeFade = 0.0;
    }
    
    // Apply final edge fading
    float finalEdgeFade = min(cumulativeFade * 1, 1) * distanceVignette;
    
    // Apply Fresnel effect
    vec3 viewDir = normalize(-viewPos);
    float NdotV = max(dot(normalize(n), viewDir), 0.0);
    float fresnel = pow(1.0 - NdotV, 5.0); // Schlick's approximation
    //collectedColor.rgb = vec3(1.0 / hits / 2);
    // Combine fades and adjust alpha
    collectedColor.a = finalEdgeFade;
    
    return float(hits);
}
