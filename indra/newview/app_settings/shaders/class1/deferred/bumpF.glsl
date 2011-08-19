/** 
 * @file bumpF.glsl
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
 


uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;

varying vec3 vary_mat0;
varying vec3 vary_mat1;
varying vec3 vary_mat2;

void main() 
{
	vec3 col = gl_Color.rgb * texture2D(diffuseMap, gl_TexCoord[0].xy).rgb;
	vec3 norm = texture2D(bumpMap, gl_TexCoord[0].xy).rgb * 2.0 - 1.0;

	vec3 tnorm = vec3(dot(norm,vary_mat0),
			  dot(norm,vary_mat1),
			  dot(norm,vary_mat2));
						
	gl_FragData[0] = vec4(col, 0.0);
	gl_FragData[1] = gl_Color.aaaa; // spec
	//gl_FragData[1] = vec4(vec3(gl_Color.a), gl_Color.a+(1.0-gl_Color.a)*gl_Color.a); // spec - from former class3 - maybe better, but not so well tested
	vec3 nvn = normalize(tnorm);
	gl_FragData[2] = vec4(nvn.xy * 0.5 + 0.5, nvn.z, 0.0);
}
