/** 
 * @file WLCloudsV.glsl
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

uniform mat4 modelview_projection_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec2 texcoord0;

//////////////////////////////////////////////////////////////////////////
// The vertex shader for creating the atmospheric sky
///////////////////////////////////////////////////////////////////////////////

// Output parameters
VARYING vec4 vary_CloudColorSun;
VARYING vec4 vary_CloudColorAmbient;
VARYING float vary_CloudDensity;

VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec2 vary_texcoord3;

// Inputs
uniform vec3 camPosLocal;

uniform vec4 lightnorm;
uniform vec4 sunlight_color;
uniform vec4 ambient;
uniform vec4 blue_horizon;
uniform vec4 blue_density;
uniform vec4 haze_horizon;
uniform vec4 haze_density;

uniform vec4 cloud_shadow;
uniform vec4 density_multiplier;
uniform vec4 max_y;

uniform vec4 glow;

uniform vec4 cloud_color;

uniform vec4 cloud_scale;

void main()
{

	// World / view / projection
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

	vary_texcoord0 = texcoord0;

	// Get relative position
	vec3 P = position.xyz - camPosLocal.xyz + vec3(0,50,0);

	// Set altitude
	if (P.y > 0.)
	{
		P *= (max_y.x / P.y);
	}
	else
	{
		P *= (-32000. / P.y);
	}

	// Can normalize then
	vec3 Pn = normalize(P);
	float  Plen = length(P);

	// Initialize temp variables
	vec4 temp1 = vec4(0.);
	vec4 temp2 = vec4(0.);
	vec4 blue_weight;
	vec4 haze_weight;
	vec4 sunlight = sunlight_color;
	vec4 light_atten;


	// Sunlight attenuation effect (hue and brightness) due to atmosphere
	// this is used later for sunlight modulation at various altitudes
	light_atten = (blue_density * 1.0 + haze_density.x * 0.25) * (density_multiplier.x * max_y.x);

	// Calculate relative weights
	temp1 = blue_density + haze_density.x;
	blue_weight = blue_density / temp1;
	haze_weight = haze_density.x / temp1;

	// Compute sunlight from P & lightnorm (for long rays like sky)
	temp2.y = max(0., max(0., Pn.y) * 1.0 + lightnorm.y );
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// Distance
	temp2.z = Plen * density_multiplier.x;

	// Transparency (-> temp1)
	// ATI Bugfix -- can't store temp1*temp2.z in a variable because the ati
	// compiler gets confused.
	temp1 = exp(-temp1 * temp2.z);


	// Compute haze glow
	temp2.x = dot(Pn, lightnorm.xyz);
	temp2.x = 1. - temp2.x;
		// temp2.x is 0 at the sun and increases away from sun
	temp2.x = max(temp2.x, .001);	
		// Set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
	temp2.x *= glow.x;
		// Higher glow.x gives dimmer glow (because next step is 1 / "angle")
	temp2.x = pow(temp2.x, glow.z);
		// glow.z should be negative, so we're doing a sort of (1 / "angle") function

	// Add "minimum anti-solar illumination"
	temp2.x += .25;

	// Increase ambient when there are more clouds
	vec4 tmpAmbient = ambient;
	tmpAmbient += (1. - tmpAmbient) * cloud_shadow.x * 0.5; 

	// Dim sunlight by cloud shadow percentage
	sunlight *= (1. - cloud_shadow.x);

	// Haze color below cloud
	vec4 additiveColorBelowCloud = (	  blue_horizon * blue_weight * (sunlight + tmpAmbient)
				+ (haze_horizon.r * haze_weight) * (sunlight * temp2.x + tmpAmbient)
			 );	

	// CLOUDS

	sunlight = sunlight_color;
	temp2.y = max(0., lightnorm.y * 2.);
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// Cloud color out
	vary_CloudColorSun = (sunlight * temp2.x) * cloud_color;
	vary_CloudColorAmbient = tmpAmbient * cloud_color;
	
	// Attenuate cloud color by atmosphere
	temp1 = sqrt(temp1);	//less atmos opacity (more transparency) below clouds
	vary_CloudColorSun *= temp1;
	vary_CloudColorAmbient *= temp1;
	vec4 oHazeColorBelowCloud = additiveColorBelowCloud * (1. - temp1);

	// Make a nice cloud density based on the cloud_shadow value that was passed in.
	vary_CloudDensity = 2. * (cloud_shadow.x - 0.25);


	// Texture coords
	vary_texcoord0 = texcoord0;
	vary_texcoord0.xy -= 0.5;
	vary_texcoord0.xy /= cloud_scale.x;
	vary_texcoord0.xy += 0.5;

	vary_texcoord1 = vary_texcoord0;
	vary_texcoord1.x += lightnorm.x * 0.0125;
	vary_texcoord1.y += lightnorm.z * 0.0125;

	vary_texcoord2 = vary_texcoord0 * 16.;
	vary_texcoord3 = vary_texcoord1 * 16.;

	// Combine these to minimize register use
	vary_CloudColorAmbient += oHazeColorBelowCloud;

	// needs this to compile on mac
	//vary_AtmosAttenuation = vec3(0.0,0.0,0.0);

	// END CLOUDS
}

