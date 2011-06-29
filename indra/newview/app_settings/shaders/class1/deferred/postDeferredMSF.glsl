/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS diffuseRect;
uniform sampler2DMS edgeMap;
uniform sampler2DMS depthMap;
uniform sampler2DMS normalMap;
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

vec4 texture2DMS(sampler2DMS tex, ivec2 tc)
{
	vec4 ret = vec4(0,0,0,0);
	for (int i = 0; i < samples; ++i)
	{
		ret += texelFetch(tex, tc, i);
	}

	return ret/samples;
}

float getDepth(ivec2 pos_screen)
{
	float z = texture2DMS(depthMap, pos_screen.xy).r;
	z = z*2.0-1.0;
	vec4 ndc = vec4(0.0, 0.0, z, 1.0);
	vec4 p = inv_proj*ndc;
	return p.z/p.w;
}

float calc_cof(float depth)
{
	float sc = abs(depth-focal_distance)/-depth*blur_constant;
		
	sc /= magnification;
	
	// tan_pixel_angle = pixel_length/-depth;
	float pixel_length =  tan_pixel_angle*-focal_distance;
	
	sc = sc/pixel_length;
	sc *= 1.414;
	
	return sc;
}

void dofSample(inout vec4 diff, inout float w, float min_sc, float cur_depth, ivec2 tc)
{
	float d = getDepth(tc);
	
	float sc = calc_cof(d);
	
	if (sc > min_sc //sampled pixel is more "out of focus" than current sample radius
	   || d < cur_depth) //sampled pixel is further away than current pixel
	{
		float wg = 0.25;
		
		vec4 s = texture2DMS(diffuseRect, tc);
		// de-weight dull areas to make highlights 'pop'
		wg += s.r+s.g+s.b;
	
		diff += wg*s;
		
		w += wg;
	}
}


void main() 
{
	ivec2 itc = ivec2(vary_fragcoord.xy);

	vec3 norm = texture2DMS(normalMap, itc).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
		
	float depth = getDepth(itc);
	
	vec4 diff = texture2DMS(diffuseRect, itc);
	
	{ 
		float w = 1.0;
		
		float sc = calc_cof(depth);
		sc = min(abs(sc), 10.0);
		
		float fd = depth*0.5f;
		
		float PI = 3.14159265358979323846264;

		int isc = int(sc);
		
		// sample quite uniformly spaced points within a circle, for a circular 'bokeh'		
		//if (depth < focal_distance)
		{
			for (int x = -isc; x <= isc; x+=2)
			{
				for (int y = -isc; y <= isc; y+=2)
				{
					ivec2 cur_samp = ivec2(x,y);
					float cur_sc = length(vec2(cur_samp));
					if (cur_sc < sc)
					{
						dofSample(diff, w, cur_sc, depth, itc+cur_samp);
					}
				}
			}
		}
		
		diff /= w;
	}
		
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
}
