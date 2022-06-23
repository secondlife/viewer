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

#define DEBUG_BASIC         0
#define DEBUG_VERTEX        0
#define DEBUG_NORMAL        0
#define DEBUG_POSITION      0

uniform sampler2D diffuseMap;  //always in sRGB space

#ifdef HAS_NORMAL_MAP
    uniform sampler2D bumpMap;
#endif

#ifdef HAS_NORMAL_MAP
uniform sampler2D bumpMap;
#endif
#ifdef HAS_SPECULAR_MAP
    uniform sampler2D specularMap; // Packed: Occlusion, Metal, Roughness
#endif

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
    VARYING vec2 vary_texcoord1;
#endif

#ifdef HAS_SPECULAR_MAP
    VARYING vec2 vary_texcoord2;
#endif

vec2 encode_normal(vec3 n);
vec3 linear_to_srgb(vec3 c);

const float M_PI = 3.141592653589793;

void main()
{
    vec3 col = vertex_color.rgb * texture2D(diffuseMap, vary_texcoord0.xy).rgb;

    vec3 emissive = vec3(0); // TODO: Need RGB emissive map

#ifdef HAS_NORMAL_MAP
    vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);
    vec3 tnorm = norm.xyz * 2 - 1;
#else
    vec4 norm = vec4(0,0,0,1.0);
//    vec3 tnorm = vary_normal;
    vec3 tnorm = vec3(0,0,1);
#endif

    // RGB = Occlusion, Roughness, Metal
    // default values
    //   occlusion ?
    //   roughness 1.0
    //   metal     1.0
#ifdef HAS_SPECULAR_MAP
    vec3 spec = texture2D(specularMap, vary_texcoord0.xy).rgb; // TODO: FIXME: vary_texcoord2
#else
    vec3 spec = vec3(0,1,1);
#endif
    norm.xyz = normalize(tnorm.xyz);

#if DEBUG_BASIC
    col.rgb = vec3( 1, 0, 1 );
#endif
#if DEBUG_VERTEX
    col.rgb = vertex_color.rgb;
#endif
#if DEBUG_NORMAL
    col.rgb = vary_normal;
#endif
#if DEBUG_POSITION
    col.rgb = vary_position.xyz;
#endif

    frag_data[0] = vec4(col, 0.0);
    frag_data[1] = vec4(spec.rgb, vertex_color.a);                                      // Occlusion, Roughness, Metal
    frag_data[2] = vec4(encode_normal(norm.xyz), vertex_color.a, GBUFFER_FLAG_HAS_PBR); //
    frag_data[3] = vec4(emissive,0);                                                    // Emissive
}
