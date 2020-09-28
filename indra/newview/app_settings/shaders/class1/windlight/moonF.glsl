/** 
 * @file class1\wl\moonF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, 2020 Linden Research, Inc.
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

uniform vec4 color;
uniform vec4 sunlight_color;
uniform vec4 moonlight_color;
uniform vec3 moon_dir;
uniform float moon_brightness;
uniform sampler2D diffuseMap;

VARYING vec2 vary_texcoord0;

vec3 scaleSoftClip(vec3 light);

void main() 
{
    // Restore Pre-EEP alpha fade moon near horizon
    float fade = 1.0;
    if( moon_dir.z > 0 )
        fade = clamp( moon_dir.z*moon_dir.z*4.0, 0.0, 1.0 );

    vec4 c     = texture2D(diffuseMap, vary_texcoord0.xy);
//       c.rgb = pow(c.rgb, vec3(0.7f)); // can't use "srgb_to_linear(color.rgb)" as that is a deferred only function
         c.rgb *= moonlight_color.rgb;
         c.rgb *= moon_brightness;

         c.rgb *= fade;
         c.a   *= fade;

         c.rgb  = scaleSoftClip(c.rgb);

    frag_color = vec4(c.rgb, c.a);
}

