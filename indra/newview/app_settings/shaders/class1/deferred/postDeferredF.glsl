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
uniform float focal_distance;
uniform float blur_constant;
uniform float tan_pixel_angle;
uniform float magnification;

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
	
	float wg = 1.0;
	//if (d < fd)
	//{
	//	diff += texture2DRect(diffuseRect, tc);
	//	w = 1.0;
	//}
	if (d > fd)
	{
		wg = max(d/fd, 0.1);
	}
	
	diff += texture2DRect(diffuseRect, tc+vec2(0.5,0.5))*wg*0.25;
	diff += texture2DRect(diffuseRect, tc+vec2(-0.5,0.5))*wg*0.25;
	diff += texture2DRect(diffuseRect, tc+vec2(0.5,-0.5))*wg*0.25;
	diff += texture2DRect(diffuseRect, tc+vec2(-0.5,-0.5))*wg*0.25;
	w += wg;
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
	
	float depth;
	depth = getDepth(tc);
		
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	
	{ //pixel is behind far focal plane
		float w = 1.0;
		
		sc = (abs(depth-focal_distance)/-depth)*blur_constant;
		
		sc /= magnification;
		
		// tan_pixel_angle = pixel_length/-depth;
		float pixel_length =  tan_pixel_angle*-focal_distance;
		
		sc = sc/pixel_length;
		
		//diff.r = sc;
		
		sc = min(abs(sc), 8.0);
		
		//sc = 4.0;
		
		float fd = depth*0.5f;
		
		while (sc > 0.5)
		{
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
		
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
	
}
