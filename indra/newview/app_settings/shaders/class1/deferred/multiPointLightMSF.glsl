/** 
 * @file multiPointLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */



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

uniform int light_count;

#define MAX_LIGHT_COUNT		16
uniform vec4 light[MAX_LIGHT_COUNT];
uniform vec4 light_col[MAX_LIGHT_COUNT];

varying vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform float far_z;

uniform mat4 inv_proj;

vec4 getPosition(ivec2 pos_screen, int sample)
{
	float depth = texelFetch(depthMap, pos_screen, sample).r;
	vec2 sc = vec2(pos_screen.xy)*2.0;
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
	vec2 frag = (vary_fragcoord.xy*0.5+0.5)*screen_res;
	ivec2 itc = ivec2(frag);

	int wght = 0;
	vec3 fcol = vec3(0,0,0);

	for (int s = 0; s < samples; ++s)
	{
		vec3 pos = getPosition(itc, s).xyz;
		if (pos.z >= far_z)
		{
			vec3 norm = texelFetch(normalMap, itc, s).xyz;
			norm = vec3((norm.xy-0.5)*2.0,norm.z); // unpack norm
			norm = normalize(norm);
			vec4 spec = texelFetch(specularRect, itc, s);
			vec3 diff = texelFetch(diffuseRect, itc, s).rgb;
			float noise = texture2D(noiseMap, frag.xy/128.0).b;
			vec3 out_col = vec3(0,0,0);
			vec3 npos = normalize(-pos);

			// As of OSX 10.6.7 ATI Apple's crash when using a variable size loop
			for (int i = 0; i < MAX_LIGHT_COUNT; ++i)
			{
				bool light_contrib = (i < light_count);
		
				vec3 lv = light[i].xyz-pos;
				float dist2 = dot(lv,lv);
				dist2 /= light[i].w;
				if (dist2 > 1.0)
				{
					light_contrib = false;
				}
		
				float da = dot(norm, lv);
				if (da < 0.0)
				{
					light_contrib = false;
				}
		
				if (light_contrib)
				{
					lv = normalize(lv);
					da = dot(norm, lv);
					
					float fa = light_col[i].a+1.0;
					float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);
					dist_atten *= noise;

					float lit = da * dist_atten;
			
					vec3 col = light_col[i].rgb*lit*diff;
					//vec3 col = vec3(dist2, light_col[i].a, lit);
			
					if (spec.a > 0.0)
					{
						//vec3 ref = dot(pos+lv, norm);
				
						float sa = dot(normalize(lv+npos),norm);
				
						if (sa > 0.0)
						{
							sa = texture2D(lightFunc,vec2(sa, spec.a)).a * min(dist_atten*4.0, 1.0);
							sa *= noise;
							col += da*sa*light_col[i].rgb*spec.rgb;
						}
					}
			
					out_col += col;
				}
			}
	
			fcol += out_col;
			++wght;
		}
	}

	if (wght <= 0)
	{
		discard;
	}

	gl_FragColor.rgb = fcol/samples;
	gl_FragColor.a = 0.0;

	
}
