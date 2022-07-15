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

uniform mat4 inv_proj;
uniform vec2 screen_res;

void calcHalfVectors(vec3 h, vec3 n, vec3 v, out float nh, out float nv, out float vh)
{
    nh = dot(n, h);
    nv = dot(n, v);
    vh = dot(v, h);
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
