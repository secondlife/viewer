/** 
 * @file sunLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect positionMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect depthMap;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2D noiseMap;

// Inputs
uniform mat4 shadow_matrix[4];
uniform vec4 shadow_clip;
uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

varying vec2 vary_fragcoord;
varying vec4 vary_light;

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm)
{
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
	
	// it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations (unrolling?)
	for (int i = 0; i < 8; i++)
	{
		vec2 samppos_screen = pos_screen + scale * reflect(kern[i], noise_reflect);
		vec3 samppos_world = texture2DRect(positionMap, samppos_screen).xyz;
		
		vec3 diff = pos_world - samppos_world;
		float dist2 = dot(diff, diff);
		
		// assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
		// --> solid angle shrinking by the square of distance
		//radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
		//(k should vary inversely with # of samples, but this is taken care of later)
		
		//if (dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0)  // -0.05*norm to shift sample point back slightly for flat surfaces
		//	angle_hidden += min(1.0/dist2, ssao_factor_inv); // dist != 0 follows from conditional.  max of 1.0 (= ssao_factor_inv * ssao_factor)
		angle_hidden = angle_hidden + float(dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) * min(1.0/dist2, ssao_factor_inv);
		
		// 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion" 
		points = points + int(diff.z > -1.0);
	}
	
	angle_hidden = min(ssao_factor*angle_hidden/float(points), 1.0);
	
	return 1.0 - (float(points != 0) * angle_hidden);
}

void main() 
{
	vec2 pos_screen = vary_fragcoord.xy;
	vec4 pos = vec4(texture2DRect(positionMap, pos_screen).xyz, 1.0);
        vec3 norm = texture2DRect(normalMap, pos_screen).xyz;
	
	/*if (pos.z == 0.0) // do nothing for sky *FIX: REMOVE THIS IF/WHEN THE POSITION MAP IS BEING USED AS A STENCIL
	{
		gl_FragColor = vec4(0.0); // doesn't matter
		return;
	}*/
	
	float shadow = 1.0;
        float dp_directional_light = max(0.0, dot(norm, vary_light.xyz));

	if (dp_directional_light == 0.0)
	{
		// if we know this point is facing away from the sun then we know it's in shadow without having to do a squirrelly shadow-map lookup
		shadow = 0.0;
	}
	else if (pos.z > -shadow_clip.w)
	{	
		if (pos.z < -shadow_clip.z)
		{
			vec4 lpos = shadow_matrix[3]*pos;
			shadow = shadow2DProj(shadowMap3, lpos).x;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}
		else if (pos.z < -shadow_clip.y)
		{
			vec4 lpos = shadow_matrix[2]*pos;
			shadow = shadow2DProj(shadowMap2, lpos).x;
		}
		else if (pos.z < -shadow_clip.x)
		{
			vec4 lpos = shadow_matrix[1]*pos;
			shadow = shadow2DProj(shadowMap1, lpos).x;
		}
		else
		{
			vec4 lpos = shadow_matrix[0]*pos;
			shadow = shadow2DProj(shadowMap0, lpos).x;
		}

		// take the most-shadowed value out of these two:
		//  * the blurred sun shadow in the light (shadow) map
		//  * an unblurred dot product between the sun and this norm
		// the goal is to err on the side of most-shadow to fill-in shadow holes and reduce artifacting
		shadow = min(shadow, dp_directional_light);
	}
	else
	{
		// more distant than the shadow map covers - just use directional shading as shadow
		shadow = dp_directional_light;
	}
	
	gl_FragColor[0] = shadow;
	gl_FragColor[1] = calcAmbientOcclusion(pos, norm);
	//gl_FragColor[2] is unused as of August 2008, may be used for debugging
}
