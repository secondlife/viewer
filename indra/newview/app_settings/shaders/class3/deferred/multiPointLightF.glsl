/**
 * @file class3\deferred\multiPointLightF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
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

#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect depthMap;
uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect emissiveRect; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
uniform sampler2D     noiseMap;
uniform sampler2D     lightFunc;

uniform vec3  env_mat[3];
uniform float sun_wash;
uniform int   light_count;
uniform vec4  light[LIGHT_COUNT];
uniform vec4  light_col[LIGHT_COUNT];

uniform vec2  screen_res;
uniform float far_z;
uniform mat4  inv_proj;

VARYING vec4 vary_fragcoord;

void calcHalfVectors(vec3 h, vec3 n, vec3 npos, out float nh, out float nv, out float vh);
vec4 getPosition(vec2 pos_screen);
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity);
vec2 getScreenXY(vec4 clip);
vec3 srgb_to_linear(vec3 c);

void main()
{
#if defined(LOCAL_LIGHT_KILL)
    discard;  // Bail immediately
#else
    vec3 final_color = vec3(0, 0, 0);
    vec2 tc          = getScreenXY(vary_fragcoord);
    vec3 pos         = getPosition(tc).xyz;
    if (pos.z < far_z)
    {
        discard;
    }

    float envIntensity; // not used for this shader
    vec3 n;
    vec4 norm = getNormalEnvIntensityFlags(tc, n, envIntensity); // need `norm.w` for GET_GBUFFER_FLAG()

    vec4 spec    = texture2DRect(specularRect, tc);
    vec3 diffuse = texture2DRect(diffuseRect, tc).rgb;

    float noise = texture2D(noiseMap, tc/128.0).b;
    vec3  v     = -normalize(pos);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 colorDiffuse  = vec3(0);
        vec3 colorSpec     = vec3(0);
        vec3 colorEmissive = spec.rgb; // PBR sRGB Emissive.  See: pbropaqueF.glsl
        vec3 packedORM     = texture2DRect(emissiveRect, tc).rgb; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl

        final_color = colorDiffuse + colorSpec;
    }
    else
    {
        // As of OSX 10.6.7 ATI Apple's crash when using a variable size loop
        for (int i = 0; i < LIGHT_COUNT; ++i)
        {
            vec3  lv   = light[i].xyz - pos;
            float dist = length(lv);
            dist /= light[i].w;
            if (dist <= 1.0)
            {
                float da = dot(n, lv);
                if (da > 0.0)
                {
                    lv = normalize(lv);
                    da = dot(n, lv);

                    float fa         = light_col[i].a + 1.0;
                    float dist_atten = clamp(1.0 - (dist - 1.0 * (1.0 - fa)) / fa, 0.0, 1.0);
                    dist_atten *= dist_atten;

                    // Tweak falloff slightly to match pre-EEP attenuation
                    // NOTE: this magic number also shows up in a great many other places, search for dist_atten *= to audit
                    dist_atten *= 2.0;

                    dist_atten *= noise;

                    float lit = da * dist_atten;

                    vec3 col = light_col[i].rgb * lit * diffuse;

                    if (spec.a > 0.0)
                    {
                        lit        = min(da * 6.0, 1.0) * dist_atten;
                        vec3  h    = normalize(lv + v);
                        float nh, nv, vh;
                        calcHalfVectors(h, n, v, nh, nv, vh);
                        float sa   = nh;
                        float fres = pow(1 - dot(h, v), 5) * 0.4 + 0.5;

                        float gtdenom = 2 * nh;
                        float gt      = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));

                        if (nh > 0.0)
                        {
                            float scol = fres * texture2D(lightFunc, vec2(nh, spec.a)).r * gt / (nh * da);
                            col += lit * scol * light_col[i].rgb * spec.rgb;
                        }
                    }

                    final_color += col;
                }
            }
        }
    }

    frag_color.rgb = final_color;
    frag_color.a   = 0.0;
#endif // LOCAL_LIGHT_KILL

#ifdef IS_AMD_CARD
    // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage
    // away which leads to unfun crashes and artifacts.
    vec4 dummy1 = light[0];
    vec4 dummy2 = light_col[0];
    vec4 dummy3 = light[LIGHT_COUNT - 1];
    vec4 dummy4 = light_col[LIGHT_COUNT - 1];
#endif
}
