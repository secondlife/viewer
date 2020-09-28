/**
 * @file class2\wl\cloudsV.glsl
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
VARYING vec4  vary_CloudColorSun;
VARYING vec4  vary_CloudColorAmbient;
VARYING float vary_CloudDensity;

VARYING vec2  vary_texcoord0;
VARYING vec2  vary_texcoord1;
VARYING vec2  vary_texcoord2;
VARYING vec2  vary_texcoord3;
VARYING float altitude_blend_factor;

// Inputs
uniform vec3 camPosLocal;

uniform vec4  lightnorm;
uniform vec4  sunlight_color;
uniform vec4  moonlight_color;
uniform int   sun_up_factor;
uniform vec4  ambient_color;
uniform vec4  blue_horizon;
uniform vec4  blue_density;
uniform float haze_horizon;
uniform float haze_density;

uniform float cloud_shadow;
uniform float density_multiplier;
uniform float max_y;

uniform vec4  glow;
uniform float sun_moon_glow_factor;

uniform vec4 cloud_color;

uniform float cloud_scale;

// NOTE: Keep these in sync!
//       indra\newview\app_settings\shaders\class1\deferred\skyV.glsl
//       indra\newview\app_settings\shaders\class1\deferred\cloudsV.glsl
//       indra\newview\app-settings\shaders\class2\windlight\cloudsV.glsl
//       indra\newview\lllegacyatmospherics.cpp
//       indra\newview\llsettingsvo.cpp
void main()
{
    // World / view / projection
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    // Texture coords
    // SL-13084 EEP added support for custom cloud textures -- flip them horizontally to match the preview of Clouds > Cloud Scroll
    vary_texcoord0 = vec2(-texcoord0.x, texcoord0.y);  // See: LLSettingsVOSky::applySpecial

    vary_texcoord0.xy -= 0.5;
    vary_texcoord0.xy /= cloud_scale;
    vary_texcoord0.xy += 0.5;

    vary_texcoord1 = vary_texcoord0;
    vary_texcoord1.x += lightnorm.x * 0.0125;
    vary_texcoord1.y += lightnorm.z * 0.0125;

    vary_texcoord2 = vary_texcoord0 * 16.;
    vary_texcoord3 = vary_texcoord1 * 16.;

    // Get relative position
    vec3 rel_pos = position.xyz - camPosLocal.xyz + vec3(0, 50, 0);

    // fade clouds beyond a certain point so the bottom of the sky dome doesn't look silly at high altitude
    altitude_blend_factor = clamp((rel_pos.y + 512.0) / max_y, 0.0, 1.0);

    // Adj position vector to clamp altitude
    if (rel_pos.y > 0.)
    {
        rel_pos *= (max_y / rel_pos.y);
    }
    if (rel_pos.y < 0.)
    {
        rel_pos *= (-32000. / rel_pos.y);
    }

    // Can normalize then
    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    // Initialize temp variables
    vec4 sunlight = sunlight_color;
    vec4 light_atten;

    // Sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    light_atten = (blue_density + vec4(haze_density * 0.25)) * (density_multiplier * max_y);

    // Calculate relative weights
    vec4 combined_haze = abs(blue_density) + vec4(abs(haze_density));
    vec4 blue_weight   = blue_density / combined_haze;
    vec4 haze_weight   = haze_density / combined_haze;

    // Compute sunlight from rel_pos & lightnorm (for long rays like sky)
    float off_axis = 1.0 / max(1e-6, max(0., rel_pos_norm.y) + lightnorm.y);
    sunlight *= exp(-light_atten * off_axis);

    // Distance
    float density_dist = rel_pos_len * density_multiplier;

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist);

    // Compute haze glow
    float haze_glow = 1.0 - dot(rel_pos_norm, lightnorm.xyz);
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);
    // Set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
    // Higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = pow(haze_glow, glow.z);
    // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    haze_glow *= sun_moon_glow_factor;

    // Add "minimum anti-solar illumination"
    // For sun, add to glow.  For moon, remove glow entirely. SL-13768
    haze_glow = (sun_moon_glow_factor < 1.0) ? 0.0 : (haze_glow + 0.25);

    // Increase ambient when there are more clouds
    vec4 tmpAmbient = ambient_color;
    tmpAmbient += (1. - tmpAmbient) * cloud_shadow * 0.5;

    // Dim sunlight by cloud shadow percentage
    sunlight *= (1. - cloud_shadow);

    // Haze color below cloud
    vec4 additiveColorBelowCloud =
        (blue_horizon * blue_weight * (sunlight + tmpAmbient) + (haze_horizon * haze_weight) * (sunlight * haze_glow + tmpAmbient));

    // CLOUDS
    off_axis = 1.0 / max(1e-6, lightnorm.y * 2.);
    sunlight *= exp(-light_atten * off_axis);

    // Cloud color out
    vary_CloudColorSun     = (sunlight * haze_glow) * cloud_color;
    vary_CloudColorAmbient = tmpAmbient * cloud_color;

    // Attenuate cloud color by atmosphere
    combined_haze = sqrt(combined_haze);  // less atmos opacity (more transparency) below clouds
    vary_CloudColorSun *= combined_haze;
    vary_CloudColorAmbient *= combined_haze;
    vec4 oHazeColorBelowCloud = additiveColorBelowCloud * (1. - combined_haze);

    // Make a nice cloud density based on the cloud_shadow value that was passed in.
    vary_CloudDensity = 2. * (cloud_shadow - 0.25);

    // Combine these to minimize register use
    vary_CloudColorAmbient += oHazeColorBelowCloud;

    // needs this to compile on mac
    // vary_AtmosAttenuation = vec3(0.0,0.0,0.0);

    // END CLOUDS
}
