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

// Debug stub for non-pbr material

#define DIFFUSE_ALPHA_MODE_BLEND    1

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
out vec4 frag_color;
#else
out vec4 frag_data[4];
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

#ifdef HAS_ALPHA_MASK
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

#ifdef HAS_ALPHA_MASK
#if DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND
    if (diffcol.a*vertex_color.a < minimum_alpha)
#else
    if (diffcol.a < minimum_alpha)
#endif
    {
        discard;
    }
#endif

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    frag_color = vec4(1,0,0,0.5);
#else
    // emissive red PBR material
    frag_data[0] = vec4(0,0,0,0);
    frag_data[1] = vec4(0,0,0,0);
    frag_data[2] = vec4(1,0,0,GBUFFER_FLAG_HAS_PBR);
    frag_data[3] = vec4(1,0,0,0);
#endif
}

