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

uniform sampler2D diffuseRect;
uniform sampler2D specularRect;
uniform sampler2D normalMap;
uniform sampler2D emissiveRect; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
uniform sampler2D lightFunc;
uniform sampler2D depthMap;

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

void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
float calcLegacyDistanceAttenuation(float distance, float falloff);
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity);
vec4 getPosition(vec2 pos_screen);
vec2 getScreenXY(vec4 clip);
vec2 getScreenCoord(vec4 clip);
vec3 srgb_to_linear(vec3 c);
float getDepth(vec2 tc);

vec3 pbrPunctual(vec3 diffuseColor, vec3 specularColor, 
                    float perceptualRoughness, 
                    float metallic,
                    vec3 n, // normal
                    vec3 v, // surface point to camera
                    vec3 l); //surface point to light

void main()
{
    vec3 final_color = vec3(0);
    vec2 tc          = getScreenCoord(vary_fragcoord);
    vec3 pos         = getPosition(tc).xyz;

    float envIntensity;
    vec3 n;
    vec4 norm = getNormalEnvIntensityFlags(tc, n, envIntensity); // need `norm.w` for GET_GBUFFER_FLAG()

    vec3 diffuse = texture2D(diffuseRect, tc).rgb;
    vec4 spec    = texture2D(specularRect, tc);

    // Common half vectors calcs
    vec3  lv = trans_center.xyz-pos;
    vec3  h, l, v = -normalize(pos);
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

    if (lightDist >= size)
    {
        discard;
    }
    float dist = lightDist / size;
    float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 colorEmissive = texture2D(emissiveRect, tc).rgb; 
        vec3 orm = spec.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;
        
        vec3 diffuseColor = baseColor.rgb*(vec3(1.0)-f0);
        diffuseColor *= 1.0 - metallic;

        vec3 specularColor = mix(f0, baseColor.rgb, metallic);

        vec3 intensity = dist_atten * color * 3.0; // Legacy attenuation
        final_color += intensity*pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic, n.xyz, v, normalize(lv));
    }
    else
    {
        if (nl < 0.0)
        {
            discard;
        }

        diffuse = srgb_to_linear(diffuse);
        spec.rgb = srgb_to_linear(spec.rgb);

        float lit = nl * dist_atten;

        final_color = color.rgb*lit*diffuse;

        if (spec.a > 0.0)
        {
            lit = min(nl*6.0, 1.0) * dist_atten;

            float sa = nh;
            float fres = pow(1 - vh, 5) * 0.4+0.5;
            float gtdenom = 2 * nh;
            float gt = max(0,(min(gtdenom * nv / vh, gtdenom * nl / vh)));

            if (nh > 0.0)
            {
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*nl);
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
