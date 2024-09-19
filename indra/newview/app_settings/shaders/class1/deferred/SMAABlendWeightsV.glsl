/**
 * @file SMAABlendWeightsV.glsl
 *
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

uniform mat4 modelview_projection_matrix;

in vec3 position;

out vec2 vary_texcoord0;
out vec2 vary_pixcoord;
out vec4 vary_offset[3];

#define float4 vec4
#define float2 vec2
void SMAABlendingWeightCalculationVS(float2 texcoord,
                                     out float2 pixcoord,
                                     out float4 offset[3]);

void main()
{
    gl_Position = vec4(position.xyz, 1.0);
    vary_texcoord0 = (gl_Position.xy*0.5+0.5);

    SMAABlendingWeightCalculationVS(vary_texcoord0,
                                    vary_pixcoord,
                                    vary_offset);
}

