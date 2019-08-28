/**
 * @file underWaterF.glsl
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
 
/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;   
uniform sampler2D screenTex;
uniform sampler2D refTex;
uniform sampler2D screenDepth;

uniform vec4 fogCol;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform vec2 fbScale;
uniform float refScale;
uniform float znear;
uniform float zfar;
uniform float kd;
uniform vec4 waterPlane;
uniform vec3 eyeVec;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;
uniform vec2 screenRes;

//bigWave is (refCoord.w, view.w);
VARYING vec4 refCoord;
VARYING vec4 littleWave;
VARYING vec4 view;

vec2 encode_normal(vec3 n);

void main() 
{
	vec4 color;
	    
	//get detail normals
	vec3 wave1 = texture2D(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0-1.0;
	vec3 wave2 = texture2D(bumpMap, littleWave.xy).xyz*2.0-1.0;
	vec3 wave3 = texture2D(bumpMap, littleWave.zw).xyz*2.0-1.0;    
	vec3 wavef = normalize(wave1+wave2+wave3);
	
	//figure out distortion vector (ripply)   
	vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
	distort = distort+wavef.xy*refScale;
		
	vec4 fb = texture2D(screenTex, distort);

	frag_data[0] = vec4(fb.rgb, 1.0); // diffuse
	frag_data[1] = vec4(0.5,0.5,0.5, 0.95); // speccolor*spec, spec
	frag_data[2] = vec4(encode_normal(wavef), 0.0, 0.0); // normalxyz, env intens, atmo kill
}
