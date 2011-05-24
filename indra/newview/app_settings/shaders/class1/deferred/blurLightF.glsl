/** 
 * @file blurLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;

uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;

varying vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).r;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

void main() 
{
    vec2 tc = vary_fragcoord.xy;
	vec3 norm = texture2DRect(normalMap, tc).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	vec3 pos = getPosition(tc).xyz;
	vec4 ccol = texture2DRect(lightMap, tc).rgba;
	
	vec2 dlt = kern_scale * delta / (1.0+norm.xy*norm.xy);
	dlt /= max(-pos.z*dist_factor, 1.0);
	
	vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
	vec4 col = defined_weight.xyxx * ccol;

	// relax tolerance according to distance to avoid speckling artifacts, as angles and distances are a lot more abrupt within a small screen area at larger distances
	float pointplanedist_tolerance_pow2 = pos.z*pos.z*0.00005;

	// perturb sampling origin slightly in screen-space to hide edge-ghosting artifacts where smoothing radius is quite large
	tc += ( (mod(tc.x+tc.y,2) - 0.5) * kern[1].z * dlt * 0.5 );

	for (int i = 1; i < 4; i++)
	{
		vec2 samptc = tc + kern[i].z*dlt;
	        vec3 samppos = getPosition(samptc).xyz; 
		float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
		if (d*d <= pointplanedist_tolerance_pow2)
		{
			col += texture2DRect(lightMap, samptc)*kern[i].xyxx;
			defined_weight += kern[i].xy;
		}
	}
	for (int i = 1; i < 4; i++)
	{
		vec2 samptc = tc - kern[i].z*dlt;
	        vec3 samppos = getPosition(samptc).xyz; 
		float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
		if (d*d <= pointplanedist_tolerance_pow2)
		{
			col += texture2DRect(lightMap, samptc)*kern[i].xyxx;
			defined_weight += kern[i].xy;
		}
	}

	col /= defined_weight.xyxx;
	col.y *= col.y;
	
	gl_FragColor = col;
}

