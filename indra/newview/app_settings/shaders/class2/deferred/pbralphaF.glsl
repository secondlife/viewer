/**
 * @file class1\deferred\pbralphaF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
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

/*[EXTRA_CODE_HERE]*/

#ifndef IS_HUD

uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
uniform sampler2D specularMap; // PBR: Packed: Occlusion, Metal, Roughness

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;
uniform float specularFactor;
uniform vec3 specularColorFactor;
uniform float emissiveStrength;
uniform float ior;

#ifdef TRANSMISSION
uniform sampler2D transmissionMap; // R: transmission
uniform sampler2D thicknessMap;    // G: thickness
uniform sampler2D transmissionSceneMap;
uniform sampler2D depthMap; // depth of the transmission copy

vec3 getPositionWithNDC(vec3 ndc);

uniform float transmissionFactor;
uniform float thicknessFactor;
uniform float attenuationDistance; // 0 = infinite
uniform vec3 attenuationColor;
uniform float dispersion;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec2 transmission_texcoord;
in vec2 thickness_texcoord;

uniform int transmissionHasCopy; // 0 = no scene copy, transmission is done in the blend stage
uniform int alphaOpaque;
uniform int transmissionSamples;

// Deepest mip the pipeline builds for the transmission copy.
const float TRANSMISSION_MAX_LOD = 6.0;
const int TRANSMISSION_MAX_SAMPLES = 16;
const float TRANSMISSION_OFF_MAX_DEPTH = 1.0; // meters of volume assumed behind the surface at quality 0
#endif

#if defined(HAS_SUN_SHADOW) || defined(HAS_SSAO)
uniform sampler2D lightMap;
#endif

uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int classic_mode;

out vec4 frag_color;

in vec3 vary_fragcoord;

uniform vec2 screen_res;

in vec3 vary_position;

in vec2 base_color_texcoord;
in vec2 normal_texcoord;
in vec2 metallic_roughness_texcoord;
in vec2 emissive_texcoord;

in vec4 vertex_color;

in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;


#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

// Lights
// See: LLRender::syncLightState()
uniform vec4 light_position[8];
uniform vec3 light_direction[8]; // spot direction
uniform vec4 light_attenuation[8]; // linear, quadratic, is omni, unused, See: LLPipeline::setupHWLights() and syncLightState()
uniform vec3 light_diffuse[8];
uniform vec2 light_deferred_attenuation[8]; // light size and falloff

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color);

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, bool transparent, vec3 amblit_linear);

void mirrorClip(vec3 pos);
void waterClip(vec3 pos);

void calcDiffuseSpecular(vec3 baseColor, float metallic, float specularFactor, vec3 specularColorFactor, float ior, inout vec3 diffuseColor, inout vec3 specularColor, inout float specularWeight);

vec3 pbrBaseLight(vec3 diffuseColor,
                  vec3 specularColor,
                  float specularWeight,
                  vec3 pos,
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
                  vec3 atten);

vec3 pbrCalcPointLightOrSpotLight(vec3 diffuseColor, vec3 specularColor,
                    float perceptualRoughness,
                    float specularWeight,
                    vec3 n, // normal
                    vec3 p, // pixel position
                    vec3 v, // view vector (negative normalized pixel position)
                    vec3 lp, // light position
                    vec3 ld, // light direction (for spotlights)
                    vec3 lightColor,
                    float lightSize, float falloff, float is_pointlight, float ambiance);

#ifdef TRANSMISSION
vec2 BRDF(float NoV, float roughness);

// Khronos Sample Viewer volume refraction, evaluated in eye space.
vec3 transmissionRay(vec3 normal, vec3 viewDir, float iorValue, float thickness)
{
    vec3 modelScale = vec3(length(modelview_matrix[0].xyz), length(modelview_matrix[1].xyz), length(modelview_matrix[2].xyz));

    vec3 refracted = refract(-viewDir, normal, 1.0 / iorValue);

    if (dot(refracted, refracted) < 1e-8)
    {
        refracted = -viewDir;
    }

    return normalize(refracted) * thickness * modelScale;
}

vec3 sampleTransmissionTap(vec3 pos, vec3 normal, vec3 viewDir, float iorValue, float thickness, float roughness, float lodBias, out float rayLength)
{
    vec3 ray = transmissionRay(normal, viewDir, iorValue, thickness);

    vec4 clip = projection_matrix * vec4(pos + ray, 1.0);
    vec2 screen_uv = gl_FragCoord.xy / screen_res;
    vec2 uv = screen_uv;

    if (clip.w > 0.0)
    {
        uv = clip.xy / clip.w * 0.5 + 0.5;
    }

    // offset ramps in over the first meter behind the surface
    float depth = texture(depthMap, screen_uv).r;
    vec3 screen_behind = getPositionWithNDC(vec3(screen_uv * 2.0 - 1.0, depth * 2.0 - 1.0));
    uv = mix(screen_uv, uv, clamp(pos.z - screen_behind.z, 0.0, 1.0));

    // same falloff as SSR's calculateEdgeFade
    vec2 edge = smoothstep(0.85, 1.0, abs(screen_uv * 2.0 - 1.0));
    uv = mix(screen_uv, uv, 1.0 - max(edge.x, edge.y));

    depth = texture(depthMap, uv).r;
    vec3 behind = getPositionWithNDC(vec3(uv * 2.0 - 1.0, depth * 2.0 - 1.0));

    if (pos.z < behind.z - 0.05)
    {
        uv = screen_uv;
        behind = screen_behind;
    }

    // volume distance runs to the scene behind the surface
    rayLength = length(behind - pos);

    // GGX cone footprint in pixels, as SSR's cone mip; a microfacet tilt bends the refracted ray by (1 - 1/ior)
    float coneTan = roughness * roughness * (1.0 - 1.0 / iorValue);
    float footprint = coneTan * rayLength / max(-pos.z, 0.01) * screen_res.y * 0.5 * projection_matrix[1][1];
    float lod = clamp(log2(max(footprint, 1.0)) - lodBias, 0.0, TRANSMISSION_MAX_LOD);

    return textureLod(transmissionSceneMap, uv, lod).rgb;
}

// GGX microfacet normals on a golden-angle spiral, rotated per pixel (interleaved gradient noise)
vec3 sampleTransmission(vec3 pos, vec3 normal, vec3 viewDir, float iorValue, float thickness, float roughness, out float rayLength)
{
    int samples = clamp(transmissionSamples, 1, TRANSMISSION_MAX_SAMPLES);
    float alpha = roughness * roughness;

    if (alpha < 0.001)
    {
        samples = 1;
    }

    vec3 up = vec3(0.0, 1.0, 0.0);

    if (abs(normal.y) > 0.999)
    {
        up = vec3(1.0, 0.0, 0.0);
    }

    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    float rotation = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    float lodBias = 0.5 * log2(float(samples));

    vec3 accumColor = vec3(0.0);
    float accumLength = 0.0;

    for (int tap = 0; tap < samples; ++tap)
    {
        vec3 microNormal = normal;

        if (samples > 1)
        {
            float radial = (float(tap) + 0.5) / float(samples);
            float theta = atan(alpha * sqrt(radial / (1.0 - radial)));
            float phi = float(tap) * 2.3999632 + rotation;
            microNormal = normalize(sin(theta) * cos(phi) * tangent + sin(theta) * sin(phi) * bitangent + cos(theta) * normal);
        }

        float tapLength;
        accumColor += sampleTransmissionTap(pos, microNormal, viewDir, iorValue, thickness, roughness, lodBias, tapLength);
        accumLength += tapLength;
    }

    rayLength = accumLength / float(samples);
    return accumColor / float(samples);
}

// fresnel, volume attenuation and base color applied to light passing through the surface
vec3 transmissionFilter(vec3 normal, vec3 viewDir, float roughness, float rayLength, vec3 baseColor, vec3 specularColor, float specularWeight)
{
    vec3 attenuation = vec3(1.0);

    if (attenuationDistance > 0.0)
    {
        vec3 coefficient = -log(max(attenuationColor, vec3(1e-6))) / attenuationDistance;
        attenuation = exp(-coefficient * rayLength);
    }

    float nv = clamp(abs(dot(normal, viewDir)), 0.001, 1.0);
    vec2 brdf = BRDF(nv, 1.0 - roughness);
    vec3 fresnel = specularColor * brdf.x + brdf.y * specularWeight;

    return (vec3(1.0) - clamp(fresnel, vec3(0.0), vec3(1.0))) * attenuation * baseColor;
}

vec3 calcTransmission(vec3 pos, vec3 normal, vec3 viewDir, float thickness, float roughness, vec3 baseColor, vec3 specularColor, float specularWeight)
{
    float iorValue = max(ior, 1.0001);
    vec3 transmitted;
    float rayLength;

    if (dispersion > 0.0)
    {
        float halfSpread = (iorValue - 1.0) * 0.025 * dispersion;
        vec3 iors = vec3(iorValue - halfSpread, iorValue, iorValue + halfSpread);
        transmitted.r = sampleTransmission(pos, normal, viewDir, iors.r, thickness, roughness, rayLength).r;
        transmitted.g = sampleTransmission(pos, normal, viewDir, iors.g, thickness, roughness, rayLength).g;
        transmitted.b = sampleTransmission(pos, normal, viewDir, iors.b, thickness, roughness, rayLength).b;
    }
    else
    {
        transmitted = sampleTransmission(pos, normal, viewDir, iorValue, thickness, roughness, rayLength);
    }

    return transmissionFilter(normal, viewDir, roughness, rayLength, baseColor, specularColor, specularWeight) * transmitted;
}
#endif

void main()
{
    mirrorClip(vary_position);

    vec3 color = vec3(0,0,0);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    vec3  pos         = vary_position;

    waterClip(pos);

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
#ifdef HAS_ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

    vec3 col = vertex_color.rgb * basecolor.rgb;

    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 norm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    norm *= gl_FrontFacing ? 1.0 : -1.0;

    float scol = 1.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, norm, light_dir, sunlit, amblit, additive, atten);
    if (classic_mode > 0)
        sunlit *= 1.35;
    vec3 sunlit_linear = sunlit;

    // vary_fragcoord divides clip.xy by clip.z (w was discarded) — radially wrong UV
    vec2 frag = gl_FragCoord.xy / screen_res;

#ifdef HAS_SUN_SHADOW
    scol = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

    vec3 orm = texture(specularMap, metallic_roughness_texcoord.xy).rgb; //orm is packed into "emissiveRect" to keep the data in linear color space

    float perceptualRoughness = orm.g * roughnessFactor;
    float metallic = orm.b * metallicFactor;
    float ao = orm.r;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor * emissiveStrength;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);

    // PBR IBL
    float gloss      = 1.0 - perceptualRoughness;
    vec3  irradiance = amblit;
    vec3  radiance  = vec3(0);
    sampleReflectionProbes(irradiance, radiance, frag, pos.xyz, norm.xyz, gloss, true, amblit);

    vec3 diffuseColor = vec3(0.0);
    vec3 specularColor = vec3(0.0);
    float specWeight = 1.0;
    calcDiffuseSpecular(col.rgb, metallic, specularFactor, specularColorFactor, ior, diffuseColor, specularColor, specWeight);

    vec3 v = -normalize(pos.xyz);

#ifdef TRANSMISSION
    float transmissionBase = clamp(transmissionFactor, 0.0, 1.0) * texture(transmissionMap, transmission_texcoord.xy).r;
    float transmission = transmissionBase * (1.0 - metallic);
    float thickness = thicknessFactor * texture(thicknessMap, thickness_texcoord.xy).g;
    diffuseColor *= (1.0 - transmissionBase);
#endif

    color = pbrBaseLight(diffuseColor, specularColor, specWeight, v, norm.xyz, perceptualRoughness, light_dir, sunlit_linear, scol, radiance, irradiance, colorEmissive, ao, additive, atten);

    vec3 light = vec3(0);

    // Punctual lights
#define LIGHT_LOOP(i) light += pbrCalcPointLightOrSpotLight(diffuseColor, specularColor, perceptualRoughness, specWeight, norm.xyz, pos.xyz, v, light_position[i].xyz, light_direction[i].xyz, light_diffuse[i].rgb, light_deferred_attenuation[i].x, light_deferred_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    color.rgb += light.rgb;

    color.rgb = applySkyAndWaterFog(pos.xyz, additive, atten, vec4(color, 1.0)).rgb;

#ifdef TRANSMISSION
    // the copied scene is already fogged, so it is added after fog
    if (transmission > 0.0 && transmissionHasCopy == 1)
    {
        color.rgb += transmission * calcTransmission(pos.xyz, norm.xyz, v, thickness, perceptualRoughness, col.rgb, specularColor, specWeight);
    }
#endif

    float a = basecolor.a*vertex_color.a;
    float final_scale = 1;
    if (classic_mode > 0)
        final_scale = 1.1;

#ifdef TRANSMISSION
    if (transmissionHasCopy == 0)
    {
        if (alphaOpaque == 1)
        {
            a = 1.0;
        }

#ifdef TRANSMISSION_TINT
        // background tint (multiply blend); no depth copy, so assume at most TRANSMISSION_OFF_MAX_DEPTH of volume behind
        vec2 screen_uv = gl_FragCoord.xy / screen_res;
        float rayLength = min(length(getPositionWithNDC(vec3(screen_uv * 2.0 - 1.0, 1.0)) - pos.xyz), TRANSMISSION_OFF_MAX_DEPTH);
        vec3 transmittance = transmission * transmissionFilter(norm.xyz, v, perceptualRoughness, rayLength, col.rgb, specularColor, specWeight);
        frag_color = vec4(mix(vec3(1.0), transmittance, a), 1.0);
#else
        // surface (additive blend), drawn after the tint
        frag_color = max(vec4(color.rgb * final_scale * a, a), vec4(0));
#endif
        return;
    }
#endif

    frag_color = max(vec4(color.rgb * final_scale,a), vec4(0));
}

#else

uniform sampler2D diffuseMap;  //always in sRGB space
uniform sampler2D emissiveMap;

uniform vec3 emissiveColor;

out vec4 frag_color;

in vec3 vary_position;

in vec2 base_color_texcoord;
in vec2 emissive_texcoord;

in vec4 vertex_color;

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);


void main()
{
    vec3 color = vec3(0,0,0);

    vec3  pos         = vary_position;

    vec4 basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);
#ifdef HAS_ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

    color = vertex_color.rgb * basecolor.rgb;

    // emissiveColor is the emissive color factor from GLTF and is already in linear space
    vec3 colorEmissive = emissiveColor;
    // emissiveMap here is a vanilla RGB texture encoded as sRGB, manually convert to linear
    colorEmissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);


    float a = basecolor.a*vertex_color.a;
    color += colorEmissive;

    color = linear_to_srgb(color);
    frag_color = max(vec4(color.rgb,a), vec4(0));
}

#endif
