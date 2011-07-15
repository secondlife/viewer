/** 
 * @file postgiF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect giLightMap;
uniform sampler2D	noiseMap;

uniform vec2 kern[32];
uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform int kern_length;
uniform float kern_scale;
uniform vec3 blur_quad;

varying vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
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
	vec3 norm = texture2DRect(normalMap, vary_fragcoord.xy).xyz;
	norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
	vec3 pos = getPosition(vary_fragcoord.xy).xyz;
	
	vec3 ccol = texture2DRect(giLightMap, vary_fragcoord.xy).rgb;
	vec2 dlt = kern_scale * delta/(1.0+norm.xy*norm.xy);
	dlt /= max(-pos.z*dist_factor, 1.0);
	float defined_weight = kern[0].x;
	vec3 col = vec3(0.0);
	
	for (int i = 0; i < kern_length; i++)
	{
		vec2 tc = vary_fragcoord.xy + kern[i].y*dlt;
		vec3 sampNorm = texture2DRect(normalMap, tc.xy).xyz;
		sampNorm = vec3((sampNorm.xy-0.5)*2.0,sampNorm.z); // unpack norm
	    
		float d = dot(norm.xyz, sampNorm);
		
		if (d > 0.8)
		{
			vec3 samppos = getPosition(tc.xy).xyz;
			samppos -= pos;
			if (dot(samppos,samppos) < -0.05*pos.z)
			{
	    		col += texture2DRect(giLightMap, tc).rgb*kern[i].x;
				defined_weight += kern[i].x;
			}
		}
	}

	col /= defined_weight;
	
	//col = ccol;
	
	col = col*col*blur_quad.x + col*blur_quad.y + blur_quad.z;
	
	gl_FragData[0].xyz = col;
	
	//gl_FragColor = ccol;
}
