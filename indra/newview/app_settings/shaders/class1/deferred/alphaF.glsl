/** 
 * @file alphaF.glsl
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
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform float display_gamma;
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

#if HAS_SHADOW
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;

uniform vec2 shadow_res;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;

#endif

#ifdef USE_DIFFUSE_TEX
uniform sampler2D diffuseMap;
#endif

VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color;
#endif

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

vec3 srgb_to_linear(vec3 cs)
{
	vec3 low_range = cs / vec3(12.92);
	vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
	bvec3 lte = lessThanEqual(cs,vec3(0.04045));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lte.r ? low_range.r : high_range.r;
	result.g = lte.g ? low_range.g : high_range.g;
	result.b = lte.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lte);
#endif

}

vec3 linear_to_srgb(vec3 cl)
{
	cl = clamp(cl, vec3(0), vec3(1));
	vec3 low_range  = cl * 12.92;
	vec3 high_range = 1.055 * pow(cl, vec3(0.41666)) - 0.055;
	bvec3 lt = lessThan(cl,vec3(0.0031308));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lt.r ? low_range.r : high_range.r;
	result.g = lt.g ? low_range.g : high_range.g;
	result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lt);
#endif

}

vec2 encode_normal(vec3 n)
{
	float f = sqrt(8 * n.z + 8);
	return n.xy / f + 0.5;
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

vec3 calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	a = pow(a, 1.0/1.3);
	return vec3(a,a,a);
}

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	float da = 1.0;

	vec3 col = vec3(0);

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist = d/la;
		float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
		dist_atten *= dist_atten;
		dist_atten *= 2.0;

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= max(dot(n, lv), 0.0);		

		float lit = max(da * dist_atten,0.0);

		col = light_col * lit * diffuse;

		// no spec for alpha shader...
	}

	return max(col, vec3(0.0,0.0,0.0));
}

#if HAS_SHADOW
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

#ifdef WATER_FOG
uniform vec4 waterPlane;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;

vec4 applyWaterFogDeferred(vec3 pos, vec4 color)
{
	//normalize view vector
	vec3 view = normalize(pos);
	float es = -(dot(view, waterPlane.xyz));

	//find intersection point with water plane and eye vector
	
	//get eye depth
	float e0 = max(-waterPlane.w, 0.0);
	
	vec3 int_v = waterPlane.w > 0.0 ? view * waterPlane.w/es : vec3(0.0, 0.0, 0.0);
	
	//get object depth
	float depth = length(pos - int_v);
		
	//get "thickness" of water
	float l = max(depth, 0.1);

	float kd = waterFogDensity;
	float ks = waterFogKS;
	vec4 kc = waterFogColor;
	
	float F = 0.98;
	
	float t1 = -kd * pow(F, ks * e0);
	float t2 = kd + ks * es;
	float t3 = pow(F, t2*l) - 1.0;
	
	float L = min(t1/t2*t3, 1.0);
	
	float D = pow(0.98, l*kd);
	
	color.rgb = color.rgb * D + kc.rgb * L;
	color.a = kc.a + color.a;
	
	return color;
}
#endif

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
	setSunlitColor(vec3(sunlight * .5));
	setAmblitColor(vec3(tmpAmbient * .25));
	setAdditiveColor(getAdditiveColor() * vec3(1.0 - temp1));
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

void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	vec4 pos = vec4(vary_position, 1.0);
	
	float shadow = 1.0;

#if HAS_SHADOW
	vec4 spos = pos;
		
	if (spos.z > -shadow_clip.w)
	{	
		shadow = 0.0;

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
#endif

#ifdef USE_INDEXED_TEX
	vec4 diff = diffuseLookup(vary_texcoord0.xy);
#else
	vec4 diff = texture2D(diffuseMap,vary_texcoord0.xy);
#endif

#ifdef FOR_IMPOSTOR
	vec4 color;
	color.rgb = diff.rgb;
	color.a = 1.0;

#ifdef USE_VERTEX_COLOR
	float final_alpha = diff.a * vertex_color.a;
	diff.rgb *= vertex_color.rgb;
#else
	float final_alpha = diff.a;
#endif
	
	// Insure we don't pollute depth with invis pixels in impostor rendering
	//
	if (final_alpha < 0.01)
	{
		discard;
	}
#else
	
#ifdef USE_VERTEX_COLOR
	float final_alpha = diff.a * vertex_color.a;
	diff.rgb *= vertex_color.rgb;
#else
	float final_alpha = diff.a;
#endif


	vec4 gamma_diff = diff;	
	diff.rgb = srgb_to_linear(diff.rgb);

	vec3 norm = vary_norm; 

	calcAtmospherics(pos.xyz, 1.0);

	vec2 abnormal	= encode_normal(norm.xyz);
		 norm.xyz   = decode_normal(abnormal.xy);

	float da = dot(norm.xyz, sun_dir.xyz);

    float final_da = da;
          final_da = min(final_da, shadow);
          final_da = max(final_da, 0.0f);
		  final_da = min(final_da, 1.0f);
		  final_da = pow(final_da, 1.0/1.3);

	vec4 color = vec4(0,0,0,0);

	color.rgb = atmosAmbient(color.rgb);
	color.a   = final_alpha;

	float ambient = abs(da);
	ambient *= 0.5;
	ambient *= ambient;
	ambient = (1.0-ambient);

	color.rgb *= ambient;
	color.rgb += atmosAffectDirectionalLight(final_da);
	color.rgb *= gamma_diff.rgb;

	//color.rgb = mix(diff.rgb, color.rgb, final_alpha);
	
	color.rgb = atmosLighting(color.rgb);
	color.rgb = scaleSoftClip(color.rgb);

	vec4 light = vec4(0,0,0,0);

	color.rgb = srgb_to_linear(color.rgb);
	
   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diff.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z);

	LIGHT_LOOP(1)
	LIGHT_LOOP(2)
	LIGHT_LOOP(3)
	LIGHT_LOOP(4)
	LIGHT_LOOP(5)
	LIGHT_LOOP(6)
	LIGHT_LOOP(7)

	// keep it linear
	//
	color.rgb += light.rgb;

	// straight to display gamma, we're post-deferred
	//
	color.rgb = linear_to_srgb(color.rgb);

#ifdef WATER_FOG
	color = applyWaterFogDeferred(pos.xyz, color);
#endif

#endif

	frag_color = color;
}

