/**
 * @file SMAAEdgeDetectF.glsl
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

in vec2 vary_texcoord0;
in vec4 vary_offset[3];

uniform sampler2D diffuseRect;
#if SMAA_PREDICATION
uniform sampler2D predicationTex;
#endif

#define float4 vec4
#define float2 vec2
#define SMAATexture2D(tex) sampler2D tex

float2 SMAAColorEdgeDetectionPS(float2 texcoord,
                                float4 offset[3],
                                SMAATexture2D(colorTex)
                                #if SMAA_PREDICATION
                                , SMAATexture2D(predicationTex)
                                #endif
                                );

void main()
{
    vec2 val = SMAAColorEdgeDetectionPS(vary_texcoord0,
                                          vary_offset,
                                          diffuseRect
                                          #if SMAA_PREDICATION
                                          , predicationTex
                                          #endif
                                          );
    frag_color = float4(val,0.0,0.0);
}