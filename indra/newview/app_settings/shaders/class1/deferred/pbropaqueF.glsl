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

uniform sampler2D diffuseMap;  //always in sRGB space

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;
uniform sampler2D bumpMap;
uniform sampler2D emissiveMap;
uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness

out vec4 frag_data[4];

VARYING vec3 vary_position;
VARYING vec4 vertex_color;
VARYING vec3 vary_normal;
VARYING vec3 vary_tangent;
flat in float vary_sign;

VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;

uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

vec2 encode_normal(vec3 n);
vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

uniform mat3 normal_matrix;

void main()
{
// IF .mFeatures.mIndexedTextureChannels = LLGLSLShader::sIndexedTextureChannels;
//    vec3 col = vertex_color.rgb * diffuseLookup(vary_texcoord0.xy).rgb;
// else
    vec4 albedo = texture2D(diffuseMap, vary_texcoord0.xy).rgba;
    if (albedo.a < minimum_alpha)
    {
        discard;
    }

    vec3 col = vertex_color.rgb * srgb_to_linear(albedo.rgb);

    // from mikktspace.com
    vec3 vNt = texture2D(bumpMap, vary_texcoord1.xy).xyz*2.0-1.0;
    float sign = vary_sign;
    vec3 vN = vary_normal;
    vec3 vT = vary_tangent.xyz;
    
    vec3 vB = sign * cross(vN, vT);
    vec3 tnorm = normalize( vNt.x * vT + vNt.y * vB + vNt.z * vN );

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
    vec3 spec = texture2D(specularMap, vary_texcoord2.xy).rgb;
    
    spec.g *= roughnessFactor;
    spec.b *= metallicFactor;

    vec3 emissive = emissiveColor;
    emissive *= srgb_to_linear(texture2D(emissiveMap, vary_texcoord0.xy).rgb);

    tnorm *= gl_FrontFacing ? 1.0 : -1.0;

    //spec.rgb = vec3(1,1,0);
    //col = vec3(0,0,0);
    //emissive = vary_tangent.xyz*0.5+0.5;
    //emissive = vec3(sign*0.5+0.5);
    //emissive = vNt * 0.5 + 0.5;
    //emissive = tnorm*0.5+0.5;
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = vec4(linear_to_srgb(col), 0.0);                                                   // Diffuse
    frag_data[1] = vec4(linear_to_srgb(emissive), vertex_color.a);                                   // PBR sRGB Emissive
    frag_data[2] = vec4(encode_normal(tnorm), vertex_color.a, GBUFFER_FLAG_HAS_PBR); // normal, environment intensity, flags
    frag_data[3] = vec4(spec.rgb,0);                                                 // PBR linear packed Occlusion, Roughness, Metal.
}
