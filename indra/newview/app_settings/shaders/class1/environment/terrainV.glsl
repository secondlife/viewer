/** 
 * @file terrainV.glsl
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
 


vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);

vec4 texgen_object(vec4  vpos, vec4 tc, mat4 mat, vec4 tp0, vec4 tp1)
{
	vec4 tcoord;
	
	tcoord.x = dot(vpos, tp0);
	tcoord.y = dot(vpos, tp1);
	tcoord.z = tc.z;
	tcoord.w = tc.w;
	
	tcoord = mat * tcoord; 
	
	return tcoord; 
}

void main()
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
			
	vec4 pos = gl_ModelViewMatrix * gl_Vertex;
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	vec4 color = calcLighting(pos.xyz, norm, vec4(1,1,1,1), gl_Color);
	
	gl_FrontColor = color;
	
	gl_TexCoord[0] = texgen_object(gl_Vertex,gl_MultiTexCoord0,gl_TextureMatrix[0],gl_ObjectPlaneS[0],gl_ObjectPlaneT[0]);
	gl_TexCoord[1] = gl_TextureMatrix[1]*gl_MultiTexCoord1;
	gl_TexCoord[2] = texgen_object(gl_Vertex,gl_MultiTexCoord2,gl_TextureMatrix[2],gl_ObjectPlaneS[2],gl_ObjectPlaneT[2]);
	gl_TexCoord[3] = gl_TextureMatrix[3]*gl_MultiTexCoord3;
}
