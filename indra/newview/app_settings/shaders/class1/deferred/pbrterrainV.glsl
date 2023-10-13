/** 
 * @file class1\environment\pbrterrainV.glsl
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

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_projection_matrix;

in vec3 position;
in vec3 normal;
in vec4 tangent;
in vec4 diffuse_color;
in vec2 texcoord0;
in vec2 texcoord1;

out vec4[2] vary_coords;
out vec3 vary_vertex_normal; // Used by pbrterrainUtilF.glsl
out vec3 vary_normal;
out vec3 vary_tangent;
flat out float vary_sign;
out vec4 vary_texcoord0;
out vec4 vary_texcoord1;

uniform vec4 object_plane_s;
uniform vec4 object_plane_t;

vec4 texgen_object_pbr(vec4 tc, mat4 mat, vec4 tp0, vec4 tp1)
{
    vec4 tcoord;
    
    tcoord.x = dot(tc, tp0);
    tcoord.y = dot(tc, tp1);
    tcoord.z = tcoord.z;
    tcoord.w = tcoord.w;
    
    tcoord = mat * tcoord;
    
    return tcoord;
}

void main()
{
    //transform vertex
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0); 

	vec3 n = normal_matrix * normal;
    // *TODO: Looks like terrain normals are per-vertex when they should be per-triangle instead, causing incorrect values on triangles touching steep edges of terrain
    vary_vertex_normal = normal;
	vec3 t = normal_matrix * tangent.xyz;

    vary_tangent = normalize(t);
    vary_sign = tangent.w;
    vary_normal = normalize(n);

    // Transform and pass tex coords
    // *NOTE: KHR texture transform is ignored for now
    vary_texcoord0.xy = texgen_object_pbr(vec4(texcoord0, 0, 1), texture_matrix0, object_plane_s, object_plane_t).xy;
    
    vec4 tc = vec4(texcoord1,0,1); // TODO: This is redundant. Better to just use position and ignore texcoord? (We still need to decide how to handle alpha ramp, though...)
    vary_coords[0].xy = texgen_object_pbr(vec4(position.xy, 0, 1), texture_matrix0, object_plane_s, object_plane_t).xy;
    vary_coords[0].zw = texgen_object_pbr(vec4(position.yz, 0, 1), texture_matrix0, object_plane_s, object_plane_t).xy;
    vary_coords[1].xy = texgen_object_pbr(vec4(position.zx, 0, 1), texture_matrix0, object_plane_s, object_plane_t).xy;
    
    vary_texcoord0.zw = tc.xy;
    vary_texcoord1.xy = tc.xy-vec2(2.0, 0.0);
    vary_texcoord1.zw = tc.xy-vec2(1.0, 0.0);
}
