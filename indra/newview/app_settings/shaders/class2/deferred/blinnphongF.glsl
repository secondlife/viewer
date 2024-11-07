/**
 * @file blinnphongF.glsl
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

// deferred opaque implementation

#ifdef DEBUG
uniform vec4 debug_color;
#endif

#ifdef HAS_FRAGMENT_NORMAL
in vec3 vary_normal;
#endif

vec4 diffuseColor;
float bp_glow;
float emissive;
float emissive_mask;
float env_intensity;
float glossiness;
vec3 specularColor;

#ifdef ALPHA_MASK
float minimum_alpha;
#endif

#ifdef SAMPLE_DIFFUSE_MAP
uniform sampler2D diffuseMap; //always in sRGB space
in vec2 diffuse_texcoord;
#endif

#ifdef SAMPLE_SPECULAR_MAP
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
in vec2 specular_texcoord;
#endif


#ifdef SAMPLE_NORMAL_MAP
uniform sampler2D bumpMap;
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 normal_texcoord;
#endif

#ifdef OUTPUT_DIFFUSE_ONLY
out vec4 frag_color;
#else
out vec4 frag_data[4];
#endif

#ifdef MIRROR_CLIP
void mirrorClip(vec3 pos);
#endif

#ifdef FRAG_POSITION
in vec3 vary_position;
#endif

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef SAMPLE_MATERIALS_UBO
layout (std140) uniform GLTFMaterials
{
    // index by gltf_material_id

    // [gltf_material_id + [0-1]] -  diffuse transform
    // [gltf_material_id + [2-3]] -  normal transform
    // [gltf_material_id + [4-5]] -  specular transform
    // [gltf_material_id + 6] - .x - emissive factor, .y - emissive mask, .z - glossiness, .w - glow
    // Transforms are packed as follows
    // packed[0] = vec4(scale.x, scale.y, rotation, offset.x)
    // packed[1] = vec4(offset.y, *, *, *)

    // packed[1].yzw varies:
    //   diffuse transform -- diffuse color
    //   normal transform -- .y - alpha factor, .z - minimum alpha, .w - environment intensity
    //   specular transform -- specular color


    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

flat in int gltf_material_id;

void unpackMaterial()
{
    int idx = gltf_material_id;

    diffuseColor.rgb = gltf_material_data[idx+1].yzw;
    diffuseColor.a = gltf_material_data[idx+3].y;
    emissive = gltf_material_data[idx+6].x;
    emissive_mask = gltf_material_data[idx+6].y;
    bp_glow = gltf_material_data[idx+6].w;
    env_intensity = gltf_material_data[idx+3].w;
    glossiness = gltf_material_data[idx+6].z;
    specularColor = gltf_material_data[idx+5].yzw;

#ifdef ALPHA_MASK
    minimum_alpha = gltf_material_data[idx+3].z;
#endif

}
#else // SAMPLE_MATERIALS_UBO
void unpackMaterial()
{
}
#endif

#ifdef ALPHA_BLEND

in vec3 vary_fragcoord;

#ifdef HAS_SUN_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif

float getShadow(vec3 pos, vec3 norm)
{
#ifdef HAS_SUN_SHADOW
   return sampleDirectionalShadow(pos, norm, vary_fragcoord.xy);
#else
    return 1.;
#endif
}

vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);

void sampleReflectionProbesLegacy(out vec3 ambenv, out vec3 glossenv, out vec3 legacyenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, float envIntensity, bool transparent, vec3 amblit_linear);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);

uniform samplerCube environmentMap;
uniform sampler2D     lightFunc;

// Inputs
uniform int sun_up_factor;
uniform vec3 sun_dir;
uniform vec3 moon_dir;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec4 light_attenuation[8];
uniform vec3 light_diffuse[8];

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 npos, vec3 diffuse, vec4 spec, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, inout float glare, float ambiance)
{
    // SL-14895 inverted attenuation work-around
    // This routine is tweaked to match deferred lighting, but previously used an inverted la value. To reconstruct
    // that previous value now that the inversion is corrected, we reverse the calculations in LLPipeline::setupHWLights()
    // to recover the `adjusted_radius` value previously being sent as la.
    float falloff_factor = (12.0 * fa) - 9.0;
    float inverted_la = falloff_factor / la;
    // Yes, it makes me want to cry as well. DJH

    vec3 col = vec3(0);

    //get light vector
    vec3 lv = lp.xyz - v;

    //get distance
    float dist = length(lv);
    float da = 1.0;

    dist /= inverted_la;

    if (dist > 0.0 && inverted_la > 0.0)
    {
        //normalize light vector
        lv = normalize(lv);

        //distance attenuation
        float dist_atten = clamp(1.0 - (dist - 1.0*(1.0 - fa)) / fa, 0.0, 1.0);
        dist_atten *= dist_atten;
        dist_atten *= 2.0f;

        if (dist_atten <= 0.0)
        {
            return col;
        }

        // spotlight coefficient.
        float spot = max(dot(-ln, lv), is_pointlight);
        da *= spot*spot; // GL_SPOT_EXPONENT=2

        //angular attenuation
        da *= dot(n, lv);

        float lit = 0.0f;

        float amb_da = ambiance;
        if (da >= 0)
        {
            lit = max(da * dist_atten, 0.0);
            col = lit * light_col * diffuse;
            amb_da += (da*0.5 + 0.5) * ambiance;
        }
        amb_da += (da*da*0.5 + 0.5) * ambiance;
        amb_da *= dist_atten;
        amb_da = min(amb_da, 1.0f - lit);

        // SL-10969 need to see why these are blown out
        //col.rgb += amb_da * light_col * diffuse;

        if (spec.a > 0.0)
        {
            //vec3 ref = dot(pos+lv, norm);
            vec3 h = normalize(lv + npos);
            float nh = dot(n, h);
            float nv = dot(n, npos);
            float vh = dot(npos, h);
            float sa = nh;
            float fres = pow(1 - dot(h, npos), 5)*0.4 + 0.5;

            float gtdenom = 2 * nh;
            float gt = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));

            if (nh > 0.0)
            {
                float scol = fres*texture(lightFunc, vec2(nh, spec.a)).r*gt / (nh*da);
                vec3 speccol = lit*scol*light_col.rgb*spec.rgb;
                speccol = clamp(speccol, vec3(0), vec3(1));
                col += speccol;

                float cur_glare = max(speccol.r, speccol.g);
                cur_glare = max(cur_glare, speccol.b);
                glare = max(glare, speccol.r);
                glare += max(cur_glare, 0.0);
            }
        }
    }

    return max(col, vec3(0.0, 0.0, 0.0));
}

#endif

vec4 getDiffuse()
{
    vec4 diffuse = vec4(1);
#ifdef SAMPLE_DIFFUSE_MAP
    diffuse = texture(diffuseMap, diffuse_texcoord.xy).rgba;
    emissive = max(emissive, emissive_mask * diffuse.a);
    bp_glow *= diffuse.a;
#ifdef ALPHA_MASK
    if (diffuse.a * diffuseColor.a < minimum_alpha)
    {
        discard;
    }
#endif
#endif
    return diffuse;
}

vec4 getSpec()
{
    vec4 spec = vec4(0);
#ifdef SAMPLE_SPECULAR_MAP
    spec = texture(specularMap, specular_texcoord.xy);
    spec.a *= env_intensity;
    env_intensity = spec.a;
    spec.rgb *= specularColor;
    spec.rgb = srgb_to_linear(spec.rgb);
#else
    spec = vec4(specularColor, env_intensity);
#endif
    return spec;
}

#ifdef HAS_FRAGMENT_NORMAL
vec3 getNorm()
{
#ifdef SAMPLE_NORMAL_MAP
    // from mikktspace.com
    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );
#else
    vec3 tnorm = normalize(vary_normal);
#endif
    return tnorm;
}
#endif

void main()
{
    unpackMaterial();
#ifdef MIRROR_CLIP
    mirrorClip(vary_position);
#endif

    vec4 diffuse = getDiffuse();

    diffuse.rgb *= diffuseColor.rgb;

#ifdef HAS_FRAGMENT_NORMAL
    vec3 tnorm = getNorm();
#endif

    vec4 spec = getSpec();

#ifdef ALPHA_BLEND
    diffuse.rgb = srgb_to_linear(diffuse.rgb);
    //forward rendering, output lit linear color
    spec.a = glossiness; // pack glossiness into spec alpha for lighting functions

    vec3 pos = vary_position;

    float shadow = getShadow(pos, tnorm);

    vec3 color = vec3(0,0,0);

    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;

    float bloom = 0.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, tnorm.xyz, light_dir, sunlit, amblit, additive, atten);

    vec3 sunlit_linear = srgb_to_linear(sunlit);
    vec3 amblit_linear = amblit;

    vec3 ambenv;
    vec3 glossenv;
    vec3 legacyenv;
    sampleReflectionProbesLegacy(ambenv, glossenv, legacyenv, pos.xy*0.5+0.5, pos.xyz, tnorm.xyz, glossiness, env_intensity, true, amblit_linear);

    color = ambenv;

    float da          = clamp(dot(tnorm.xyz, light_dir.xyz), 0.0, 1.0);
    vec3 sun_contrib = min(da, shadow) * sunlit_linear;
    color.rgb += sun_contrib;
    color *= diffuse.rgb;

    vec3 refnormpersp = reflect(pos.xyz, tnorm.xyz);

    float glare = 0.0;

    if (glossiness > 0.0)
    {
        vec3  lv = light_dir.xyz;
        vec3  h, l, v = -normalize(pos.xyz);
        float nh, nl, nv, vh, lightDist;
        vec3 n = tnorm.xyz;
        calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

        if (nl > 0.0 && nh > 0.0)
        {
            float lit = min(nl*6.0, 1.0);

            float sa = nh;
            float fres = pow(1 - vh, 5) * 0.4+0.5;
            float gtdenom = 2 * nh;
            float gt = max(0,(min(gtdenom * nv / vh, gtdenom * nl / vh)));

            float scol = shadow*fres*texture(lightFunc, vec2(nh, glossiness)).r*gt/(nh*nl);
            color.rgb += lit*scol*sunlit_linear.rgb*spec.rgb;
        }

        // add radiance map
        applyGlossEnv(color, glossenv, spec, pos.xyz, tnorm.xyz);
    }

    color = mix(color.rgb, diffuse.rgb, emissive);

    if (env_intensity > 0.0)
    {  // add environmentmap
        applyLegacyEnv(color, legacyenv, spec, pos.xyz, tnorm.xyz, env_intensity);

        float cur_glare = max(max(legacyenv.r, legacyenv.g), legacyenv.b);
        cur_glare = clamp(cur_glare, 0, 1);
        cur_glare *= env_intensity;
        glare += cur_glare;
    }

    vec3 npos = normalize(-pos.xyz);
    vec3 light = vec3(0, 0, 0);

#define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, npos, diffuse.rgb, spec, pos.xyz, tnorm.xyz, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, glare, light_attenuation[i].w );

    LIGHT_LOOP(1)
        LIGHT_LOOP(2)
        LIGHT_LOOP(3)
        LIGHT_LOOP(4)
        LIGHT_LOOP(5)
        LIGHT_LOOP(6)
        LIGHT_LOOP(7)

    color += light;

    color.rgb = applySkyAndWaterFog(pos.xyz, additive, atten, vec4(color, 1.0)).rgb;

    glare *= 1.0-emissive;
    glare = min(glare, 1.0);
    float al = max(diffuse.a, glare) * diffuseColor.a;

    diffuse.rgb = color.rgb;
    diffuse.a = al;
#endif

#ifdef OUTPUT_DIFFUSE_ONLY
#ifdef OUTPUT_SRGB
    diffuse.rgb = linear_to_srgb(diffuse.rgb);
#endif

#ifdef DEBUG
    diffuse = debug_color;
#endif

    frag_color = diffuse;
#else
    //diffuse.rgb = vec3(0.85);
    //spec.rgb = vec3(0);
    //env_intensity = 0;
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(diffuse.rgb, bp_glow), vec4(0));
    frag_data[1] = max(vec4(spec.rgb,glossiness), vec4(0));
    frag_data[2] = vec4(tnorm, GBUFFER_FLAG_HAS_ATMOS);
    frag_data[3] = max(vec4(env_intensity, emissive, 0, 0), vec4(0));
#endif
}

