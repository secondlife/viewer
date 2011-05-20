/** 
 * @file sunLightSSAOF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

//class 1 -- no shadow, SSAO only

uniform sampler2DMS depthMap;
uniform sampler2DMS normalMap;
uniform sampler2D noiseMap;


// Inputs
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

varying vec2 vary_fragcoord;
varying vec4 vary_light;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform float shadow_bias;
uniform float shadow_offset;

vec4 getPosition(ivec2 pos_screen, int sample)
{
	float depth = texelFetch(depthMap, pos_screen, sample).r;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm, int sample)
{
	float ret = 1.0;
	
	vec2 kern[8];
	// exponentially (^2) distant occlusion samples spread around origin
	kern[0] = vec2(-1.0, 0.0) * 0.125*0.125;
	kern[1] = vec2(1.0, 0.0) * 0.250*0.250;
	kern[2] = vec2(0.0, 1.0) * 0.375*0.375;
	kern[3] = vec2(0.0, -1.0) * 0.500*0.500;
	kern[4] = vec2(0.7071, 0.7071) * 0.625*0.625;
	kern[5] = vec2(-0.7071, -0.7071) * 0.750*0.750;
	kern[6] = vec2(-0.7071, 0.7071) * 0.875*0.875;
	kern[7] = vec2(0.7071, -0.7071) * 1.000*1.000;

	vec2 pos_screen = vary_fragcoord.xy;
	vec3 pos_world = pos.xyz;
	vec2 noise_reflect = texture2D(noiseMap, vary_fragcoord.xy/128.0).xy;
		
	float angle_hidden = 0.0;
	int points = 0;
		
	float scale = min(ssao_radius / -pos_world.z, ssao_max_radius);
		
	// it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations unrolling?)
	for (int i = 0; i < 8; i++)
	{
		ivec2 samppos_screen = ivec2(pos_screen + scale * reflect(kern[i], noise_reflect));
		vec3 samppos_world = getPosition(samppos_screen, sample).xyz; 
			
		vec3 diff = pos_world - samppos_world;
		float dist2 = dot(diff, diff);
			
		// assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
		// --> solid angle shrinking by the square of distance
		//radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
		//(k should vary inversely with # of samples, but this is taken care of later)
			
		angle_hidden = angle_hidden + float(dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) * min(1.0/dist2, ssao_factor_inv);
			
		// 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion" 
		points = points + int(diff.z > -1.0);
	}
		
	angle_hidden = min(ssao_factor*angle_hidden/float(points), 1.0);
		
	ret = (1.0 - (float(points != 0) * angle_hidden));
	
	return min(ret, 1.0);
}

void main() 
{
	int samples = 4;

	vec2 pos_screen = vary_fragcoord.xy;
	ivec2 itc = ivec2(pos_screen);
		
	float col = 0;

	for (int i = 0; i < samples; i++)
	{
		vec4 pos = getPosition(itc, i);
		vec3 norm = texelFetch(normalMap, itc, i).xyz;
		norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
		col += calcAmbientOcclusion(pos,norm,i);
	}

	col /= samples;

	gl_FragColor[0] = 1.0;
	gl_FragColor[1] = col;
	gl_FragColor[2] = 1.0; 
	gl_FragColor[3] = 1.0;
}
