/** 
 * @file class1\deferred\terrainF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

out vec4 frag_data[4];

uniform sampler2D detail_0;
uniform sampler2D detail_1;
uniform sampler2D detail_2;
uniform sampler2D detail_3;
uniform sampler2D alpha_ramp;

in vec3 pos;
in vec3 vary_normal;
in vec3 vary_tangent; // TODO: Decide if we want to keep this
flat in float vary_sign; // TODO: Decide if we want to keep this
in vec4 vary_texcoord0;
in vec4 vary_texcoord1;

vec2 encode_normal(vec3 n);

void main()
{
    vec4 color0 = texture2D(detail_0, vary_texcoord0.xy);
    vec4 color1 = texture2D(detail_1, vary_texcoord0.xy);
    vec4 color2 = texture2D(detail_2, vary_texcoord0.xy);
    vec4 color3 = texture2D(detail_3, vary_texcoord0.xy);

    float alpha1 = texture2D(alpha_ramp, vary_texcoord0.zw).a;
    float alpha2 = texture2D(alpha_ramp,vary_texcoord1.xy).a;
    float alphaFinal = texture2D(alpha_ramp, vary_texcoord1.zw).a;
    vec4 outColor = mix( mix(color3, color2, alpha2), mix(color1, color0, alpha1), alphaFinal );
   
    outColor.a = 0.0; // yes, downstream atmospherics 
    
    frag_data[0] = outColor;
    frag_data[0] = vec4((0.5 * (1.0 + vary_sign)) * vary_tangent.xyz, 1.0); // TODO: Remove
    frag_data[1] = vec4(0.0,0.0,0.0,-1.0);
    vec3 nvn = normalize(vary_normal);
    frag_data[2] = vec4(encode_normal(nvn.xyz), 0.0, GBUFFER_FLAG_HAS_ATMOS);
    frag_data[3] = vec4(0);
}

