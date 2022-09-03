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

#define DEBUG_PBR_LIGHT_TYPE 0 // Output Diffuse=0.75, Emissive=0, ORM=0,0,0

#define DEBUG_BASIC         0
#define DEBUG_VERTEX        0
#define DEBUG_NORMAL_MAP    0 // Output packed normal map "as is" to diffuse
#define DEBUG_NORMAL_OUT    0 // Output unpacked normal to diffuse
#define DEBUG_ORM           0 // Output Occlusion Roughness Metal "as is" to diffuse
#define DEBUG_POSITION      0

uniform sampler2D diffuseMap;  //always in sRGB space

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveColor;

#ifdef HAS_NORMAL_MAP
    uniform sampler2D bumpMap;
#endif

#ifdef HAS_EMISSIVE_MAP
    uniform sampler2D emissiveMap;
#endif

#ifdef HAS_SPECULAR_MAP
    uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
#endif

uniform samplerCube environmentMap;
uniform mat3        env_mat;

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[4];
#else
#define frag_data gl_FragData
#endif

VARYING vec3 vary_position;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
#ifdef HAS_NORMAL_MAP
VARYING vec3 vary_normal;
VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;
VARYING vec2 vary_texcoord1;
#endif

#ifdef HAS_SPECULAR_MAP
    VARYING vec2 vary_texcoord2;
#endif

uniform float minimum_alpha; // PBR alphaMode: MASK, See: mAlphaCutoff, setAlphaCutoff()

vec2 encode_normal(vec3 n);
vec3 linear_to_srgb(vec3 c);

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

    vec3 col = vertex_color.rgb * albedo.rgb;

#ifdef HAS_NORMAL_MAP
    vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);
    norm.xyz = normalize(norm.xyz * 2 - 1);

    vec3 tnorm = vec3(dot(norm.xyz,vary_mat0),
                      dot(norm.xyz,vary_mat1),
                      dot(norm.xyz,vary_mat2));
#else
    vec4 norm = vec4(0,0,0,1.0);
//    vec3 tnorm = vary_normal;
    vec3 tnorm = vec3(0,0,1);
#endif

    tnorm = normalize(tnorm.xyz);
    norm.xyz = tnorm.xyz;

    // RGB = Occlusion, Roughness, Metal
    // default values, see LLViewerTexture::sDefaultPBRORMImagep
    //   occlusion 1.0
    //   roughness 0.0
    //   metal     0.0
#ifdef HAS_SPECULAR_MAP
    vec3 spec = texture2D(specularMap, vary_texcoord2.xy).rgb;
#else
    vec3 spec = vec3(1,0,0);
#endif
    
    spec.g *= roughnessFactor;
    spec.b *= metallicFactor;

    vec3 emissive = emissiveColor;
#ifdef HAS_EMISSIVE_MAP
    emissive *= texture2D(emissiveMap, vary_texcoord0.xy).rgb;
#endif

#if DEBUG_PBR_LIGHT_TYPE
    col.rgb  = vec3(0.75);
    emissive = vec3(0);
    spec.rgb = vec3(0);
#endif
#if DEBUG_BASIC
    col.rgb = vec3( 1, 0, 1 );
#endif
#if DEBUG_VERTEX
    col.rgb = vertex_color.rgb;
#endif
#if DEBUG_NORMAL_MAP
    col.rgb = texture2D(bumpMap, vary_texcoord1.xy).rgb;
#endif
#if DEBUG_NORMAL_OUT
    col.rgb = vary_normal;
#endif
#if DEBUG_ORM
    col.rgb = linear_to_srgb(spec);
#endif
#if DEBUG_POSITION
    col.rgb = vary_position.xyz;
#endif

    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = vec4(col, 0.0);                                                   // Diffuse
    frag_data[1] = vec4(emissive, vertex_color.a);                                   // PBR sRGB Emissive
    frag_data[2] = vec4(encode_normal(tnorm), vertex_color.a, GBUFFER_FLAG_HAS_PBR); // normal, environment intensity, flags
    frag_data[3] = vec4(spec.rgb,0);                                                 // PBR linear packed Occlusion, Roughness, Metal.
}
