/**
 * @file transmissiveUtil.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
uniform vec2 screen_res;

float calcLegacyDistanceAttenuation(float distance, float falloff);

vec2 getScreenCoord(vec4 clip);

vec2 BRDF(float NoV, float roughness);

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

vec3 diffuse(PBRInfo pbrInputs);
vec3 specularReflection(PBRInfo pbrInputs);
float geometricOcclusion(PBRInfo pbrInputs);
float microfacetDistribution(PBRInfo pbrInputs);


// set colorDiffuse and colorSpec to the results of GLTF PBR style IBL
vec3 pbrIblTransmission(vec3 diffuseColor,
            vec3 specularColor,
            vec3 radiance, // radiance map sample
            vec3 irradiance, // irradiance map sample
            float ao,       // ambient occlusion factor
            float nv,       // normal dot view vector
            float perceptualRough,
            float transmission,
            vec3 transmission_btdf)
{
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec2 brdf = BRDF(clamp(nv, 0, 1), 1.0-perceptualRough);
    vec3 diffuseLight = irradiance;
    vec3 specularLight = radiance;

    vec3 diffuse = diffuseLight * diffuseColor;
    diffuse = mix(diffuse, transmission_btdf, transmission);
    vec3 specular = specularLight * (specularColor * brdf.x + brdf.y);

    return (diffuse + specular) * ao;
}

float applyIorToRoughness(float roughness, float ior)
{
    // Scale roughness with IOR so that an IOR of 1.0 results in no microfacet refraction and
    // an IOR of 1.5 results in the default amount of microfacet refraction.
    return roughness * clamp(ior * 2.0 - 2.0, 0.0, 1.0);
}

in mat4 transform_mat;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

vec3 getVolumeTransmissionRay(vec3 n, vec3 v, float thickness, float ior)
{
    // Direction of refracted light.
    vec3 refractionVector = refract(-v, normalize(n), 1.0 / ior);
    mat4 scale = projection_matrix * modelview_matrix;

    float scale_x = length(scale[0].xyz);
    float scale_y = length(scale[1].xyz);
    float scale_z = length(scale[2].xyz);

    // The thickness is specified in local space.
    return normalize(refractionVector) * thickness;
}

vec3 pbrPunctualTransmission(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l, // surface point to light
                    vec3 tr, // Transmission ray.
                    inout vec3 transmission_light, // Transmissive lighting.
                    vec3 intensity,
                    float ior
                    )
{
    // make sure specular highlights from punctual lights don't fall off of polished surfaces
    perceptualRoughness = max(perceptualRoughness, 8.0/255.0);

    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    vec3 h = normalize(l+v);                        // Half vector between both l and v
    vec3 reflection = -normalize(reflect(v, n));
    reflection.y *= -1.0f;

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        perceptualRoughness,
        metallic,
        specularEnvironmentR0,
        specularEnvironmentR90,
        alphaRoughness,
        diffuseColor,
        specularColor
    );

    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    vec3 lt = l - tr;

    // We use modified light and half vectors for the BTDF calculation, but the math is the same at the end of the day.
    float transmissionRougness = applyIorToRoughness(perceptualRoughness, ior);

    vec3 l_mirror = normalize(lt + 2.0 * n * dot(-lt, n));     // Mirror light reflection vector on surface
    h = normalize(l_mirror + v);            // Halfway vector between transmission light vector and v

    pbrInputs.NdotH = clamp(dot(n, h), 0.0, 1.0);
    pbrInputs.LdotH = clamp(dot(l_mirror, h), 0.0, 1.0);
    pbrInputs.NdotL = clamp(dot(n, l_mirror), 0.0, 1.0);
    pbrInputs.VdotH = clamp(dot(h, v), 0.0, 1.0);

    float G_transmission = geometricOcclusion(pbrInputs);
    vec3 F_transmission = specularReflection(pbrInputs);
    float D_transmission = microfacetDistribution(pbrInputs);

    transmission_light += intensity * (1.0 - F_transmission) * diffuseColor * D_transmission * G_transmission;

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    vec3 color = NdotL * (diffuseContrib + specContrib);

    return clamp(color, vec3(0), vec3(10));
}

vec3 pbrCalcPointLightOrSpotLightTransmission(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float metallic,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize,
                    float falloff,
                    float is_pointlight,
                    float ambiance,
                    float ior,
                    float thickness,
                    float transmissiveness)
{
    vec3 color = vec3(0,0,0);

    vec3 lv = lp.xyz - p;

    float lightDist = length(lv);

    float dist = lightDist / lightSize;
    if (dist <= 1.0)
    {
        lv /= lightDist;

        float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);

        // spotlight coefficient.
        float spot = max(dot(-ld, lv), is_pointlight);
        // spot*spot => GL_SPOT_EXPONENT=2
        float spot_atten = spot*spot;

        vec3 intensity = spot_atten * dist_atten * lightColor * 3.0; //magic number to balance with legacy materials

        vec3 tr = getVolumeTransmissionRay(n, v, thickness, ior);

        vec3 transmissive_light = vec3(0);

        color = intensity*pbrPunctualTransmission(diffuseColor, specularColor, perceptualRoughness, metallic, n.xyz, v, lv, tr, transmissive_light, intensity, ior);

        color = mix(color, transmissive_light, transmissiveness);
    }

    return color;
}

vec3 POISSON3D_SAMPLES[128] = vec3[128](
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

float random (vec2 uv);

vec3 getTransmissionSample(vec2 fragCoord, float roughness, float ior)
{
    float framebufferLod = log2(float(screen_res.x)) * applyIorToRoughness(roughness, ior);

    vec3 totalColor = textureLod(sceneMap, fragCoord.xy, framebufferLod).rgb;
    int samples = 16;
    for (int i = 0; i < samples; i++)
    {
        vec2 pixelScale = ((getPoissonSample(i).xy * 2 - 1) / screen_res) * framebufferLod * 4;
        totalColor += textureLod(sceneMap, fragCoord.xy + pixelScale, framebufferLod).rgb;
    }

    totalColor /= samples + 1;

    return totalColor;
}

vec3 applyVolumeAttenuation(vec3 radiance, float transmissionDistance, vec3 attenuationColor, float attenuationDistance)
{
    if (attenuationDistance == 0.0)
    {
        // Attenuation distance is +∞ (which we indicate by zero), i.e. the transmitted color is not attenuated at all.
        return radiance;
    }
    else
    {
        // Compute light attenuation using Beer's law.
        vec3 transmittance = pow(attenuationColor, vec3(transmissionDistance / attenuationDistance));
        return transmittance * radiance;
    }
}

vec3 getIBLVolumeRefraction(vec3 n, vec3 v, float perceptualRoughness, vec3 baseColor, vec3 f0, vec3 f90,
    vec4 position, float ior, float thickness, vec3 attenuationColor, float attenuationDistance, float dispersion)
{
    // Dispersion will spread out the ior values for each r,g,b channel
    float halfSpread = (ior - 1.0) * 0.025 * dispersion;
    vec3 iors = vec3(ior - halfSpread, ior, ior + halfSpread);
    vec3 transmittedLight;
    float transmissionRayLength;

    for (int i = 0; i < 3; i++)
    {
        vec3 transmissionRay = getVolumeTransmissionRay(n, v, thickness, iors[i]);
        // TODO: taking length of blue ray, ideally we would take the length of the green ray. For now overwriting seems ok
        transmissionRayLength = length(transmissionRay);
        vec3 refractedRayExit = vec3(getScreenCoord(position), 1) + transmissionRay;

        // Project refracted vector on the framebuffer, while mapping to normalized device coordinates.
        vec2 refractionCoords = refractedRayExit.xy;

        // Sample framebuffer to get pixel the refracted ray hits for this color channel.
        transmittedLight[i] = getTransmissionSample(refractionCoords, perceptualRoughness, iors[i])[i];
    }
    vec3 attenuatedColor = applyVolumeAttenuation(transmittedLight, transmissionRayLength, attenuationColor, attenuationDistance);

    // Sample GGX LUT to get the specular component.
    float NdotV = max(0, dot(n, v));
    vec2 brdfSamplePoint = clamp(vec2(NdotV, perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));

    vec2 brdf = BRDF(brdfSamplePoint.x, brdfSamplePoint.y);
    vec3 specularColor = f0 * brdf.x + f90 * brdf.y;

    //return (1.0 - specularColor);

    //return attenuatedColor;

    //return baseColor;

    return (1.0 - specularColor) * attenuatedColor * baseColor;
}

vec3 pbrBaseLightTransmission(vec3 diffuseColor,
                  vec3 specularColor,
                  float metallic,
                  vec4 pos,
                  vec3 view,
                  vec3 norm,
                  float perceptualRoughness,
                  vec3 light_dir,
                  vec3 sunlit,
                  float scol,
                  vec3 radiance,
                  vec3 irradiance,
                  vec3 colorEmissive,
                  float ao,
                  vec3 additive,
                  vec3 atten,
                  float thickness,
                  vec3 atten_color,
                  float atten_dist,
                  float ior,
                  float dispersion,
                  float transmission)
{
    vec3 color = vec3(0);

    float NdotV = clamp(abs(dot(norm, view)), 0.001, 1.0);

    vec3 btdf = vec3(0);

    btdf = getIBLVolumeRefraction(norm, view, perceptualRoughness, diffuseColor, vec3(0.04), vec3(1), pos, ior, thickness, atten_color, atten_dist, dispersion);

    color += pbrIblTransmission(diffuseColor, specularColor, radiance, irradiance, ao, NdotV, perceptualRoughness, transmission, btdf);

    vec3 tr = getVolumeTransmissionRay(norm, view, thickness, ior);

    vec3 transmissive_light = vec3(0);

    vec3 puncLight = pbrPunctualTransmission(diffuseColor, specularColor, perceptualRoughness, metallic, norm, view, normalize(light_dir), tr, transmissive_light, vec3(1), ior);

    color += mix(puncLight, transmissive_light, vec3(transmission)) * sunlit * 3.0 * scol;

    color += colorEmissive;

    return color;
}
