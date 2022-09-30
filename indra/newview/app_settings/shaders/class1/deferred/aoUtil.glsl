/** 
 * @file class1/deferred/aoUtil.glsl
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

uniform sampler2D       noiseMap;
uniform sampler2DRect   normalMap;
uniform sampler2DRect   depthMap;

uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec2 getScreenCoordinateAo(vec2 screenpos)
{
    vec2 sc = screenpos.xy * 2.0;
    if (screen_res.x > 0 && screen_res.y > 0)
    {
       sc /= screen_res;
    }
    return sc - vec2(1.0, 1.0);
}

float getDepthAo(vec2 pos_screen)
{
    float depth = texture2DRect(depthMap, pos_screen).r;
    return depth;
}

vec4 getPositionAo(vec2 pos_screen)
{
    float depth = getDepthAo(pos_screen);
    vec2 sc = getScreenCoordinateAo(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec2 getKern(int i)
{
    vec2 kern[8];
    // exponentially (^2) distant occlusion samples spread around origin
    kern[0] = vec2(-1.0, 0.0) * 0.125*0.125;
    kern[1] = vec2(1.0, 0.0) * 0.250*0.250;
    kern[2] = vec2(0.0, 1.0) * 0.375*0.375;
    kern[3] = vec2(0.0, -1.0) * 0.500*0.500;
    kern[4] = vec2(0.7071, 0.7071) * 0.625*0.625;
    kern[5] = vec2(-0.7071, -0.7071) * 0.750*0.750;
    kern[6] = vec2(-0.7071, 0.7071) * 0.875*0.875;
    kern[7] = vec2(0.7071, -0.7071) * 1.000*1.000;
       
    return kern[i];
}

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm, vec2 pos_screen)
{
    float ret = 1.0;
    vec3 pos_world = pos.xyz;
    vec2 noise_reflect = texture2D(noiseMap, pos_screen.xy/128.0).xy;
        
    float angle_hidden = 0.0;
    float points = 0;
        
    float scale = min(ssao_radius / -pos_world.z, ssao_max_radius);
    
    // it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations (unrolling?)
    for (int i = 0; i < 8; i++)
    {
        vec2 samppos_screen = pos_screen + scale * reflect(getKern(i), noise_reflect);
        vec3 samppos_world = getPositionAo(samppos_screen).xyz; 
        
        vec3 diff = pos_world - samppos_world;
        float dist2 = dot(diff, diff);
            
        // assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
        // --> solid angle shrinking by the square of distance
        //radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
        //(k should vary inversely with # of samples, but this is taken care of later)
        
        float funky_val = (dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) ? 1.0 : 0.0;
        angle_hidden = angle_hidden + funky_val * min(1.0/dist2, ssao_factor_inv);
            
        // 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion" 
        float diffz_val = (diff.z > -1.0) ? 1.0 : 0.0;
        points = points + diffz_val;
    }
        
    angle_hidden = min(ssao_factor*angle_hidden/points, 1.0);
    
    float points_val = (points > 0.0) ? 1.0 : 0.0;
    ret = (1.0 - (points_val * angle_hidden));

    ret = max(ret, 0.0);
    return min(ret, 1.0);
}

