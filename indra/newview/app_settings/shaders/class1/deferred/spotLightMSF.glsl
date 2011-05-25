/** 
 * @file multiSpotLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

#version 120

//class 1 -- no shadows

#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS diffuseRect;
uniform sampler2DMS specularRect;
uniform sampler2DMS depthMap;
uniform sampler2DMS normalMap;
uniform sampler2D noiseMap;
uniform sampler2D lightFunc;
uniform sampler2D projectionMap;

uniform mat4 proj_mat; //screen space to light space
uniform float proj_near; //near clip for projection
uniform vec3 proj_p; //plane projection is emitting from (in screen space)
uniform vec3 proj_n;
uniform float proj_focus; //distance from plane to begin blurring
uniform float proj_lod;  //(number of mips in proj map)
uniform float proj_range; //range between near clip and far clip plane of projection
uniform float proj_ambient_lod;
uniform float proj_ambiance;
uniform float near_clip;
uniform float far_clip;

uniform vec3 proj_origin; //origin of projection to be used for angular attenuation
uniform float sun_wash;
uniform int proj_shadow_idx;
uniform float shadow_fade;

varying vec4 vary_light;

varying vec4 vary_fragcoord;
uniform vec2 screen_res;

uniform mat4 inv_proj;

vec4 texture2DLodSpecular(sampler2D projectionMap, vec2 tc, float lod)
{
	vec4 ret = texture2DLod(projectionMap, tc, lod);
	
	vec2 dist = tc-vec2(0.5);
	
	float det = max(1.0-lod/(proj_lod*0.5), 0.0);
	
	float d = dot(dist,dist);
		
	ret *= min(clamp((0.25-d)/0.25, 0.0, 1.0)+det, 1.0);
	
	return ret;
}

vec4 texture2DLodDiffuse(sampler2D projectionMap, vec2 tc, float lod)
{
	vec4 ret = texture2DLod(projectionMap, tc, lod);
	
	vec2 dist = vec2(0.5) - abs(tc-vec2(0.5));
	
	float det = min(lod/(proj_lod*0.5), 1.0);
	
	float d = min(dist.x, dist.y);
	
	float edge = 0.25*det;
		
	ret *= clamp(d/edge, 0.0, 1.0);
	
	return ret;
}

vec4 texture2DLodAmbient(sampler2D projectionMap, vec2 tc, float lod)
{
	vec4 ret = texture2DLod(projectionMap, tc, lod);
	
	vec2 dist = tc-vec2(0.5);
	
	float d = dot(dist,dist);
		
	ret *= min(clamp((0.25-d)/0.25, 0.0, 1.0), 1.0);
	
	return ret;
}


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
	vec4 frag = vary_fragcoord;
	frag.xyz /= frag.w;
	frag.xyz = frag.xyz*0.5+0.5;
	frag.xy *= screen_res;
	ivec2 itc = ivec2(frag.xy);

	vec3 fcol = vec3(0,0,0);
	int wght = 0;
	
	for (int i = 0; i < samples; ++i)
	{
		vec3 pos = getPosition(itc, i).xyz;
		vec3 lv = vary_light.xyz-pos.xyz;
		float dist2 = dot(lv,lv);
		dist2 /= vary_light.w;
		if (dist2 <= 1.0)
		{
			vec3 norm = texelFetch(normalMap, itc, i).xyz*2.0-1.0;
	
			norm = normalize(norm);
			float l_dist = -dot(lv, proj_n);
	
			vec4 proj_tc = (proj_mat * vec4(pos.xyz, 1.0));
			if (proj_tc.z >= 0.0)
			{
				proj_tc.xyz /= proj_tc.w;
	
				float fa = gl_Color.a+1.0;
				float dist_atten = min(1.0-(dist2-1.0*(1.0-fa))/fa, 1.0);
				if (dist_atten > 0.0)
				{
					lv = proj_origin-pos.xyz;
					lv = normalize(lv);
					float da = dot(norm, lv);
		
					vec3 col = vec3(0,0,0);
		
					vec3 diff_tex = texelFetch(diffuseRect, itc, i).rgb;
		
					float noise = texture2D(noiseMap, frag.xy/128.0).b;
					if (proj_tc.z > 0.0 &&
						proj_tc.x < 1.0 &&
						proj_tc.y < 1.0 &&
						proj_tc.x > 0.0 &&
						proj_tc.y > 0.0)
					{
						float lit = 0.0;
						float amb_da = proj_ambiance;
		
						if (da > 0.0)
						{
							float diff = clamp((l_dist-proj_focus)/proj_range, 0.0, 1.0);
							float lod = diff * proj_lod;
			
							vec4 plcol = texture2DLodDiffuse(projectionMap, proj_tc.xy, lod);
		
							vec3 lcol = gl_Color.rgb * plcol.rgb * plcol.a;
			
							lit = da * dist_atten * noise;
			
							col = lcol*lit*diff_tex;
							amb_da += (da*0.5)*proj_ambiance;
						}
		
						//float diff = clamp((proj_range-proj_focus)/proj_range, 0.0, 1.0);
						vec4 amb_plcol = texture2DLodAmbient(projectionMap, proj_tc.xy, proj_lod);
							
						amb_da += (da*da*0.5+0.5)*proj_ambiance;
				
						amb_da *= dist_atten * noise;
			
						amb_da = min(amb_da, 1.0-lit);
			
						col += amb_da*gl_Color.rgb*diff_tex.rgb*amb_plcol.rgb*amb_plcol.a;
					}
	
	
					vec4 spec = texelFetch(specularRect, itc, i);
					if (spec.a > 0.0)
					{
						vec3 ref = reflect(normalize(pos), norm);
		
						//project from point pos in direction ref to plane proj_p, proj_n
						vec3 pdelta = proj_p-pos;
						float ds = dot(ref, proj_n);
		
						if (ds < 0.0)
						{
							vec3 pfinal = pos + ref * dot(pdelta, proj_n)/ds;
			
							vec4 stc = (proj_mat * vec4(pfinal.xyz, 1.0));

							if (stc.z > 0.0)
							{
								stc.xy /= stc.w;

								float fatten = clamp(spec.a*spec.a+spec.a*0.5, 0.25, 1.0);
				
								stc.xy = (stc.xy - vec2(0.5)) * fatten + vec2(0.5);
								
								if (stc.x < 1.0 &&
									stc.y < 1.0 &&
									stc.x > 0.0 &&
									stc.y > 0.0)
								{
									vec4 scol = texture2DLodSpecular(projectionMap, stc.xy, proj_lod-spec.a*proj_lod);
									col += dist_atten*scol.rgb*gl_Color.rgb*scol.a*spec.rgb;
								}
							}
						}
					}
	
					fcol += col;
					++wght;
				}
			}
		}
	}

	if (wght <= 0)
	{
		discard;
	}

	gl_FragColor.rgb = fcol/samples;	
	gl_FragColor.a = 0.0;
}
