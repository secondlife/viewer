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
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2DRect depthMap;
uniform samplerCube environmentMap;
uniform sampler2D     lightFunc;

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform mat3 env_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int sun_up_factor;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec3 getNorm(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
float getAmbientClamp();
vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3 scaleSoftClipFrag(vec3 l);
vec3 fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3 fullbrightScaleSoftClip(vec3 light);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    float depth = texture2DRect(depthMap, tc.xy).r;
    vec4 pos = getPositionWithDepth(tc, depth);
    vec4 norm = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz = getNorm(tc);
    
    vec3 light_dir = (sun_up_factor == 1) ? sun_dir : moon_dir;
    float da = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
    float light_gamma = 1.0/1.3;
    da = pow(da, light_gamma);
    
    vec4 diffuse = texture2DRect(diffuseRect, tc);

    //convert to gamma space
    //diffuse.rgb = linear_to_srgb(diffuse.rgb);

    vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
    vec3 color = vec3(0);
    float bloom = 0.0;
    {
        float ambocc = 1.0; // no AO...

        vec3 sunlit;
        vec3 amblit;
        vec3 additive;
        vec3 atten;
    
        calcAtmosphericVars(pos.xyz, light_dir, ambocc, sunlit, amblit, additive, atten, false);

        color.rgb = amblit;

        float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
        ambient *= 0.5;
        ambient *= ambient;
        ambient = (1.0 - ambient);

        color.rgb *= ambient;

        vec3 sun_contrib = da * sunlit;

        color.rgb += sun_contrib;

        color.rgb *= diffuse.rgb;

        vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

        if (spec.a > 0.0) // specular reflection
        {
            float sa        = dot(refnormpersp, light_dir.xyz);
            vec3  dumbshiny = sunlit * (texture2D(lightFunc, vec2(sa, spec.a)).r);

            // add the two types of shiny together
            vec3 spec_contrib = dumbshiny * spec.rgb;
            bloom             = dot(spec_contrib, spec_contrib) / 6;
            color.rgb += spec_contrib;
        }

       color.rgb = mix(color.rgb, diffuse.rgb, diffuse.a);

        if (envIntensity > 0.0)
        { //add environmentmap
            vec3 env_vec = env_mat * refnormpersp;
            vec3 reflected_color = textureCube(environmentMap, env_vec).rgb;
            color = mix(color.rgb, reflected_color, envIntensity);
        }
       
        if (norm.w < 0.5)
        {
            color = mix(atmosFragLighting(color, additive, atten), fullbrightAtmosTransportFrag(color, additive, atten), diffuse.a);
            color = mix(scaleSoftClipFrag(color), fullbrightScaleSoftClip(color), diffuse.a);
        }

        #ifdef WATER_FOG
            vec4 fogged = applyWaterFogView(pos.xyz,vec4(color, bloom));
            color = fogged.rgb;
            bloom = fogged.a;
        #endif

    }

// linear debuggables
//color.rgb = vec3(final_da);
//color.rgb = vec3(ambient);
//color.rgb = vec3(scol);
//color.rgb = diffuse_srgb.rgb;

    // convert to linear as fullscreen lights need to sum in linear colorspace
    // and will be gamma (re)corrected downstream...
    
    frag_color.rgb = srgb_to_linear(color.rgb);
    frag_color.a = bloom;
}
