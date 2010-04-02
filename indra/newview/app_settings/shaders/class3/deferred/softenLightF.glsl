/** 
 * @file softenLightF.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2D	  noiseMap;
uniform samplerCube environmentMap;
uniform sampler2D	  lightFunc;
uniform vec3 gi_quad;

uniform float blur_size;
uniform float blur_fidelity;

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
uniform vec4 haze_horizon;
uniform vec4 haze_density;
uniform vec4 cloud_shadow;
uniform vec4 density_multiplier;
uniform vec4 distance_multiplier;
uniform vec4 max_y;
uniform vec4 glow;
uniform float scene_light_strength;
uniform vec3 env_mat[3];
uniform vec4 shadow_clip;
uniform mat3 ssao_effect_mat;

uniform sampler2DRect depthMap;
uniform mat4 inv_proj;
uniform vec2 screen_res;

varying vec4 vary_light;
varying vec2 vary_fragcoord;

vec3 vary_PositionEye;

vec3 vary_SunlitColor;
vec3 vary_AmblitColor;
vec3 vary_AdditiveColor;
vec3 vary_AtmosAttenuation;
uniform float gi_ambiance;

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

vec4 getPosition(vec2 pos_screen)
{ //get position in screen space (world units) given window coordinate and depth map
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
	return getPosition_d(pos_screen, depth);
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
	
	//(TERRAIN) limit altitude
	if (P.y > max_y.x) P *= (max_y.x / P.y);
	if (P.y < -max_y.x) P *= (-max_y.x / P.y);

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
	light_atten = (blue_density * 1.0 + vec4(haze_density.r) * 0.25) * (density_multiplier.x * max_y.x);
		//I had thought blue_density and haze_density should have equal weighting,
		//but attenuation due to haze_density tends to seem too strong

	temp1 = blue_density + vec4(haze_density.r);
	blue_weight = blue_density / temp1;
	haze_weight = vec4(haze_density.r) / temp1;

	//(TERRAIN) compute sunlight from lightnorm only (for short rays like terrain)
	temp2.y = max(0.0, tmpLightnorm.y);
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// main atmospheric scattering line integral
	temp2.z = Plen * density_multiplier.x;

	// Transparency (-> temp1)
	// ATI Bugfix -- can't store temp1*temp2.z*distance_multiplier.x in a variable because the ati
	// compiler gets confused.
	temp1 = exp(-temp1 * temp2.z * distance_multiplier.x);

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
	vec4 tmpAmbient = ambient*gi_ambiance + (vec4(1.) - ambient*gi_ambiance) * cloud_shadow.x * 0.5;
	
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
		vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow.x) + tmpAmbient)
	  + (haze_horizon.r * haze_weight) * (sunlight*(1.-cloud_shadow.x) * temp2.x
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
	return (light / scene_light_strength );
}

vec3 scaleUpLight(vec3 light)
{
	return (light * scene_light_strength);
}

vec3 atmosAmbient(vec3 light)
{
	return getAmblitColor() + light / 2.0;
}

vec3 atmosAffectDirectionalLight(float lightIntensity)
{
	return getSunlitColor() * lightIntensity;
}

vec3 scaleSoftClip(vec3 light)
{
	//soft clip effect:
	light = 1. - clamp(light, vec3(0.), vec3(1.));
	light = 1. - pow(light, gamma.xxx);

	return light;
}

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	float depth = texture2DRect(depthMap, tc.xy).a;
	vec3 pos = getPosition_d(tc, depth).xyz;
	vec3 norm = texture2DRect(normalMap, tc).xyz*2.0-1.0;
	//vec3 nz = texture2D(noiseMap, vary_fragcoord.xy/128.0).xyz;
	
	float da = max(dot(norm.xyz, vary_light.xyz), 0.0);
	
	vec4 diffuse = texture2DRect(diffuseRect, tc);
	vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
	
	da = texture2D(lightFunc, vec2(da, 0.0)).a;
		
	vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;
	float scol = max(scol_ambocc.r, diffuse.a); 
	float ambocc = scol_ambocc.g;
	
	calcAtmospherics(pos.xyz, ambocc);
	
	vec3 col = atmosAmbient(vec3(0));
	col += atmosAffectDirectionalLight(max(min(da, scol), diffuse.a));
	
	col *= diffuse.rgb;
	
	if (spec.a > 0.0) // specular reflection
	{
		// the old infinite-sky shiny reflection
		//
		vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));
		float sa = dot(refnormpersp, vary_light.xyz);
		vec3 dumbshiny = vary_SunlitColor*scol*texture2D(lightFunc, vec2(sa, spec.a)).a;

		// screen-space cheap fakey reflection map
		//
		vec3 refnorm = normalize(reflect(vec3(0,0,-1), norm.xyz));
		depth -= 0.5; // unbias depth
		// first figure out where we'll make our 2D guess from
		vec2 ref2d = (0.25 * screen_res.y) * (refnorm.xy) * abs(refnorm.z) / depth;
		// Offset the guess source a little according to a trivial
		// checkerboard dither function and spec.a.
		// This is meant to be similar to sampling a blurred version
		// of the diffuse map.  LOD would be better in that regard.
		// The goal of the blur is to soften reflections in surfaces
		// with low shinyness, and also to disguise our lameness.
		float checkerboard = floor(mod(tc.x+tc.y, 2.0)); // 0.0, 1.0
		ref2d += normalize(ref2d)*14.0*(1.0-spec.a)*(checkerboard-0.5);
		ref2d += tc.xy; // use as offset from destination
		// get attributes from the 2D guess point
		float refdepth = texture2DRect(depthMap, ref2d).a;
		vec3 refpos = getPosition_d(ref2d, refdepth).xyz;
		vec3 refcol = texture2DRect(diffuseRect, ref2d).rgb;
		float refshad = texture2DRect(lightMap, ref2d).r;
		vec3 refn = normalize(texture2DRect(normalMap, ref2d).rgb * 2.0 - 1.0);
		// figure out how appropriate our guess actually was
		float refapprop = max(0.0, dot(-refnorm, normalize(pos - refpos)));
		// darken reflections from points which face away from the reflected ray - our guess was a back-face
		//refapprop *= step(dot(refnorm, refn), 0.0);
		refapprop = min(refapprop, max(0.0, -dot(refnorm, refn))); // more conservative variant
		// get appropriate light strength for guess-point.
		// reflect light direction to increase the illusion that
		// these are reflections.
		vec3 reflight = reflect(lightnorm.xyz, norm.xyz);
		float reflit = max(dot(refn, reflight.xyz), 0.0);
		// apply sun color to guess-point, dampen according to inappropriateness of guess
		float refmod = min(refapprop, reflit);
		vec3 refprod = vary_SunlitColor * refcol.rgb * refmod;
		vec3 ssshiny = (refprod * spec.a);

		// add the two types of shiny together
		col += (ssshiny + dumbshiny) * spec.rgb;
	}
	
	col = atmosLighting(col);
	col = scaleSoftClip(col);
		
	gl_FragColor.rgb = col;
	
	//gl_FragColor.rgb = gi_col.rgb;
	gl_FragColor.a = 0.0;
	
	//gl_FragColor.rg = scol_ambocc.rg;
	//gl_FragColor.rgb = texture2DRect(lightMap, vary_fragcoord.xy).rgb;
	//gl_FragColor.rgb = norm.rgb*0.5+0.5;
	//gl_FragColor.rgb = vec3(ambocc);
	//gl_FragColor.rgb = vec3(scol);
}
