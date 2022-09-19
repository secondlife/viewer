/** 
 * @file reflectionmipF.glsl
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

uniform sampler2DRect screenMap;

VARYING vec2 vary_texcoord0;

void main() 
{
#if 0
    float w[9];

    float c = 1.0/16.0;  //corner weight
    float e = 1.0/8.0; //edge weight
    float m = 1.0/4.0; //middle weight

    //float wsum = c*4+e*4+m;

    w[0] = c;   w[1] = e;    w[2] = c;
    w[3] = e;    w[4] = m;     w[5] = e;
    w[6] = c;   w[7] = e;    w[8] = c;
    
    vec2 tc[9];

    float ed = 1;
    float cd = 1;


    tc[0] = vec2(-cd, cd);    tc[1] = vec2(0, ed);    tc[2] = vec2(cd, cd);
    tc[3] = vec2(-ed, 0);    tc[4] = vec2(0, 0);    tc[5] = vec2(ed, 0);
    tc[6] = vec2(-cd, -cd);    tc[7] = vec2(0, -ed);   tc[8] = vec2(cd, -1);

    vec3 color = vec3(0,0,0);

    for (int i = 0; i < 9; ++i)
    {
        color += texture2DRect(screenMap, vary_texcoord0.xy+tc[i]).rgb * w[i];
        //color += texture2DRect(screenMap, vary_texcoord0.xy+tc[i]*2.0).rgb * w[i]*0.5;
    }

    //color /= wsum;

    frag_color = vec4(color, 1.0);
#else
    frag_color = vec4(texture2DRect(screenMap, vary_texcoord0.xy).rgb, 1.0);
#endif
}
