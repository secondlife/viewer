/** 
 * @file class1\deferred\pbralphaV.glsl
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

#define DIFFUSE_ALPHA_MODE_IGNORE 0
#define DIFFUSE_ALPHA_MODE_BLEND 1
#define DIFFUSE_ALPHA_MODE_MASK 2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

#ifdef HAS_SKIN
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
  #if !defined(HAS_SKIN)
    uniform mat4 modelview_matrix;
  #endif
    VARYING vec3 vary_position;
#endif

uniform mat4 texture_matrix0;

#ifdef HAS_SHADOW
VARYING vec3 vary_fragcoord;
uniform float near_clip;
#endif

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec4 tangent;
ATTRIBUTE vec2 texcoord0;
ATTRIBUTE vec2 texcoord1;
ATTRIBUTE vec2 texcoord2;


VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec3 vary_normal;
VARYING vec3 vary_tangent;
flat out float vary_sign;


void main()
{
#ifdef HAS_SKIN
	mat4 mat = getObjectSkinnedTransform();
	mat = modelview_matrix * mat;
	vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
	vary_position = pos;
#endif
    vec4 vert = projection_matrix * vec4(pos,1.0);
#else
	//transform vertex
    vec4 vert = modelview_projection_matrix * vec4(position.xyz, 1.0);
#endif
    gl_Position = vert;

#ifdef HAS_SHADOW
    vary_fragcoord.xyz = vert.xyz + vec3(0,0,near_clip);
#endif

	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
	vary_texcoord1 = (texture_matrix0 * vec4(texcoord1,0,1)).xy;
	vary_texcoord2 = (texture_matrix0 * vec4(texcoord2,0,1)).xy;

#ifdef HAS_SKIN
	vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
	vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;
#else //HAS_SKIN
	vec3 n = normal_matrix * normal;
  	vec3 t = normal_matrix * tangent.xyz;
#endif //HAS_SKIN

    vary_tangent = normalize(t);
    vary_sign = tangent.w;
    vary_normal = normalize(n);

	vertex_color = diffuse_color;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
  #if !defined(HAS_SKIN)
	vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
  #endif
#endif
}
