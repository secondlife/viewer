/** 
 * @file class1/deferred/waterF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

vec3 scaleSoftClip(vec3 inColor);
vec3 atmosTransport(vec3 inColor);

uniform sampler2D bumpMap;   
uniform sampler2D bumpMap2;
uniform float blend_factor;
uniform sampler2D screenTex;
uniform sampler2D refTex;
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
uniform int water_edge;

//bigWave is (refCoord.w, view.w);
VARYING vec4 refCoord;
VARYING vec4 littleWave;
VARYING vec4 view;
VARYING vec4 vary_position;

vec2 encode_normal(vec3 n);
vec3 scaleSoftClip(vec3 l);
vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

vec3 BlendNormal(vec3 bump1, vec3 bump2)
{
    vec3 n = mix(bump1, bump2, blend_factor);
    return n;
}

void main() 
{
    vec4 color;
    float dist = length(view.xyz);
    
    //normalize view vector
    vec3 viewVec = normalize(view.xyz);
    
    //get wave normals
    vec3 wave1_a = texture2D(bumpMap, vec2(refCoord.w, view.w)).xyz*2.0-1.0;
    vec3 wave2_a = texture2D(bumpMap, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_a = texture2D(bumpMap, littleWave.zw).xyz*2.0-1.0;


    vec3 wave1_b = texture2D(bumpMap2, vec2(refCoord.w, view.w)).xyz*2.0-1.0;
    vec3 wave2_b = texture2D(bumpMap2, littleWave.xy).xyz*2.0-1.0;
    vec3 wave3_b = texture2D(bumpMap2, littleWave.zw).xyz*2.0-1.0;

    vec3 wave1 = BlendNormal(wave1_a, wave1_b);
    vec3 wave2 = BlendNormal(wave2_a, wave2_b);
    vec3 wave3 = BlendNormal(wave3_a, wave3_b);

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
    wavef.z *= max(-viewVec.z, 0.1);
    wavef = normalize(wavef);
    
    float df2 = dot(viewVec, wavef) * fresnelScale+fresnelOffset;
    
    vec2 refdistort4 = wavef.xy*0.125;
    refdistort4.y -= abs(refdistort4.y);
    vec2 refvec4 = distort+refdistort4/dmod;
    float dweight = min(dist2*blurMultiplier, 1.0);
    vec4 baseCol = texture2D(refTex, refvec4);

    refcol = mix(baseCol*df2, refcol, dweight);

    //get specular component
	float spec = clamp(dot(lightDir, (reflect(viewVec,wavef))),0.0,1.0);
		
	//harden specular
	spec = pow(spec, 128.0);

	//figure out distortion vector (ripply)   
	vec2 distort2 = distort+wavef.xy*refScale*0.16/max(dmod*df1, 1.0);
		
	vec4 fb = texture2D(screenTex, distort2);
	
	//mix with reflection
	// Note we actually want to use just df1, but multiplying by 0.999999 gets around an nvidia compiler bug
	color.rgb = mix(fb.rgb, refcol.rgb, df1 * 0.99999f);
	
	vec4 pos = vary_position;
	
	color.rgb += spec * specular;

	color.rgb = atmosTransport(color.rgb);
	color.rgb = scaleSoftClip(color.rgb);
    
	color.a   = spec * sunAngle2;
    
	vec3 screenspacewavef = normalize((norm_mat*vec4(wavef, 1.0)).xyz);

	//frag_data[0] = color;
    frag_data[0] = color;
    frag_data[1] = vec4(0);		// speccolor, spec
	frag_data[2] = vec4(encode_normal(screenspacewavef.xyz), 0.05, 0);// normalxy, 0, 0
}
