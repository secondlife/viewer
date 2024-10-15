/**
 * @file blinnphongF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
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

#ifdef SAMPLE_DIFFUSE_MAP
uniform sampler2D diffuseMap;  //always in sRGB space
vec4 diffuseColor;
in vec2 diffuse_texcoord;
float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()
#endif

#ifdef SAMPLE_SPECULAR_MAP
vec3 specularColor;
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
in vec2 specular_texcoord;
#endif

#ifdef SAMPLE_NORMAL_MAP
uniform sampler2D bumpMap;
in vec3 vary_normal;
in vec3 vary_tangent;
flat in float vary_sign;
in vec2 normal_texcoord;
float env_intensity;
#endif

#ifdef OUTPUT_DIFFUSE_ONLY
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

#ifdef SAMPLE_MATERIALS_UBO
layout (std140) uniform GLTFMaterials
{
    // index by gltf_material_id

    // [gltf_material_id + [0-1]] -  diffuse transform
    // [gltf_material_id + [2-3]] -  normal transform
    // [gltf_material_id + [4-5]] -  specular transform

    // Transforms are packed as follows
    // packed[0] = vec4(scale.x, scale.y, rotation, offset.x)
    // packed[1] = vec4(offset.y, *, *, *)

    // packed[1].yzw varies:
    //   diffuse transform -- diffuse color
    //   normal transform -- .y - alpha factor, .z - minimum alpha, .w - environment intensity
    //   specular transform -- specular color

    vec4 gltf_material_data[MAX_UBO_VEC4S];
};

flat in int gltf_material_id;

void unpackMaterial()
{
    int idx = gltf_material_id;

#ifdef SAMPLE_DIFFUSE_MAP
    diffuseColor.rgb = gltf_material_data[idx+1].yzw;
    diffuseColor.a = gltf_material_data[idx+3].y;
    minimum_alpha = gltf_material_data[idx+3].z;
#endif

#ifdef SAMPLE_NORMAL_MAP
    env_intensity = gltf_material_data[idx+3].w;
#endif

#ifdef SAMPLE_SPECULAR_MAP
    specularColor = gltf_material_data[idx+5].yzw;
#endif
}
#else // SAMPLE_MATERIALS_UBO
void unpackMaterial()
{
}
#endif

void main()
{
    unpackMaterial();
#ifdef MIRROR_CLIP
    mirrorClip(vary_position);
#endif

    vec4 diffuse = vec4(1);
#ifdef SAMPLE_DIFFUSE_MAP
    diffuse = texture(diffuseMap, diffuse_texcoord.xy).rgba;
    diffuse *= diffuseColor;
    diffuse.rgb = srgb_to_linear(diffuse.rgb);

#ifdef ALPHA_MASK
    if (diffuse.a < minimum_alpha)
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
#endif

    vec3 spec = vec3(0);
#ifdef SAMPLE_SPECULAR_MAP
    spec = texture(specularMap, specular_texcoord.xy).rgb;

    spec *= specularColor;
#endif

#ifdef OUTPUT_DIFFUSE_ONLY
#ifdef OUTPUT_SRGB
    diffuse.rgb = linear_to_srgb(diffuse.rgb);
#endif
    frag_color = diffuse;
#else
    diffuse.rgb = vec3(0.85);
    spec.rgb = vec3(0);
    env_intensity = 0;
    float emissive = 0.0;
    float glossiness = 0;

    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = max(vec4(diffuse.rgb, emissive), vec4(0));
    frag_data[1] = max(vec4(spec.rgb,glossiness), vec4(0));
    frag_data[2] = vec4(tnorm, GBUFFER_FLAG_HAS_ATMOS);
    frag_data[3] = max(vec4(env_intensity, 0, 0, 0), vec4(0));
#endif
}

