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

#define DIFFUSE_ALPHA_MODE_IGNORE   0
#define DIFFUSE_ALPHA_MODE_BLEND    1
#define DIFFUSE_ALPHA_MODE_MASK     2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

uniform float emissive_brightness;
uniform float display_gamma;
uniform int sun_up_factor;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3 scaleSoftClipFrag(vec3 l);

void calcAtmosphericVars(vec3 inPositionEye, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten);

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
//uniform vec4 camPosWorld;
uniform vec4 gamma;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
VARYING vec2 vary_fragcoord;

VARYING vec3 vary_position;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec4 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 npos, vec3 diffuse, vec4 spec, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, inout float glare, float ambiance)
{
    //get light vector
    vec3 lv = lp.xyz-v;
    
    //get distance
    float d = length(lv);
    
    float da = 1.0;

    vec3 col = vec3(0,0,0);

    if (d > 0.0)
    {
        //normalize light vector
        lv = normalize(lv);
    
        //distance attenuation
        float dist = (la > 0) ? d/la : 1.0f;
        fa += 1.0f;
        float dist_atten = ( fa > 0) ? clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0) : 1.0f;
        dist_atten *= dist_atten;
        dist_atten *= 2.0f;

        // spotlight coefficient.
        float spot = max(dot(-ln, lv), is_pointlight);

        //angular attenuation
        da = dot(n, lv);
        da = clamp(da, 0.0, 1.0);

        da *= spot*spot; // GL_SPOT_EXPONENT=2

        float lit = max(da * dist_atten, 0.0);

        col = light_col*lit*diffuse;
        
        float amb_da = ambiance;
        if (da > 0)
        {
            amb_da += (da*0.5 + 0.5) * ambiance;
        }
        amb_da += (da*da*0.5+0.5) * ambiance;
        amb_da = min(amb_da, 1.0f - lit);

        col.rgb += amb_da * 0.5 * light_col * diffuse;

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

    vec4 diffcol = texture2D(diffuseMap, vary_texcoord0.xy);
    diffcol.rgb *= vertex_color.rgb;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
    if (diffcol.a < minimum_alpha)
    {
        discard;
    }
#endif

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    vec3 gamma_diff = diffcol.rgb;
#endif

#ifdef HAS_SPECULAR_MAP
    vec4 spec = texture2D(specularMap, vary_texcoord2.xy);
    spec.rgb *= specular_color.rgb;
#else
    vec4 spec = vec4(specular_color.rgb, 1.0);
#endif

#ifdef HAS_NORMAL_MAP
    vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);

    norm.xyz = norm.xyz * 2 - 1;

    vec3 tnorm = vec3(dot(norm.xyz,vary_mat0),
              dot(norm.xyz,vary_mat1),
              dot(norm.xyz,vary_mat2));
#else
    vec4 norm = vec4(0,0,0,1.0);
    vec3 tnorm = vary_normal;
#endif

    norm.xyz = tnorm;
    norm.xyz = normalize(norm.xyz);

    vec2 abnormal   = encode_normal(norm.xyz);

    vec4 final_color = diffcol;
    
#if (DIFFUSE_ALPHA_MODE != DIFFUSE_ALPHA_MODE_EMISSIVE)
    final_color.a = emissive_brightness;
#else
    final_color.a = max(final_color.a, emissive_brightness);
#endif

    vec4 final_specular = spec;
#ifdef HAS_SPECULAR_MAP
    vec4 final_normal = vec4(encode_normal(normalize(tnorm)), env_intensity * spec.a, 0.0);
    final_specular.a = specular_color.a * norm.a;
#else
    vec4 final_normal = vec4(encode_normal(normalize(tnorm)), env_intensity, 0.0);
    final_specular.a = specular_color.a;
#endif
    

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
        //forward rendering, output just lit RGBA
    vec3 pos = vary_position;

    float shadow = 1.0f;

#ifdef HAS_SUN_SHADOW
    shadow = sampleDirectionalShadow(pos.xyz, norm.xyz, pos_screen);
#endif
    
    spec = final_specular;
    vec4 diffuse = final_color;

    diffuse.rgb = srgb_to_linear(diffuse.rgb);

    float envIntensity = final_normal.z;

    vec3 col = vec3(0.0f,0.0f,0.0f);

    float bloom = 0.0;
    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVars(pos.xyz, 1.0, sunlit, amblit, additive, atten);

    vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;

    float da = dot(norm.xyz, light_dir.xyz);
          da = clamp(da, -1.0, 1.0);

    float final_da = da;
          final_da = clamp(final_da, 0.0, 1.0);
          final_da = pow(final_da, 1.0 / 1.3);

    float ambient = da;
    ambient *= 0.5;
    ambient *= ambient;
    ambient = max(0.66, ambient);
    ambient = 1.0 - ambient;

    vec3 sun_contrib = min(final_da, shadow) * sunlit;
   
    col.rgb = amblit;
    col.rgb *= ambient;

vec3 post_ambient = col.rgb;

    col.rgb += sun_contrib;

vec3 post_sunlight = col.rgb;

    col.rgb *= diffuse.rgb;
 
vec3 post_diffuse = col.rgb;

    float glare = 0.0;

    if (spec.a > 0.0) // specular reflection
    {
        vec3 npos = -normalize(pos.xyz);

        //vec3 ref = dot(pos+lv, norm);
        vec3 h = normalize(light_dir.xyz+npos);
        float nh = dot(norm.xyz, h);
        float nv = dot(norm.xyz, npos);
        float vh = dot(npos, h);
        float sa = nh;
        float fres = pow(1 - dot(h, npos), 5)*0.4+0.5;

        float gtdenom = 2 * nh;
        float gt = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));
                                
        if (nh > 0.0)
        {
            float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*da);
            vec3 speccol = sun_contrib*scol*spec.rgb;
            speccol = clamp(speccol, vec3(0), vec3(1));
            bloom = dot(speccol, speccol) / 6;
            col += speccol;
        }
    }

vec3 post_spec = col.rgb;

    col = mix(col.rgb, diffuse.rgb, diffuse.a);

    if (envIntensity > 0.0)
    {
        //add environmentmap
        vec3 env_vec = env_mat * refnormpersp;
        
        vec3 refcol = textureCube(environmentMap, env_vec).rgb;

        col = mix(col.rgb, refcol, 
            envIntensity);  

        float cur_glare = max(refcol.r, refcol.g);
        cur_glare = max(cur_glare, refcol.b);
        cur_glare *= envIntensity*4.0;
        glare += cur_glare;
    }

    col = atmosFragLighting(col, additive, atten);
    col = scaleSoftClipFrag(col);

    vec3 npos = normalize(-pos.xyz);
            
    vec3 light = vec3(0,0,0);

    vec3 prelight_linearish_maybe = srgb_to_linear(col.rgb);

 #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, npos, diffuse.rgb, final_specular, pos.xyz, norm.xyz, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, glare, light_attenuation[i].w * 0.5);

        LIGHT_LOOP(1)
        LIGHT_LOOP(2)
        LIGHT_LOOP(3)
        LIGHT_LOOP(4)
        LIGHT_LOOP(5)
        LIGHT_LOOP(6)
        LIGHT_LOOP(7)

vec3 light_linear = light.rgb;

    col.rgb += light.rgb;

vec3 postlight_linear = col.rgb;

    glare = min(glare, 1.0);
    float al = max(diffcol.a,glare)*vertex_color.a;

#ifdef WATER_FOG
    vec4 temp = applyWaterFogView(pos, vec4(col.rgb, al));
    col.rgb = temp.rgb;
    al = temp.a;
#endif

//col.rgb = light_linear;

    col.rgb = linear_to_srgb(col.rgb);

    frag_color.rgb = col.rgb;
    frag_color.a   = al;

#else
    frag_data[0] = final_color;
    frag_data[1] = final_specular; // XYZ = Specular color. W = Specular exponent.
    frag_data[2] = final_normal; // XY = Normal.  Z = Env. intensity.
#endif
}

