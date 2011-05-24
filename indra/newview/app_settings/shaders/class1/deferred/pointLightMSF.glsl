/** 
 * @file pointLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
 #version 120

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS depthMap;
uniform sampler2DMS diffuseRect;
uniform sampler2DMS specularRect;
uniform sampler2DMS normalMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;


uniform vec3 env_mat[3];
uniform float sun_wash;

varying vec4 vary_light;

varying vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform mat4 inv_proj;
uniform vec4 viewport;

vec4 getPosition(ivec2 pos_screen, int sample)
{
	float depth = texelFetch(depthMap, pos_screen, sample).r;
	vec2 sc = (vec2(pos_screen.xy)-viewport.xy)*2.0;
	sc /= viewport.zw;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

void main() 
{
	vec4 frag = vary_fragcoord;
	frag.xyz /= frag.w;
	frag.xyz = frag.xyz*0.5+0.5;
	frag.xy *= screen_res;
	
	ivec2 itc = ivec2(frag.xy);

	int wght = 0;
	vec3 fcol = vec3(0,0,0);

	for (int s = 0; s < samples; ++s)
	{
		vec3 pos = getPosition(itc, s).xyz;
		vec3 lv = vary_light.xyz-pos;
		float dist2 = dot(lv,lv);
		dist2 /= vary_light.w;
		if (dist2 <= 1.0)
		{
			vec3 norm = texelFetch(normalMap, itc, s).xyz;
			norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
			float da = dot(norm, lv);
			if (da >= 0.0)
			{
				norm = normalize(norm);
				lv = normalize(lv);
				da = dot(norm, lv);
	
				float noise = texture2D(noiseMap, frag.xy/128.0).b;
	
				vec3 col = texelFetch(diffuseRect, itc, s).rgb;
				float fa = gl_Color.a+1.0;
				float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);
				float lit = da * dist_atten * noise;
	
				col = gl_Color.rgb*lit*col;

				vec4 spec = texelFetch(specularRect, itc, s);
				if (spec.a > 0.0)
				{
					float sa = dot(normalize(lv-normalize(pos)),norm);
					if (sa > 0.0)
					{
						sa = texture2D(lightFunc, vec2(sa, spec.a)).a * min(dist_atten*4.0, 1.0);
						sa *= noise;
						col += da*sa*gl_Color.rgb*spec.rgb;
					}
				}

				fcol += col;
				++wght;
			}
		}
	}
	
	if (wght <= 0)
	{
		discard;
	}
		
	gl_FragColor.rgb = fcol/wght;	
	gl_FragColor.a = 0.0;
}
