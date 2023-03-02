/**
* @file materialF.glsl
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

/*[EXTRA_CODE_HERE]*/

//class1/deferred/materialF.glsl

// This shader is used for both writing opaque/masked content to the gbuffer and writing blended content to the framebuffer during the alpha pass.

#define DIFFUSE_ALPHA_MODE_NONE     0
#define DIFFUSE_ALPHA_MODE_BLEND    1
#define DIFFUSE_ALPHA_MODE_MASK     2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

uniform float emissive_brightness;  // fullbright flag, 1.0 == fullbright, 0.0 otherwise
uniform int sun_up_factor;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 atmosFragLightingLinear(vec3 l, vec3 additive, vec3 atten);
vec3 scaleSoftClipFragLinear(vec3 l);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
vec3 fullbrightAtmosTransportFragLinear(vec3 light, vec3 additive, vec3 atten);

vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cs);

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

#ifdef HAS_SUN_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif

void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, float envIntensity);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);

uniform samplerCube environmentMap;
uniform sampler2D     lightFunc;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
uniform mat3 env_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
VARYING vec2 vary_fragcoord;

VARYING vec3 vary_position;

uniform mat4 proj_mat;
uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec4 light_attenuation[8];
uniform vec3 light_diffuse[8];

float getAmbientClamp();
void waterClip(vec3 pos);

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
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt / (nh*da);
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

#else
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif
#endif

uniform sampler2D diffuseMap;  //always in sRGB space

#ifdef HAS_NORMAL_MAP
uniform sampler2D bumpMap;
#endif

#ifdef HAS_SPECULAR_MAP
uniform sampler2D specularMap;

VARYING vec2 vary_texcoord2;
#endif

uniform float env_intensity;
uniform vec4 specular_color;  // specular color RGB and specular exponent (glossiness) in alpha

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
uniform float minimum_alpha;
#endif

#ifdef HAS_NORMAL_MAP
VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;
VARYING vec2 vary_texcoord1;
#else
VARYING vec3 vary_normal;
#endif

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

vec2 encode_normal(vec3 n);

// get the transformed normal and apply glossiness component from normal map
vec3 getNormal(inout float glossiness)
{
#ifdef HAS_NORMAL_MAP
	vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);
    glossiness *= norm.a;

	norm.xyz = norm.xyz * 2 - 1;

	return normalize(vec3(dot(norm.xyz,vary_mat0),
			  dot(norm.xyz,vary_mat1),
			  dot(norm.xyz,vary_mat2)));
#else
	return normalize(vary_normal);
#endif
}

vec4 getSpecular()
{
#ifdef HAS_SPECULAR_MAP
    vec4 spec = texture2D(specularMap, vary_texcoord2.xy);
    spec.rgb *= specular_color.rgb;
#else
    vec4 spec = vec4(specular_color.rgb, 1.0);
#endif
    return spec;
}

void alphaMask(float alpha)
{
#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
    // Comparing floats cast from 8-bit values, produces acne right at the 8-bit transition points
    float bias = 0.001953125; // 1/512, or half an 8-bit quantization
    if (alpha < minimum_alpha-bias)
    {
        discard;
    }
#endif
}

void waterClip()
{
#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    waterClip(vary_position.xyz);
#endif
}

float getEmissive(vec4 diffcol)
{
#if (DIFFUSE_ALPHA_MODE != DIFFUSE_ALPHA_MODE_EMISSIVE)
	return emissive_brightness;
#else
	return max(diffcol.a, emissive_brightness);
#endif
}

float getShadow(vec3 pos, vec3 norm)
{
#ifdef HAS_SUN_SHADOW
    #if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
        return sampleDirectionalShadow(pos, norm, vary_texcoord0.xy);
    #else
        return 1;
    #endif
#else
    return 1;
#endif
}

void main()
{
    waterClip();

    // diffcol == diffuse map combined with vertex color
    vec4 diffcol = texture2D(diffuseMap, vary_texcoord0.xy);
	diffcol.rgb *= vertex_color.rgb;

    alphaMask(diffcol.a);

    // spec == specular map combined with specular color
    vec4 spec = getSpecular();
    float env = env_intensity * spec.a;
    float glossiness = specular_color.a;
    vec3 norm = getNormal(glossiness);

    vec2 abnormal = encode_normal(norm.xyz);

    float emissive = getEmissive(diffcol);

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    //forward rendering, output lit linear color
    diffcol.rgb = srgb_to_linear(diffcol.rgb);
    spec.rgb = srgb_to_linear(spec.rgb);

    vec3 pos = vary_position;

    float shadow = getShadow(pos, norm);

    vec4 diffuse = diffcol;

    vec3 color = vec3(0,0,0);

    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;

    float bloom = 0.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVarsLinear(pos.xyz, norm.xyz, light_dir, sunlit, amblit, additive, atten);
    
    vec3 sunlit_linear = srgb_to_linear(sunlit);
    vec3 amblit_linear = amblit;

    vec3 ambenv;
    vec3 glossenv;
    vec3 legacyenv;
    sampleReflectionProbesLegacy(ambenv, glossenv, legacyenv, pos.xy*0.5+0.5, pos.xyz, norm.xyz, glossiness, env);
    
    // use sky settings ambient or irradiance map sample, whichever is brighter
    color = max(amblit_linear, ambenv);

    float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
    vec3 sun_contrib = min(da, shadow) * sunlit_linear;
    color.rgb += sun_contrib;
    color *= diffcol.rgb;

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    float glare = 0.0;

    if (glossiness > 0.0)  // specular reflection
    {
        float sa        = dot(normalize(refnormpersp), light_dir.xyz);
        vec3  dumbshiny = sunlit_linear * shadow * (texture2D(lightFunc, vec2(sa, glossiness)).r);

        // add the two types of shiny together
        vec3 spec_contrib = dumbshiny * spec.rgb;
        bloom             = dot(spec_contrib, spec_contrib) / 6;

        glare = max(spec_contrib.r, spec_contrib.g);
        glare = max(glare, spec_contrib.b);

        color += spec_contrib;

        applyGlossEnv(color, glossenv, spec, pos.xyz, norm.xyz);
    }

    color = mix(color.rgb, diffcol.rgb, emissive);

    if (env > 0.0)
    {  // add environmentmap
        applyLegacyEnv(color, legacyenv, spec, pos.xyz, norm.xyz, env);

        float cur_glare = max(max(legacyenv.r, legacyenv.g), legacyenv.b);
        cur_glare *= env*4.0;
        glare += cur_glare;
    }

    color.rgb = linear_to_srgb(color.rgb);
    color.rgb = atmosFragLightingLinear(color.rgb, additive, atten); 
    color.rgb = scaleSoftClipFragLinear(color.rgb);
    color.rgb = srgb_to_linear(color.rgb);

    vec3 npos = normalize(-pos.xyz);
    vec3 light = vec3(0, 0, 0);

#define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, npos, diffuse.rgb, spec, pos.xyz, norm.xyz, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, glare, light_attenuation[i].w );

    LIGHT_LOOP(1)
        LIGHT_LOOP(2)
        LIGHT_LOOP(3)
        LIGHT_LOOP(4)
        LIGHT_LOOP(5)
        LIGHT_LOOP(6)
        LIGHT_LOOP(7)

    light *= 1.0-emissive;
    color += light;

    glare *= 1.0-emissive;
    glare = min(glare, 1.0);
    float al = max(diffcol.a, glare) * vertex_color.a;
    
#ifdef WATER_FOG
    vec4 temp = applyWaterFogView(pos, vec4(color, 0.0));
    color = temp.rgb;
#endif

    frag_color = vec4(color, al);

#else // mode is not DIFFUSE_ALPHA_MODE_BLEND, encode to gbuffer 
    // deferred path               // See: C++: addDeferredAttachment(), shader: softenLightF.glsl
    frag_data[0] = vec4(diffcol.rgb, emissive);        // gbuffer is sRGB for legacy materials
    frag_data[1] = vec4(spec.rgb, glossiness);           // XYZ = Specular color. W = Specular exponent.
    frag_data[2] = vec4(encode_normal(norm), env, GBUFFER_FLAG_HAS_ATMOS);;   // XY = Normal.  Z = Env. intensity. W = 1 skip atmos (mask off fog)
#endif
}

