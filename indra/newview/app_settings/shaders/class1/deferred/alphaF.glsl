/** 
 * @file alphaF.glsl
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

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform float display_gamma;
uniform vec4 gamma;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;

#ifdef USE_DIFFUSE_TEX
uniform sampler2D diffuseMap;
#endif

VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform int sun_up_factor;
uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec4 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

vec2 encode_normal (vec3 n);
vec3 scaleSoftClipFrag(vec3 l);
vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten);

#if defined(VERT_ATMOSPHERICS)
vec3 getSunlitColor();
vec3 getAmblitColor();
vec3 getAdditiveColor();
vec3 getAtmosAttenuation();
void calcAtmospherics(vec3 inPositionEye, float ambFactor);
#else
void calcFragAtmospherics(vec3 inPositionEye, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);
#endif

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, float ambiance ,float shadow)
{
    //get light vector
    vec3 lv = lp.xyz-v;
    
    //get distance
    float d = length(lv);
    float da = 1.0;
    vec3 col = vec3(0);

    if (d > 0.0 && fa > 0.0)
    {
        //normalize light vector
        lv = normalize(lv);
        vec3 norm = normalize(n);

        da = max(0.0, dot(norm, lv));
        da = clamp(da, 0.0, 1.0);
 
        //distance attenuation
        float dist = d/la;
        float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
        dist_atten *= dist_atten;

        // spotlight coefficient.
        float spot = max(dot(-ln, lv), is_pointlight);
        da *= spot*spot; // GL_SPOT_EXPONENT=2

        // to match spotLight (but not multiSpotLight) *sigh*
        float lit = max(min(da, shadow) * dist_atten,0.0);
        col = lit * light_col * diffuse;

        float amb_da = ambiance;
        amb_da *= dist_atten;
        amb_da += (da*0.5) * ambiance;
        amb_da += (da*da*0.5 + 0.5) * ambiance;
        amb_da = min(amb_da, 1.0f - lit);

        col.rgb += amb_da * light_col * diffuse;
        // no spec for alpha shader...
    }
    col = max(col, vec3(0));
    return col;
}

void main() 
{
    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
    frag *= screen_res;
    
    vec4 pos = vec4(vary_position, 1.0);
    vec3 norm = vary_norm;

    float shadow = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);

#ifdef USE_INDEXED_TEX
    vec4 diff = diffuseLookup(vary_texcoord0.xy);
#else
    vec4 diff = texture2D(diffuseMap,vary_texcoord0.xy);
#endif

#ifdef FOR_IMPOSTOR
    vec4 color;
    color.rgb = diff.rgb;
    color.a = 1.0;

#ifdef USE_VERTEX_COLOR
    float final_alpha = diff.a * vertex_color.a;
    diff.rgb *= vertex_color.rgb;
#else
    float final_alpha = diff.a;
#endif
    
    // Insure we don't pollute depth with invis pixels in impostor rendering
    //
    if (final_alpha < 0.01)
    {
        discard;
    }
#else
    
#ifdef USE_VERTEX_COLOR
    float final_alpha = diff.a * vertex_color.a;
    diff.rgb *= vertex_color.rgb;
#else
    float final_alpha = diff.a;
#endif

    diff.rgb = srgb_to_linear(diff.rgb);

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

#if defined(VERT_ATMOSPHERICS)
    sunlit = getSunlitColor();
    amblit = getAmblitColor();
    additive = getAdditiveColor();
    atten = getAtmosAttenuation();
#else
    calcFragAtmospherics(pos.xyz, 1.0, sunlit, amblit, additive, atten);
#endif

    vec2 abnormal   = encode_normal(norm.xyz);

    vec3 light_dir = (sun_up_factor == 1) ? sun_dir: moon_dir;
    float da = dot(norm.xyz, light_dir.xyz);
          da = clamp(da, 0.0, 1.0);

    vec4 color = vec4(0,0,0,0);

    color.rgb = amblit;
    color.a   = final_alpha;

    float ambient = abs(da);
    ambient *= 0.5;
    ambient *= ambient;
    ambient = 1.0 - ambient * smoothstep(0.0, 0.3, shadow);

    vec3 sun_contrib = min(da, shadow) * sunlit;

    color.rgb *= ambient;
    color.rgb += sun_contrib;
    color.rgb *= diff.rgb;

    //color.rgb = mix(diff.rgb, color.rgb, final_alpha);
    
    color.rgb = atmosFragLighting(color.rgb, additive, atten);
    color.rgb = scaleSoftClipFrag(color.rgb);

    vec4 light = vec4(0,0,0,0);

   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diff.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w, shadow);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    // keep it linear
    //
    color.rgb += light.rgb;

    color.rgb = linear_to_srgb(color.rgb);

#endif

#ifdef WATER_FOG
    color = applyWaterFogView(pos.xyz, color);
#endif

    frag_color = color;
}

