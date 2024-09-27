/**
 * @file pbropaqueF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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


// deferred opaque implementation

#ifdef SAMPLE_BASE_COLOR_MAP
uniform sampler2D diffuseMap;  //always in sRGB space
uniform vec4 baseColorFactor;
in vec2 base_color_texcoord;
uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

#ifdef SAMPLE_ORM_MAP
uniform float metallicFactor;
uniform float roughnessFactor;
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
in vec2 metallic_roughness_texcoord;
#endif

#ifdef SAMPLE_NORMAL_MAP
uniform sampler2D bumpMap;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 normal_texcoord;
#endif

#ifdef SAMPLE_EMISSIVE_MAP
uniform vec3 emissiveColor;
uniform sampler2D emissiveMap;
in vec2 emissive_texcoord;
#endif

#ifdef OUTPUT_BASE_COLOR_ONLY
out vec4 frag_color;
#else
out vec4 frag_data[4];
#endif

#ifdef MIRROR_CLIP
in vec3 vary_position;
void mirrorClip(vec3 pos);
#endif

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

void main()
{
#ifdef MIRROR_CLIP
    mirrorClip(vary_position);
#endif

    vec4 basecolor = vec4(1);
#ifdef SAMPLE_BASE_COLOR_MAP
    basecolor = texture(diffuseMap, base_color_texcoord.xy).rgba;
    basecolor.rgb = srgb_to_linear(basecolor.rgb);

    basecolor *= baseColorFactor;

#ifdef ALPHA_MASK
    if (basecolor.a < minimum_alpha)
    {
        discard;
    }
#endif

#endif

#ifdef SAMPLE_NORMAL_MAP
    // from mikktspace.com
    vec3 vNt = texture(bumpMap, normal_texcoord.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;

    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

#ifdef DOUBLE_SIDED
    tnorm *= gl_FrontFacing ? 1.0 : -1.0;
#endif

#endif

#ifdef SAMPLE_ORM_MAP
    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 spec = texture(specularMap, metallic_roughness_texcoord.xy).rgb;

    spec.g *= roughnessFactor;
    spec.b *= metallicFactor;
#endif

#ifdef SAMPLE_EMISSIVE_MAP
    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture(emissiveMap, emissive_texcoord.xy).rgb);
#endif

#ifdef OUTPUT_BASE_COLOR_ONLY
#ifdef SAMPLE_EMISSIVE_MAP
    basecolor.rgb += emissive;
#endif
#ifdef OUTPUT_SRGB
    basecolor.rgb = linear_to_srgb(basecolor.rgb);
#endif
    frag_color = basecolor;
#else
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(basecolor.rgb, 0.0), vec4(0));
    frag_data[1] = max(vec4(spec.rgb,0.0), vec4(0));
    frag_data[2] = vec4(tnorm, GBUFFER_FLAG_HAS_PBR);
    frag_data[3] = max(vec4(emissive,0), vec4(0));
#endif
}

