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

#define DIFFUSE_ALPHA_MODE_NONE     0
#define DIFFUSE_ALPHA_MODE_BLEND    1
#define DIFFUSE_ALPHA_MODE_MASK     2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

uniform float emissive_brightness;
uniform int sun_up_factor;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3 scaleSoftClipFrag(vec3 l);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);

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

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 npos, vec3 diffuse, vec4 spec, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, inout float glare, float ambiance)
{
    vec3 col = vec3(0);

    //get light vector
    vec3 lv = lp.xyz-v;

    //get distance
    float dist = length(lv);
    float da = 1.0;

    dist /= la;

    /* clip to projector bounds
     vec4 proj_tc = proj_mat * lp;

    if (proj_tc.z < 0
     || proj_tc.z > 1
     || proj_tc.x < 0
     || proj_tc.x > 1 
     || proj_tc.y < 0
     || proj_tc.y > 1)
    {
        return col;
    }*/

    if (dist > 0.0 && la > 0.0)
    {
        //normalize light vector
        lv = normalize(lv);
    
        //distance attenuation
        float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
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
            lit = max(da * dist_atten,0.0);
            col = lit * light_col * diffuse;
            amb_da += (da*0.5+0.5) * ambiance;
        }
        amb_da += (da*da*0.5 + 0.5) * ambiance;
        amb_da *= dist_atten;
        amb_da = min(amb_da, 1.0f - lit);

        // SL-10969 need to see why these are blown out
        //col.rgb += amb_da * light_col * diffuse;

        if (spec.a > 0.0)
        {
            //vec3 ref = dot(pos+lv, norm);
            vec3 h = normalize(lv+npos);
            float nh = dot(n, h);
            float nv = dot(n, npos);
            float vh = dot(npos, h);
            float sa = nh;
            float fres = pow(1 - dot(h, npos), 5)*0.4+0.5;

            float gtdenom = 2 * nh;
            float gt = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));
                                
            if (nh > 0.0)
            {
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*da);
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

    return max(col, vec3(0.0,0.0,0.0));
}

#else
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif
#endif

uniform sampler2D diffuseMap;

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

void main()
{
    vec2 pos_screen = vary_texcoord0.xy;

    vec4 diffuse_tap = texture2D(diffuseMap, vary_texcoord0.xy);

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    vec4 diffuse_srgb = diffuse_tap;
    vec4 diffuse_linear = vec4(srgb_to_linear(diffuse_srgb.rgb), diffuse_srgb.a);
#else
    vec4 diffuse_linear = diffuse_tap;
    vec4 diffuse_srgb = vec4(linear_to_srgb(diffuse_linear.rgb), diffuse_linear.a);
#endif

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
    if (diffuse_linear.a < minimum_alpha)
    {
        discard;
    }
#endif

    diffuse_linear.rgb *= vertex_color.rgb;
    diffuse_srgb.rgb *= linear_to_srgb(vertex_color.rgb);

#ifdef HAS_SPECULAR_MAP
    vec4 spec = texture2D(specularMap, vary_texcoord2.xy);
    spec.rgb *= specular_color.rgb;
#else
    vec4 spec = vec4(specular_color.rgb, 1.0);
#endif

    vec3 norm = vec3(0);
    float bmap_specular = 1.0;

#ifdef HAS_NORMAL_MAP
    vec4 bump_sample = texture2D(bumpMap, vary_texcoord1.xy);
    norm = (bump_sample.xyz * 2) - vec3(1);
    bmap_specular = bump_sample.w;

    // convert sampled normal to tangent space normal
    norm = vec3(dot(norm, vary_mat0),
                dot(norm, vary_mat1),
                dot(norm, vary_mat2));
#else
    norm = vary_normal;
#endif

    norm = normalize(norm);

    vec2 abnormal   = encode_normal(norm);

    vec4 final_color = vec4(diffuse_linear.rgb, 0.0);

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_EMISSIVE)
    final_color.a = diffuse_linear.a;
    final_color.rgb = mix( diffuse_linear.rgb, final_color.rgb*0.5, diffuse_tap.a ); // SL-12171: Fix emissive texture portion being twice as bright.
#endif

    final_color.a = max(final_color.a, emissive_brightness);

    // Texture
    //     [x] Full Bright (emissive_brightness >= 1.0)
    //     Shininess (specular)
    //       [X] Texture
    //       Environment Intensity = 1
    // NOTE: There are two shaders that are used depending on the EI byte value:
    //     EI = 0        fullbright
    //     EI > 0 .. 255 material
    // When it is passed to us it is normalized.
    // We can either modify the output environment intensity
    //   OR
    // adjust the final color via:
    //     final_color *= 0.666666;
    // We don't remap the environment intensity but adjust the final color to closely simulate what non-EEP is doing.
    vec4 final_normal = vec4(abnormal, env_intensity, 0.0);

    vec3 color = vec3(0.0);
    float al   = 0.0;

    if (emissive_brightness >= 1.0)
    {
#ifdef HAS_SPECULAR_MAP
        // Note: We actually need to adjust all 4 channels not just .rgb
        final_color *= 0.666666;
#endif
        color.rgb = final_color.rgb;
        al        = vertex_color.a;
    }

    vec4 final_specular = spec;

    final_specular.a = specular_color.a;

#ifdef HAS_SPECULAR_MAP
    final_specular.a *= bmap_specular;
    final_normal.z *= spec.a;
#endif


#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    if (emissive_brightness <= 1.0)
    {
        //forward rendering, output just lit RGBA
        vec3 pos = vary_position;

        float shadow = 1.0f;

#ifdef HAS_SUN_SHADOW
        shadow = sampleDirectionalShadow(pos.xyz, norm, pos_screen);
#endif

        spec = final_specular;

        float envIntensity = final_normal.z;

        vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;

        float bloom = 0.0;
        vec3 sunlit;
        vec3 amblit;
        vec3 additive;
        vec3 atten;

        calcAtmosphericVars(pos.xyz, light_dir, 1.0, sunlit, amblit, additive, atten, false);

        vec3 refnormpersp = normalize(reflect(pos.xyz, norm));

        float da = dot(norm, normalize(light_dir));
        da = clamp(da, 0.0, 1.0);   // No negative light contributions

        float ambient = da;
        ambient *= 0.5;
        ambient *= ambient;
        ambient = (1.0 - ambient);

        vec3 sun_contrib = min(da, shadow) * sunlit;

// vec3 debug_sun_contrib = sun_contrib;

#if !defined(AMBIENT_KILL)
        color.rgb = amblit;
        color.rgb *= ambient;
#endif

//vec3 debug_post_ambient = color.rgb;

#if !defined(SUNLIGHT_KILL)
        color.rgb += sun_contrib;
#endif

//vec3 debug_post_sunlight = color.rgb;

        //color.rgb *= diffuse_srgb.rgb;
        color.rgb *= diffuse_linear.rgb; // SL-12006

//vec3 debug_post_diffuse = color.rgb;

        float glare = 0.0;

        if (spec.a > 0.0) // specular reflection
        {
            vec3 npos = -normalize(pos.xyz);

            //vec3 ref = dot(pos+lv, norm);
            vec3 h = normalize(light_dir.xyz+npos);
            float nh = dot(norm, h);
            float nv = dot(norm, npos);
            float vh = dot(npos, h);
            float sa = nh;
            float fres = pow(1 - dot(h, npos), 5)*0.4+0.5;

            float gtdenom = 2 * nh;
            float gt = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));

            if (nh > 0.0)
            {
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*da);
                vec3 sp = sun_contrib*scol / 16.0f;
                sp = clamp(sp, vec3(0), vec3(1));
                bloom = dot(sp, sp) / 6.0;
#if !defined(SUNLIGHT_KILL)
                color += sp * spec.rgb;
#endif
            }
        }

//vec3 debug_post_spec = color.rgb;

        if (envIntensity > 0.0)
        {
            //add environmentmap
            vec3 env_vec = env_mat * refnormpersp;
            
            vec3 reflected_color = textureCube(environmentMap, env_vec).rgb;

#if !defined(SUNLIGHT_KILL)
            color = mix(color.rgb, reflected_color, envIntensity);
#endif
            float cur_glare = max(reflected_color.r, reflected_color.g);
            cur_glare = max(cur_glare, reflected_color.b);
            cur_glare *= envIntensity*4.0;
            glare += cur_glare;
        }

//vec3 debug_post_env = color.rgb;

        color = atmosFragLighting(color, additive, atten);

        //convert to linear space before adding local lights
        color = srgb_to_linear(color);

//vec3 debug_post_atmo = color.rgb;

        vec3 npos = normalize(-pos.xyz);

        vec3 light = vec3(0,0,0);

#define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, npos, diffuse_linear.rgb, final_specular, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, glare, light_attenuation[i].w );

            LIGHT_LOOP(1)
            LIGHT_LOOP(2)
            LIGHT_LOOP(3)
            LIGHT_LOOP(4)
            LIGHT_LOOP(5)
            LIGHT_LOOP(6)
            LIGHT_LOOP(7)

        glare = min(glare, 1.0);
        al = max(diffuse_linear.a,glare)*vertex_color.a;

#if !defined(LOCAL_LIGHT_KILL)
        color.rgb += light.rgb;
#endif

        color = scaleSoftClipFrag(color);

        // (only) post-deferred needs inline gamma correction
        color.rgb = linear_to_srgb(color.rgb);

//color.rgb = amblit;
//color.rgb = vec3(ambient);
//color.rgb = sunlit;
//color.rgb = debug_post_ambient;
//color.rgb = vec3(da);
//color.rgb = debug_sun_contrib;
//color.rgb = debug_post_sunlight;
//color.rgb = diffuse_srgb.rgb;
//color.rgb = debug_post_diffuse;
//color.rgb = debug_post_spec;
//color.rgb = debug_post_env;
//color.rgb = debug_post_atmo;

#ifdef WATER_FOG
        vec4 temp = applyWaterFogView(pos, vec4(color.rgb, al));
        color.rgb = temp.rgb;
        al = temp.a;
#endif
    } // !fullbright

    frag_color.rgb = color.rgb;
    frag_color.a   = al;

#else // if DIFFUSE_ALPHA_MODE_BLEND ...

    // deferred path
    frag_data[0] = final_color;
    frag_data[1] = final_specular; // XYZ = Specular color. W = Specular exponent.
    frag_data[2] = final_normal; // XY = Normal.  Z = Env. intensity.
#endif
}

