/**
 * @file pbrShadowAlphaBlendF.glsl
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

out vec4 frag_color;

uniform sampler2D diffuseMap;

in vec4 post_pos;
in float target_pos_x;
in vec4 vertex_color;
in vec2 vary_texcoord0;
uniform float minimum_alpha;

uniform sampler2D transmissionMap;
uniform float transmissionFactor;
uniform float ior;
uniform float shadowFresnelStrength;
uniform sampler2D bumpMap;
in vec2 transmission_texcoord;
in vec2 normal_texcoord;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;

void bayerDitherDiscard(float alpha, float threshold);

void main()
{
    float alpha = texture(diffuseMap, vary_texcoord0.xy).a;
    alpha *= vertex_color.a;

    float transmission = clamp(transmissionFactor, 0.0, 1.0) * texture(transmissionMap, transmission_texcoord.xy).r;
    float f0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    vec3 tangent_normal = texture(bumpMap, normal_texcoord).xyz * 2.0 - 1.0;
    vec3 bitangent = vary_sign * cross(vary_normal, vary_tangent);
    vec3 shading_normal = normalize(tangent_normal.x * vary_tangent + tangent_normal.y * bitangent + tangent_normal.z * vary_normal);
    float cos_theta = abs(shading_normal.z);
    float fresnel = clamp((f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0)) * shadowFresnelStrength, 0.0, 1.0);
    alpha *= 1.0 - transmission * (1.0 - fresnel);

    bayerDitherDiscard(alpha, 0.88);

    frag_color = vec4(1,1,1,1);
}
