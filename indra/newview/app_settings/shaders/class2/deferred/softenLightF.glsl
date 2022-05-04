/**
 * @file class2/deferred/softenLightF.glsl
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

#define REFMAP_COUNT 8

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
uniform samplerCube   environmentMap;
uniform samplerCube   reflectionMap[REFMAP_COUNT];
uniform sampler2D     lightFunc;

uniform int refmapCount;

uniform vec3 refOrigin[REFMAP_COUNT];

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform mat3 env_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int  sun_up_factor;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec3 getNorm(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
float getAmbientClamp();
vec3  atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3  scaleSoftClipFrag(vec3 l);
vec3  fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3  fullbrightScaleSoftClip(vec3 light);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif



// from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

// original reference implementation:
/*
bool intersect(const Ray &ray) const 
{ 
        float t0, t1; // solutions for t if the ray intersects 
#if 0 
        // geometric solution
        Vec3f L = center - orig; 
        float tca = L.dotProduct(dir); 
        // if (tca < 0) return false;
        float d2 = L.dotProduct(L) - tca * tca; 
        if (d2 > radius2) return false; 
        float thc = sqrt(radius2 - d2); 
        t0 = tca - thc; 
        t1 = tca + thc; 
#else 
        // analytic solution
        Vec3f L = orig - center; 
        float a = dir.dotProduct(dir); 
        float b = 2 * dir.dotProduct(L); 
        float c = L.dotProduct(L) - radius2; 
        if (!solveQuadratic(a, b, c, t0, t1)) return false; 
#endif 
        if (t0 > t1) std::swap(t0, t1); 
 
        if (t0 < 0) { 
            t0 = t1; // if t0 is negative, let's use t1 instead 
            if (t0 < 0) return false; // both t0 and t1 are negative 
        } 
 
        t = t0; 
 
        return true; 
} */

// adapted -- assume that origin is inside sphere, return distance from origin to edge of sphere
float sphereIntersect(vec3 origin, vec3 dir, vec4 sph )
{ 
        float t0, t1; // solutions for t if the ray intersects 

        vec3 center = sph.xyz;
        float radius2 = sph.w * sph.w;

        vec3 L = center - origin; 
        float tca = dot(L,dir);

        float d2 = dot(L,L) - tca * tca; 

        float thc = sqrt(radius2 - d2); 
        t0 = tca - thc; 
        t1 = tca + thc; 
 
        return t1; 
} 

vec3 sampleRefMap(vec3 pos, vec3 dir, float lod)
{
    float wsum = 0.0;

    vec3 col = vec3(0,0,0);

    for (int i = 0; i < refmapCount; ++i)
    //int i = 0;
    {
        float r = 16.0;
        vec3 delta = pos.xyz-refOrigin[i].xyz;
        if (length(delta) < r)
        {
            float w = 1.0/max(dot(delta, delta), r);
            w *= w;
            w *= w;

            // parallax adjustment
            float d = sphereIntersect(pos, dir, vec4(refOrigin[i].xyz, r));

            {
                vec3 v = pos + dir * d;
                v -= refOrigin[i].xyz;
                v = env_mat * v;

                float min_lod = textureQueryLod(reflectionMap[i],v).y; // lower is higher res
                col += textureLod(reflectionMap[i], v, max(min_lod, lod)).rgb*w;
                wsum += w;
            }
        }
    }

    if (wsum > 0.0)
    {
        col *= 1.0/wsum;
    }
    else
    {
        // this pixel not covered by a probe, fallback to "full scene" environment map
        vec3 v = env_mat * dir;
        float min_lod = textureQueryLod(environmentMap, v).y; // lower is higher res
        col = textureLod(environmentMap, v, max(min_lod, lod)).rgb;
    }
    
    return col;
}

vec3 sampleAmbient(vec3 pos, vec3 dir, float lod)
{
    vec3 col = sampleRefMap(pos, dir, lod);

    //desaturate
    vec3 hcol = col *0.5;
    
    col *= 2.0;
    col = vec3(
        col.r + hcol.g + hcol.b,
        col.g + hcol.r + hcol.b,
        col.b + hcol.r + hcol.g
    );
    
    col *= 0.333333;

    return col*0.6; // fudge darker

}

void main()
{
    float reflection_lods = 11; // TODO -- base this on resolution of reflection map instead of hard coding

    vec2  tc           = vary_fragcoord.xy;
    float depth        = texture2DRect(depthMap, tc.xy).r;
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = getNorm(tc);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
    float light_gamma = 1.0 / 1.3;
    da                = pow(da, light_gamma);

    vec4 diffuse     = texture2DRect(diffuseRect, tc);
         diffuse.rgb = linear_to_srgb(diffuse.rgb); // SL-14025
    vec4 spec        = texture2DRect(specularRect, vary_fragcoord.xy);

    vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;
    scol_ambocc      = pow(scol_ambocc, vec2(light_gamma));
    float scol       = max(scol_ambocc.r, diffuse.a);
    float ambocc     = scol_ambocc.g;

    vec3  color = vec3(0);
    float bloom = 0.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVars(pos.xyz, light_dir, ambocc, sunlit, amblit, additive, atten, true);

    //vec3 amb_vec = env_mat * norm.xyz;

    vec3 ambenv = sampleAmbient(pos.xyz, norm.xyz, reflection_lods-1);
    amblit = mix(ambenv, amblit, amblit);
    color.rgb = amblit;
    

    //float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
    //ambient *= 0.5;
    //ambient *= ambient;
    //ambient = (1.0 - ambient);
    //color.rgb *= ambient;

    vec3 sun_contrib = min(da, scol) * sunlit;
    color.rgb += sun_contrib;
    color.rgb *= diffuse.rgb;

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    vec3 env_vec         = env_mat * refnormpersp;
    if (spec.a > 0.0)  // specular reflection
    {
        float sa        = dot(normalize(refnormpersp), light_dir.xyz);
        vec3  dumbshiny = sunlit * scol * (texture2D(lightFunc, vec2(sa, spec.a)).r);

        // add the two types of shiny together
        vec3 spec_contrib = dumbshiny * spec.rgb;
        bloom             = dot(spec_contrib, spec_contrib) / 6;
        color.rgb += spec_contrib;

        // add reflection map - EXPERIMENTAL WORK IN PROGRESS
        
        float lod = (1.0-spec.a)*reflection_lods;
        vec3 reflected_color = sampleRefMap(pos.xyz, normalize(refnormpersp), lod);
        reflected_color *= 0.5; // fudge darker, not sure where there's a multiply by two and it's late
        float fresnel = 1.0+dot(normalize(pos.xyz), norm.xyz);
        fresnel += spec.a;
        fresnel *= fresnel;
        //fresnel *= spec.a;
        reflected_color *= spec.rgb*min(fresnel, 1.0);
        //reflected_color = srgb_to_linear(reflected_color);
        vec3 mixer = clamp(color.rgb + vec3(1,1,1) - spec.rgb, vec3(0,0,0), vec3(1,1,1));
        color.rgb = mix(reflected_color, color, mixer);
    }

    color.rgb = mix(color.rgb, diffuse.rgb, diffuse.a);

    if (envIntensity > 0.0)
    {  // add environmentmap
        vec3 reflected_color = sampleRefMap(pos.xyz, normalize(refnormpersp), 0.0);
        float fresnel = 1.0+dot(normalize(pos.xyz), norm.xyz);
        fresnel *= fresnel;
        fresnel = fresnel * 0.95 + 0.05;
        reflected_color *= fresnel;
        color = mix(color.rgb, reflected_color, envIntensity);
    }

    if (norm.w < 0.5)
    {
        color = mix(atmosFragLighting(color, additive, atten), fullbrightAtmosTransportFrag(color, additive, atten), diffuse.a);
        color = mix(scaleSoftClipFrag(color), fullbrightScaleSoftClip(color), diffuse.a);
    }

#ifdef WATER_FOG
    vec4 fogged = applyWaterFogView(pos.xyz, vec4(color, bloom));
    color       = fogged.rgb;
    bloom       = fogged.a;
#endif

    // convert to linear as fullscreen lights need to sum in linear colorspace
    // and will be gamma (re)corrected downstream...
    frag_color.rgb = srgb_to_linear(color.rgb);
}
