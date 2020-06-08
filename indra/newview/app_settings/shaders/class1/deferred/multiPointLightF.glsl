/**
 * @file class1/deferred/multiPointLightF.glsl
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

uniform sampler2DRect depthMap;
uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform samplerCube   environmentMap;
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

vec4 getPosition(vec2 pos_screen);
vec3 getNorm(vec2 pos_screen);
vec3 srgb_to_linear(vec3 c);

void main()
{
#if defined(LOCAL_LIGHT_KILL)
    discard;  // Bail immediately
#endif

    vec3 out_col = vec3(0, 0, 0);
    vec2 frag    = (vary_fragcoord.xy * 0.5 + 0.5) * screen_res;
    vec3 pos     = getPosition(frag.xy).xyz;
    if (pos.z < far_z)
    {
        discard;
    }

    vec3 norm = getNorm(frag.xy);

    vec4 spec = texture2DRect(specularRect, frag.xy);
    spec.rgb  = srgb_to_linear(spec.rgb);
    vec3 diff = texture2DRect(diffuseRect, frag.xy).rgb;
    diff.rgb  = srgb_to_linear(diff.rgb);

    float noise = texture2D(noiseMap, frag.xy / 128.0).b;
    vec3  npos  = normalize(-pos);

    // As of OSX 10.6.7 ATI Apple's crash when using a variable size loop
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        vec3  lv   = light[i].xyz - pos;
        float dist = length(lv);
        dist /= light[i].w;
        if (dist <= 1.0)
        {
            float da = dot(norm, lv);
            if (da > 0.0)
            {
                lv = normalize(lv);
                da = dot(norm, lv);

                float fa         = light_col[i].a + 1.0;
                float dist_atten = clamp(1.0 - (dist - 1.0 * (1.0 - fa)) / fa, 0.0, 1.0);
                dist_atten *= dist_atten;

                // Tweak falloff slightly to match pre-EEP attenuation
                // NOTE: this magic number also shows up in a great many other places, search for dist_atten *= to audit
                dist_atten *= 2.0;

                dist_atten *= noise;

                float lit = da * dist_atten;

                vec3 col = light_col[i].rgb * lit * diff;

                if (spec.a > 0.0)
                {
                    lit        = min(da * 6.0, 1.0) * dist_atten;
                    vec3  h    = normalize(lv + npos);
                    float nh   = dot(norm, h);
                    float nv   = dot(norm, npos);
                    float vh   = dot(npos, h);
                    float sa   = nh;
                    float fres = pow(1 - dot(h, npos), 5) * 0.4 + 0.5;

                    float gtdenom = 2 * nh;
                    float gt      = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));

                    if (nh > 0.0)
                    {
                        float scol = fres * texture2D(lightFunc, vec2(nh, spec.a)).r * gt / (nh * da);
                        col += lit * scol * light_col[i].rgb * spec.rgb;
                    }
                }

                out_col += col;
            }
        }
    }

    frag_color.rgb = out_col;
    frag_color.a   = 0.0;

#ifdef IS_AMD_CARD
    // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage
    // awawy which leads to unfun crashes and artifacts.
    vec4 dummy1 = light[0];
    vec4 dummy2 = light_col[0];
    vec4 dummy3 = light[LIGHT_COUNT - 1];
    vec4 dummy4 = light_col[LIGHT_COUNT - 1];
#endif
}
