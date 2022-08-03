/** 
 * @file class3\deferred\spotLightF.glsl
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
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

#define DEBUG_PBR_LIGHT_TYPE         0
#define DEBUG_PBR_SPOT               0
#define DEBUG_PBR_NL                 0 // monochome area effected by light
#define DEBUG_PBR_SPOT_DIFFUSE       0
#define DEBUG_PBR_SPOT_SPECULAR      0

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect emissiveRect; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
uniform samplerCube environmentMap;
uniform sampler2DRect lightMap;
uniform sampler2D noiseMap;
uniform sampler2D projectionMap; // rgba
uniform sampler2D lightFunc;

uniform mat4 proj_mat; //screen space to light space
uniform float proj_near; //near clip for projection
uniform vec3 proj_p; //plane projection is emitting from (in screen space)
uniform vec3 proj_n;
uniform float proj_focus; //distance from plane to begin blurring
uniform float proj_lod;  //(number of mips in proj map)
uniform float proj_range; //range between near clip and far clip plane of projection
uniform float proj_ambient_lod;
uniform float proj_ambiance;
uniform float near_clip;
uniform float far_clip;

uniform vec3 proj_origin; //origin of projection to be used for angular attenuation
uniform float sun_wash;
uniform int proj_shadow_idx;
uniform float shadow_fade;

uniform float size;
uniform vec3 color;
uniform float falloff;

VARYING vec3 trans_center;
VARYING vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform mat4 inv_proj;

vec3 BRDFLambertian( vec3 reflect0, vec3 reflect90, vec3 c_diff, float specWeight, float vh );
vec3 BRDFSpecularGGX( vec3 reflect0, vec3 reflect90, float alphaRoughness, float specWeight, float vh, float nl, float nv, float nh );
void calcHalfVectors(vec3 lv, vec3 n, vec3 v, out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist);
bool clipProjectedLightVars(vec3 center, vec3 pos, out float dist, out float l_dist, out vec3 lv, out vec4 proj_tc );
vec3 getLightIntensitySpot(vec3 lightColor, float lightRange, float lightDistance, vec3 v);
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity);
vec3 getProjectedLightDiffuseColor(float light_distance, vec2 projected_uv );
vec3 getProjectedLightSpecularColor(vec3 pos, vec3 n);
vec2 getScreenXY(vec4 clip_point);
void initMaterial( vec3 diffuse, vec3 packedORM, out float alphaRough, out vec3 c_diff, out vec3 reflect0, out vec3 reflect90, out float specWeight );
vec3 srgb_to_linear(vec3 c);
vec4 texture2DLodSpecular(vec2 tc, float lod);

vec4 texture2DLodAmbient(sampler2D projectionMap, vec2 tc, float lod)
{
    vec4 ret = texture2DLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = tc-vec2(0.5);
    float d = dot(dist,dist);
    ret *= min(clamp((0.25-d)/0.25, 0.0, 1.0), 1.0);

    return ret;
}

vec4 getPosition(vec2 pos_screen);

void main()
{
#if defined(LOCAL_LIGHT_KILL)
    discard;
#else
    vec3 final_color = vec3(0,0,0);
    vec2 tc          = getScreenXY(vary_fragcoord);
    vec3 pos         = getPosition(tc).xyz;

    vec3 lv;
    vec4 proj_tc;
    float dist, l_dist;
    if (clipProjectedLightVars(trans_center, pos, dist, l_dist, lv, proj_tc))
    {
        discard;
    }

    float shadow = 1.0;

    if (proj_shadow_idx >= 0)
    {
        vec4 shd = texture2DRect(lightMap, tc);
        shadow = (proj_shadow_idx == 0) ? shd.b : shd.a;
        shadow += shadow_fade;
        shadow = clamp(shadow, 0.0, 1.0);
    }

    float envIntensity;
    vec3 n;
    vec4 norm = getNormalEnvIntensityFlags(tc, n, envIntensity); // need `norm.w` for GET_GBUFFER_FLAG()

    float dist_atten = 1.0 - (dist + falloff)/(1.0 + falloff);

    lv = proj_origin-pos.xyz; // NOTE: Re-using lv
    vec3  h, l, v = -normalize(pos);
    float nh, nl, nv, vh, lightDist;
    calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

    vec3 diffuse = texture2DRect(diffuseRect, tc).rgb;
    vec4 spec    = texture2DRect(specularRect, tc);
    vec3 dlit    = vec3(0, 0, 0);
    vec3 slit    = vec3(0, 0, 0);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
        vec3 colorDiffuse  = vec3(0);
        vec3 colorSpec     = vec3(0);
        vec3 colorEmissive = spec.rgb; // PBR sRGB Emissive.  See: pbropaqueF.glsl
        vec3 packedORM     = texture2DRect(emissiveRect, tc).rgb; // PBR linear packed Occlusion, Roughness, Metal. See: pbropaqueF.glsl
        float metal        = packedORM.b;

//        if (proj_tc.x > 0.0 && proj_tc.x < 1.0
//        &&  proj_tc.y > 0.0 && proj_tc.y < 1.0)
        if (nl > 0.0)
        {
            vec3 c_diff, reflect0, reflect90;
            float alphaRough, specWeight;
            initMaterial( diffuse, packedORM, alphaRough, c_diff, reflect0, reflect90, specWeight );

            dlit = getProjectedLightDiffuseColor( l_dist, proj_tc.xy );
            slit = getProjectedLightSpecularColor( pos, n );

//          vec3 intensity = getLightIntensitySpot( color, size, lightDist, v );
//          colorDiffuse = shadow * dlit * nl;
//          colorSpec    = shadow * slit * nl;

//            colorDiffuse *= BRDFLambertian ( reflect0, reflect90, c_diff    , specWeight, vh );
//            colorSpec    *= BRDFSpecularGGX( reflect0, reflect90, alphaRough, specWeight, vh, nl, nv, nh );

            colorDiffuse = shadow * dlit * nl * dist_atten;
            colorSpec    = shadow * slit * nl * dist_atten;

  #if DEBUG_PBR_SPOT_DIFFUSE
            colorDiffuse = dlit.rgb; colorSpec = vec3(0);
  #endif
  #if DEBUG_PBR_SPOT_SPECULAR
            colorDiffuse = vec3(0); colorSpec = slit.rgb;
  #endif
  #if DEBUG_PBR_SPOT
            colorDiffuse = dlit; colorSpec = vec3(0);
            colorDiffuse *= nl;
            colorDiffuse *= shadow;
  #endif

        }

  #if DEBUG_SPOT_DIFFUSE
        colorDiffuse = vec3(nl * dist_atten);
  #endif
  #if DEBUG_PBR_NL
            colorDiffuse = vec3(nl); colorSpec = vec3(0);
  #endif
  #if DEBUG_PBR_LIGHT_TYPE
        colorDiffuse = vec3(0.5,0,0); colorSpec = vec3(0.0);
  #endif

        final_color = colorDiffuse + colorSpec;
    }
    else
    {
        dist_atten *= dist_atten;
        dist_atten *= 2.0;

        if (dist_atten <= 0.0)
        {
            discard;
        }

        float noise = texture2D(noiseMap, tc/128.0).b;
        if (proj_tc.z > 0.0 &&
            proj_tc.x < 1.0 &&
            proj_tc.y < 1.0 &&
            proj_tc.x > 0.0 &&
            proj_tc.y > 0.0)
        {
            float amb_da = proj_ambiance;
            float lit = 0.0;

            if (nl > 0.0)
            {
                lit = nl * dist_atten * noise;

                dlit = getProjectedLightDiffuseColor( l_dist, proj_tc.xy );

                final_color = dlit*lit*diffuse*shadow;

                amb_da += (nl*0.5+0.5) /* * (1.0-shadow) */ * proj_ambiance;
            }

            //float diff = clamp((proj_range-proj_focus)/proj_range, 0.0, 1.0);
            vec4 amb_plcol = texture2DLodAmbient(projectionMap, proj_tc.xy, proj_lod);

            amb_da += (nl*nl*0.5+0.5) /* * (1.0-shadow) */ * proj_ambiance;
            amb_da *= dist_atten * noise;
            amb_da = min(amb_da, 1.0-lit);

            final_color += amb_da*color.rgb*diffuse.rgb*amb_plcol.rgb*amb_plcol.a;
        }

        if (spec.a > 0.0)
        {
            dlit *= min(nl*6.0, 1.0) * dist_atten;
            float fres = pow(1 - dot(h, v), 5)*0.4+0.5;

            float gtdenom = 2 * nh;
            float gt = max(0, min(gtdenom * nv / vh, gtdenom * nl / vh));

            if (nh > 0.0)
            {
                float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*nl);
                vec3 speccol = dlit*scol*spec.rgb*shadow;
                speccol = clamp(speccol, vec3(0), vec3(1));
                final_color += speccol;
            }
        }

        if (envIntensity > 0.0)
        {
            vec3 ref = reflect(normalize(pos), n);

            //project from point pos in direction ref to plane proj_p, proj_n
            vec3 pdelta = proj_p-pos;
            float ds = dot(ref, proj_n);

            if (ds < 0.0)
            {
                vec3 pfinal = pos + ref * dot(pdelta, proj_n)/ds;

                vec4 stc = (proj_mat * vec4(pfinal.xyz, 1.0));

                if (stc.z > 0.0)
                {
                    stc /= stc.w;

                    if (stc.x < 1.0 &&
                        stc.y < 1.0 &&
                        stc.x > 0.0 &&
                        stc.y > 0.0)
                    {
                        final_color += color.rgb * texture2DLodSpecular(stc.xy, (1 - spec.a) * (proj_lod * 0.6)).rgb * shadow * envIntensity;
                    }
                }
            }
        }
    }

    //not sure why, but this line prevents MATBUG-194
    final_color = max(final_color, vec3(0.0));

    //output linear colors as gamma correction happens down stream
    frag_color.rgb = final_color;
    frag_color.a = 0.0;
#endif // LOCAL_LIGHT_KILL
}
