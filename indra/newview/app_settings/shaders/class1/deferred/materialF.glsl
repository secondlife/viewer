/** 
 * @file materialF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
 
#define DIFFUSE_ALPHA_MODE_IGNORE 0
#define DIFFUSE_ALPHA_MODE_BLEND 1
#define DIFFUSE_ALPHA_MODE_MASK 2
#define DIFFUSE_ALPHA_MODE_EMISSIVE 3

uniform float emissive_brightness;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

#if HAS_SUN_SHADOW

uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform vec2 shadow_res;
uniform float shadow_bias;

float pcfShadow(sampler2DShadow shadowMap, vec4 stc)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias;
		
	stc.x = floor(stc.x*shadow_res.x + fract(stc.y*shadow_res.y*12345))/shadow_res.x; // add some chaotic jitter to X sample pos according to Y to disguise the snapping going on here
	
	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;
	
    shadow += shadow2D(shadowMap, stc.xyz+vec3(2.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(1.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-1.0/shadow_res.x, 1.5/shadow_res.y, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-2.0/shadow_res.x, -1.5/shadow_res.y, 0.0)).x;
                       
    return shadow*0.2;
}

#endif

uniform samplerCube environmentMap;
uniform sampler2D	  lightFunc;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
//uniform vec4 camPosWorld;
uniform vec4 gamma;
uniform vec4 lightnorm;
uniform vec4 sunlight_color;
uniform vec4 ambient;
uniform vec4 blue_horizon;
uniform vec4 blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec4 glow;
uniform float scene_light_strength;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
VARYING vec2 vary_fragcoord;

VARYING vec3 vary_position;

vec3 vary_PositionEye;

vec3 vary_SunlitColor;
vec3 vary_AmblitColor;
vec3 vary_AdditiveColor;
vec3 vary_AtmosAttenuation;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec3 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

vec3 calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return vec3(a,a,a);
}


vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 npos, vec3 diffuse, vec4 spec, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, inout float glare)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = dot(lv,lv);
	
	float da = 1.0;

	vec3 col = vec3(0,0,0);

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist2 = d/la;
		float dist_atten = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= max(dot(n, lv), 0.0);		
		
		float lit = max(da * dist_atten, 0.0);

		col = light_col*lit*diffuse;

		if (spec.a > 0.0)
		{
			//vec3 ref = dot(pos+lv, norm);
			vec3 h = normalize(lv+npos);
			float nh = dot(n, h);
			float nv = dot(n, npos);
			float vh = dot(npos, h);
			float sa = nh;
			float fres = pow(1 - dot(h, npos), 5)*0.4+0.5;

			float gtdenom = 2 * nh;
			float gt = max(0, min(gtdenom * nv / vh, gtdenom * da / vh));
								
			if (nh > 0.0)
			{
				float scol = fres*texture2D(lightFunc, vec2(nh, spec.a)).r*gt/(nh*da);
				vec3 speccol = lit*scol*light_col.rgb*spec.rgb;
				col += speccol;

				float cur_glare = max(speccol.r, speccol.g);
				cur_glare = max(cur_glare, speccol.b);
				glare = max(glare, speccol.r);
				glare += max(cur_glare, 0.0);
				//col += spec.rgb;
			}
		}
	}

	return max(col, vec3(0.0,0.0,0.0));	

}

vec3 decode_normal (vec2 enc)
{
    vec2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

vec4 getPosition_d(vec2 pos_screen, float depth)
{
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

vec3 getPositionEye()
{
	return vary_PositionEye;
}
vec3 getSunlitColor()
{
	return vary_SunlitColor;
}
vec3 getAmblitColor()
{
	return vary_AmblitColor;
}
vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}
vec3 getAtmosAttenuation()
{
	return vary_AtmosAttenuation;
}

void setPositionEye(vec3 v)
{
	vary_PositionEye = v;
}

void setSunlitColor(vec3 v)
{
	vary_SunlitColor = v;
}

void setAmblitColor(vec3 v)
{
	vary_AmblitColor = v;
}

void setAdditiveColor(vec3 v)
{
	vary_AdditiveColor = v;
}

void setAtmosAttenuation(vec3 v)
{
	vary_AtmosAttenuation = v;
}

void calcAtmospherics(vec3 inPositionEye, float ambFactor) {

	vec3 P = inPositionEye;
	setPositionEye(P);
	
	vec3 tmpLightnorm = lightnorm.xyz;

	vec3 Pn = normalize(P);
	float Plen = length(P);

	vec4 temp1 = vec4(0);
	vec3 temp2 = vec3(0);
	vec4 blue_weight;
	vec4 haze_weight;
	vec4 sunlight = sunlight_color;
	vec4 light_atten;

	//sunlight attenuation effect (hue and brightness) due to atmosphere
	//this is used later for sunlight modulation at various altitudes
	light_atten = (blue_density + vec4(haze_density * 0.25)) * (density_multiplier * max_y);
		//I had thought blue_density and haze_density should have equal weighting,
		//but attenuation due to haze_density tends to seem too strong

	temp1 = blue_density + vec4(haze_density);
	blue_weight = blue_density / temp1;
	haze_weight = vec4(haze_density) / temp1;

	//(TERRAIN) compute sunlight from lightnorm only (for short rays like terrain)
	temp2.y = max(0.0, tmpLightnorm.y);
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// main atmospheric scattering line integral
	temp2.z = Plen * density_multiplier;

	// Transparency (-> temp1)
	// ATI Bugfix -- can't store temp1*temp2.z*distance_multiplier in a variable because the ati
	// compiler gets confused.
	temp1 = exp(-temp1 * temp2.z * distance_multiplier);

	//final atmosphere attenuation factor
	setAtmosAttenuation(temp1.rgb);
	
	//compute haze glow
	//(can use temp2.x as temp because we haven't used it yet)
	temp2.x = dot(Pn, tmpLightnorm.xyz);
	temp2.x = 1. - temp2.x;
		//temp2.x is 0 at the sun and increases away from sun
	temp2.x = max(temp2.x, .03);	//was glow.y
		//set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
	temp2.x *= glow.x;
		//higher glow.x gives dimmer glow (because next step is 1 / "angle")
	temp2.x = pow(temp2.x, glow.z);
		//glow.z should be negative, so we're doing a sort of (1 / "angle") function

	//add "minimum anti-solar illumination"
	temp2.x += .25;
	
	//increase ambient when there are more clouds
	vec4 tmpAmbient = ambient + (vec4(1.) - ambient) * cloud_shadow * 0.5;
	
	/*  decrease value and saturation (that in HSV, not HSL) for occluded areas
	 * // for HSV color/geometry used here, see http://gimp-savvy.com/BOOK/index.html?node52.html
	 * // The following line of code performs the equivalent of:
	 * float ambAlpha = tmpAmbient.a;
	 * float ambValue = dot(vec3(tmpAmbient), vec3(0.577)); // projection onto <1/rt(3), 1/rt(3), 1/rt(3)>, the neutral white-black axis
	 * vec3 ambHueSat = vec3(tmpAmbient) - vec3(ambValue);
	 * tmpAmbient = vec4(RenderSSAOEffect.valueFactor * vec3(ambValue) + RenderSSAOEffect.saturationFactor *(1.0 - ambFactor) * ambHueSat, ambAlpha);
	 */
	tmpAmbient = vec4(mix(ssao_effect_mat * tmpAmbient.rgb, tmpAmbient.rgb, ambFactor), tmpAmbient.a);

	//haze color
	setAdditiveColor(
		vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow) + tmpAmbient)
	  + (haze_horizon * haze_weight) * (sunlight*(1.-cloud_shadow) * temp2.x
		  + tmpAmbient)));

	//brightness of surface both sunlight and ambient
	setSunlitColor(pow(vec3(sunlight * .5), vec3(2.2)) * 2.2);
	setAmblitColor(pow(vec3(tmpAmbient * .25), vec3(2.2)) * 2.2);
	setAdditiveColor(pow(getAdditiveColor() * vec3(1.0 - temp1), vec3(2.2)) * 2.2);
}

vec3 atmosLighting(vec3 light)
{
	light *= getAtmosAttenuation().r;
	light += getAdditiveColor();
	return (2.0 * light);
}

vec3 atmosTransport(vec3 light) {
	light *= getAtmosAttenuation().r;
	light += getAdditiveColor() * 2.0;
	return light;
}
vec3 atmosGetDiffuseSunlightColor()
{
	return getSunlitColor();
}

vec3 scaleDownLight(vec3 light)
{
	return (light / vec3(scene_light_strength, scene_light_strength, scene_light_strength));
}

vec3 scaleUpLight(vec3 light)
{
	return (light * vec3(scene_light_strength, scene_light_strength, scene_light_strength));
}

vec3 atmosAmbient(vec3 light)
{
	return getAmblitColor() + (light * vec3(0.5f, 0.5f, 0.5f));
}

vec3 atmosAffectDirectionalLight(float lightIntensity)
{
	return getSunlitColor() * vec3(lightIntensity, lightIntensity, lightIntensity);
}

vec3 scaleSoftClip(vec3 light)
{
	//soft clip effect:
    vec3 zeroes = vec3(0.0f, 0.0f, 0.0f);
    vec3 ones   = vec3(1.0f, 1.0f, 1.0f);

	light = ones - clamp(light, zeroes, ones);
	light = ones - pow(light, gamma.xxx);

	return light;
}

vec3 fullbrightAtmosTransport(vec3 light) {
	float brightness = dot(light.rgb, vec3(0.33333));

	return mix(atmosTransport(light.rgb), light.rgb + getAdditiveColor().rgb, brightness * brightness);
}

vec3 fullbrightScaleSoftClip(vec3 light)
{
	//soft clip effect:
	return light;
}

#else
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif
#endif

uniform sampler2D diffuseMap;

#if HAS_NORMAL_MAP
uniform sampler2D bumpMap;
#endif

#if HAS_SPECULAR_MAP
uniform sampler2D specularMap;

VARYING vec2 vary_texcoord2;
#endif

uniform float env_intensity;
uniform vec4 specular_color;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
uniform float minimum_alpha;
#endif

#if HAS_NORMAL_MAP
VARYING vec3 vary_mat0;
VARYING vec3 vary_mat1;
VARYING vec3 vary_mat2;
VARYING vec2 vary_texcoord1;
#else
VARYING vec3 vary_normal;
#endif

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

vec2 encode_normal(vec3 n)
{
	float f = sqrt(8 * n.z + 8);
	return n.xy / f + 0.5;
}

void main() 
{
	vec4 diffcol = texture2D(diffuseMap, vary_texcoord0.xy);
	diffcol.rgb *= vertex_color.rgb;

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_MASK)
	if (diffcol.a < minimum_alpha)
	{
		discard;
	}
#endif

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
	vec3 old_diffcol = diffcol.rgb;
	diffcol.rgb = pow(diffcol.rgb, vec3(2.2));
#endif

#if HAS_SPECULAR_MAP
	vec4 spec = texture2D(specularMap, vary_texcoord2.xy)*specular_color;
#else
	vec4 spec = specular_color;
#endif

#if HAS_NORMAL_MAP
	vec4 norm = texture2D(bumpMap, vary_texcoord1.xy);

	norm.xyz = norm.xyz * 2 - 1;

	vec3 tnorm = vec3(dot(norm.xyz,vary_mat0),
			  dot(norm.xyz,vary_mat1),
			  dot(norm.xyz,vary_mat2));
#else
	vec4 norm = vec4(0,0,0,1.0);
	vec3 tnorm = vary_normal;
#endif

    norm.xyz = tnorm;
    norm.xyz = normalize(norm.xyz);

	vec4 final_color = diffcol;
	
#if (DIFFUSE_ALPHA_MODE != DIFFUSE_ALPHA_MODE_EMISSIVE)
	final_color.a = emissive_brightness;
#else
	final_color.a = max(final_color.a, emissive_brightness);
#endif

	vec4 final_specular = spec;
#if HAS_SPECULAR_MAP
	//final_color.rgb *= 1 - spec.a * env_intensity;
	//final_specular.rgb *= specular_color.rgb;
	
	vec4 final_normal = vec4(encode_normal(normalize(tnorm)), env_intensity, 0.0);
	final_specular.a = specular_color.a * norm.a;
#else
	vec4 final_normal = vec4(encode_normal(normalize(tnorm)), env_intensity, 0.0);
	final_specular.a = specular_color.a;
#endif
	

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
		//forward rendering, output just lit RGBA
	vec3 pos = vary_position;

#if HAS_SUN_SHADOW
	float shadow = 0.0;
	
	vec4 spos = vec4(pos,1.0);
		
	if (spos.z > -shadow_clip.w)
	{	
		vec4 lpos;
		
		vec4 near_split = shadow_clip*-0.75;
		vec4 far_split = shadow_clip*-1.25;
		vec4 transition_domain = near_split-far_split;
		float weight = 0.0;

		if (spos.z < near_split.z)
		{
			lpos = shadow_matrix[3]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap3, lpos)*w;
			weight += w;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}

		if (spos.z < near_split.y && spos.z > far_split.z)
		{
			lpos = shadow_matrix[2]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
			w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap2, lpos)*w;
			weight += w;
		}

		if (spos.z < near_split.x && spos.z > far_split.y)
		{
			lpos = shadow_matrix[1]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
			w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
			shadow += pcfShadow(shadowMap1, lpos)*w;
			weight += w;
		}

		if (spos.z > far_split.x)
		{
			lpos = shadow_matrix[0]*spos;
							
			float w = 1.0;
			w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
				
			shadow += pcfShadow(shadowMap0, lpos)*w;
			weight += w;
		}
		

		shadow /= weight;
	}
	else
	{
		shadow = 1.0;
	}
#else
	float shadow = 1.0;
#endif

	spec = final_specular;
		vec4 diffuse = final_color;
		float envIntensity = final_normal.z;

    vec3 col = vec3(0.0f,0.0f,0.0f);

		float bloom = 0.0;
			calcAtmospherics(pos.xyz, 1.0);
	
	vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

	float da =dot(norm.xyz, sun_dir.xyz);
    float final_da = da;
          final_da = min(final_da, shadow);
          final_da = max(final_da, diffuse.a);
          final_da = max(final_da, 0.0f);

	col.rgb = atmosAmbient(col);
	
	float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
	ambient *= 0.5;
	ambient *= ambient;
	ambient = (1.0-ambient);

	col.rgb *= ambient;

	col.rgb = col.rgb + atmosAffectDirectionalLight(final_da * 2.6);
	col.rgb *= diffuse.rgb;
	

			float glare = 0.0;

			if (spec.a > 0.0) // specular reflection
			{
				// the old infinite-sky shiny reflection
				//
				
				float sa = dot(refnormpersp, sun_dir.xyz);
				vec3 dumbshiny = vary_SunlitColor*shadow*(texture2D(lightFunc, vec2(sa, spec.a)).r);
							
				// add the two types of shiny together
				vec3 spec_contrib = dumbshiny * spec.rgb;
				bloom = dot(spec_contrib, spec_contrib) / 6;

				glare = max(spec_contrib.r, spec_contrib.g);
				glare = max(glare, spec_contrib.b);

				col += spec_contrib;
			}

			col = mix(col.rgb, old_diffcol.rgb, diffuse.a);

			if (envIntensity > 0.0)
			{
				//add environmentmap
				vec3 env_vec = env_mat * refnormpersp;
				float exponent = mix(2.2, 1.0, diffuse.a);

				vec3 refcol = pow(textureCube(environmentMap, env_vec).rgb, vec3(exponent))*exponent;

				col = mix(col.rgb, refcol, 
					envIntensity);  

				float cur_glare = max(refcol.r, refcol.g);
				cur_glare = max(cur_glare, refcol.b);
				cur_glare *= envIntensity*4.0;
				glare += cur_glare;
			}

			float exponent = mix(1.0, 2.2, diffuse.a);
			col = pow(col, vec3(exponent));
				
	
			col = mix(atmosLighting(col), fullbrightAtmosTransport(col), diffuse.a);
			col = mix(scaleSoftClip(col), fullbrightScaleSoftClip(col), diffuse.a);

			
		vec3 npos = normalize(-pos.xyz);

 #define LIGHT_LOOP(i) col.rgb = col.rgb + calcPointLightOrSpotLight(light_diffuse[i].rgb, npos, diffuse.rgb, final_specular, pos.xyz, norm.xyz, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, glare);

		LIGHT_LOOP(1)
		LIGHT_LOOP(2)
		LIGHT_LOOP(3)
		LIGHT_LOOP(4)
		LIGHT_LOOP(5)
		LIGHT_LOOP(6)
		LIGHT_LOOP(7)


	col.rgb = pow(col.rgb, vec3(1.0/2.2));

	frag_color.rgb = col.rgb;
	glare = min(glare, 1.0);
	frag_color.a = max(diffcol.a*vertex_color.a, glare);
#else

	frag_data[0] = final_color;

#ifdef UGLY_MAC_HACK
	// magic spec exp clamp fixes rendering artifacts on older mac GF drivers
	//
	final_specular = min(final_specular, vec4(1.0f, 1.0f, 1.0f, 0.125f));
#endif

	frag_data[1] = final_specular; // XYZ = Specular color. W = Specular exponent.
	frag_data[2] = final_normal; // XY = Normal.  Z = Env. intensity.
#endif
}
