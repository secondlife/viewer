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


#ifndef IS_HUD

// default alpha implementation

#ifdef HAS_SKIN
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif
uniform mat4 texture_matrix0;

#if !defined(HAS_SKIN)
uniform mat4 modelview_matrix;
#endif

out vec3 vary_position;

uniform vec2 texture_base_color_scale;
uniform float texture_base_color_rotation;
uniform vec2 texture_base_color_offset;
uniform vec2 texture_normal_scale;
uniform float texture_normal_rotation;
uniform vec2 texture_normal_offset;
uniform vec2 texture_metallic_roughness_scale;
uniform float texture_metallic_roughness_rotation;
uniform vec2 texture_metallic_roughness_offset;
uniform vec2 texture_emissive_scale;
uniform float texture_emissive_rotation;
uniform vec2 texture_emissive_offset;

out vec3 vary_fragcoord;

uniform float near_clip;

in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec4 tangent;
in vec2 texcoord0;

out vec2 base_color_texcoord;
out vec2 normal_texcoord;
out vec2 metallic_roughness_texcoord;
out vec2 emissive_texcoord;

out vec4 vertex_color;

out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;

vec2 texture_transform(vec2 vertex_texcoord, vec2 khr_gltf_scale, float khr_gltf_rotation, vec2 khr_gltf_offset, mat4 sl_animation_transform);


void main()
{
#ifdef HAS_SKIN
	mat4 mat = getObjectSkinnedTransform();
	mat = modelview_matrix * mat;
	vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;
	vary_position = pos;
    vec4 vert = projection_matrix * vec4(pos,1.0);
#else
	//transform vertex
    vec4 vert = modelview_projection_matrix * vec4(position.xyz, 1.0);
#endif
    gl_Position = vert;

    vary_fragcoord.xyz = vert.xyz + vec3(0,0,near_clip);

	base_color_texcoord = texture_transform(texcoord0, texture_base_color_scale, texture_base_color_rotation, texture_base_color_offset, texture_matrix0);
	normal_texcoord = texture_transform(texcoord0, texture_normal_scale, texture_normal_rotation, texture_normal_offset, texture_matrix0);
	metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_scale, texture_metallic_roughness_rotation, texture_metallic_roughness_offset, texture_matrix0);
	emissive_texcoord = texture_transform(texcoord0, texture_emissive_scale, texture_emissive_rotation, texture_emissive_offset, texture_matrix0);

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

#if !defined(HAS_SKIN)
	vary_position = (modelview_matrix*vec4(position.xyz, 1.0)).xyz;
#endif
}

#else

// fullbright HUD alpha implementation

uniform mat4 modelview_projection_matrix;

uniform mat4 texture_matrix0;

uniform mat4 modelview_matrix;

out vec3 vary_position;

uniform vec2 texture_base_color_scale;
uniform float texture_base_color_rotation;
uniform vec2 texture_base_color_offset;
uniform vec2 texture_emissive_scale;
uniform float texture_emissive_rotation;
uniform vec2 texture_emissive_offset;

in vec3 position;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec2 base_color_texcoord;
out vec2 emissive_texcoord;

out vec4 vertex_color;

vec2 texture_transform(vec2 vertex_texcoord, vec2 khr_gltf_scale, float khr_gltf_rotation, vec2 khr_gltf_offset, mat4 sl_animation_transform);


void main()
{
	//transform vertex
    vec4 vert = modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position = vert;
    vary_position = vert.xyz;

	base_color_texcoord = texture_transform(texcoord0, texture_base_color_scale, texture_base_color_rotation, texture_base_color_offset, texture_matrix0);
	emissive_texcoord = texture_transform(texcoord0, texture_emissive_scale, texture_emissive_rotation, texture_emissive_offset, texture_matrix0);

	vertex_color = diffuse_color;
}

#endif
