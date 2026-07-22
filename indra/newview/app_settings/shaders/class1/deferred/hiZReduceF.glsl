/**
 * @file class1/deferred/hiZReduceF.glsl
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

// Min-reduce downsample for Hi-Z pyramid generation.
// Reads 2x2 block from source mip level, outputs min to gl_FragDepth.
// Handles odd source dimensions by sampling the extra column/row,
// matching Godot's MODE_ODD_WIDTH / MODE_ODD_HEIGHT behavior.

uniform sampler2D depthMap;
uniform int srcLevel;

void main()
{
    ivec2 destCoord = ivec2(gl_FragCoord.xy);
    ivec2 srcCoord  = destCoord * 2;
    ivec2 srcSize   = textureSize(depthMap, srcLevel);

    float d0 = texelFetch(depthMap, srcCoord, srcLevel).r;
    float d1 = texelFetch(depthMap, min(srcCoord + ivec2(1, 0), srcSize - 1), srcLevel).r;
    float d2 = texelFetch(depthMap, min(srcCoord + ivec2(0, 1), srcSize - 1), srcLevel).r;
    float d3 = texelFetch(depthMap, min(srcCoord + ivec2(1, 1), srcSize - 1), srcLevel).r;

    float depth = min(min(d0, d1), min(d2, d3));

    // When the source has an odd dimension, the last dest texel's 2x2 block
    // doesn't cover the final row/column. Sample the extra texels so the
    // Hi-Z guarantee (min of all covered texels) is maintained.
    if ((srcSize.x & 1) == 1)
    {
        depth = min(depth, texelFetch(depthMap, min(srcCoord + ivec2(2, 0), srcSize - 1), srcLevel).r);
        depth = min(depth, texelFetch(depthMap, min(srcCoord + ivec2(2, 1), srcSize - 1), srcLevel).r);
    }
    if ((srcSize.y & 1) == 1)
    {
        depth = min(depth, texelFetch(depthMap, min(srcCoord + ivec2(0, 2), srcSize - 1), srcLevel).r);
        depth = min(depth, texelFetch(depthMap, min(srcCoord + ivec2(1, 2), srcSize - 1), srcLevel).r);
    }
    if ((srcSize.x & 1) == 1 && (srcSize.y & 1) == 1)
    {
        depth = min(depth, texelFetch(depthMap, min(srcCoord + ivec2(2, 2), srcSize - 1), srcLevel).r);
    }

    gl_FragDepth = depth;
}
