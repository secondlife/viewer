/** 
 * @file class3/deferred/softenLightF.glsl
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
uniform sampler2D     lightFunc;

uniform samplerCube environmentMap;
uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
uniform float cloud_shadow;
uniform float max_y;
uniform vec4 glow;
uniform mat3 env_mat;
uniform vec4 shadow_clip;

uniform vec3 sun_dir;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform mat4 inv_modelview;

uniform vec2 screen_res;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;

uniform sampler2D sh_input_r;
uniform sampler2D sh_input_g;
uniform sampler2D sh_input_b;

vec3 GetSunAndSkyIrradiance(vec3 camPos, vec3 norm, vec3 dir, out vec3 sky_irradiance);
vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 dir, out vec3 transmittance);
vec3 GetSkyLuminanceToPoint(vec3 camPos, vec3 pos, float shadow_length, vec3 dir, out vec3 transmittance);

vec3 ColorFromRadiance(vec3 radiance);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);
vec4 getPosition(vec2 pos_screen);
vec3 getNorm(vec2 pos_screen);

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    float depth = texture2DRect(depthMap, tc.xy).r;
    vec3 pos = getPositionWithDepth(tc, depth).xyz;
    vec4 norm = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz = getNorm(tc);

    float da = max(dot(norm.xyz, sun_dir.xyz), 0.0);

    vec4 diffuse = texture2DRect(diffuseRect, tc); // sRGB
    diffuse.rgb = srgb_to_linear(diffuse.rgb);

    vec3 col;
    float bloom = 0.0;
    {
        vec3 camPos = (camPosLocal / 1000.0f) + vec3(0, 0, 6360.0f);

        vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
        
        vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;

        float scol = max(scol_ambocc.r, diffuse.a); 

        float ambocc = scol_ambocc.g;

        vec4 l1tap = vec4(1.0/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265));
        vec4 l1r = texture2D(sh_input_r, vec2(0,0));
        vec4 l1g = texture2D(sh_input_g, vec2(0,0));
        vec4 l1b = texture2D(sh_input_b, vec2(0,0));

        vec3 indirect = vec3(dot(l1r, l1tap * vec4(1, norm.xyz)),
                             dot(l1g, l1tap * vec4(1, norm.xyz)),
                             dot(l1b, l1tap * vec4(1, norm.xyz)));

        indirect = clamp(indirect, vec3(0), vec3(1.0));

        vec3 transmittance;
        vec3 sky_irradiance;
        vec3 sun_irradiance = GetSunAndSkyIrradiance(camPos, norm.xyz, sun_dir, sky_irradiance);
        vec3 inscatter = GetSkyLuminanceToPoint(camPos, (pos / 1000.f) + vec3(0, 0, 6360.0f), scol, sun_dir, transmittance);

        vec3 radiance   = scol * (sun_irradiance + sky_irradiance) + inscatter;
        vec3 atmo_color = ColorFromRadiance(radiance);

        col = atmo_color + indirect;
        col *= transmittance;
        col *= diffuse.rgb;

        vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

        if (spec.a > 0.0) // specular reflection
        {
            // the old infinite-sky shiny reflection
            //
            float sa = dot(refnormpersp, sun_dir.xyz);
            vec3 dumbshiny = scol * texture2D(lightFunc, vec2(sa, spec.a)).r * atmo_color;
            
            // add the two types of shiny together
            vec3 spec_contrib = dumbshiny * spec.rgb * 0.25;
            bloom = dot(spec_contrib, spec_contrib);
            col += spec_contrib;
        }

        col = mix(col, diffuse.rgb, diffuse.a);

        if (envIntensity > 0.0)
        { //add environmentmap
            vec3 env_vec = env_mat * refnormpersp;
            vec3 sun_direction  = (inv_modelview * vec4(sun_dir, 1.0)).xyz;
            vec3 radiance_sun  = GetSkyLuminance(camPos, env_vec, 0.0f, sun_direction, transmittance);
            vec3 refcol = ColorFromRadiance(radiance_sun);
            col = mix(col.rgb, refcol, envIntensity);
        }
                        
        /*if (norm.w < 0.5)
        {
            col = scaleSoftClipFrag(col);
        }*/

        #ifdef WATER_FOG
            vec4 fogged = applyWaterFogView(pos,vec4(col, bloom));
            col = fogged.rgb;
            bloom = fogged.a;
        #endif
    }

    //output linear since gamma correction happens down stream
    frag_color.rgb = col;
    frag_color.a = bloom;
}
