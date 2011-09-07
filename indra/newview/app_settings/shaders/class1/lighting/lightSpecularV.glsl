/** 
 * @file lightV.glsl
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
 


float calcDirectionalLight(vec3 n, vec3 l);

// Same as non-specular lighting in lightV.glsl
vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	specularColor.rgb = vec3(0.0, 0.0, 0.0);
	vec4 col;
	col.a = color.a;

	col.rgb = gl_LightModel.ambient.rgb + baseCol.rgb;

	col.rgb += gl_LightSource[0].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[0].position.xyz);
	col.rgb += gl_LightSource[1].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[1].position.xyz);

	col.rgb = min(col.rgb*color.rgb, 1.0);

	return col;	
}

