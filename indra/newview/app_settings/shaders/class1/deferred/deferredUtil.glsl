/** 
 * @file class1/deferred/deferredUtil.glsl
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

uniform sampler2DRect   normalMap;
uniform sampler2DRect   depthMap;
uniform sampler2D projectionMap; // rgba

// projected lighted params
uniform mat4 proj_mat; //screen space to light space projector
uniform vec3 proj_n; // projector normal
uniform vec3 proj_p; //plane projection is emitting from (in screen space)
uniform float proj_focus; // distance from plane to begin blurring
uniform float proj_lod  ; // (number of mips in proj map)
uniform float proj_range; // range between near clip and far clip plane of projection

// light params
uniform vec3 color; // light_color
uniform float size; // light_size

uniform mat4 inv_proj;
uniform vec2 screen_res;

const float M_PI = 3.14159265;

vec3 srgb_to_linear(vec3 cs);

// In:
//   lv  unnormalized surface to light vector
//   n   normal of the surface
//   pos unnormalized camera to surface vector
// Out:
//   l   normalized surace to light vector
//   nl  diffuse angle
//   nh  specular angle
void calcHalfVectors(vec3 lv, vec3 n, vec3 v,
    out vec3 h, out vec3 l, out float nh, out float nl, out float nv, out float vh, out float lightDist)
{
    l  = normalize(lv);
    h  = normalize(l + v);
    nh = clamp(dot(n, h), 0.0, 1.0);
    nl = clamp(dot(n, l), 0.0, 1.0);
    nv = clamp(dot(n, v), 0.0, 1.0);
    vh = clamp(dot(v, h), 0.0, 1.0);

    lightDist = length(lv);
}

// In:
//   light_center
//   pos
// Out:
//   dist
//   l_dist
//   lv
//   proj_tc  Projector Textue Coordinates
bool clipProjectedLightVars(vec3 light_center, vec3 pos, out float dist, out float l_dist, out vec3 lv, out vec4 proj_tc )
{
    lv = light_center - pos.xyz;
    dist = length(lv);
    bool clipped = (dist >= size);
    if ( !clipped )
    {
        dist /= size;

        l_dist = -dot(lv, proj_n);
        vec4 projected_point = (proj_mat * vec4(pos.xyz, 1.0));
        clipped = (projected_point.z < 0.0);
        projected_point.xyz /= projected_point.w;
        proj_tc = projected_point;
    }

    return clipped;
}

vec2 getScreenCoordinate(vec2 screenpos)
{
    vec2 sc = screenpos.xy * 2.0;
    if (screen_res.x > 0 && screen_res.y > 0)
    {
       sc /= screen_res;
    }
    return sc - vec2(1.0, 1.0);
}

// See: https://aras-p.info/texts/CompactNormalStorage.html
//      Method #4: Spheremap Transform, Lambert Azimuthal Equal-Area projection
vec3 getNorm(vec2 screenpos)
{
   vec2 enc = texture2DRect(normalMap, screenpos.xy).xy;
   vec2 fenc = enc*4-2;
   float f = dot(fenc,fenc);
   float g = sqrt(1-f/4);
   vec3 n;
   n.xy = fenc*g;
   n.z = 1-f/2;
   return n;
}

vec3 getNormalFromPacked(vec4 packedNormalEnvIntensityFlags)
{
   vec2 enc = packedNormalEnvIntensityFlags.xy;
   vec2 fenc = enc*4-2;
   float f = dot(fenc,fenc);
   float g = sqrt(1-f/4);
   vec3 n;
   n.xy = fenc*g;
   n.z = 1-f/2;
   return normalize(n); // TODO: Is this normalize redundant?
}

// return packedNormalEnvIntensityFlags since GBUFFER_FLAG_HAS_PBR needs .w
// See: C++: addDeferredAttachments(), GLSL: softenLightF
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity)
{
    vec4 packedNormalEnvIntensityFlags = texture2DRect(normalMap, screenpos.xy);
    n = getNormalFromPacked( packedNormalEnvIntensityFlags );
    envIntensity = packedNormalEnvIntensityFlags.z;
    return packedNormalEnvIntensityFlags;
}

float getDepth(vec2 pos_screen)
{
    float depth = texture2DRect(depthMap, pos_screen).r;
    return depth;
}

vec4 getTexture2DLodDiffuse(vec2 tc, float lod)
{
    vec4 ret = texture2DLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc-vec2(0.5));
    float det = min(lod/(proj_lod*0.5), 1.0);
    float d = min(dist.x, dist.y);
    float edge = 0.25*det;
    ret *= clamp(d/edge, 0.0, 1.0);

    return ret;
}

// Returns projected light in Linear
// Uses:
//  color
// NOTE: projected.a will be pre-multiplied with projected.rgb
vec3 getProjectedLightDiffuseColor(float light_distance, vec2 projected_uv)
{
    float diff = clamp((light_distance - proj_focus)/proj_range, 0.0, 1.0);
    float lod = diff * proj_lod;
    vec4 plcol = getTexture2DLodDiffuse(projected_uv.xy, lod);

    return color.rgb * plcol.rgb * plcol.a;
}

vec4 texture2DLodSpecular(vec2 tc, float lod)
{
    vec4 ret = texture2DLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc-vec2(0.5));
    float det = min(lod/(proj_lod*0.5), 1.0);
    float d = min(dist.x, dist.y);
    d *= min(1, d * (proj_lod - lod)); // BUG? extra factor compared to diffuse causes N repeats
    float edge = 0.25*det;
    ret *= clamp(d/edge, 0.0, 1.0);

    return ret;
}

// See: clipProjectedLightVars()
vec3 getProjectedLightSpecularColor(vec3 pos, vec3 n )
{
    vec3 slit = vec3(0);
    vec3 ref = reflect(normalize(pos), n);

    //project from point pos in direction ref to plane proj_p, proj_n
    vec3 pdelta = proj_p-pos;
    float l_dist = length(pdelta);
    float ds = dot(ref, proj_n);
    if (ds < 0.0)
    {
        vec3 pfinal = pos + ref * dot(pdelta, proj_n)/ds;
        vec4 stc = (proj_mat * vec4(pfinal.xyz, 1.0));
        if (stc.z > 0.0)
        {
            stc /= stc.w;
            slit = getProjectedLightDiffuseColor( l_dist, stc.xy ); // NOTE: Using diffuse due to texture2DLodSpecular() has extra: d *= min(1, d * (proj_lod - lod));
        }
    }
    return slit; // specular light
}

vec3 getProjectedLightSpecularColor(float light_distance, vec2 projected_uv)
{
    float diff = clamp((light_distance - proj_focus)/proj_range, 0.0, 1.0);
    float lod = diff * proj_lod;
    vec4 plcol = getTexture2DLodDiffuse(projected_uv.xy, lod); // NOTE: Using diffuse due to texture2DLodSpecular() has extra: d *= min(1, d * (proj_lod - lod));

    return color.rgb * plcol.rgb * plcol.a;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec4 getPositionWithDepth(vec2 pos_screen, float depth)
{
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec2 getScreenXY(vec4 clip)
{
    vec4 ndc = clip;
         ndc.xyz /= clip.w;
    vec2 screen = vec2( ndc.xy * 0.5 );
         screen += 0.5;
         screen *= screen_res;
    return screen;
}

// PBR Utils

vec3 fresnelSchlick( vec3 reflect0, vec3 reflect90, float vh)
{
    return reflect0 + (reflect90 - reflect0) * pow(clamp(1.0 - vh, 0.0, 1.0), 5.0);
}

// Approximate Environment BRDF
vec2 getGGXApprox( vec2 uv )
{
    // Reference: Physically Based Shading on Mobile
    // https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
    //     EnvBRDFApprox( vec3 SpecularColor, float Roughness, float NoV )
    float nv        = uv.x;
    float roughness = uv.y;

    const vec4  c0        = vec4( -1, -0.0275, -0.572, 0.022 );
    const vec4  c1        = vec4(  1,  0.0425,  1.04 , -0.04 );
          vec4  r         = roughness * c0 + c1;
          float a004      = min( r.x * r.x, exp2( -9.28 * nv ) ) * r.x + r.y;
          vec2  ScaleBias = vec2( -1.04, 1.04 ) * a004 + r.zw;
    return ScaleBias;
}

#define PBR_USE_GGX_APPROX 1
vec2 getGGX( vec2 brdfPoint )
{
#if PBR_USE_GGX_APPROX
    return getGGXApprox( brdfPoint);
#else
    return texture2D(GGXLUT, brdfPoint).rg;   // TODO: use GGXLUT
#endif
}


// Reference: float getRangeAttenuation(float range, float distance)
float getLightAttenuationPointSpot(float range, float distance)
{
#if 1
    return distance;
#else
    float range2 = pow(range, 2.0);

    // support negative range as unlimited
    if (range <= 0.0)
    {
        return 1.0 / range2;
    }

    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / range2;
#endif
}

vec3 getLightIntensityPoint(vec3 lightColor, float lightRange, float lightDistance)
{
    float  rangeAttenuation = getLightAttenuationPointSpot(lightRange, lightDistance);
    return rangeAttenuation * lightColor;
}

float getLightAttenuationSpot(vec3 spotDirection)
{
    return 1.0;
}

vec3 getLightIntensitySpot(vec3 lightColor, float lightRange, float lightDistance, vec3 v)
{
    float  spotAttenuation = getLightAttenuationSpot(-v);
    return spotAttenuation * getLightIntensityPoint( lightColor, lightRange, lightDistance );
}

// NOTE: This is different from the GGX texture
float D_GGX( float nh, float alphaRough )
{
    float rough2 = alphaRough * alphaRough;
    float f      = (nh * nh) * (rough2 - 1.0) + 1.0;
    return rough2 / (M_PI * f * f);
}

// NOTE: This is different from the GGX texture
float V_GGX( float nl, float nv, float alphaRough )
{
    float rough2 = alphaRough * alphaRough;
    float ggxv   = nl * sqrt(nv * nv * (1.0 - rough2) + rough2);
    float ggxl   = nv * sqrt(nl * nl * (1.0 - rough2) + rough2);
    float ggx    = ggxv + ggxl;
    if (ggx > 0.0)
    {
        return 0.5 / ggx;
    }
    return 0.0;
}

void initMaterial( vec3 diffuse, vec3 packedORM, out float alphaRough, out vec3 c_diff, out vec3 reflect0, out vec3 reflect90, out float specWeight )
{
    float metal      = packedORM.b;
          c_diff     = mix(diffuse.rgb, vec3(0), metal);
    float IOR        = 1.5;                                // default Index Of Refraction 1.5 (dielectrics)
          reflect0   = vec3(0.04);                         // -> incidence reflectance 0.04
          reflect0   = mix( reflect0, diffuse.rgb, metal); // reflect at 0 degrees
          reflect90  = vec3(1);                            // reflect at 90 degrees
          specWeight = 1.0;

    float perceptualRough = packedORM.g;
          alphaRough      = perceptualRough * perceptualRough;
}

vec3 BRDFLambertian( vec3 reflect0, vec3 reflect90, vec3 c_diff, float specWeight, float vh )
{
    return (1.0 - specWeight * fresnelSchlick( reflect0, reflect90, vh)) * (c_diff / M_PI);
}

vec3 BRDFSpecularGGX( vec3 reflect0, vec3 reflect90, float alphaRough, float specWeight, float vh, float nl, float nv, float nh )
{
    vec3  fresnel    = fresnelSchlick( reflect0, reflect90, vh );
    float vis       = V_GGX( nl, nv, alphaRough );
    float d         = D_GGX( nh, alphaRough );
    return specWeight * fresnel * vis * d;
}
