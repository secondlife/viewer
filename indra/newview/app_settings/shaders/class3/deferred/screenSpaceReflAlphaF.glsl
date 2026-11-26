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
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 base_color_texcoord;
in vec2 metallic_roughness_texcoord;
in vec2 normal_texcoord;
in vec4 vertex_color;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D bumpMap;
uniform sampler2D sceneMap;
uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform float roughnessFactor;
uniform float minimum_alpha;
uniform int ssrDebug; // TEMP: hit classification view

float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);
void bayerBlueDitherDiscard(float alpha, float threshold);

void main()
{
    vec4 baseColor = texture(diffuseMap, base_color_texcoord);
    float alpha = baseColor.a * vertex_color.a;

    // Alpha mask test
    if (minimum_alpha >= 0.0 && alpha < minimum_alpha)
        discard;

    bayerBlueDitherDiscard(alpha, 1.0);

    // Per-pixel roughness from ORM green channel, scaled by material factor
    float roughness = texture(specularMap, metallic_roughness_texcoord).g * roughnessFactor;
    float glossiness = 1.0 - roughness;

    // Derive tc from view-space position via projection rather than
    // gl_FragCoord / screen_res — the SSR buffer may be at reduced resolution.
    vec4 projPos = projection_matrix * vec4(vary_position, 1.0);
    vec2 tc = (projPos.xy / projPos.w) * 0.5 + 0.5;
    vec3 vNt = texture(bumpMap, normal_texcoord).xyz * 2.0 - 1.0;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent;
    vec3 vB = vary_sign * cross(vN, vT);
    vec3 norm = normalize(vNt.x * vT + vNt.y * vB + vNt.z * vN);
    norm *= gl_FrontFacing ? 1.0 : -1.0;

    vec4 ssrColor = vec4(0.0);
    tapScreenSpaceReflection(1, tc, vary_position, norm, ssrColor, sceneMap, glossiness);

    // dither density carries alpha; scaling .a alone breaks the premultiplied decode
    frag_color = ssrColor;

    // TEMP: alpha-prepass contributions show magenta (pre-compressed for the composite's luma recovery)
    if (ssrDebug == 1)
    {
        vec3 debug_color = vec3(1.0, 0.0, 1.0);
        frag_color = vec4(debug_color / (1.0 + dot(debug_color, vec3(0.2126, 0.7152, 0.0722))), 1.0);
    }
}
