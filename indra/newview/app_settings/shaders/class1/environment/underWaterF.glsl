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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
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

vec4 applyWaterFog(vec4 color, vec3 viewVec)
{
	//normalize view vector
	vec3 view = normalize(viewVec);
	float es = -view.z;

	//find intersection point with water plane and eye vector
	
	//get eye depth
	float e0 = max(-waterPlane.w, 0.0);
	
	//get object depth
	float depth = length(viewVec);
		
	//get "thickness" of water
	float l = max(depth, 0.1);

	float kd = waterFogDensity;
	float ks = waterFogKS;
	vec4 kc = waterFogColor;
	
	float F = 0.98;
	
	float t1 = -kd * pow(F, ks * e0);
	float t2 = kd + ks * es;
	float t3 = pow(F, t2*l) - 1.0;
	
	float L = min(t1/t2*t3, 1.0);
	
	float D = pow(0.98, l*kd);
	//return vec4(1.0, 0.0, 1.0, 1.0);
	return color * D + kc * L;
	//depth /= 10.0;
	//return vec4(depth,depth,depth,0.0);
}

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
	
	gl_FragColor = applyWaterFog(fb,view.xyz);
}
