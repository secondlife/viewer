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

uniform mat3 env_mat;
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

uniform mat4 proj_mat;
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

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive, bool use_ao);

#ifdef HAS_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif

float getAmbientClamp();

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, float ambiance)
{
    vec3 col = vec3(0);

	//get light vector
	vec3 lv = lp.xyz-v;

	//get distance
	float dist = length(lv);
	float da = 1.0;

    /*if (dist > la)
    {
        return col;
    }

    clip to projector bounds
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
        dist /= la;

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
        da = max(0.0, da);

		float lit = 0.0f;

        float amb_da = 0.0;//ambiance;
        if (da > 0)
        {
		    lit = max(da * dist_atten,0.0);
            col = lit * light_col * diffuse;
            amb_da += (da*0.5+0.5) * ambiance;
        }
        amb_da += (da*da*0.5 + 0.5) * ambiance;
        amb_da *= dist_atten;
        amb_da = min(amb_da, 1.0f - lit);

        // SL-10969 ... need to work out why this blows out in many setups...
        //col.rgb += amb_da * light_col * diffuse;

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

    float shadow = 1.0f;

#ifdef HAS_SHADOW
    shadow = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

#ifdef USE_DIFFUSE_TEX
    vec4 diffuse_tap = texture2D(diffuseMap,vary_texcoord0.xy);
#endif

#ifdef USE_INDEXED_TEX
    vec4 diffuse_tap = diffuseLookup(vary_texcoord0.xy);
#endif

    vec4 diffuse_srgb = diffuse_tap;
    vec4 diffuse_linear = vec4(srgb_to_linear(diffuse_srgb.rgb), diffuse_srgb.a);

#ifdef FOR_IMPOSTOR
    vec4 color;
    color.rgb = diffuse_srgb.rgb;
    color.a = 1.0;

    float final_alpha = diffuse_srgb.a * vertex_color.a;
    diffuse_srgb.rgb *= vertex_color.rgb;
    diffuse_linear.rgb *= vertex_color.rgb;
    
    // Insure we don't pollute depth with invis pixels in impostor rendering
    //
    if (final_alpha < 0.01)
    {
        discard;
    }
#else
    
    vec3 light_dir = (sun_up_factor == 1) ? sun_dir: moon_dir;

    float final_alpha = diffuse_linear.a;

#ifdef USE_VERTEX_COLOR
    final_alpha *= vertex_color.a;
    diffuse_srgb.rgb *= vertex_color.rgb;
    diffuse_linear.rgb *= vertex_color.rgb;
#endif

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVars(pos.xyz, light_dir, 1.0, sunlit, amblit, additive, atten, false);

    vec2 abnormal = encode_normal(norm.xyz);

    float da = dot(norm.xyz, light_dir.xyz);
          da = clamp(da, -1.0, 1.0);
 
    float final_da = da;
          final_da = clamp(final_da, 0.0f, 1.0f);

    vec4 color = vec4(0.0);

    color.a   = final_alpha;

    float ambient = da;
    ambient *= 0.5;
    ambient *= ambient;
    ambient = (1.0 - ambient);

    vec3 sun_contrib = min(final_da, shadow) * sunlit;

#if !defined(AMBIENT_KILL)
    color.rgb = amblit;
    color.rgb *= ambient;
#endif

vec3 post_ambient = color.rgb;

#if !defined(SUNLIGHT_KILL)
    color.rgb += sun_contrib;
#endif

vec3 post_sunlight = color.rgb;

    color.rgb *= diffuse_srgb.rgb;

vec3 post_diffuse = color.rgb;

    color.rgb = atmosFragLighting(color.rgb, additive, atten);

vec3 post_atmo = color.rgb;

    vec4 light = vec4(0,0,0,0);

    // to linear!
    color.rgb = srgb_to_linear(color.rgb);

   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diffuse_linear.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

    // sum local light contrib in linear colorspace
#if !defined(LOCAL_LIGHT_KILL)
    color.rgb += light.rgb;
#endif

    color.rgb = scaleSoftClipFrag(color.rgb);

    // back to sRGB as we're going directly to the final RT post-deferred gamma correction
    color.rgb = linear_to_srgb(color.rgb);

//color.rgb = amblit;
//color.rgb = vec3(ambient);
//color.rgb = sunlit;
//color.rgb = vec3(final_da);
//color.rgb = post_ambient;
//color.rgb = post_sunlight;
//color.rgb = sun_contrib;
//color.rgb = diffuse_srgb.rgb;
//color.rgb = post_diffuse;
//color.rgb = post_atmo;

#ifdef WATER_FOG
    color = applyWaterFogView(pos.xyz, color);
#endif // WATER_FOG

#endif

    frag_color = color;
}

