/** 
 * @file class3\deferred\pointLightF.glsl
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

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;
uniform sampler2DRect depthMap;

uniform vec3 env_mat[3];
uniform float sun_wash;

// light params
uniform vec3 color;
uniform float falloff;
uniform float size;

VARYING vec4 vary_fragcoord;
VARYING vec3 trans_center;

uniform vec2 screen_res;

uniform mat4 inv_proj;
uniform vec4 viewport;

void calcHalfVectors(vec3 h, vec3 n, vec3 v, out float nh, out float nv, out float vh);
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity);
vec4 getPosition(vec2 pos_screen);
vec2 getScreenXY(vec4 clip);
vec3 srgb_to_linear(vec3 c);

void main()
{
    vec3 final_color = vec3(0);
    vec2 tc          = getScreenXY(vary_fragcoord);
    vec3 pos         = getPosition(tc).xyz;

    vec3 lv = trans_center.xyz-pos;
    float dist = length(lv);
    if (dist >= size)
    {
        discard;
    }
    dist /= size;

    float envIntensity;
    vec3 n;
    vec4 norm = getNormalEnvIntensityFlags(tc, n, envIntensity); // need `norm.w` for GET_GBUFFER_FLAG()

    float da = dot(n, lv);
    if (da < 0.0)
    {
        discard;
    }

    lv = normalize(lv);
    da = dot(n, lv);

    float noise = texture2D(noiseMap, tc/128.0).b;

    vec3 diffuse = texture2DRect(diffuseRect, tc).rgb;
    vec4 spec    = texture2DRect(specularRect, tc);

    // Common half vectors calcs
    vec3  v = -normalize(pos);
    vec3  h = normalize(lv + v);
    float nh, nv, vh;
    calcHalfVectors(h, n, v, nh, nv, vh);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 colorDiffuse = vec3(0);
        vec3 colorSpec    = vec3(0);

        final_color = colorDiffuse + colorSpec;
    }
    else
    {
        float fa = falloff+1.0;
        float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
        dist_atten *= dist_atten;
        dist_atten *= 2.0;
    
        float lit = da * dist_atten * noise;

        final_color = color.rgb*lit*diffuse;

        if (spec.a > 0.0)
        {
            lit = min(da*6.0, 1.0) * dist_atten;

            float sa = nh;
            float fres = pow(1 - dot(h, v), 5) * 0.4+0.5;
            float gtdenom = 2 * nh;
            float gt = max(0,(min(gtdenom * nv / vh, gtdenom * da / vh)));

            if (nh > 0.0)
            {
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*da);
                final_color += lit*scol*color.rgb*spec.rgb;
            }
        }
    
        if (dot(final_color, final_color) <= 0.0)
        {
            discard;
        }
    }

    frag_color.rgb = final_color;
    frag_color.a = 0.0;
}
