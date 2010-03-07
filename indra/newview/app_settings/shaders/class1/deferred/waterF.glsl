/** 
 * @file waterF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

vec3 scaleSoftClip(vec3 inColor);
vec3 atmosTransport(vec3 inColor);

uniform sampler2D bumpMap;   
uniform sampler2D screenTex;
uniform sampler2D refTex;
uniform sampler2DRectShadow shadowMap0;
uniform sampler2DRectShadow shadowMap1;
uniform sampler2DRectShadow shadowMap2;
uniform sampler2DRectShadow shadowMap3;
uniform sampler2D noiseMap;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;

uniform float sunAngle;
uniform float sunAngle2;
uniform vec3 lightDir;
uniform vec3 specular;
uniform float lightExp;
uniform float refScale;
uniform float kd;
uniform vec2 screenRes;
uniform vec3 normScale;
uniform float fresnelScale;
uniform float fresnelOffset;
uniform float blurMultiplier;
uniform vec2 screen_res;
uniform mat4 norm_mat; //region space to screen space

//bigWave is (refCoord.w, view.w);
varying vec4 refCoord;
varying vec4 littleWave;
varying vec4 view;
varying vec4 vary_position;

void main() 
{
	vec4 color;
	float dist = length(view.xy);
	
	//normalize view vector
	vec3 viewVec = normalize(view.xyz);
	
	//get wave normals
	vec3 wave1 = texture2D(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0-1.0;
	vec3 wave2 = texture2D(bumpMap, littleWave.xy).xyz*2.0-1.0;
	vec3 wave3 = texture2D(bumpMap, littleWave.zw).xyz*2.0-1.0;
	//get base fresnel components	
	
	vec3 df = vec3(
					dot(viewVec, wave1),
					dot(viewVec, (wave2 + wave3) * 0.5),
					dot(viewVec, wave3)
				 ) * fresnelScale + fresnelOffset;
	df *= df;
		    
	vec2 distort = (refCoord.xy/refCoord.z) * 0.5 + 0.5;
	
	float dist2 = dist;
	dist = max(dist, 5.0);
	
	float dmod = sqrt(dist);
	
	vec2 dmod_scale = vec2(dmod*dmod, dmod);
	
	//get reflected color
	vec2 refdistort1 = wave1.xy*normScale.x;
	vec2 refvec1 = distort+refdistort1/dmod_scale;
	vec4 refcol1 = texture2D(refTex, refvec1);
	
	vec2 refdistort2 = wave2.xy*normScale.y;
	vec2 refvec2 = distort+refdistort2/dmod_scale;
	vec4 refcol2 = texture2D(refTex, refvec2);
	
	vec2 refdistort3 = wave3.xy*normScale.z;
	vec2 refvec3 = distort+refdistort3/dmod_scale;
	vec4 refcol3 = texture2D(refTex, refvec3);

	vec4 refcol = refcol1 + refcol2 + refcol3;
	float df1 = df.x + df.y + df.z;
	refcol *= df1 * 0.333;
	
	vec3 wavef = (wave1 + wave2 * 0.4 + wave3 * 0.6) * 0.5;
	//wavef.z *= max(-viewVec.z, 0.1);
	wavef = normalize(wavef);
	
	float df2 = dot(viewVec, wavef) * fresnelScale+fresnelOffset;
	
	vec2 refdistort4 = wavef.xy*0.125;
	refdistort4.y -= abs(refdistort4.y);
	vec2 refvec4 = distort+refdistort4/dmod;
	float dweight = min(dist2*blurMultiplier, 1.0);
	vec4 baseCol = texture2D(refTex, refvec4);
	refcol = mix(baseCol*df2, refcol, dweight);

	//get specular component
	//float spec = clamp(dot(lightDir, (reflect(viewVec,wavef))),0.0,1.0);
		
	//harden specular
	//spec = pow(spec, 128.0);

	//figure out distortion vector (ripply)   
	vec2 distort2 = distort+wavef.xy*refScale/max(dmod*df1, 1.0);
		
	vec4 fb = texture2D(screenTex, distort2);
	
	//mix with reflection
	// Note we actually want to use just df1, but multiplying by 0.999999 gets around and nvidia compiler bug
	color.rgb = mix(fb.rgb, refcol.rgb, df1 * 0.99999);
	
	float shadow = 1.0;
	vec4 pos = vary_position;
	
	//vec3 nz = texture2D(noiseMap, gl_FragCoord.xy/128.0).xyz;
	vec4 spos = pos;
		
	//spec *= shadow;
	//color.rgb += spec * specular;
	
	//color.rgb = atmosTransport(color.rgb);
	//color.rgb = scaleSoftClip(color.rgb);
	//color.a = spec * sunAngle2;

	//wavef.z *= 0.1f;
	wavef = normalize(wavef);
	wavef = (norm_mat*vec4(wavef, 1.0)).xyz;
	
	gl_FragData[0] = vec4(color.rgb, 0.5);
	gl_FragData[1] = vec4(0.5,0.5,0.5, 0.95);
	gl_FragData[2] = vec4(wavef*0.5+0.5, 0.0);
}
