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

#define DEBUG_BASIC         1
#define DEBUG_COLOR         0
#define DEBUG_NORMAL        0
#define DEBUG_POSITION      0
#define DEBUG_REFLECT_VEC   0
#define DEBUG_REFLECT_COLOR 0

#ifdef HAS_SPECULAR_MAP
uniform sampler2D specularMap;
#endif
uniform samplerCube environmentMap;
uniform mat3        env_mat;

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

VARYING vec3 vary_position;
VARYING vec3 vary_normal;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
#ifdef HAS_SPECULAR_MAP
VARYING vec2 vary_texcoord2;
#endif

vec2 encode_normal(vec3 n);
vec3 linear_to_srgb(vec3 c);

struct PBR
{
    float LdotH; // Light and Half
    float NdotL; // Normal and Light
    float NdotH; // Normal and Half
    float VdotH; // View and Half
};

const float M_PI = 3.141592653589793;

void main()
{
    vec3 col = vertex_color.rgb * diffuseLookup(vary_texcoord0.xy).rgb;

//#ifdef HAS_SPECULAR_MAP
//#else
    vec4 norm  = vec4(0,0,0,1.0);
    vec3 tnorm = vary_normal;
//#endif
    norm.xyz = normalize(tnorm.xyz);

    vec3 spec;
    spec.rgb = vec3(vertex_color.a);

    vec3 pos = vary_position;
    vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));
    vec3 env_vec = env_mat * refnormpersp;
    vec3 reflected_color = textureCube(environmentMap, env_vec).rgb;

#if DEBUG_BASIC
    col.rgb = vec3( 1, 0, 1 ); // DEBUG
#endif
#if DEBUG_COLOR
    col.rgb = vertex_color.rgb;
#endif
#if DEBUG_NORMAL
    col.rgb = vary_normal;
#endif
#if DEBUG_POSITION
    col.rgb = vary_position.xyz;
#endif
#if DEBUG_REFLECT_VEC
    col.rgb = refnormpersp;
#endif
#if DEBUG_REFLECT_COLOR
    col.rgb = reflected_color;
#endif

    frag_data[0] = vec4(col, 0.0);
    frag_data[1] = vec4(spec, vertex_color.a); // spec
    frag_data[2] = vec4(encode_normal(norm.xyz), vertex_color.a, 0.0);
}
