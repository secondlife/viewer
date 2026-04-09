/**
 * @file class3/deferred/screenSpaceReflAlphaF.glsl
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

in vec3 vary_position;
in vec3 vary_normal;
in vec2 base_color_texcoord;
in vec2 metallic_roughness_texcoord;
in vec4 vertex_color;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D sceneMap;
uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform float roughnessFactor;
uniform float minimum_alpha;

float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);
void bayerDitherDiscard(float alpha, float threshold);

void main()
{
    vec4 baseColor = texture(diffuseMap, base_color_texcoord);
    float alpha = baseColor.a * vertex_color.a;

    // Alpha mask test
    if (minimum_alpha >= 0.0 && alpha < minimum_alpha)
        discard;

    bayerDitherDiscard(alpha, 1.0);

    // Per-pixel roughness from ORM green channel, scaled by material factor
    float roughness = texture(specularMap, metallic_roughness_texcoord).g * roughnessFactor;
    float glossiness = 1.0 - roughness;

    // Derive tc from view-space position via projection rather than
    // gl_FragCoord / screen_res — the SSR buffer may be at reduced resolution.
    vec4 projPos = projection_matrix * vec4(vary_position, 1.0);
    vec2 tc = (projPos.xy / projPos.w) * 0.5 + 0.5;
    vec3 norm = normalize(vary_normal);

    vec4 ssrColor = vec4(0.0);
    tapScreenSpaceReflection(1, tc, vary_position, norm, ssrColor, sceneMap, glossiness);
    frag_color = ssrColor;
    frag_color.a *= alpha;
}
