/**
 * @file class3/deferred/screenSpaceReflWaterF.glsl
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D bumpMap;
uniform sampler2D bumpMap2;
uniform float     blend_factor;
uniform vec3      normScale;
uniform float     blurMultiplier;
uniform vec2      screen_res;
uniform mat4      projection_matrix;

in vec4 refCoord;
in vec4 littleWave;
in vec4 view;
in vec3 vary_position;
in vec3 vary_normal;
in vec3 vary_tangent;

float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);

uniform sampler2D sceneMap;

vec3 BlendNormal(vec3 bump1, vec3 bump2)
{
    return mix(bump1, bump2, blend_factor);
}

void main()
{
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent;
    vec3 vB = cross(vN, vT);

    // Generate wave normals (same as waterF.glsl)
    vec2 bigwave = vec2(refCoord.w, view.w);

    vec3 wave1_a = texture(bumpMap, bigwave).xyz * 2.0 - 1.0;
    vec3 wave2_a = texture(bumpMap, littleWave.xy).xyz * 2.0 - 1.0;
    vec3 wave3_a = texture(bumpMap, littleWave.zw).xyz * 2.0 - 1.0;

    vec3 wave1_b = texture(bumpMap2, bigwave).xyz * 2.0 - 1.0;
    vec3 wave2_b = texture(bumpMap2, littleWave.xy).xyz * 2.0 - 1.0;
    vec3 wave3_b = texture(bumpMap2, littleWave.zw).xyz * 2.0 - 1.0;

    vec3 wave1 = BlendNormal(wave1_a, wave1_b);
    vec3 wave2 = BlendNormal(wave2_a, wave2_b);
    vec3 wave3 = BlendNormal(wave3_a, wave3_b);

    vec3 wavef = (wave1 + wave2 * 0.4 + wave3 * 0.6) * 0.5;

    // Same IBL normal computation as waterF.glsl
    vec3 wave_ibl = wavef * normScale;
    wave_ibl.z *= 2.0;
    wave_ibl = normalize(wave_ibl.x * vT + wave_ibl.y * vB + wave_ibl.z * vN);

    // Water glossiness (same as waterF.glsl)
    float perceptualRoughness = blurMultiplier * blurMultiplier;
    float glossiness = 1.0 - perceptualRoughness;

    // Derive tc from view-space position via projection rather than
    // gl_FragCoord / screen_res.  screen_res is the deferred buffer size,
    // but this shader may render into a reduced-resolution SSR buffer whose
    // viewport (and thus gl_FragCoord) is smaller, causing an off-centre
    // vignette and incorrect screen-edge fade.
    vec4 projPos = projection_matrix * vec4(vary_position, 1.0);
    vec2 tc = (projPos.xy / projPos.w) * 0.5 + 0.5;

    vec4 ssrColor = vec4(0.0);
    tapScreenSpaceReflection(1, tc, vary_position, wave_ibl, ssrColor, sceneMap, glossiness);
    frag_color = ssrColor;
}
