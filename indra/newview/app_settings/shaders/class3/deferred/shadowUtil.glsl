/** 
 * @file class3/deferred/shadowUtil.glsl
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

uniform sampler2D       shadowMap0;
uniform sampler2D       shadowMap1;
uniform sampler2D       shadowMap2;
uniform sampler2D       shadowMap3;
uniform sampler2D       shadowMap4;
uniform sampler2D       shadowMap5;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform vec2 shadow_res;
uniform vec2 proj_shadow_res;
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;

uniform float spot_shadow_bias;
uniform float spot_shadow_offset;

float getDepth(vec2 screenpos);
vec3 getNorm(vec2 screenpos);
vec4 getPosition(vec2 pos_screen);

float ReduceLightBleeding(float p_max, float Amount)
{
    return smoothstep(Amount, 1, p_max);
}

float ChebyshevUpperBound(vec2 m, float t, float min_v, float Amount)
{
    float p = (t <= m.x) ? 1.0 : 0.0;

    float v = m.y - (m.x*m.x);
    v = max(v, min_v);

    float d = t - m.x;

    float p_max = v / (v + d*d);

    p_max = ReduceLightBleeding(p_max, Amount);

    return max(p, p_max);
}

vec4 computeMoments(float depth, float a)
{
    float m1 = depth;
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    float m2 = m1*m1 + 0.25 * a * (dx*dx + dy*dy);
    return vec4(m1, m2, a, max(dx, dy));
}

float vsmDirectionalSample(vec4 stc, float depth, sampler2D shadowMap, mat4 shadowMatrix)
{
    vec4 lpos = shadowMatrix * stc;
    vec4 moments = texture2D(shadowMap, lpos.xy);
    return ChebyshevUpperBound(moments.rg, depth - shadow_bias * 256.0f, 0.125, 0.9);
}

float vsmSpotSample(vec4 stc, float depth, sampler2D shadowMap, mat4 shadowMatrix)
{
    vec4 lpos = shadowMatrix * stc;
    vec4 moments = texture2D(shadowMap, lpos.xy);
    lpos.xyz /= lpos.w;
    lpos.xy *= 0.5;
    lpos.xy += 0.5;
    return ChebyshevUpperBound(moments.rg, depth - spot_shadow_bias * 16.0f, 0.125, 0.9);
}

#if VSM_POINT_SHADOWS
float vsmPointSample(float lightDistance, vec3 lightDirection, samplerCube shadow_cube_map)
{
    vec4 moments = textureCube(shadow_cube_map, light_direction);
    return ChebyshevUpperBound(moments.rg, light_distance, 0.01, 0.25);
}
#endif

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen)
{
	if (pos.z < -shadow_clip.w)
    {
        discard;
    }

    float depth = getDepth(pos_screen);

    vec4 spos       = vec4(pos,1.0);
    vec4 near_split = shadow_clip*-0.75;
    vec4 far_split  = shadow_clip*-1.25;

    float shadow = 0.0f;
    float weight = 1.0;

    if (spos.z < near_split.z)
    {
        shadow += vsmDirectionalSample(spos, depth, shadowMap3, shadow_matrix[3]);
        weight += 1.0f;
    }
    if (spos.z < near_split.y)
    {
        shadow += vsmDirectionalSample(spos, depth, shadowMap2, shadow_matrix[2]);
        weight += 1.0f;
    }
    if (spos.z < near_split.x)
    {
        shadow += vsmDirectionalSample(spos, depth, shadowMap1, shadow_matrix[1]);
        weight += 1.0f;
    }
    if (spos.z > far_split.x)
    {
        shadow += vsmDirectionalSample(spos, depth, shadowMap0, shadow_matrix[0]);
        weight += 1.0f;
    }

    shadow /= weight;

    return shadow;
}

float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen)
{
	if (pos.z < -shadow_clip.w)
    {
        discard;
    }

    float depth = getDepth(pos_screen);

    pos += norm * spot_shadow_offset;
    return vsmSpotSample(vec4(pos, 1.0), depth, (index == 0) ? shadowMap4 : shadowMap5, shadow_matrix[4 + index]);
}

