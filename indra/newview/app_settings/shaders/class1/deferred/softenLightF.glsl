/** 
 * @file class1/deferred/softenLightF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect positionMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2DRect depthMap;
uniform samplerCube environmentMap;
uniform sampler2D     lightFunc;

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
//uniform vec4 camPosWorld;
uniform vec4 gamma;
uniform float global_gamma;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int sun_up_factor;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 getNorm(vec2 pos_screen);
vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);

float getAmbientClamp();
void calcAtmosphericVars(vec3 inPositionEye, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten);

vec3 scaleSoftClipFrag(vec3 l);
vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    float depth = texture2DRect(depthMap, tc.xy).r;
    vec4 pos = getPositionWithDepth(tc, depth);
    vec4 norm = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz = getNorm(tc);
    
    float light_gamma = 1.0/1.3;
    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;
    
    float da = dot(normalize(norm.xyz), light_dir.xyz);
          da = clamp(da, -1.0, 1.0);

    float final_da = da;
          final_da = clamp(final_da, 0.0, 1.0);
	      //final_da = pow(final_da, light_gamma);

    vec4 gamma_diff = texture2DRect(diffuseRect, tc);
    vec4 diffuse = gamma_diff;
         diffuse.rgb = srgb_to_linear(gamma_diff.rgb);

    vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
    vec3 col;
    float bloom = 0.0;
    {
        vec3 sunlit;
        vec3 amblit;
        vec3 additive;
        vec3 atten;

        calcAtmosphericVars(pos.xyz, 1.0, sunlit, amblit, additive, atten);

        float ambient = da;
        ambient *= 0.5;
        ambient *= ambient;
        ambient = 1.0 - ambient;

        vec3 sun_contrib = final_da * sunlit;

        col.rgb = amblit;
        col.rgb *= ambient;

vec3 post_ambient = col.rgb;

        col.rgb += sun_contrib;

vec3 post_sunlight = col.rgb;

        col.rgb *= gamma_diff.rgb;

vec3 post_diffuse = col.rgb;

        vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

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
 
        col.rgb += diffuse.a * diffuse.rgb;

        if (envIntensity > 0.0)
        { //add environmentmap
            vec3 env_vec = env_mat * refnormpersp;
            vec3 refcol = textureCube(environmentMap, env_vec).rgb;
            col = mix(col.rgb, refcol, envIntensity); 
        }
                
        if (norm.w < 1)
        {
            col = atmosFragLighting(col, additive, atten);
            col = scaleSoftClipFrag(col);
        }

vec3 post_atmo = col.rgb;

        #ifdef WATER_FOG
            vec4 fogged = applyWaterFogView(pos.xyz,vec4(col, bloom));
            col = fogged.rgb;
            bloom = fogged.a;
        #endif
    }

    frag_color.rgb = col.rgb;
    frag_color.a = bloom;
}
