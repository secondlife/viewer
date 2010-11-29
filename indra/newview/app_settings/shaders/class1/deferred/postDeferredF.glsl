/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect edgeMap;
uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2D bloomMap;

uniform float depth_cutoff;
uniform float norm_cutoff;
uniform float near_focal_distance;
uniform float far_focal_distance;

uniform mat4 inv_proj;
uniform vec2 screen_res;

varying vec2 vary_fragcoord;

float getDepth(vec2 pos_screen)
{
	float z = texture2DRect(depthMap, pos_screen.xy).a;
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

void dofSample(inout vec4 diff, inout float w, float fd, float x, float y)
{
	vec2 tc = vary_fragcoord.xy+vec2(x,y);
	float d = getDepth(tc);
	
	if (d < fd)
	{
		diff += texture2DRect(diffuseRect, tc);
		w += 1.0;
	}
}

void dofSampleNear(inout vec4 diff, inout float w, float x, float y)
{
	vec2 tc = vary_fragcoord.xy+vec2(x,y);
		
	diff += texture2DRect(diffuseRect, tc);
	w += 1.0;
}

void main() 
{
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	
	
	vec2 tc = vary_fragcoord.xy;
	
	float sc = 0.75;
	
	float depth[5];
	depth[0] = getDepth(tc);
		
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	bool do_aa = true;
	
	if (depth[0] < far_focal_distance)
	{ //pixel is behind far focal plane
		float w = 1.0;
		
		float fd = far_focal_distance;
		float sc = far_focal_distance - depth[0];
		sc /= near_focal_distance-far_focal_distance;
		
		sc = min(sc, 8.0);
					
		while (sc > 1.0)
		{
			do_aa = false;
			dofSample(diff,w, fd, sc,sc);
			dofSample(diff,w, fd, -sc,sc);
			dofSample(diff,w, fd, sc,-sc);
			dofSample(diff,w, fd, -sc,-sc);
			
			sc -= 0.5;
			float sc2 = sc*1.414;
			dofSample(diff,w, fd, 0,sc2);
			dofSample(diff,w, fd, 0,-sc2);
			dofSample(diff,w, fd, -sc2,0);
			dofSample(diff,w, fd, sc2,0);
			sc -= 0.5;
		}
		diff /= w;
	}
	else
	{
		float fd = near_focal_distance;
		
		if (depth[0] > fd)
		{ //pixel is in front of near focal plane
			//diff.r = 1.0;
			float w = 1.0;
			float sc = depth[0] - fd;
			sc = min(-sc/fd*16.0, 8.0);
						
			fd = depth[0];
			while (sc > 1.0)
			{
				do_aa = false;
				dofSampleNear(diff,w, sc,sc);
				dofSampleNear(diff,w, -sc,sc);
				dofSampleNear(diff,w, sc,-sc);
				dofSampleNear(diff,w, -sc,-sc);
				
				sc -= 0.5;
				float sc2 = sc*1.414;
				dofSampleNear(diff,w, 0,sc2);
				dofSampleNear(diff,w, 0,-sc2);
				dofSampleNear(diff,w, -sc2,0);
				dofSampleNear(diff,w, sc2,0);
				sc -= 0.5;
			}
			diff /= w;
		}	
		
		if (do_aa)
		{
			depth[1] = getDepth(tc+vec2(sc,sc));
			depth[2] = getDepth(tc+vec2(-sc,-sc));
			depth[3] = getDepth(tc+vec2(-sc,sc));
			depth[4] = getDepth(tc+vec2(sc, -sc));
				
			
			vec2 de;
			de.x = (depth[0]-depth[1]) + (depth[0]-depth[2]);
			de.y = (depth[0]-depth[3]) + (depth[0]-depth[4]);
			de /= depth[0];
			de *= de;
			de = step(depth_cutoff, de);
			
			vec2 ne;
			vec3 nexnorm = texture2DRect(normalMap, tc+vec2(-sc,-sc)).rgb;
			nexnorm = vec3((nexnorm.xy-0.5)*2.0,nexnorm.z); // unpack norm
			ne.x = dot(nexnorm, norm);
			vec3 neynorm = texture2DRect(normalMap, tc+vec2(sc,sc)).rgb;
			neynorm = vec3((neynorm.xy-0.5)*2.0,neynorm.z); // unpack norm
			ne.y = dot(neynorm, norm);
			
			ne = 1.0-ne;
			
			ne = step(norm_cutoff, ne);
			
			float edge_weight = clamp(dot(de,de)+dot(ne,ne), 0.0, 1.0);
			//edge_weight *= 0.0;
			
			//diff.r = edge_weight;
			
			if (edge_weight > 0.0)
			{
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,1))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,-1))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,1))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,-1))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(-1,0))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(1,0))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(0,1))*edge_weight;
				diff += texture2DRect(diffuseRect, vary_fragcoord.xy+vec2(0,-1))*edge_weight;
				diff /= 1.0+edge_weight*8.0;
			}
		}
	}
			
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
	
}
