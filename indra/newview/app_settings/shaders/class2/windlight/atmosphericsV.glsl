/**
 * @file atmosphericsV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 


// VARYING param funcs
void setSunlitColor(vec3 v);
void setAmblitColor(vec3 v);
void setAdditiveColor(vec3 v);
void setAtmosAttenuation(vec3 v);
void setPositionEye(vec3 v);

vec3 getAdditiveColor();

//VARYING vec4 vary_CloudUVs;
//VARYING float vary_CloudDensity;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
//uniform vec4 camPosWorld;

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

void calcAtmospherics(vec3 inPositionEye) {

	vec3 P = inPositionEye;
	setPositionEye(P);
	
	//(TERRAIN) limit altitude
	if (P.y > max_y) P *= (max_y / P.y);
	if (P.y < -max_y) P *= (-max_y / P.y);

	vec3 tmpLightnorm = lightnorm.xyz;

	vec3 Pn = normalize(P);
	float  Plen = length(P);

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
	//vary_AtmosAttenuation = distance_multiplier / 10000.;
	//vary_AtmosAttenuation = density_multiplier * 100.;
	//vary_AtmosAttenuation = vec4(Plen / 100000., 0., 0., 1.);

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

	//haze color
	setAdditiveColor(
		vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow) + tmpAmbient)
	  + (haze_horizon * haze_weight) * (sunlight*(1.-cloud_shadow) * temp2.x
		  + tmpAmbient)));

	//brightness of surface both sunlight and ambient
	setSunlitColor(vec3(sunlight * .5));
	setAmblitColor(vec3(tmpAmbient * .25));
	setAdditiveColor(getAdditiveColor() * vec3(1.0 - temp1));

	// vary_SunlitColor = vec3(0);
	// vary_AmblitColor = vec3(0);
	// vary_AdditiveColor = vec4(Pn, 1.0);

	/*
	const float cloudShadowScale = 100.;
	// Get cloud uvs for shadowing
	vec3 cloudPos = inPositionEye + camPosWorld - cloudShadowScale / 2.;
	vary_CloudUVs.xy = cloudPos.xz / cloudShadowScale;

	// We can take uv1 and multiply it by (TerrainSpan / CloudSpan)
//	cloudUVs *= (((worldMaxZ - worldMinZ) * 20) /40000.);
	vary_CloudUVs *= (10000./40000.);

	// Offset by sun vector * (CloudAltitude / CloudSpan)
	vary_CloudUVs.x += tmpLightnorm.x / tmpLightnorm.y * (3000./40000.);
	vary_CloudUVs.y += tmpLightnorm.z / tmpLightnorm.y * (3000./40000.);
	*/
}

