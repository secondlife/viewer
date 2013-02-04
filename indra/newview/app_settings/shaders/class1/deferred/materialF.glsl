/** 
 * @file materialF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform sampler2D diffuseMap;

uniform sampler2D bumpMap;

uniform sampler2D specularMap;
uniform float env_intensity;
uniform vec4 specular_color;

#ifdef ALPHA_TEST
uniform float minimum_alpha;
#endif

VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

void main() 
{
	vec4 col = texture2D(diffuseMap, vary_texcoord0.xy) * vertex_color;

	#ifdef ALPHA_TEST
	if (col.a < minimum_alpha)
	{
		discard;
	}
	#endif
	
	vec4 spec = texture2D(specularMap, vary_texcoord0.xy);

	vec4 norm = texture2D(bumpMap, vary_texcoord0.xy);

	norm.xyz = norm.xyz * 2 - 1;

	vec3 tnorm = vec3(dot(norm.xyz,vary_mat0),
			  dot(norm.xyz,vary_mat1),
			  dot(norm.xyz,vary_mat2));

	vec4 final_color = col;
	final_color.rgb *= 1 - spec.a * env_intensity;

	#ifndef EMISSIVE_MASK
	final_color.a = 0;
	#endif

	vec4 final_specular = spec;
	final_specular.rgb *= specular_color.rgb;
	final_specular.a = specular_color.a * norm.a;

	vec4 final_normal = vec4(normalize(tnorm), spec.a * env_intensity);
	final_normal.xyz = final_normal.xyz * 0.5 + 0.5;
	
	frag_data[0] = final_color;
	frag_data[1] = final_specular; // XYZ = Specular color. W = Specular exponent.
	frag_data[2] = final_normal; // XYZ = Normal.  W = Env. intensity.
}
