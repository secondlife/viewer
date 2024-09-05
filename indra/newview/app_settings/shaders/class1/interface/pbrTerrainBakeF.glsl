/**
 * @file terrainBakeF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

struct TerrainMix
{
    vec4 weight;
    int type;
};

TerrainMix get_terrain_mix_weights(float alpha1, float alpha2, float alphaFinal);

uniform sampler2D alpha_ramp;

// vary_texcoord* are used for terrain composition
in vec4 vary_texcoord0;
in vec4 vary_texcoord1;

void main()
{
    TerrainMix tm;
    float alpha1 = texture(alpha_ramp, vary_texcoord0.zw).a;
    float alpha2 = texture(alpha_ramp,vary_texcoord1.xy).a;
    float alphaFinal = texture(alpha_ramp, vary_texcoord1.zw).a;

    tm = get_terrain_mix_weights(alpha1, alpha2, alphaFinal);

    // tm.weight.x can be ignored. The paintmap saves a channel by not storing
    // swatch 1, and assuming the weights of the 4 swatches add to 1.
    // TERRAIN_PAINT_PRECISION emulates loss of precision at lower bit depth
    // when a corresponding low-bit image format is not available. Because
    // integral values at one depth cannot be precisely represented at another
    // bit depth, rounding is required. To maximize numerical stability for
    // future conversions, bit depth conversions should round to the nearest
    // integer, not floor or ceil.
    frag_color = max(vec4(round(tm.weight.yzw * TERRAIN_PAINT_PRECISION) / TERRAIN_PAINT_PRECISION, 1.0), vec4(0));
}
