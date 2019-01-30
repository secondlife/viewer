/** 
 * @file blurLightF.glsl
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

uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;

uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;

VARYING vec2 vary_fragcoord;

vec4 getPosition(vec2 pos_screen);
vec3 getNorm(vec2 pos_screen);

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    vec3 norm = getNorm(tc);
    vec3 pos = getPosition(tc).xyz;
    vec4 ccol = texture2DRect(lightMap, tc).rgba;
    
    vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
    dlt /= max(-pos.z*dist_factor, 1.0);
    
    vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
    vec4 col = defined_weight.xyxx * ccol;

    // relax tolerance according to distance to avoid speckling artifacts, as angles and distances are a lot more abrupt within a small screen area at larger distances
    float pointplanedist_tolerance_pow2 = pos.z*pos.z*0.00005;

    // perturb sampling origin slightly in screen-space to hide edge-ghosting artifacts where smoothing radius is quite large
    float tc_mod = 0.5*(tc.x + tc.y); // mod(tc.x+tc.y,2)
    tc_mod -= floor(tc_mod);
    tc_mod *= 2.0;
    tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.5 );

    for (int i = 1; i < 4; i++)
    {
        vec2 samptc = tc + kern[i].z*dlt;
        vec3 samppos = getPosition(samptc).xyz; 

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
        
        if (d*d <= pointplanedist_tolerance_pow2)
        {
            col += texture2DRect(lightMap, samptc)*kern[i].xyxx;
            defined_weight += kern[i].xy;
        }
    }

    for (int i = 1; i < 4; i++)
    {
        vec2 samptc = tc - kern[i].z*dlt;
        vec3 samppos = getPosition(samptc).xyz; 

        float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
        
        if (d*d <= pointplanedist_tolerance_pow2)
        {
            col += texture2DRect(lightMap, samptc)*kern[i].xyxx;
            defined_weight += kern[i].xy;
        }
    }

    col /= defined_weight.xyxx;
    col.y *= col.y;
    
    frag_color = col;

#ifdef IS_AMD_CARD
    // If it's AMD make sure the GLSL compiler sees the arrays referenced once by static index. Otherwise it seems to optimise the storage awawy which leads to unfun crashes and artifacts.
    vec3 dummy1 = kern[0];
    vec3 dummy2 = kern[3];
#endif
}

